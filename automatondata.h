#ifndef AUTOMATONDATA_H
#define AUTOMATONDATA_H

#include <qobject.h>
#include <QMap>
#include <QList>

enum class VariantType {
    MealyToMoore,
    MooreToMealy
};

struct CellData {
    QString nextState;
    QStringList outputSignals;   // empty for Mealy
    bool isEmpty() const { return nextState.isEmpty() && outputSignals.isEmpty(); }
};

class AutomatonData
{
public:
    AutomatonData();

    // Gettors
    VariantType getType() const { return m_type; }
    int getVariantNumber() const { return m_variantNumber; }

    const QStringList& getStateNames() const { return m_stateNames; }
    const QStringList& getInputSignalNames() const { return m_inputSignalNames; }
    const QStringList& getOutputSignalNames() const { return m_outputSignalNames; }

    const QMap<QString, QStringList>& getMooreOutputs() const { return m_mooreOutputs; }
    const QVector<QVector<CellData>>& getTransitionTable() const { return m_transitionTable; }

    // Settors
    void setVariantNumber(int n);
    void setVariantType(VariantType t);

    void setStateNames(const QStringList& names);
    void setInputSignalNames(const QStringList& names);
    void setOutputSignalNames(const QStringList& names);

    void setTransition(int inputIndex, int stateIndex, const CellData& cell);
    void setMooreOutput(const QString& state, const QStringList& outputs);


private:
    VariantType m_type = VariantType::MealyToMoore;
    int m_variantNumber = 1;

    QStringList m_stateNames;
    QStringList m_inputSignalNames;
    QStringList m_outputSignalNames;

    QMap<QString, QStringList> m_mooreOutputs; // only Moore
    QVector<QVector<CellData>> m_transitionTable;

    void rebuildTransitionTable();
};

#endif // AUTOMATONDATA_H
