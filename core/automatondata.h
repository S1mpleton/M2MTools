#ifndef AUTOMATONDATA_H
#define AUTOMATONDATA_H

#include "validationresult.h"

#include <QObject>
#include <QMap>
#include <QList>
#include <QHash>
#include <QString>
#include <QJsonObject>

enum class FieldType {
    State,
    Input,
    Output
};

static const QHash<FieldType, QString> kNameFieldNames = {
    { FieldType::State,  "State"  },
    { FieldType::Input,  "Input"  },
    { FieldType::Output, "Output" },
    };

QString nameFieldToString(FieldType f);

enum class VariantType {
    MealyToMoore,
    MooreToMealy
};

inline VariantType variantStringToType(QString typeString) {
    if (typeString == QString("MealyToMoore")) return VariantType::MealyToMoore;
    if (typeString == QString("MooreToMealy")) return VariantType::MooreToMealy;

    qDebug() << "Не тот ВАРИАНТ ТИП";
    return VariantType::MealyToMoore;
}

inline QString variantTypeToString(VariantType type) {
    switch (type) {
    case VariantType::MooreToMealy: return QStringLiteral("MooreToMealy");
    case VariantType::MealyToMoore: return QStringLiteral("MealyToMoore");
    }
    return {};
}

inline QString variantTypeDisplayName(VariantType type) {
    switch (type) {
    case VariantType::MooreToMealy: return QObject::tr("Мур → Мили");
    case VariantType::MealyToMoore: return QObject::tr("Мили → Мур");
    }
    return {};
}

enum class CellKind {
    Empty,              // something uneditable
    StateHeader,        // "State" / Name state
    InputHeader,        // "x1", "x2"
    MooreOutput,        // "Output" — state output
    Transition          // transition cell (Moore or Mealy)
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

    QString getInitialState() const { return m_initialState; }
    const QStringList& getStateNames() const { return m_stateNames; }
    const QStringList& getInputSignalNames() const { return m_inputSignalNames; }
    const QStringList& getOutputSignalNames() const { return m_outputSignalNames; }

    const QMap<QString, QStringList>& getMooreOutputs() const { return m_mooreOutputs; }
    const QVector<QVector<CellData>>& getTransitionTable() const { return m_transitionTable; }

    // Settors
    void setVariantNumber(int n);
    void setVariantType(VariantType t);

    ValidationResult setInitialState(const QString& name);
    ValidationResult setStateNames(const QStringList& names);
    ValidationResult setInputSignalNames(const QStringList& names);
    ValidationResult setOutputSignalNames(const QStringList& names);

    ValidationResult setTransitionCell(int row, int col, const QString& text);
    ValidationResult setMooreOutputCell(int col,const QString& text);

    ValidationResult setTransitionCellByName(const QString& inputName, const QString& stateName, const QString& text);
    ValidationResult setMooreOutputCellByName(const QString& stateName, const QString& text);

    void setTransition(int inputIndex, int stateIndex, const CellData& cell);
    void setMooreOutput(const QString& state, const QStringList& outputs);

    // Other
    CellKind cellKind(int row, int col) const;

private:
    VariantType m_type = VariantType::MealyToMoore;
    int m_variantNumber = 1;

    QStringList m_stateNames;
    QString m_initialState;
    QStringList m_inputSignalNames;
    QStringList m_outputSignalNames;

    QMap<QString, QStringList> m_mooreOutputs; // only Moore
    QVector<QVector<CellData>> m_transitionTable;

    static constexpr int kSchemaVersion = 1;

    void rebuildTransitionTable();
    ValidationResult checkInvariant(const QStringList& candidate, FieldType field) const;
    ValidationResult validateNameList(const QStringList& names) const;
    ValidationResult validateCellContent(int row, int col, const QString& text) const;
};

#endif // AUTOMATONDATA_H
