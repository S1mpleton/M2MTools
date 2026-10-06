#include "automatondata.h"
#include "namelistparser.h"

#include <QSet>
#include <QRegularExpression>
#include <QJsonArray>

QString nameFieldToString(FieldType f)
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
    ValidationResult validNames = validateNameList(names);
    if (!validNames.ok) return validNames;

    ValidationResult validInvariant = checkInvariant(names, FieldType::State);

    if (!validInvariant.ok) return validInvariant;
    if (m_stateNames == names) return validInvariant;

    // if (!m_initialState.isEmpty() && !names.contains(m_initialState)) {
    //     m_initialState.clear();
    // }

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

    return validInvariant;
}

ValidationResult AutomatonData::setInputSignalNames(const QStringList& names) {
    ValidationResult validNames = validateNameList(names);
    if (!validNames.ok) return validNames;

    ValidationResult validInvariant = checkInvariant(names, FieldType::Input);

    if (!validInvariant.ok) return validInvariant;
    if (m_inputSignalNames == names) return validInvariant;

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

    return validInvariant;
}

ValidationResult AutomatonData::setOutputSignalNames(const QStringList& names) {
    ValidationResult validNames = validateNameList(names);
    if (!validNames.ok) return validNames;

    ValidationResult validInvariant = checkInvariant(names, FieldType::Output);

    if (!validInvariant.ok) return validInvariant;
    if (m_outputSignalNames == names) return validInvariant;

    m_outputSignalNames = names;

    return validInvariant;
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

ValidationResult AutomatonData::setMooreOutputCellByName(const QString& stateName, const QString& text) {
    const int stateIdx = m_stateNames.indexOf(stateName);
    if (stateIdx < 0)
        return ValidationResult::failure(
            QStringLiteral("Неизвестное состояние: «%1»").arg(stateName),
            stateName);

    // Строка 0 — выходы Мура, столбец stateIdx + 1
    return setMooreOutputCell(stateIdx + 1, text);
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

ValidationResult AutomatonData::setTransitionCellByName(
    const QString& inputName,
    const QString& stateName,
    const QString& text) {

    // 1. Проверяем, что имена существуют
    const int inputIdx = m_inputSignalNames.indexOf(inputName);
    if (inputIdx < 0)
        return ValidationResult::failure(
            QStringLiteral("Неизвестный входной сигнал: «%1»").arg(inputName),
            inputName);

    const int stateIdx = m_stateNames.indexOf(stateName);
    if (stateIdx < 0)
        return ValidationResult::failure(
            QStringLiteral("Неизвестное состояние: «%1»").arg(stateName),
            stateName);

    // 2. Вычисляем координаты в UI-таблице
    const int rowOffset = (m_type == VariantType::MooreToMealy) ? 2 : 1;
    const int row = inputIdx + rowOffset;
    const int col = stateIdx + 1;

    // 3. Делегируем основному сеттеру — валидация и запись там
    return setTransitionCell(row, col, text);
}

ValidationResult AutomatonData::setInitialState(const QString& name) {
    qDebug() << " start setInitialState";
    if (!name.isEmpty() && !m_stateNames.contains(name))
        return ValidationResult::failure(
            QStringLiteral("Начальное состояние «%1» отсутствует "
                           "в списке состояний").arg(name),
            name);
    m_initialState = name;

    qDebug() << "end setInitialState";
    return ValidationResult::success();
}


ValidationResult AutomatonData::checkInvariant(const QStringList& candidate, FieldType field) const {
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

    // Intersection with other fields
    const QStringList* others[2];
    QStringList otherNames;

    switch (field) {
    case FieldType::State:
        others[0] = &m_inputSignalNames;
        otherNames.append(nameFieldToString(FieldType::Input));

        others[1] = &m_outputSignalNames;
        otherNames.append(nameFieldToString(FieldType::Output));
        break;

    case FieldType::Input:
        others[0] = &m_stateNames;
        otherNames.append(nameFieldToString(FieldType::State));

        others[1] = &m_outputSignalNames;
        otherNames.append(nameFieldToString(FieldType::Output));
        break;

    case FieldType::Output:
        others[0] = &m_stateNames;
        otherNames.append(nameFieldToString(FieldType::State));

        others[1] = &m_inputSignalNames;
        otherNames.append(nameFieldToString(FieldType::Input));
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

ValidationResult AutomatonData::validateNameList(const QStringList& names) const {
    static const QRegularExpression allowedChars(QStringLiteral("^[A-Za-zА-Яа-яЁё0-9]+$"));
    static const QRegularExpression startsWithLetter(QStringLiteral("^[A-Za-zА-Яа-яЁё]"));

    for (const QString& name : names) {
        // Empty name
        if (name.isEmpty()) {
            return ValidationResult::failure(QStringLiteral("The name cannot be empty."));

        }

        // Letters and numbers only
        if (!allowedChars.match(name).hasMatch())
            return ValidationResult::failure(
                QStringLiteral(
                    "The name '%1' contains invalid characters. "
                    "Only letters and numbers are allowed."
                    ).arg(name), name);

        // The word must start with the letter.
        if (!startsWithLetter.match(name).hasMatch())
            return ValidationResult::failure(
                QStringLiteral(
                    "The name '%1' must start with a letter."
                    ).arg(name), name);
    }

    return ValidationResult::success();
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
        ValidationResult validNames = validateNameList(names);
        if (!validNames.ok) return validNames;

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
            ValidationResult validNames = validateNameList(outs);
            if (!validNames.ok) return validNames;

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
