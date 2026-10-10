#ifndef VARIANTREPOSITORY_H
#define VARIANTREPOSITORY_H

#include "core/result.h"
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

namespace {
    QSqlQuery runSelect(Database* db, const QString& sql, const QVariantMap& params = {});
    Result makeDatabaseError(const QSqlQuery& q, const QString& context);
}

class VariantRepository : public QObject {
    Q_OBJECT
public:
    explicit VariantRepository(Database* db, QObject* parent = nullptr);

    // --- Variants ---
    Result findVariant(int number) const;

    Result findAllVariants() const;

    Result insertVariant(int number, const QString& conversion);

    Result removeVariant(int variantId);

    // --- States / Inputs / Outputs ---
    Result  findStates(int variantId) const;
    Result  findInputs(int variantId) const;
    Result findOutputs(int variantId) const;

    Result insertState(int variantId, const QString& name, bool isInit);
    Result insertInput(int variantId, const QString& name);
    Result insertOutput(int variantId, const QString& name);

    // --- Transitions ---
    Result findTransitions(int variantId) const;
    Result findMealyOutputs(int transitionId) const;
    Result findMooreOutputs(int variantId) const;

    Result insertTransition(
        int variantId, int fromStateId,
        std::optional<int> toStateId, int inputSignalId
    );

    Result insertMealyOutput(int transitionId, int outputId);
    Result insertMooreOutput(int stateId, int outputId);

    Result findMealyOutputsByVariant(int variantId) const;

private:
    Database* m_db;
};

#endif // VARIANTREPOSITORY_H
