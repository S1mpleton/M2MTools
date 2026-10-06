#include "variantservice.h"

#include "data/database.h"
#include "data/transaction.h"
#include "core/automatondata.h"

VariantService::VariantService(Database* db, VariantRepository* repo, QObject* parent)
    : m_db(db)
    , m_repo(repo)
    , QObject(parent)
{}


ServiceResult VariantService::saveVariant(const AutomatonData& data) {
    const QString conversion = variantTypeToString(data.getType());

    // 1. Проверяем, существует ли уже такой вариант
    auto existing = m_repo->findVariant(data.getVariantNumber());

    Transaction tx(m_db);   // BEGIN

    int variantId = -1;
    if (existing) {
        variantId = existing->id;
        // Чистим содержимое — проще, чем делать diff
        auto clearResult = m_repo->removeVariant(variantId);
        if (!clearResult) return ServiceResult::failure("Хуйня с удалением варианта");
    } else {
        QString error;
        variantId = m_repo->insertVariant(data.getVariantNumber(), conversion, &error);
        if (variantId < 0)
            return ServiceResult::failure(
                QStringLiteral("Не удалось создать вариант: %1").arg(error));
    }

    // 2. Сохраняем States, Inputs, Outputs
    QHash<QString, int> stateIds;
    QHash<QString, int> inputIds;
    QHash<QString, int> outputIds;

    QString error;

    for (const QString& name : data.getStateNames()) {
        const bool isInit = (name == data.getInitialState());
        const int id = m_repo->insertState(variantId, name, isInit, &error);
        if (id < 0) return ServiceResult::failure(error);
        stateIds[name] = id;
    }

    for (const QString& name : data.getInputSignalNames()) {
        const int id = m_repo->insertInput(variantId, name, &error);
        if (id < 0) return ServiceResult::failure(error);
        inputIds[name] = id;
    }

    for (const QString& name : data.getOutputSignalNames()) {
        const int id = m_repo->insertOutput(variantId, name, &error);
        if (id < 0) return ServiceResult::failure(error);
        outputIds[name] = id;
    }

    // 3. Transitions + outputs (зависит от типа)
    ServiceResult contentResult = (data.getType() == VariantType::MooreToMealy)
        ? saveMoore(data, variantId, stateIds, inputIds, outputIds)
        : saveMealy(data, variantId, stateIds, inputIds, outputIds);

    if (!contentResult.ok) return contentResult;

    // 4. Commit
    if (!tx.commit(&error))
        return ServiceResult::failure(
            QStringLiteral("COMMIT не удался: %1").arg(error));

    return ServiceResult::success();
}

ServiceResult VariantService::saveMoore(
    const AutomatonData& data, int variantId,
    const QHash<QString, int>& stateIds,
    const QHash<QString, int>& inputIds,
    const QHash<QString, int>& outputIds) {
    QString error;

    // Переходы: from_state, input → to_state
    for (int inputIdx = 0; inputIdx < data.getInputSignalNames().size(); ++inputIdx) {
        const QString& inputName = data.getInputSignalNames()[inputIdx];

        for (int stateIdx = 0; stateIdx < data.getStateNames().size(); ++stateIdx) {
            const QString& fromStateName = data.getStateNames()[stateIdx];
            const CellData& cell = data.getTransitionTable()[inputIdx][stateIdx];

            std::optional<int> toStateId;
            if (!cell.nextState.isEmpty() && cell.nextState != "—") {
                toStateId = m_repo->findVariant(variantId)->id;  // заглушка
                // на самом деле — ищем state id по имени (см. ниже)
            }

            const int transitionId = m_repo->insertTransition(
                variantId,
                stateIds[fromStateName],
                toStateId,
                inputIds[inputName],
                &error);
            if (transitionId < 0) return ServiceResult::failure(error);
        }
    }

    // Выходы состояний: state → outputs
    for (const QString& stateName : data.getStateNames()) {
        const QStringList outputs = data.getMooreOutputs().value(stateName);
        for (const QString& outName : outputs) {
            if (!m_repo->insertMooreOutput(stateIds[stateName],
                                           outputIds[outName], &error))
                return ServiceResult::failure(error);
        }
    }

    return ServiceResult::success();
}

ServiceResult VariantService::saveMealy(
        const AutomatonData& data,
        int variantId,
        const QHash<QString, int>& stateIds,
        const QHash<QString, int>& inputIds,
        const QHash<QString, int>& outputIds
    ) {
    QString error;

    for (int inputIdx = 0; inputIdx < data.getInputSignalNames().size(); ++inputIdx) {
        const QString& inputName = data.getInputSignalNames()[inputIdx];

        for (int stateIdx = 0; stateIdx < data.getStateNames().size(); ++stateIdx) {
            const QString& fromStateName = data.getStateNames()[stateIdx];
            const CellData& cell = data.getTransitionTable()[inputIdx][stateIdx];

            std::optional<int> toStateId;
            if (!cell.nextState.isEmpty() && cell.nextState != "—") {
                toStateId = stateIds.value(cell.nextState, -1);
                if (*toStateId < 0)
                    return ServiceResult::failure(
                        QStringLiteral("Переход ведёт в несуществующее состояние: %1")
                            .arg(cell.nextState));
            }

            const int transitionId = m_repo->insertTransition(
                variantId,
                stateIds[fromStateName],
                toStateId,
                inputIds[inputName],
                &error);
            if (transitionId < 0) return ServiceResult::failure(error);

            // Выходы перехода
            for (const QString& outName : cell.outputSignals) {
                if (!m_repo->insertMealyOutput(transitionId, outputIds[outName], &error))
                    return ServiceResult::failure(error);
            }
        }
    }

    return ServiceResult::success();
}


std::optional<AutomatonData> VariantService::loadVariant(int variantNumber, ServiceResult* result) {
    std::optional<VariantRow> variant = m_repo->findVariant(variantNumber);

    if (!variant) {
        if (result) *result = ServiceResult::failure("Вариант не найден");
        return std::nullopt;
    }

    // Загружаем общие данные
    AutomatonData data;
    data.setVariantType(variantStringToType(variant->conversion));
    data.setVariantNumber(variantNumber);

    auto states  = m_repo->findStates(variant->id);
    auto inputs  = m_repo->findInputs(variant->id);
    auto outputs = m_repo->findOutputs(variant->id);

    QStringList stateNames, inputNames, outputNames;
    QHash<int, QString> stateNameById;
    QHash<int, QString> inputNameById;
    QHash<int, QString> outputNameById;
    QString initialState;

    for (const auto& s : states) {
        stateNames << s.name;
        stateNameById[s.id] = s.name;
        if (s.isInit) initialState = s.name;
    }
    for (const auto& i : inputs)  { inputNames << i.name; inputNameById[i.id]   = i.name; }
    for (const auto& o : outputs) { outputNames << o.name; outputNameById[o.id] = o.name; }

    data.setStateNames(stateNames);
    data.setInputSignalNames(inputNames);
    data.setOutputSignalNames(outputNames);
    data.setInitialState(initialState);

    // Загружаем переходы и выходы
    if (variantStringToType(variant->conversion) == VariantType::MooreToMealy) {
        auto loaded = loadMoore(variant->id, result);
        if (!loaded) return std::nullopt;
        return loaded;
    } else {
        return loadMealy(variant->id, result);
    }
}


std::optional<AutomatonData> VariantService::loadMoore(
    int variantId, ServiceResult* result) {

    // --- 1. Общие данные варианта ---
    // Нам нужен номер и тип. Прочитаем Variants.
    // Но variantId у нас уже есть — используем отдельный поиск по id.
    QSqlQuery vq = m_db->query(
        QStringLiteral("SELECT number, conversion FROM Variants WHERE id = :id"),
        { {":id", variantId} });

    if (!vq.next()) {
        if (result) *result = ServiceResult::failure(
                QStringLiteral("Вариант с id = %1 не найден").arg(variantId));
        return std::nullopt;
    }

    AutomatonData data;
    data.setVariantType(VariantType::MooreToMealy);
    data.setVariantNumber(vq.value(0).toInt());

    // --- 2. States / Inputs / Outputs ---
    auto states  = m_repo->findStates(variantId);
    auto inputs  = m_repo->findInputs(variantId);
    auto outputs = m_repo->findOutputs(variantId);

    if (states.isEmpty() || inputs.isEmpty()) {
        if (result) *result = ServiceResult::failure(
                "У варианта нет состояний или входных сигналов");
        return std::nullopt;
    }

    // Строим словари id → name
    QHash<int, QString> stateNameById;
    QHash<int, QString> inputNameById;
    QHash<int, QString> outputNameById;

    QStringList stateNames;
    QStringList inputNames;
    QStringList outputNames;
    QString initialState;

    for (const auto& s : states) {
        stateNames << s.name;
        stateNameById[s.id] = s.name;
        if (s.isInit) initialState = s.name;
    }
    for (const auto& i : inputs) {
        inputNames << i.name;
        inputNameById[i.id] = i.name;
    }
    for (const auto& o : outputs) {
        outputNames << o.name;
        outputNameById[o.id] = o.name;
    }

    // Записываем в модель — сеттеры сами пересоберут таблицы
    data.setStateNames(stateNames);
    data.setInputSignalNames(inputNames);
    data.setOutputSignalNames(outputNames);
    if (!initialState.isEmpty())
        data.setInitialState(initialState);

    // --- 3. Transition table ---
    auto transitions = m_repo->findTransitions(variantId);

    // Строим карту (inputName → index) и (stateName → index)
    QHash<QString, int> inputIndex;
    QHash<QString, int> stateIndex;
    for (int i = 0; i < inputNames.size();  ++i) inputIndex[inputNames[i]]   = i;
    for (int i = 0; i < stateNames.size(); ++i) stateIndex[stateNames[i]] = i;

    for (const auto& t : transitions) {
        const QString fromName  = stateNameById.value(t.fromStateId);
        const QString inputName = inputNameById.value(t.inputSignalId);

        if (fromName.isEmpty() || inputName.isEmpty())
            continue;  // битая строка — пропускаем

        const int inputIdx = inputIndex.value(inputName, -1);
        const int stateIdx = stateIndex.value(fromName, -1);
        if (inputIdx < 0 || stateIdx < 0) continue;

        QString nextStateName;
        if (t.toStateId.has_value())
            nextStateName = stateNameById.value(*t.toStateId);

        // Пустая строка = "—" (нет перехода)
        const QString text = nextStateName.isEmpty()
                                 ? QStringLiteral("—")
                                 : nextStateName;

        // Сеттер ячейки сам проверит и запишет
        data.setTransitionCell(
            inputIdx + 2,     // rowOffset для Мура = 2
            stateIdx + 1,     // col = stateIdx + 1
            text);
    }

    // --- 4. Moore outputs ---
    auto mooreOutputs = m_repo->findMooreOutputs(variantId);

    // Группируем: state_id → список output_id
    QHash<int, QList<int>> outputsByState;
    for (const auto& row : mooreOutputs)
        outputsByState[row.stateId].append(row.outputId);

    for (auto it = outputsByState.constBegin();
         it != outputsByState.constEnd(); ++it) {
        const QString stateName = stateNameById.value(it.key());
        if (stateName.isEmpty()) continue;

        QStringList outputNamesForState;
        for (int outputId : it.value()) {
            const QString outName = outputNameById.value(outputId);
            if (!outName.isEmpty())
                outputNamesForState.append(outName);
        }

        // Строка 0 — выходы Мура
        const int stateIdx = stateIndex.value(stateName, -1);
        if (stateIdx < 0) continue;

        data.setMooreOutputCell(stateIdx + 1,
                                outputNamesForState.join(", "));
    }

    if (result) *result = ServiceResult::success();
    return data;
}


std::optional<AutomatonData> VariantService::loadMealy(
    int variantId, ServiceResult* result) {

    // --- 1. Общие данные ---
    QSqlQuery vq = m_db->query(
        QStringLiteral("SELECT number FROM Variants WHERE id = :id"),
        { {":id", variantId} });

    if (!vq.next()) {
        if (result) *result = ServiceResult::failure(
                QStringLiteral("Вариант с id = %1 не найден").arg(variantId));
        return std::nullopt;
    }

    AutomatonData data;
    data.setVariantType(VariantType::MealyToMoore);
    data.setVariantNumber(vq.value(0).toInt());

    // --- 2. States / Inputs / Outputs ---
    auto states  = m_repo->findStates(variantId);
    auto inputs  = m_repo->findInputs(variantId);
    auto outputs = m_repo->findOutputs(variantId);

    if (states.isEmpty() || inputs.isEmpty()) {
        if (result) *result = ServiceResult::failure(
                "У варианта нет состояний или входных сигналов");
        return std::nullopt;
    }

    QHash<int, QString> stateNameById;
    QHash<int, QString> inputNameById;
    QHash<int, QString> outputNameById;

    QStringList stateNames, inputNames, outputNames;
    QString initialState;

    for (const auto& s : states) {
        stateNames << s.name;
        stateNameById[s.id] = s.name;
        if (s.isInit) initialState = s.name;
    }
    for (const auto& i : inputs)  { inputNames  << i.name; inputNameById[i.id]   = i.name; }
    for (const auto& o : outputs) { outputNames << o.name; outputNameById[o.id] = o.name; }

    data.setStateNames(stateNames);
    data.setInputSignalNames(inputNames);
    data.setOutputSignalNames(outputNames);
    if (!initialState.isEmpty())
        data.setInitialState(initialState);

    // --- 3. Transitions + Mealy outputs ---
    auto transitions = m_repo->findTransitions(variantId);

    for (const auto& t : transitions) {
        const QString fromName  = stateNameById.value(t.fromStateId);
        const QString inputName = inputNameById.value(t.inputSignalId);

        if (fromName.isEmpty() || inputName.isEmpty()) continue;

        // Целевое состояние
        QString nextStateName;
        if (t.toStateId.has_value())
            nextStateName = stateNameById.value(*t.toStateId);
        if (nextStateName.isEmpty())
            nextStateName = QStringLiteral("—");

        // Выходы этого перехода
        auto mealyOutputs = m_repo->findMealyOutputs(t.id);
        QStringList outputNamesForTransition;
        for (const auto& row : mealyOutputs) {
            const QString outName = outputNameById.value(row.outputId);
            if (!outName.isEmpty())
                outputNamesForTransition.append(outName);
        }

        // Собираем текст ячейки: "state / out1, out2"
        const QString outputsText = outputNamesForTransition.isEmpty()
                                        ? QStringLiteral("—")
                                        : outputNamesForTransition.join(", ");
        const QString cellText = QStringLiteral("%1 / %2")
                                     .arg(nextStateName, outputsText);

        data.setTransitionCellByName(inputName, fromName, cellText);
    }

    if (result) *result = ServiceResult::success();
    return data;
}