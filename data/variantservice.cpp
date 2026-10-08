#include "variantservice.h"

#include "data/database.h"
#include "data/transaction.h"
#include "core/automatondata.h"

VariantService::VariantService(Database* db, VariantRepository* repo, QObject* parent)
    : QObject(parent)
    , m_repo(repo)
    , m_db(db)
{}


Result VariantService::saveVariant(const AutomatonData& data) {
    const QString conversion = variantTypeToString(data.getType());

    Result findResult = m_repo->findVariant(data.getVariantNumber());
    if (!findResult.ok()) return findResult;

    Transaction tx(m_db);   // BEGIN

    if (findResult.hasPayload()) {
        return Result::error(
            QString("The option with the number '%1' already exists.")
            .arg(data.getVariantNumber()));
    }

    Result insertResult = m_repo->insertVariant(data.getVariantNumber(), conversion);
    if (!insertResult.ok()) return insertResult;

    int variantId = insertResult.payloadAs<int>();

    // Save States, Inputs, Outputs
    QHash<QString, int> stateIds;
    QHash<QString, int> inputIds;
    QHash<QString, int> outputIds;

    Result r = insertStates(variantId, data.getStateNames(),
        data.getInitialState(), stateIds);
    if (!r.ok()) return r;

    r = insertInputs(variantId, data.getInputSignalNames(), inputIds);
    if (!r.ok()) return r;

    r = insertOutputs(variantId, data.getOutputSignalNames(), outputIds);
    if (!r.ok()) return r;

    // 3. save transitions and outputs
    Result contentResult = (data.getType() == VariantType::MooreToMealy)
       ? saveMoore(data, variantId, stateIds, inputIds, outputIds)
       : saveMealy(data, variantId, stateIds, inputIds, outputIds);
    if (!contentResult.ok()) return contentResult;

    // 4. COMMIT
    if (!tx.commit()) {
        return Result::error(
            QString("Failed to commit transaction"),
            ResultCategory::Database);
    }

    return Result::success(
        QString("Variant №%1 saved successfully").arg(data.getVariantNumber()));

}

Result VariantService::removeVariant(int variantNumber) {
    Result existing = m_repo->findVariant(variantNumber);
    VariantRow variant = existing.payloadAs<VariantRow>();

    Transaction tx(m_db);

    if (existing.hasPayload()) {
        Result clearResult = m_repo->removeVariant(variant.id);
        if (!clearResult.ok()) return clearResult;
    }

    if (!tx.commit()) {
        return Result::error(
            QString("Failed to commit remove variant"),
            ResultCategory::Database);
    }

    return Result::success(
        QString("Variant №%1 deleted successfully").arg(variant.number));
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

    return (variantTypeStringToType(variant.conversion) == VariantType::MooreToMealy)
       ? loadMoore(variant.id, variantNumber)
       : loadMealy(variant.id, variantNumber);
}

Result VariantService::isExist(int variantNumber) {
    Result findResult = m_repo->findVariant(variantNumber);
    if (!findResult.ok()) {
        return findResult;
    }

    const bool exists = findResult.hasPayload();
    return Result::success().withPayload(exists);
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
    return Result::success();
}

Result VariantService::insertInputs(int variantId,
    const QStringList& names,
    QHash<QString, int>& inputIds) {
    for (const QString& name : names) {
        Result r = m_repo->insertInput(variantId, name);
        if (!r.ok()) return r;
        inputIds[name] = r.payloadAs<int>();
    }
    return Result::success();
}

Result VariantService::insertOutputs(int variantId,
    const QStringList& names,
    QHash<QString, int>& outputIds) {
    for (const QString& name : names) {
        Result r = m_repo->insertOutput(variantId, name);
        if (!r.ok()) return r;
        outputIds[name] = r.payloadAs<int>();
    }
    return Result::success();
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
                    .withOffender(outName);
            }

            Result r = m_repo->insertMooreOutput(
                stateIds.value(stateName),
                outputIds.value(outName));
            if (!r.ok()) return r;
        }
    }

    return Result::success();
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
                        .withOffender(outName);
                }

                Result r = m_repo->insertMealyOutput(
                    transitionId,
                    outputIds.value(outName));
                if (!r.ok()) return r;
            }
        }
    }

    return Result::success();
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

    // state_id → список output_id
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