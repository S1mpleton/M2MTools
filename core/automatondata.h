#ifndef AUTOMATONDATA_H
#define AUTOMATONDATA_H

#include "core/result.h"

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

enum class VariantType {
    MealyToMoore,
    MooreToMealy
};

enum class CellKind {
    Empty,              // something uneditable
    StateHeader,        // "State" / Name state
    InputHeader,        // "x1", "x2"
    MooreOutput,        // "Output" — state output
    Transition          // transition cell (Moore or Mealy)
};

enum class ValidationLevel {
    Soft,
    Strict
};

FieldType fieldTypeStringToType(QString fieldTypeString);
QString fieldTypeToString(FieldType fieldType);
QString fieldTypeDisplayName(FieldType fieldType);

VariantType variantTypeStringToType(QString variantTypeString);
QString variantTypeToString(VariantType variantType);
QString variantTypeDisplayName(VariantType variantType);

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

    Result setInitialState(const QString& name);
    Result setStateNames(const QStringList& names);
    Result setInputSignalNames(const QStringList& names);
    Result setOutputSignalNames(const QStringList& names);

    Result setTransitionCell(int row, int col, const QString& text);
    Result setMooreOutputCell(int col,const QString& text);

    Result setTransitionCellByName(const QString& inputName, const QString& stateName, const QString& text);
    Result setMooreOutputCellByName(const QString& stateName, const QString& text);

    void setTransition(int inputIndex, int stateIndex, const CellData& cell);
    void setMooreOutput(const QString& state, const QStringList& outputs);

    // Validate automat
    Result validate() const;

    using StateUsageChecker = std::function<Result(const AutomatonData&)>;
    void setStateUsageChecker(StateUsageChecker checker);

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
    StateUsageChecker m_stateUsageChecker;

    static constexpr int kSchemaVersion = 1;

    void rebuildTransitionTable();
    Result checkInvariant(const QStringList& candidate, FieldType field) const;
    Result validateName(const QString& name) const;
    Result validateNameList(const QStringList& names) const;
    Result validateCellContent(int row, int col, const QString& text) const;

    // Validate automat
    Result validateStateUsage() const;    // default sweat
    Result validateInputUsage() const;
    Result validateOutputUsage() const;

    Result validateStateUsageStrict() const;
    Result validateStateUsageLenient() const;
\
};

Q_DECLARE_METATYPE(AutomatonData)

#endif // AUTOMATONDATA_H
