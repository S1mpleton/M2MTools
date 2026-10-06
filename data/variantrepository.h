#ifndef VARIANTREPOSITORY_H
#define VARIANTREPOSITORY_H

#include "data/database.h"

#include <QString>

struct VariantRow {
    int id = -1;
    int number = 0;
    QString conversion;
};

struct StateRow {
    int id = -1;
    int variantId = -1;
    QString name;
    bool isInit = false;
};

struct InputRow {
    int id = -1;
    int variantId = -1;
    QString name;
};

struct OutputRow {
    int id = -1;
    int variantId = -1;
    QString name;
};

struct TransitionRow {
    int id = -1;
    int variantId = -1;
    int fromStateId = -1;
    std::optional<int> toStateId;
    int inputSignalId = -1;
};

struct MealyTransitionOutputRow {
    int transitionId = -1;
    int outputId = -1;
};

struct MooreStateOutputRow {
    int stateId = -1;
    int outputId = -1;
};


class VariantRepository : public QObject {
    Q_OBJECT
public:
    explicit VariantRepository(Database* db, QObject* parent = nullptr);

    // --- Variants ---
    std::optional<VariantRow> findVariant(int number) const;

    QList<VariantRow> findAllVariants() const;

    int insertVariant(int number, const QString& conversion, QString* error = nullptr);

    bool removeVariant(int variantId, QString* error = nullptr);

    // --- States / Inputs / Outputs ---
    QList<StateRow>  findStates(int variantId) const;
    QList<InputRow>  findInputs(int variantId) const;
    QList<OutputRow> findOutputs(int variantId) const;

    int insertState(int variantId, const QString& name, bool isInit, QString* error = nullptr);
    int insertInput(int variantId, const QString& name, QString* error = nullptr);
    int insertOutput(int variantId, const QString& name, QString* error = nullptr);

    // --- Transitions ---
    QList<TransitionRow> findTransitions(int variantId) const;
    QList<MealyTransitionOutputRow> findMealyOutputs(int transitionId) const;
    QList<MooreStateOutputRow> findMooreOutputs(int variantId) const;

    int insertTransition(
        int variantId, int fromStateId, std::optional<int> toStateId,
        int inputSignalId, QString* error = nullptr
    );

    bool insertMealyOutput(int transitionId, int outputId, QString* error = nullptr);
    bool insertMooreOutput(int stateId, int outputId, QString* error = nullptr);

private:
    Database* m_db;
};

#endif // VARIANTREPOSITORY_H
