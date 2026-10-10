#include "automatondata.h"
#include "namelistparser.h"
#include "infrastructure/logger.h"

#include <QSet>
#include <QQueue>
#include <QRegularExpression>
#include <QJsonArray>
#include <QCoreApplication>

// FieldType
FieldType fieldTypeStringToType(QString fieldTypeString) {
    if (fieldTypeString == QString("State")) return FieldType::State;
    if (fieldTypeString == QString("Input")) return FieldType::Input;
    if (fieldTypeString == QString("Output")) return FieldType::Output;

    Logger::error(QString("function 'fieldTypeStringToType'. Do not defined name '%1'")
        .arg(fieldTypeString));

    return FieldType::State;
}

QString fieldTypeToString(FieldType fieldType) {
    switch (fieldType) {
        case FieldType::State: return QStringLiteral("State");
        case FieldType::Input: return QStringLiteral("Input");
        case FieldType::Output: return QStringLiteral("Output");
    }

    Logger::error(QString("function 'fieldTypeToString'. Do not defined field"));
    return QStringLiteral("State");
}

QString fieldTypeDisplayName(FieldType fieldType) {
    switch (fieldType) {
        case FieldType::State: return QCoreApplication::translate("FieldTypeDisplay", "State");
        case FieldType::Input: return QCoreApplication::translate("FieldTypeDisplay", "Input");
        case FieldType::Output: return QCoreApplication::translate("FieldTypeDisplay", "Output");
    }

    Logger::error(QString("function 'fieldTypeDisplayName'. Do not defined field"));
    return QStringLiteral("State");
}

// VariantType
VariantType variantTypeStringToType(QString variantTypeString) {
    if (variantTypeString == QString("MealyToMoore")) return VariantType::MealyToMoore;
    if (variantTypeString == QString("MooreToMealy")) return VariantType::MooreToMealy;

    Logger::error(QString("function 'variantTypeStringToType'. Do not defined name '%1'")
        .arg(variantTypeString));

    return VariantType::MealyToMoore;
}

QString variantTypeToString(VariantType variantType) {
    switch (variantType) {
        case VariantType::MealyToMoore: return QStringLiteral("MealyToMoore");
        case VariantType::MooreToMealy: return QStringLiteral("MooreToMealy");
    }

    Logger::error(QString("function 'variantTypeToString'. Do not defined field"));
    return QStringLiteral("MealyToMoore");
}

QString variantTypeDisplayName(VariantType variantType) {
    switch (variantType) {
        case VariantType::MealyToMoore: return QStringLiteral("MealyToMoore");
        case VariantType::MooreToMealy: return QStringLiteral("MooreToMealy");
    }

    Logger::error(QString("function 'variantTypeDisplayName'. Do not defined field"));
    return QStringLiteral("MealyToMoore");
}

// AutomatonData
AutomatonData::AutomatonData() {
    setStateUsageChecker([](const AutomatonData& d) {
        return d.validateStateUsageLenient();
    });
}

CellKind AutomatonData::cellKind(int row, int col) const  {
    const int stateCount = m_stateNames.size();
    const int inputCount = m_inputSignalNames.size();

    if (m_type == VariantType::MooreToMealy) {
        if (row == 0 && col == 0) return CellKind::Empty;
        if (row == 1 && col == 0) return CellKind::Empty;
        if (row == 0 && col >= 1 && col <= stateCount)
            return CellKind::MooreOutput;
        if (row == 1 && col >= 1 && col <= stateCount)
            return CellKind::StateHeader;
        if (col == 0 && row >= 2 && row < 2 + inputCount)
            return CellKind::InputHeader;
        if (row >= 2 && row < 2 + inputCount
            && col >= 1 && col <= stateCount)
            return CellKind::Transition;
        return CellKind::Empty;
    } else {
        if (row == 0 && col == 0) return CellKind::Empty;
        if (row == 0 && col >= 1 && col <= stateCount)
            return CellKind::StateHeader;
        if (col == 0 && row >= 1 && row < 1 + inputCount)
            return CellKind::InputHeader;
        if (row >= 1 && row < 1 + inputCount
            && col >= 1 && col <= stateCount)
            return CellKind::Transition;
        return CellKind::Empty;
    }

}

Result AutomatonData::setVariantNumber(int n){
    m_variantNumber = n;

    if (n < 0) {
        return Result::error(
                    QStringLiteral("Variant number cannot be negative (%1)")
                        .arg(n),
                    ResultCategory::Validation)
            .withCode(ErrorCode::ADInvalidVariantNumber);
    }

    return Result::success();

}

Result AutomatonData::setVariantType(VariantType t){
    m_type = t;
    return Result::success();
}

// Set name lists
Result AutomatonData::setStateNames(const QStringList& names) {
    Result validNames = validateNameList(names);
    if (!validNames.ok()) return validNames;

    Result validInvariant = checkInvariant(names, FieldType::State);
    if (!validInvariant.ok()) return validInvariant;

    if (m_stateNames == names) {
        return Result::success();
    }

    QMap<QString, int> oldIndex;
    for (int i = 0; i < m_stateNames.size(); ++i)
        oldIndex[m_stateNames[i]] = i;

    QVector<QVector<CellData>> newTable(m_inputSignalNames.size());
    for (int i = 0; i < m_inputSignalNames.size(); ++i) {
        newTable[i].resize(names.size());
        for (int j = 0; j < names.size(); ++j) {
            if (oldIndex.contains(names[j])) {
                newTable[i][j] = m_transitionTable[i][oldIndex[names[j]]];
            }
        }
    }

    m_stateNames = names;
    m_transitionTable = newTable;

    for (auto it = m_mooreOutputs.begin(); it != m_mooreOutputs.end(); ) {
        if (!names.contains(it.key()))
            it = m_mooreOutputs.erase(it);
        else
            ++it;
    }

    setInitialState(m_stateNames.isEmpty() ? QString("") : m_stateNames.first()); // hotfix

    return Result::success();
}

Result AutomatonData::setInputSignalNames(const QStringList& names) {
    Result validNames = validateNameList(names);
    if (!validNames.ok()) return validNames;

    Result validInvariant = checkInvariant(names, FieldType::Input);

    if (!validInvariant.ok()) return validInvariant;
    if (m_inputSignalNames == names) {
        return Result::success();
    }

    QMap<QString, int> oldIndex;
    for (int i = 0; i < m_inputSignalNames.size(); ++i)
        oldIndex[m_inputSignalNames[i]] = i;

    const int stateCount = m_stateNames.size();
    QVector<QVector<CellData>> newTable(names.size());
    for (int i = 0; i < names.size(); ++i) {
        newTable[i].resize(stateCount);

        if (oldIndex.contains(names[i])) {
            int oldRow = oldIndex[names[i]];
            if (oldRow < m_transitionTable.size()) {
                newTable[i] = m_transitionTable[oldRow];
            }
        }
    }

    m_inputSignalNames = names;
    m_transitionTable = newTable;

    return Result::success();
}

Result AutomatonData::setOutputSignalNames(const QStringList& names) {
    Result validNames = validateNameList(names);
    if (!validNames.ok()) return validNames;

    Result validInvariant = checkInvariant(names, FieldType::Output);

    if (!validInvariant.ok()) return validInvariant;
    if (m_outputSignalNames == names) {
        return Result::success();
    }

    m_outputSignalNames = names;

    return Result::success();
}

Result AutomatonData::setInitialState(const QString& name) {
    if (!name.isEmpty() && !m_stateNames.contains(name))
        return Result::error(
                    QStringLiteral("Initial state '%1' is not in the state list")
                        .arg(name),
                    ResultCategory::Validation)
            .withCode(ErrorCode::ADInitialStateNotFound)
            .withOffender(name);

    m_initialState = name;
    return Result::success();
}

// Set tabel cell
Result AutomatonData::setMooreOutputCell(int col, const QString& text) {
    // Boundaries of the land
    const int stateIndex = col - 1;
    if (stateIndex < 0 || stateIndex >= m_stateNames.size())
        return Result::error(
                    QStringLiteral("Column index %1 is out of range").arg(col),
                    ResultCategory::Validation)
            .withCode(ErrorCode::ADInvalidCellIndex);

    // Validation
    const Result check = validateCellContent(0, col, text);
    if (!check.ok()) return check;

    // Recording
    const QString trimmed = text.trimmed();
    QStringList outputs;

    if (!(trimmed.isEmpty() || trimmed == "—" || trimmed == "-")) {
        outputs = NameListParser::parse(trimmed);
    }

    m_mooreOutputs[m_stateNames[stateIndex]] = outputs;
    return Result::success();
}

Result AutomatonData::setMooreOutputCellByName(const QString& stateName, const QString& text) {
    const int stateIdx = m_stateNames.indexOf(stateName);
    if (stateIdx < 0) {
        return Result::error(
                    QStringLiteral("State '%1' not found").arg(stateName),
                    ResultCategory::Validation)
            .withCode(ErrorCode::ADUnknownState)
            .withOffender(stateName);
    }

    return setMooreOutputCell(stateIdx + 1, text);
}


Result AutomatonData::setTransitionCell(int row, int col, const QString& text) {
    // Boundaries
    const int rowOffset = (m_type == VariantType::MooreToMealy) ? 2 : 1;
    const int inputIndex = row - rowOffset;
    const int stateIndex = col - 1;

    if (inputIndex < 0 || inputIndex >= m_inputSignalNames.size()) {
        return Result::error(
                    QStringLiteral("Row index %1 is out of range").arg(row),
                    ResultCategory::Validation)
            .withCode(ErrorCode::ADInvalidCellIndex);
    }

    if (stateIndex < 0 || stateIndex >= m_stateNames.size()) {
        return Result::error(
                    QStringLiteral("Column index %1 is out of range").arg(col),
                    ResultCategory::Validation)
            .withCode(ErrorCode::ADInvalidCellIndex);

    }

    // Validation
    const Result check = validateCellContent(row, col, text);
    if (!check.ok()) return check;

    // Recording
    const QString trimmed = text.trimmed();
    CellData cell;

    if (trimmed.isEmpty() || trimmed == "—" || trimmed == "-") {
    } else if (m_type == VariantType::MooreToMealy) {
        cell.nextState = trimmed;
    } else {
        const int slashPos = trimmed.indexOf('/');
        if (slashPos < 0) {
            return Result::error(
                       QStringLiteral("Cell '%1' has invalid format").arg(trimmed),
                       ResultCategory::Validation)
                .withCode(ErrorCode::ADInvalidCellFormat);
        }

        const QString statePart = trimmed.left(slashPos).trimmed();
        const QString outputPart = trimmed.mid(slashPos + 1).trimmed();

        if (statePart != "—" && statePart != "-") {
            cell.nextState = statePart;
        }

        if (outputPart != "—" && outputPart != "-") {
            cell.outputSignals = NameListParser::parse(outputPart);
        }
    }

    m_transitionTable[inputIndex][stateIndex] = cell;
    return Result::success();
}

Result AutomatonData::setTransitionCellByName(const QString& inputName, const QString& stateName, const QString& text) {

    // 1. We check that the names exist.
    const int inputIdx = m_inputSignalNames.indexOf(inputName);
    if (inputIdx < 0) {
        return Result::error(
                    QStringLiteral("Input signal '%1' not found").arg(inputName),
                    ResultCategory::Validation)
            .withCode(ErrorCode::ADUnknownInput)
            .withOffender(inputName);
    }

    const int stateIdx = m_stateNames.indexOf(stateName);
    if (stateIdx < 0) {
        return Result::error(
                    QStringLiteral("State '%1' not found").arg(stateName),
                    ResultCategory::Validation)
            .withCode(ErrorCode::ADUnknownState)
            .withOffender(stateName);
    }

    // 2. We calculate the coordinates in the UI table.
    const int rowOffset = (m_type == VariantType::MooreToMealy) ? 2 : 1;
    const int row = inputIdx + rowOffset;
    const int col = stateIdx + 1;

    // 3. We delegate the main setter — validation and recording there.
    return setTransitionCell(row, col, text);
}

// Validate table data
Result AutomatonData::checkInvariant(const QStringList& candidate, FieldType field) const {
    // Duplicates within the list itself
    QSet<QString> seen;
    for (const QString& name : candidate) {
        if (seen.contains(name))
            return Result::error(
                QStringLiteral("There are %1 duplicates of '%2' in the '%3' list.")
                    .arg(QString::number(candidate.count(name) - 1), name, fieldTypeToString(field)))
                .withCode(ErrorCode::ADDuplicateNameInList)
                .withOffender(name);
        seen.insert(name);
    }

    // Intersection with other fields
    const QStringList* others[2];
    QStringList otherNames;

    switch (field) {
    case FieldType::State:
        others[0] = &m_inputSignalNames;
        otherNames.append(fieldTypeDisplayName(FieldType::Input));

        others[1] = &m_outputSignalNames;
        otherNames.append(fieldTypeDisplayName(FieldType::Output));
        break;

    case FieldType::Input:
        others[0] = &m_stateNames;
        otherNames.append(fieldTypeDisplayName(FieldType::State));

        others[1] = &m_outputSignalNames;
        otherNames.append(fieldTypeDisplayName(FieldType::Output));
        break;

    case FieldType::Output:
        others[0] = &m_stateNames;
        otherNames.append(fieldTypeDisplayName(FieldType::State));

        others[1] = &m_inputSignalNames;
        otherNames.append(fieldTypeDisplayName(FieldType::Input));
        break;
    }

    for (int i = 0; i < 2; ++i) {
        for (const QString& name : candidate) {
            if (others[i]->contains(name)) {
                return Result::error(
                        QStringLiteral("The name '%1' is already in use among '%2' list.")
                        .arg(name, otherNames[i]))
                    .withCode(ErrorCode::ADDuplicateNameInAllList)
                    .withOffender(name);
            }
        }
    }

    return Result::success();
}

Result AutomatonData::validateName(const QString& name) const {
    static const QRegularExpression allowedChars(QStringLiteral("^[A-Za-zА-Яа-яЁё0-9]+$"));
    static const QRegularExpression startsWithLetter(QStringLiteral("^[A-Za-zА-Яа-яЁё]"));

    // Empty name
    if (name.isEmpty()) {
        return Result::error(
                    QStringLiteral("Name cannot be empty"),
                    ResultCategory::Validation)
            .withCode(ErrorCode::ADEmptyName);
    }

    // Letters and numbers only
    if (!allowedChars.match(name).hasMatch()) {
        return Result::error(
                    QStringLiteral("Name '%1' contains invalid characters")
                        .arg(name),
                    ResultCategory::Validation)
            .withCode(ErrorCode::ADInvalidName)
            .withOffender(name)
            .withDetails(QStringLiteral("Allowed: letters and digits only"));
    }

    // The word must start with the letter.
    if (!startsWithLetter.match(name).hasMatch()) {
        return Result::error(
                    QStringLiteral("Name '%1' must start with a letter").arg(name),
                    ResultCategory::Validation)
            .withCode(ErrorCode::ADInvalidName)
            .withOffender(name);
    }

    return Result::success();

}

Result AutomatonData::validateNameList(const QStringList& names) const {
    for (const QString& name : names) {
        Result nameValid = validateName(name);
        if (!nameValid.ok()) return nameValid;
    }

    return Result::success();

}

Result AutomatonData::validateCellContent(int row, int col, const QString& text) const {
    const CellKind kind = cellKind(row, col);

    if (kind == CellKind::Empty || kind == CellKind::StateHeader || kind == CellKind::InputHeader) {
        return Result::error(
                    QStringLiteral("Cell (%1,%2) is not editable").arg(row).arg(col),
                    ResultCategory::Validation)
            .withCode(ErrorCode::ADCellNotEditable);
    }

    const QString trimmed = text.trimmed();

    if (trimmed.isEmpty() || trimmed == "—" || trimmed == "-") {
        return Result::success();
    }

    switch (kind) {
    case CellKind::MooreOutput: {
        QStringList names = NameListParser::parse(trimmed);
        Result validNames = validateNameList(names);
        if (!validNames.ok()) return validNames;

        QSet<QString> seen;
        for (const QString& name : std::as_const(names)) {
            if (!m_outputSignalNames.contains(name)) {
                return Result::error(
                            QStringLiteral("Unknown output signal '%1'").arg(name),
                            ResultCategory::Validation)
                    .withCode(ErrorCode::ADUnknownOutput)
                    .withOffender(name);
            }

            if (seen.contains(name)) {
                return Result::error(
                            QStringLiteral("Cell (%1:%2) has duplicate output signal '%3'")
                               .arg(row)
                               .arg(col)
                               .arg(name),
                            ResultCategory::Validation)
                    .withCode(ErrorCode::ADCellDuplicateName)
                    .withOffender(name);
            }
            seen.insert(name);
        }
        return Result::success();
    }

    case CellKind::Transition: {
        if (m_type == VariantType::MooreToMealy) {
            Result nameValid = validateName(trimmed);
            if (!nameValid.ok()) return nameValid;

            if (!m_stateNames.contains(trimmed)) {
                return Result::error(
                            QStringLiteral("Unknown state '%1'").arg(trimmed),
                            ResultCategory::Validation)
                    .withCode(ErrorCode::ADUnknownState)
                    .withOffender(trimmed);
            }

            return Result::success();
        }

        const int slashPos = trimmed.indexOf('/');
        if (slashPos < 0) {
            return Result::error(
                        QStringLiteral("Cell '%1' has invalid format").arg(trimmed),
                        ResultCategory::Validation)
                .withCode(ErrorCode::ADInvalidCellFormat)
                .withDetails(QStringLiteral("Expected: state / outputs"));
        }

        const QString statePart = trimmed.left(slashPos).trimmed();
        const QString outputPart = trimmed.mid(slashPos + 1).trimmed();

        // Left side
        if (!statePart.isEmpty() && statePart != "—" && statePart != "-") {
            Result nameValid = validateName(statePart);
            if (!nameValid.ok()) return nameValid;

            if (!m_stateNames.contains(statePart)) {
                return Result::error(
                            QStringLiteral("Unknown state '%1'").arg(statePart),
                            ResultCategory::Validation)
                    .withCode(ErrorCode::ADUnknownState)
                    .withOffender(statePart);
            }
        }

        // Right side
        if (!outputPart.isEmpty() && outputPart != "—") {
            QStringList outs = NameListParser::parse(outputPart);
            Result validNames = validateNameList(outs);
            if (!validNames.ok()) return validNames;

            QSet<QString> seen;
            for (const QString& name : std::as_const(outs)) {
                if (!m_outputSignalNames.contains(name)) {
                    return Result::error(
                                QStringLiteral("Unknown output signal '%1'").arg(name),
                                ResultCategory::Validation)
                        .withCode(ErrorCode::ADUnknownOutput)
                        .withOffender(name);
                }

                if (seen.contains(name)) {
                    return Result::error(
                               QStringLiteral("Cell (%1:%2) has duplicate output signal '%3'")
                                   .arg(row)
                                   .arg(col)
                                   .arg(name),
                               ResultCategory::Validation)
                        .withCode(ErrorCode::ADCellDuplicateName)
                        .withOffender(name);

                }

                seen.insert(name);
            }
        }
        return Result::success();
    }

    default:
        return Result::error(
                    QStringLiteral("Unsupported cell kind at (%1,%2)").arg(row).arg(col),
                    ResultCategory::Validation)
            .withCode(ErrorCode::ADInvalidCellKind);
    }
}

// Validate automat
Result AutomatonData::validate() const {
    // 1. No void lists
    if (m_stateNames.isEmpty()) {
        return Result::error(
                    QStringLiteral("Automaton has no states"),
                    ResultCategory::Validation)
            .withCode(ErrorCode::ADEmptyStateList);
    }
    if (m_inputSignalNames.isEmpty()) {
        return Result::error(
                   QStringLiteral("Automaton has no input signals"),
                   ResultCategory::Validation)
            .withCode(ErrorCode::ADEmptyInputList);
    }
    if (m_outputSignalNames.isEmpty()) {
        return Result::error(
                   QStringLiteral("Automaton has no output signals"),
                   ResultCategory::Validation)
            .withCode(ErrorCode::ADEmptyOutputList);
    }

    // 2. Consistency of dimensions
    if (m_transitionTable.size() != m_inputSignalNames.size()) {
        return Result::error(
                    QStringLiteral("Transition table row count %1 does not match "
                                    "input signal count %2")
                    .arg(m_transitionTable.size())
                    .arg(m_inputSignalNames.size()),
                    ResultCategory::Validation)
            .withCode(ErrorCode::ADTransitionTableSizeMismatch);
    }
    for (int i = 0; i < m_transitionTable.size(); ++i) {
        if (m_transitionTable[i].size() != m_stateNames.size()) {
            return Result::error(
                        QStringLiteral("Row %1 has %2 columns, expected %3")
                        .arg(i)
                        .arg(m_transitionTable[i].size())
                        .arg(m_stateNames.size()),
                        ResultCategory::Validation)
                .withCode(ErrorCode::ADTransitionTableSizeMismatch)
                .withDetails(
                    QStringLiteral("input='%1'")
                    .arg(m_inputSignalNames.value(i)));
        }
    }

    // 3. Usage validate
    if (Result r = validateStateUsage(); !r.ok()) return r;
    if (Result r = validateInputUsage(); !r.ok()) return r;
    if (Result r = validateOutputUsage(); !r.ok()) return r;

    return Result::success();
}

void AutomatonData::setStateUsageChecker(StateUsageChecker checker) {
    m_stateUsageChecker = std::move(checker);
}

Result AutomatonData::validateStateUsage() const {
    if (m_stateUsageChecker) {
        return m_stateUsageChecker(*this);
    }

    return Result::error(
                QStringLiteral("The check has not been established."),
                ResultCategory::Validation)
        .withCode(ErrorCode::ADDisableChecker);
}

Result AutomatonData::validateInputUsage() const {
    for (int i = 0; i < m_inputSignalNames.size(); ++i) {
        bool anyTransition = false;
        for (int j = 0; j < m_transitionTable[i].size(); ++j) {
            if (!m_transitionTable[i][j].isEmpty()) {
                anyTransition = true;
                break;
            }
        }

        if (!anyTransition) {
            return Result::error(
                        QStringLiteral("Input signal '%1' is not used in any transition")
                            .arg(m_inputSignalNames[i]),
                        ResultCategory::Validation)
                .withCode(ErrorCode::ADUnusedInput)
                .withOffender(m_inputSignalNames[i]);
        }
    }
    return Result::success();
}

Result AutomatonData::validateOutputUsage() const {
    QSet<QString> usedOutputs;

    if (m_type == VariantType::MooreToMealy) {
        for (auto it = m_mooreOutputs.constBegin();
             it != m_mooreOutputs.constEnd(); ++it) {
            for (const QString& out : it.value()) {
                usedOutputs.insert(out);
            }
        }
    } else {
        for (const auto& row : m_transitionTable) {
            for (const CellData& cell : row) {
                for (const QString& out : cell.outputSignals) {
                    usedOutputs.insert(out);
                }
            }
        }
    }

    for (const QString& out : m_outputSignalNames) {
        if (!usedOutputs.contains(out)) {
            return Result::error(
                        QStringLiteral("Output signal '%1' is not used")
                            .arg(out),
                        ResultCategory::Validation)
                .withCode(ErrorCode::ADUnusedOutput)
                .withOffender(out);
        }
    }
    return Result::success();
}

Result AutomatonData::validateStateUsageStrict() const {
    // 1. We check that the initial state is set.
    if (m_initialState.isEmpty()) {
        return Result::error(
                   QStringLiteral("Initial state is not set"),
                   ResultCategory::Validation)
            .withCode(ErrorCode::ADInitialStateNotSet);
    }

    if (!m_stateNames.contains(m_initialState)) {
        return Result::error(
                    QStringLiteral("Initial state '%1' is not in the state list")
                        .arg(m_initialState),
                    ResultCategory::Validation)
            .withCode(ErrorCode::ADInitialStateNotFound)
            .withOffender(m_initialState);
    }

    // 2. BFS from the initial state
    QSet<QString> reachable;
    QQueue<QString> queue;
    queue.enqueue(m_initialState);

    while (!queue.isEmpty()) {
        const QString state = queue.dequeue();
        if (reachable.contains(state)) continue;
        reachable.insert(state);

        const int stateIdx = m_stateNames.indexOf(state);
        if (stateIdx < 0) continue;

        for (int i = 0; i < m_inputSignalNames.size(); ++i) {
            const CellData& cell = m_transitionTable[i][stateIdx];
            if (!cell.nextState.isEmpty() && cell.nextState != "—")
                queue.enqueue(cell.nextState);
        }
    }

    // 3. All states must be achievable.
    for (const QString& state : m_stateNames) {
        if (!reachable.contains(state)) {
            return Result::error(
                        QStringLiteral("State '%1' is not reachable from '%2'")
                            .arg(state, m_initialState),
                        ResultCategory::Validation)
                .withCode(ErrorCode::ADUnreachableState)
                .withOffender(state);
        }
    }

    return Result::success();
}

Result AutomatonData::validateStateUsageLenient() const {
    QSet<QString> usedStates;

    if (!m_initialState.isEmpty())
        usedStates.insert(m_initialState);

    for (int i = 0; i < m_transitionTable.size(); ++i) {
        for (int j = 0; j < m_transitionTable[i].size(); ++j) {
            const CellData& cell = m_transitionTable[i][j];

            if (j < m_stateNames.size()) {
                usedStates.insert(m_stateNames[j]);
            }

            if (!cell.nextState.isEmpty() && cell.nextState != "—") {
                usedStates.insert(cell.nextState);
            }
        }
    }

    for (const QString& state : m_stateNames) {
        if (!usedStates.contains(state)) {
            return Result::error(
                        QStringLiteral("State '%1' is not used in any transition")
                            .arg(state),
                        ResultCategory::Validation)
                .withCode(ErrorCode::ADUnusedState)
                .withOffender(state);
        }
    }

    return Result::success();
}


