#include "automatondata.h"
#include "namelistparser.h"

#include <QSet>

QString nameFieldToString(NameField f)
{
    return kNameFieldNames.value(f);
}

AutomatonData::AutomatonData() {}

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

void AutomatonData::setVariantNumber(int n){
    m_variantNumber = n;
}

void AutomatonData::setVariantType(VariantType t){
    m_type = t;
}

ValidationResult AutomatonData::setStateNames(const QStringList& names) {
    ValidationResult result = checkInvariant(names, NameField::State);

    if (!result.ok) return result;
    if (m_stateNames == names) return result;

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

    return result;
}

ValidationResult AutomatonData::setInputSignalNames(const QStringList& names) {
    ValidationResult result = checkInvariant(names, NameField::Input);

    if (!result.ok) return result;
    if (m_inputSignalNames == names) return result;

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

    return result;
}

ValidationResult AutomatonData::setOutputSignalNames(const QStringList& names) {
    ValidationResult result = checkInvariant(names, NameField::Output);

    if (!result.ok) return result;
    if (m_outputSignalNames == names) return result;

    m_outputSignalNames = names;

    return result;
}

ValidationResult AutomatonData::setMooreOutputCell(int col, const QString& text) {
    // Boundaries of the land
    const int stateIndex = col - 1;
    if (stateIndex < 0 || stateIndex >= m_stateNames.size())
        return ValidationResult::failure("Invalid column index");

    // Validation
    const ValidationResult check = validateCellContent(0, col, text);
    if (!check.ok)
        return check;

    // Recording
    const QString trimmed = text.trimmed();
    QStringList outputs;

    if (!(trimmed.isEmpty() || trimmed == "—" || trimmed == "-")) {
        outputs = NameListParser::parse(trimmed);
    }

    m_mooreOutputs[m_stateNames[stateIndex]] = outputs;
    return ValidationResult::success("Changes applied");
}

ValidationResult AutomatonData::setTransitionCell(int row, int col, const QString& text) {
    // Boundaries
    const int rowOffset = (m_type == VariantType::MooreToMealy) ? 2 : 1;
    const int inputIndex = row - rowOffset;
    const int stateIndex = col - 1;

    if (inputIndex < 0 || inputIndex >= m_inputSignalNames.size())
        return ValidationResult::failure("Invalid row index");
    if (stateIndex < 0 || stateIndex >= m_stateNames.size())
        return ValidationResult::failure("Invalid column index");

    // Validation
    const ValidationResult check = validateCellContent(row, col, text);
    if (!check.ok)
        return check;

    // Recording
    const QString trimmed = text.trimmed();
    CellData cell;

    if (trimmed.isEmpty() || trimmed == "—" || trimmed == "-") {
    } else if (m_type == VariantType::MooreToMealy) {
        cell.nextState = trimmed;
    } else {
        const int slashPos = trimmed.indexOf('/');
        if (slashPos < 0)
            return ValidationResult::failure("Internal error: format not verified");

        const QString statePart  = trimmed.left(slashPos).trimmed();
        const QString outputPart = trimmed.mid(slashPos + 1).trimmed();

        if (statePart != "—" && statePart != "-")
            cell.nextState = statePart;

        if (outputPart != "—" && outputPart != "-")
            cell.outputSignals = NameListParser::parse(outputPart);
    }

    m_transitionTable[inputIndex][stateIndex] = cell;
    return ValidationResult::success("Changes applied");
}

ValidationResult AutomatonData::checkInvariant(const QStringList& candidate, NameField field) const {
    // Duplicates within the list itself
    QSet<QString> seen;
    for (const QString& name : candidate) {
        if (name.isEmpty())
            return ValidationResult::failure("An empty name is not allowed.");
        if (seen.contains(name))
            return ValidationResult::failure(
                QString("There are %1 duplicates of '%2' in the '%3' list.")
                    .arg(QString::number(candidate.count(name)), name, nameFieldToString(field)),
                name
            );
        seen.insert(name);
    }

    // 2. Intersection with other fields
    const QStringList* others[2];
    QStringList otherNames;

    switch (field) {
    case NameField::State:
        others[0] = &m_inputSignalNames;
        otherNames.append(nameFieldToString(NameField::Input));

        others[1] = &m_outputSignalNames;
        otherNames.append(nameFieldToString(NameField::Output));
        break;

    case NameField::Input:
        others[0] = &m_stateNames;
        otherNames.append(nameFieldToString(NameField::State));

        others[1] = &m_outputSignalNames;
        otherNames.append(nameFieldToString(NameField::Output));
        break;

    case NameField::Output:
        others[0] = &m_stateNames;
        otherNames.append(nameFieldToString(NameField::State));

        others[1] = &m_inputSignalNames;
        otherNames.append(nameFieldToString(NameField::Input));
        break;
    }

    for (int i = 0; i < 2; ++i) {
        for (const QString& name : candidate) {
            if (others[i]->contains(name)) {
                return ValidationResult::failure(
                    QString("The name '%1' is already in use among '%2' list.")
                        .arg(name, otherNames[i]),
                    name);
            }
        }
    }

    return ValidationResult::success(
        QString("The list '%1' is valid.")
            .arg(nameFieldToString(field))
    );
}

ValidationResult AutomatonData::validateCellContent(int row, int col, const QString& text) const {
    const CellKind kind = cellKind(row, col);

    if (kind == CellKind::Empty
        || kind == CellKind::StateHeader
        || kind == CellKind::InputHeader) {
        return ValidationResult::failure("This cell cannot be edited.");
    }

    const QString trimmed = text.trimmed();

    if (trimmed.isEmpty() || trimmed == "—" || trimmed == "-")
        return ValidationResult::success();

    switch (kind) {
    case CellKind::MooreOutput: {
        QStringList names = NameListParser::parse(trimmed);
        QSet<QString> seen;
        for (const QString& name : std::as_const(names)) {
            if (!m_outputSignalNames.contains(name))
                return ValidationResult::failure(
                    QString("Unknown output signal: %1").arg(name), name);
            if (seen.contains(name))
                return ValidationResult::failure(
                    QString("Duplicate: %1").arg(name), name);
            seen.insert(name);
        }
        return ValidationResult::success();
    }

    case CellKind::Transition: {
        if (m_type == VariantType::MooreToMealy) {
            if (trimmed.contains(','))
                return ValidationResult::failure("The state cannot contain commas.");
            if (!m_stateNames.contains(trimmed))
                return ValidationResult::failure(
                    QString("Unknown state: %1").arg(trimmed), trimmed);
            return ValidationResult::success();
        }

        const int slashPos = trimmed.indexOf('/');
        if (slashPos < 0)
            return ValidationResult::failure("Expected format: state / output");

        const QString statePart  = trimmed.left(slashPos).trimmed();
        const QString outputPart = trimmed.mid(slashPos + 1).trimmed();

        // Left side
        if (!statePart.isEmpty() && statePart != "—") {
            if (statePart.contains(','))
                return ValidationResult::failure("The state cannot contain commas.", statePart);
            if (!m_stateNames.contains(statePart))
                return ValidationResult::failure(
                    QString("Unknown state: %1").arg(statePart), statePart);
        }

        // Right side
        if (!outputPart.isEmpty() && outputPart != "—") {
            QStringList outs = NameListParser::parse(outputPart);
            QSet<QString> seen;
            for (const QString& name : std::as_const(outs)) {
                if (!m_outputSignalNames.contains(name))
                    return ValidationResult::failure(
                        QString("Unknown output signal: %1").arg(name), name);
                if (seen.contains(name))
                    return ValidationResult::failure(
                        QString("Duplicate: %1").arg(name), name);
                seen.insert(name);
            }
        }
        return ValidationResult::success();
    }

    default:
        return ValidationResult::failure("Unsupported cell type");
    }
}
