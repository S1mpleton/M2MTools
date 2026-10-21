#include "variantservice.h"

#include "data/database.h"
#include "data/transaction.h"
#include "core/automatondata.h"
#include "core/variantarchive.h"

#include <QSet>
#include <algorithm>

VariantService::VariantService(Database* db, VariantRepository* repo, QObject* parent)
    : QObject(parent)
    , m_repo(repo)
    , m_db(db)
{}


Result VariantService::saveVariant(const AutomatonData& data) {
    Result findResult = m_repo->findVariant(data.getVariantNumber());
    if (!findResult.ok()) return findResult;

    Transaction tx(m_db);   // BEGIN

    if (findResult.hasPayload()) {
        return Result::error(
                   QStringLiteral("Variant №%1 already exists").arg(data.getVariantNumber()),
                   ResultCategory::Service)
            .withCode(ErrorCode::VariantAlreadyExists)
            .withOffender(QString::number(data.getVariantNumber()))
            .withDetails(findResult.details());
    }

    Result insertResult = insertVariantData(data);
    if (!insertResult.ok()) return insertResult;

    // 4. COMMIT
    if (!tx.commit()) {
        return Result::error(
            QString("Failed to commit transaction"),
            ResultCategory::Database);
    }

    return Result::success(
        QString("Variant №%1 saved successfully").arg(data.getVariantNumber()));

}

Result VariantService::replaceVariant(const AutomatonData& data) {
    Result existing = m_repo->findVariant(data.getVariantNumber());
    if (!existing.ok()) return existing;

    Transaction transaction(m_db);
    if (existing.hasPayload()) {
        Result removeResult =
            m_repo->removeVariant(existing.payloadAs<VariantRow>().id);
        if (!removeResult.ok()) return removeResult;
    }
    Result insertResult = insertVariantData(data);
    if (!insertResult.ok()) return insertResult;

    QString error;
    if (!transaction.commit(&error)) {
        return Result::error(
            QStringLiteral("Failed to commit variant replacement"),
            ResultCategory::Database)
            .withCode(ErrorCode::DatabaseCommitFailed)
            .withDetails(error);
    }
    return Result::success(
        QStringLiteral("Variant №%1 replaced successfully")
            .arg(data.getVariantNumber()));
}

Result VariantService::insertVariantData(const AutomatonData& data) {
    const QString conversion = variantTypeToString(data.getType());
    Result insertResult = m_repo->insertVariant(
        data.getVariantNumber(), conversion,
        data.getDiagramImage(), data.getDiagramFileName());
    if (!insertResult.ok()) return insertResult;

    const int variantId = insertResult.payloadAs<int>();
    QHash<QString, int> stateIds;
    QHash<QString, int> inputIds;
    QHash<QString, int> outputIds;

    Result result = insertStates(
        variantId, data.getStateNames(), data.getInitialState(), stateIds);
    if (!result.ok()) return result;
    result = insertInputs(
        variantId, data.getInputSignalNames(), inputIds);
    if (!result.ok()) return result;
    result = insertOutputs(
        variantId, data.getOutputSignalNames(), outputIds);
    if (!result.ok()) return result;

    return data.getType() == VariantType::MooreToMealy
        ? saveMoore(data, variantId, stateIds, inputIds, outputIds)
        : saveMealy(data, variantId, stateIds, inputIds, outputIds);
}

Result VariantService::removeVariant(int variantNumber) {
    Result existing = m_repo->findVariant(variantNumber);
    if (!existing.ok()) return existing;
    if (!existing.hasPayload()) {
        return Result::warning(
            QStringLiteral("Variant №%1 not found").arg(variantNumber),
            ResultCategory::Service)
            .withCode(ErrorCode::VariantNotFound)
            .withOffender(QString::number(variantNumber));
    }
    VariantRow variant = existing.payloadAs<VariantRow>();

    Transaction tx(m_db);

    if (existing.hasPayload()) {
        Result clearResult = m_repo->removeVariant(variant.id);
        if (!clearResult.ok()) return clearResult;
    }

    if (!tx.commit()) {
        return Result::error(
                QString("Failed to commit remove variant"),
                ResultCategory::Database)
            .withCode(ErrorCode::DatabaseCommitFailed);
    }

    return Result::success(
        QString("Variant №%1 deleted successfully").arg(variant.number));
}

Result VariantService::swapVariants(int firstNumber, int secondNumber) {
    if (firstNumber <= 0 || secondNumber <= 0) {
        return Result::error(QStringLiteral("Variant numbers must be positive"),
                             ResultCategory::Validation);
    }
    if (firstNumber == secondNumber)
        return Result::success(QStringLiteral("Variant numbers are equal"));

    Result firstResult = m_repo->findVariant(firstNumber);
    if (!firstResult.ok()) return firstResult;
    Result secondResult = m_repo->findVariant(secondNumber);
    if (!secondResult.ok()) return secondResult;
    if (!firstResult.hasPayload() && !secondResult.hasPayload()) {
        return Result::error(
            QStringLiteral("Neither variant №%1 nor №%2 exists")
                .arg(firstNumber).arg(secondNumber),
            ResultCategory::Service)
            .withCode(ErrorCode::VariantNotFound);
    }

    Transaction tx(m_db);
    if (firstResult.hasPayload() && secondResult.hasPayload()) {
        const VariantRow first = firstResult.payloadAs<VariantRow>();
        const VariantRow second = secondResult.payloadAs<VariantRow>();

        int temporaryNumber = -1;
        while (m_repo->findVariant(temporaryNumber).hasPayload())
            --temporaryNumber;

        Result r = m_repo->updateVariantNumber(first.id, temporaryNumber);
        if (!r.ok()) return r;
        r = m_repo->updateVariantNumber(second.id, firstNumber);
        if (!r.ok()) return r;
        r = m_repo->updateVariantNumber(first.id, secondNumber);
        if (!r.ok()) return r;
    } else {
        const VariantRow existing = firstResult.hasPayload()
            ? firstResult.payloadAs<VariantRow>()
            : secondResult.payloadAs<VariantRow>();
        const int target = firstResult.hasPayload() ? secondNumber : firstNumber;
        Result r = m_repo->updateVariantNumber(existing.id, target);
        if (!r.ok()) return r;
    }

    QString error;
    if (!tx.commit(&error)) {
        return Result::error(QStringLiteral("Failed to commit variant swap"),
                             ResultCategory::Database)
            .withCode(ErrorCode::DatabaseCommitFailed)
            .withDetails(error);
    }
    return Result::success(
        QStringLiteral("Variants №%1 and №%2 swapped")
            .arg(firstNumber).arg(secondNumber));
}

Result VariantService::compactVariantNumbers() {
    Result allResult = m_repo->findAllVariants();
    if (!allResult.ok()) return allResult;
    QList<VariantRow> variants = allResult.payloadAs<QList<VariantRow>>();
    std::sort(variants.begin(), variants.end(),
              [](const VariantRow& left, const VariantRow& right) {
                  return left.number < right.number;
              });

    bool needsCompaction = false;
    for (int i = 0; i < variants.size(); ++i) {
        if (variants[i].number != i + 1) {
            needsCompaction = true;
            break;
        }
    }
    if (!needsCompaction)
        return Result::success(QStringLiteral("Variant numbers are already compact"));

    Transaction tx(m_db);
    for (int i = 0; i < variants.size(); ++i) {
        Result result = m_repo->updateVariantNumber(
            variants[i].id, -1000000 - i);
        if (!result.ok()) return result;
    }
    for (int i = 0; i < variants.size(); ++i) {
        Result result = m_repo->updateVariantNumber(
            variants[i].id, i + 1);
        if (!result.ok()) return result;
    }

    QString error;
    if (!tx.commit(&error)) {
        return Result::error(
            QStringLiteral("Failed to commit variant renumbering"),
            ResultCategory::Database)
            .withCode(ErrorCode::DatabaseCommitFailed)
            .withDetails(error);
    }
    return Result::success(QStringLiteral("Variant numbers compacted"));
}

Result VariantService::exportArchive(
    const QString& path, const QString& password) {
    Result allResult = m_repo->findAllVariants();
    if (!allResult.ok()) return allResult;

    QList<AutomatonData> variants;
    for (const VariantRow& row :
         allResult.payloadAs<QList<VariantRow>>()) {
        Result loadResult = loadVariant(row.number);
        if (!loadResult.ok() || !loadResult.hasPayload())
            return loadResult;
        AutomatonData data = loadResult.payloadAs<AutomatonData>();
        if (!data.hasDiagramImage()) {
            return Result::error(
                QStringLiteral(
                    "Variant #%1 is incomplete: the image is missing")
                    .arg(row.number),
                ResultCategory::Validation)
                .withCode(ErrorCode::VariantMissingImage)
                .withOffender(QString::number(row.number));
        }
        Result validation = data.validate();
        if (!validation.ok()) {
            return Result::error(
                QStringLiteral("Variant #%1 is incomplete: %2")
                    .arg(row.number).arg(validation.message()),
                ResultCategory::Validation)
                .withCode(validation.code())
                .withOffender(validation.offender())
                .withDetails(validation.details());
        }
        variants.append(data);
    }
    return VariantArchive::save(path, variants, password);
}

Result VariantService::importArchive(
    const QString& path, const QString& password) {
    QList<AutomatonData> variants;
    Result loadResult =
        VariantArchive::load(path, password, &variants);
    if (!loadResult.ok()) return loadResult;

    Transaction transaction(m_db);
    Result allResult = m_repo->findAllVariants();
    if (!allResult.ok()) return allResult;
    for (const VariantRow& row :
         allResult.payloadAs<QList<VariantRow>>()) {
        Result removeResult = m_repo->removeVariant(row.id);
        if (!removeResult.ok()) return removeResult;
    }
    for (const AutomatonData& data : variants) {
        Result insertResult = insertVariantData(data);
        if (!insertResult.ok()) return insertResult;
    }

    QString error;
    if (!transaction.commit(&error)) {
        return Result::error(
            QStringLiteral("Failed to apply the BazaVar archive"),
            ResultCategory::Database)
            .withCode(ErrorCode::DatabaseCommitFailed)
            .withDetails(error);
    }
    return Result::success(
        QStringLiteral("Imported %1 variants from the archive").arg(variants.size()));
}

Result VariantService::loadVariant(int variantNumber) {
    Result findResult = m_repo->findVariant(variantNumber);
    if (!findResult.ok()) return findResult;

    if (!findResult.hasPayload()) {
        return Result::warning(
            QString("Variant №%1 not found").arg(variantNumber),
            ResultCategory::Service);
    }

    const VariantRow variant = findResult.payloadAs<VariantRow>();

    Result loaded = (variantTypeStringToType(variant.conversion) == VariantType::MooreToMealy)
       ? loadMoore(variant.id, variantNumber)
       : loadMealy(variant.id, variantNumber);
    if (!loaded.ok() || !loaded.hasPayload()) return loaded;

    AutomatonData data = loaded.payloadAs<AutomatonData>();
    if (!variant.imageData.isEmpty())
        data.setDiagramImage(variant.imageData, variant.imageName);
    return Result::success(
        QStringLiteral("Variant №%1 loaded").arg(variantNumber))
        .withPayload(data);
}

Result VariantService::isExist(int variantNumber) {
    Result findResult = m_repo->findVariant(variantNumber);
    if (!findResult.ok()) {
        return findResult;
    }

    const bool exists = findResult.hasPayload();
    return Result::success(QStringLiteral("Variant #%1 existence check completed: %2")
                               .arg(variantNumber)
                               .arg(exists ? QStringLiteral("exists")
                                           : QStringLiteral("not found")))
        .withPayload(exists);
}

Result VariantService::findAllVariants() const {
    return m_repo->findAllVariants();
}

int VariantService::firstAvailableVariantNumber() const {
    Result result = m_repo->findAllVariants();
    if (!result.ok() || !result.hasPayload()) return 1;

    QSet<int> used;
    for (const VariantRow& row : result.payloadAs<QList<VariantRow>>())
        if (row.number > 0) used.insert(row.number);
    int number = 1;
    while (used.contains(number)) ++number;
    return number;
}


Result VariantService::insertStates(int variantId,
                                    const QStringList& names,
                                    const QString& initialState,
                                    QHash<QString, int>& stateIds) {
    for (const QString& name : names) {
        const bool isInit = (name == initialState);
        Result r = m_repo->insertState(variantId, name, isInit);
        if (!r.ok()) return r;
        stateIds[name] = r.payloadAs<int>();
    }
    return Result::success(QStringLiteral("Inserted %1 states for variant id=%2")
                               .arg(names.size()).arg(variantId));
}

Result VariantService::insertInputs(int variantId, const QStringList& names, QHash<QString, int>& inputIds) {
    for (const QString& name : names) {
        Result r = m_repo->insertInput(variantId, name);
        if (!r.ok()) return r;
        inputIds[name] = r.payloadAs<int>();
    }
    return Result::success(QStringLiteral("Inserted %1 inputs for variant id=%2")
                               .arg(names.size()).arg(variantId));
}

Result VariantService::insertOutputs(int variantId,
    const QStringList& names,
    QHash<QString, int>& outputIds) {
    for (const QString& name : names) {
        Result r = m_repo->insertOutput(variantId, name);
        if (!r.ok()) return r;
        outputIds[name] = r.payloadAs<int>();
    }
    return Result::success(QStringLiteral("Inserted %1 outputs for variant id=%2")
                               .arg(names.size()).arg(variantId));
}

Result VariantService::saveMoore(
    const AutomatonData& data, int variantId,
    const QHash<QString, int>& stateIds,
    const QHash<QString, int>& inputIds,
    const QHash<QString, int>& outputIds) {
    const QStringList& stateNames = data.getStateNames();
    const QStringList& inputNames = data.getInputSignalNames();
    const auto& table = data.getTransitionTable();

    // 1. transition
    for (int i = 0; i < inputNames.size(); ++i) {
        const QString& inputName = inputNames[i];

        for (int j = 0; j < stateNames.size(); ++j) {
            const QString& fromName = stateNames[j];
            const CellData& cell = table[i][j];

            std::optional<int> toStateId;
            if (!cell.nextState.isEmpty() && cell.nextState != "—") {
                if (!stateIds.contains(cell.nextState)) {
                    return Result::error(
                                QStringLiteral("Transition points to unknown state '%1'")
                                   .arg(cell.nextState),
                                ResultCategory::Validation)
                        .withCode(ErrorCode::VariantUnknownState)
                        .withOffender(cell.nextState);
                }
                toStateId = stateIds.value(cell.nextState);
            }

            Result r = m_repo->insertTransition(
                variantId,
                stateIds.value(fromName),
                toStateId,
                inputIds.value(inputName));
            if (!r.ok()) return r;
        }
    }

    // 2. output states
    for (const QString& stateName : stateNames) {
        const QStringList outputs = data.getMooreOutputs().value(stateName);

        for (const QString& outName : outputs) {
            if (!outputIds.contains(outName)) {
                return Result::error(
                           QStringLiteral("Unknown output signal '%1'").arg(outName),
                           ResultCategory::Validation)
                    .withCode(ErrorCode::VariantUnknownOutput)
                    .withOffender(outName);
            }

            Result r = m_repo->insertMooreOutput(
                stateIds.value(stateName),
                outputIds.value(outName));
            if (!r.ok()) return r;
        }
    }

    return Result::success(QStringLiteral("Saved Moore data for variant id=%1")
                               .arg(variantId));
}

Result VariantService::saveMealy(
        const AutomatonData& data,
        int variantId,
        const QHash<QString, int>& stateIds,
        const QHash<QString, int>& inputIds,
        const QHash<QString, int>& outputIds
    ) {
    const QStringList& stateNames = data.getStateNames();
    const QStringList& inputNames = data.getInputSignalNames();
    const auto& table = data.getTransitionTable();

    for (int i = 0; i < inputNames.size(); ++i) {
        const QString& inputName = inputNames[i];

        for (int j = 0; j < stateNames.size(); ++j) {
            const QString& fromName = stateNames[j];
            const CellData& cell = table[i][j];

            std::optional<int> toStateId;
            if (!cell.nextState.isEmpty() && cell.nextState != "—") {
                if (!stateIds.contains(cell.nextState)) {
                    return Result::error(
                               QStringLiteral("Transition points to unknown state '%1'")
                                   .arg(cell.nextState),
                               ResultCategory::Validation)
                        .withCode(ErrorCode::VariantUnknownState)
                        .withOffender(cell.nextState);
                }
                toStateId = stateIds.value(cell.nextState);
            }

            Result transResult = m_repo->insertTransition(
                variantId,
                stateIds.value(fromName),
                toStateId,
                inputIds.value(inputName));
            if (!transResult.ok()) return transResult;

            const int transitionId = transResult.payloadAs<int>();

            // Outputs transition
            for (const QString& outName : cell.outputSignals) {
                if (!outputIds.contains(outName)) {
                    return Result::error(
                               QStringLiteral("Unknown output signal '%1'").arg(outName),
                               ResultCategory::Validation)
                        .withCode(ErrorCode::VariantUnknownOutput)
                        .withOffender(outName);
                }

                Result r = m_repo->insertMealyOutput(
                    transitionId,
                    outputIds.value(outName));
                if (!r.ok()) return r;
            }
        }
    }

    return Result::success(QStringLiteral("Saved Mealy data for variant id=%1")
                               .arg(variantId));
}


Result VariantService::loadMoore(int variantId, int variantNumber) {
    Result findResult = m_repo->findVariant(variantNumber);
    if (!findResult.ok()) return findResult;

    // 1. Lists
    Result statesResult  = m_repo->findStates(variantId);
    if (!statesResult.ok()) return statesResult;

    Result inputsResult  = m_repo->findInputs(variantId);
    if (!inputsResult.ok()) return inputsResult;

    Result outputsResult = m_repo->findOutputs(variantId);
    if (!outputsResult.ok()) return outputsResult;

    const QList<StateRow> states = statesResult.payloadAs<QList<StateRow>>();
    const QList<InputRow> inputs = inputsResult.payloadAs<QList<InputRow>>();
    const QList<OutputRow> outputs = outputsResult.payloadAs<QList<OutputRow>>();

    if (states.isEmpty() || inputs.isEmpty()) {
        return Result::error(
            QStringLiteral("Variant №%1 has no states or inputs")
                .arg(variantNumber),
            ResultCategory::Service);
    }

    // 2. Create automaton data
    AutomatonData data;
    data.setVariantType(VariantType::MooreToMealy);
    data.setVariantNumber(variantNumber);

    QStringList stateNames, inputNames, outputNames;
    QHash<int, QString> stateNameById, inputNameById, outputNameById;
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

    Result r = data.setStateNames(stateNames);
    if (!r.ok()) return r;

    r = data.setInputSignalNames(inputNames);
    if (!r.ok()) return r;

    r = data.setOutputSignalNames(outputNames);
    if (!r.ok()) return r;

    if (!initialState.isEmpty()) {
        r = data.setInitialState(initialState);
        if (!r.ok()) return r;
    }

    // 3. transitions
    Result transResult = m_repo->findTransitions(variantId);
    if (!transResult.ok()) return transResult;

    const QList<TransitionRow> transitions =
        transResult.payloadAs<QList<TransitionRow>>();

    for (const auto& t : transitions) {
        const QString fromName  = stateNameById.value(t.fromStateId);
        const QString inputName = inputNameById.value(t.inputSignalId);

        if (fromName.isEmpty() || inputName.isEmpty()) {
            return Result::error(
                QStringLiteral("Broken transition id=%1: state or input missing")
                    .arg(t.id),
                ResultCategory::Database);
        }

        QString nextStateName;
        if (t.toStateId.has_value())
            nextStateName = stateNameById.value(*t.toStateId);

        const QString cellText = nextStateName.isEmpty()
            ? QStringLiteral("—")
            : nextStateName;

        Result cellResult = data.setTransitionCellByName(inputName,
            fromName,
            cellText);
        if (!cellResult.ok()) return cellResult;
    }

    // 4. Outputs state (Moore)
    Result mooreResult = m_repo->findMooreOutputs(variantId);
    if (!mooreResult.ok()) return mooreResult;

    const QList<MooreStateOutputRow> mooreOutputs =
        mooreResult.payloadAs<QList<MooreStateOutputRow>>();

    // state_id to output_id list
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

        Result outResult = data.setMooreOutputCellByName(
            stateName,
            outputNamesForState.join(", "));
        if (!outResult.ok()) return outResult;
    }

    return Result::success(
               QStringLiteral("Variant №%1 loaded").arg(variantNumber))
        .withPayload(data);
}

Result VariantService::loadMealy(int variantId, int variantNumber) {
    Result statesResult  = m_repo->findStates(variantId);
    if (!statesResult.ok()) return statesResult;

    Result inputsResult  = m_repo->findInputs(variantId);
    if (!inputsResult.ok()) return inputsResult;

    Result outputsResult = m_repo->findOutputs(variantId);
    if (!outputsResult.ok()) return outputsResult;

    const auto states  = statesResult.payloadAs<QList<StateRow>>();
    const auto inputs  = inputsResult.payloadAs<QList<InputRow>>();
    const auto outputs = outputsResult.payloadAs<QList<OutputRow>>();

    if (states.isEmpty() || inputs.isEmpty()) {
        return Result::error(
            QStringLiteral("Variant №%1 has no states or inputs")
                .arg(variantNumber),
            ResultCategory::Service);
    }

    AutomatonData data;
    data.setVariantType(VariantType::MealyToMoore);
    data.setVariantNumber(variantNumber);

    QStringList stateNames, inputNames, outputNames;
    QHash<int, QString> stateNameById, inputNameById, outputNameById;
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

    Result r = data.setStateNames(stateNames);
    if (!r.ok()) return r;
    r = data.setInputSignalNames(inputNames);
    if (!r.ok()) return r;
    r = data.setOutputSignalNames(outputNames);
    if (!r.ok()) return r;
    if (!initialState.isEmpty()) {
        r = data.setInitialState(initialState);
        if (!r.ok()) return r;
    }

    Result transResult = m_repo->findTransitions(variantId);
    if (!transResult.ok()) return transResult;
    const auto transitions = transResult.payloadAs<QList<TransitionRow>>();

    Result mealyResult = m_repo->findMealyOutputsByVariant(variantId);
    if (!mealyResult.ok()) return mealyResult;
    const auto outputsByTransition =
        mealyResult.payloadAs<QHash<int, QList<int>>>();

    for (const auto& t : transitions) {
        const QString fromName  = stateNameById.value(t.fromStateId);
        const QString inputName = inputNameById.value(t.inputSignalId);

        if (fromName.isEmpty() || inputName.isEmpty()) {
            return Result::error(
                QStringLiteral("Broken transition id=%1").arg(t.id),
                ResultCategory::Database);
        }

        QString nextStateName;
        if (t.toStateId.has_value())
            nextStateName = stateNameById.value(*t.toStateId);
        if (nextStateName.isEmpty())
            nextStateName = QStringLiteral("—");

        QStringList outputsForTransition;
        const QList<int> outputIds = outputsByTransition.value(t.id);
        for (int outputId : outputIds) {
            const QString outName = outputNameById.value(outputId);
            if (!outName.isEmpty())
                outputsForTransition.append(outName);
        }

        const QString outputsText = outputsForTransition.isEmpty()
                                        ? QStringLiteral("—")
                                        : outputsForTransition.join(", ");
        const QString cellText = QStringLiteral("%1 / %2")
                                     .arg(nextStateName, outputsText);

        Result cellResult = data.setTransitionCellByName(inputName,
                                                         fromName,
                                                         cellText);
        if (!cellResult.ok()) return cellResult;
    }

    return Result::success(
               QStringLiteral("Variant №%1 loaded").arg(variantNumber))
        .withPayload(data);
}
