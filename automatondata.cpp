#include "automatondata.h"

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

ValidationResult AutomatonData::checkInvariant(const QStringList& candidate, NameField field) const
{
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
