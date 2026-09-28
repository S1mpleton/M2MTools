#include "automatondata.h"

AutomatonData::AutomatonData() {}

void AutomatonData::setVariantNumber(int n){
    m_variantNumber = n;
}

void AutomatonData::setVariantType(VariantType t){
    m_type = t;
}

void AutomatonData::setStateNames(const QStringList& names) {
    if (m_stateNames == names) return;

    // Запоминаем старые индексы
    QMap<QString, int> oldIndex;
    for (int i = 0; i < m_stateNames.size(); ++i)
        oldIndex[m_stateNames[i]] = i;

    // Переносим данные в новую матрицу
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

    // Чистим mooreOutputs от удаленных состояний
    for (auto it = m_mooreOutputs.begin(); it != m_mooreOutputs.end(); ) {
        if (!names.contains(it.key()))
            it = m_mooreOutputs.erase(it);
        else
            ++it;
    }
}

void AutomatonData::setInputSignalNames(const QStringList& names) {
    if (m_inputSignalNames == names) return;

    // Запоминаем старые индексы входных сигналов
    QMap<QString, int> oldIndex;
    for (int i = 0; i < m_inputSignalNames.size(); ++i)
        oldIndex[m_inputSignalNames[i]] = i;

    // Новая матрица: строк = |names|, столбцов = |m_stateNames|
    const int stateCount = m_stateNames.size();
    QVector<QVector<CellData>> newTable(names.size());
    for (int i = 0; i < names.size(); ++i) {
        newTable[i].resize(stateCount);

        // Если такой вход уже был — копируем всю строку целиком
        if (oldIndex.contains(names[i])) {
            int oldRow = oldIndex[names[i]];
            // Защита: матрица могла быть меньше из-за несогласованности
            if (oldRow < m_transitionTable.size()) {
                newTable[i] = m_transitionTable[oldRow];
            }
        }
        // Иначе строка остается пустой — это новый вход
    }

    m_inputSignalNames = names;
    m_transitionTable = newTable;
}

void AutomatonData::setOutputSignalNames(const QStringList& names) {
    if (m_outputSignalNames == names) return;

    m_outputSignalNames = names;
}
