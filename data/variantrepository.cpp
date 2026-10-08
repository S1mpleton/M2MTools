#include "variantrepository.h"
#include "data/sqlrequests.h"
#include "data/database.h"

#include "core/result.h"

namespace {
    QSqlQuery runSelect(Database* db, const QString& sql, const QVariantMap& params = {}) {
        return db->query(sql, params);
    }

    Result makeDatabaseError(const QSqlQuery& q, const QString& context) {
        return Result::error(context, ResultCategory::Database)
        .withDetails(q.lastError().text())
            .withCode(q.lastError().nativeErrorCode().toInt());
    }
}

VariantRepository::VariantRepository(Database* db, QObject* parent)
    : QObject(parent)
    , m_db(db)
{}

Result VariantRepository::findVariant(int number) const {
    QSqlQuery q = m_db->query(
        SQLRequests::SELECT_VARIANT_BY_NUMBER_AND_TYPE,
        { {":num", number} });

    if (!q.isActive()) {
        return makeDatabaseError(q,
            QStringLiteral("Failed to search variant (number=%1)").arg(number));
    }

    if (!q.next()) {
        return Result::warning(
            QStringLiteral("Variant '%1' not found").arg(number));
    }

    VariantRow row;
    row.id = q.value(0).toInt();
    row.number = q.value(1).toInt();
    row.conversion = q.value(2).toString();

    return Result::success(
        QString("Variant '%1' found").arg(number))
        .withPayload(row);
}

Result VariantRepository::findAllVariants() const {
    QSqlQuery q = m_db->query(SQLRequests::SELECT_ALL_VARIANTS);

    if (!q.isActive()) {
        return makeDatabaseError(q,
            QStringLiteral("Failed to search all variants"));
    }

    QList<VariantRow> result;
    while (q.next()) {
        VariantRow row;
        row.id = q.value(0).toInt();
        row.number = q.value(1).toInt();
        row.conversion = q.value(2).toString();
        result.append(row);
    }

    return Result::success().withPayload(result);
}

Result VariantRepository::insertVariant(int number, const QString& conversion) {
    QSqlQuery q(m_db->handle());
    q.prepare(SQLRequests::INSERT_VARIANT);
    q.bindValue(":num",  number);
    q.bindValue(":conv", conversion);

    if (!q.exec()) {
        return makeDatabaseError(q,
            QStringLiteral("Failed to insert variant (number=%1)").arg(number));
    }

    return Result::success(
        QStringLiteral("Variant '%1' inserted").arg(number))
        .withPayload(q.lastInsertId().toInt());
}

Result VariantRepository::removeVariant(int variantId) {
    QSqlQuery q(m_db->handle());
    q.prepare(SQLRequests::DELETE_VARIANT);
    q.bindValue(":id", variantId);

    if (!q.exec()) {
        return makeDatabaseError(q,
            QStringLiteral("Failed to remove variant (id=%1)").arg(variantId));
    }

    return Result::success(
        QStringLiteral("Variant (id=%1) removed").arg(variantId));
}

// States / Inputs / Outputs: read
Result VariantRepository::findStates(int variantId) const {
    QSqlQuery q = m_db->query(
        SQLRequests::SELECT_STATES_BY_VARIANT,
        { {":vid", variantId} });

    if (!q.isActive()) {
        return makeDatabaseError(q,
            QStringLiteral("Failed to search states (variant=%1)").arg(variantId));
    }

    QList<StateRow> result;
    while (q.next()) {
        StateRow row;
        row.id        = q.value(0).toInt();
        row.variantId = q.value(1).toInt();
        row.name      = q.value(2).toString();
        row.isInit    = q.value(3).toInt() != 0;
        result.append(row);
    }

    return Result::success().withPayload(result);
}

Result VariantRepository::findInputs(int variantId) const {
    QSqlQuery q = m_db->query(
        SQLRequests::SELECT_INPUTS_BY_VARIANT,
        { {":vid", variantId} });

    if (!q.isActive()) {
        return makeDatabaseError(q,
            QStringLiteral("Failed to search inputs (variant=%1)").arg(variantId));
    }

    QList<InputRow> result;
    while (q.next()) {
        InputRow row;
        row.id        = q.value(0).toInt();
        row.variantId = q.value(1).toInt();
        row.name      = q.value(2).toString();
        result.append(row);
    }

    return Result::success().withPayload(result);
}

Result VariantRepository::findOutputs(int variantId) const {
    QSqlQuery q = m_db->query(
        SQLRequests::SELECT_OUTPUTS_BY_VARIANT,
        { {":vid", variantId} });

    if (!q.isActive()) {
        return makeDatabaseError(q,
            QStringLiteral("Failed to search outputs (variant=%1)").arg(variantId));
    }

    QList<OutputRow> result;
    while (q.next()) {
        OutputRow row;
        row.id        = q.value(0).toInt();
        row.variantId = q.value(1).toInt();
        row.name      = q.value(2).toString();
        result.append(row);
    }

    return Result::success().withPayload(result);
}

// States / Inputs / Outputs: write
Result VariantRepository::insertState(int variantId, const QString& name, bool isInit) {
    QSqlQuery q(m_db->handle());
    q.prepare(SQLRequests::INSERT_STATE);
    q.bindValue(":vid",  variantId);
    q.bindValue(":name", name);
    q.bindValue(":init", isInit ? 1 : 0);

    if (!q.exec()) {
        return makeDatabaseError(q,
            QStringLiteral("Failed to insert state '%1'").arg(name))
            .withOffender(name);
    }

    return Result::success()
        .withPayload(q.lastInsertId().toInt());
}

Result VariantRepository::insertInput(int variantId, const QString& name) {
    QSqlQuery q(m_db->handle());
    q.prepare(SQLRequests::INSERT_INPUT);
    q.bindValue(":vid",  variantId);
    q.bindValue(":name", name);

    if (!q.exec()) {
        return makeDatabaseError(q,
            QStringLiteral("Failed to insert input '%1'").arg(name))
            .withOffender(name);
    }

    return Result::success()
        .withPayload(q.lastInsertId().toInt());
}

Result VariantRepository::insertOutput(int variantId, const QString& name) {
    QSqlQuery q(m_db->handle());
    q.prepare(SQLRequests::INSERT_OUTPUT);
    q.bindValue(":vid",  variantId);
    q.bindValue(":name", name);

    if (!q.exec()) {
        return makeDatabaseError(q,
            QStringLiteral("Failed to insert output '%1'").arg(name))
            .withOffender(name);
    }

    return Result::success()
        .withPayload(q.lastInsertId().toInt());
}

// Transitions: read
Result VariantRepository::findTransitions(int variantId) const {
    QSqlQuery q = m_db->query(
        SQLRequests::SELECT_TRANSITIONS_BY_VARIANT,
        { {":vid", variantId} });

    if (!q.isActive()) {
        return makeDatabaseError(q,
            QStringLiteral("Failed to search transitions (variant=%1)")
            .arg(variantId));
    }

    QList<TransitionRow> result;
    while (q.next()) {
        TransitionRow row;
        row.id            = q.value(0).toInt();
        row.variantId     = q.value(1).toInt();
        row.fromStateId   = q.value(2).toInt();
        row.inputSignalId = q.value(4).toInt();

        // to_state_id can be NULL
        const QVariant toState = q.value(3);
        if (!toState.isNull())
            row.toStateId = toState.toInt();

        result.append(row);
    }

    return Result::success().withPayload(result);
}

Result VariantRepository::findMealyOutputs(int transitionId) const {
    QSqlQuery q = m_db->query(
        SQLRequests::SELECT_MEALY_OUTPUTS_BY_TRANSITION,
        { {":tid", transitionId} });

    if (!q.isActive()) {
        return makeDatabaseError(q,
            QStringLiteral("Failed to search Mealy outputs (transition=%1)")
            .arg(transitionId));
    }

    QList<MealyTransitionOutputRow> result;
    while (q.next()) {
        MealyTransitionOutputRow row;
        row.transitionId = q.value(0).toInt();
        row.outputId     = q.value(1).toInt();
        result.append(row);
    }

    return Result::success().withPayload(result);
}

Result VariantRepository::findMooreOutputs(int variantId) const {
    QSqlQuery q = m_db->query(
        SQLRequests::SELECT_MOORE_OUTPUTS_BY_VARIANT,
        { {":vid", variantId} });

    if (!q.isActive()) {
        return makeDatabaseError(q,
        QStringLiteral("Failed to search Moore outputs (variant=%1)")
        .arg(variantId));
    }

    QList<MooreStateOutputRow> result;
    while (q.next()) {
        MooreStateOutputRow row;
        row.stateId  = q.value(0).toInt();
        row.outputId = q.value(1).toInt();
        result.append(row);
    }

    return Result::success().withPayload(result);
}

// Transitions: write
Result VariantRepository::insertTransition(int variantId,
       int fromStateId,
       std::optional<int> toStateId,
       int inputSignalId) {
    QSqlQuery q(m_db->handle());
    q.prepare(SQLRequests::INSERT_TRANSITION);
    q.bindValue(":vid",   variantId);
    q.bindValue(":from",  fromStateId);
    q.bindValue(":input", inputSignalId);

    if (toStateId.has_value())
        q.bindValue(":to", *toStateId);
    else
        q.bindValue(":to", QVariant(QMetaType(QMetaType::Int)));

    if (!q.exec()) {
        return makeDatabaseError(q,
            QStringLiteral("Failed to insert transition "
                "(variant=%1, from=%2, input=%3)")
            .arg(variantId).arg(fromStateId).arg(inputSignalId));
    }

    return Result::success()
        .withPayload(q.lastInsertId().toInt());
}

Result VariantRepository::insertMealyOutput(int transitionId, int outputId) {
    QSqlQuery q(m_db->handle());
    q.prepare(SQLRequests::INSERT_MEALY_OUTPUT);
    q.bindValue(":tid", transitionId);
    q.bindValue(":oid", outputId);

    if (!q.exec()) {
        return makeDatabaseError(q,
            QStringLiteral("Failed to link transition %1 with output %2")
            .arg(transitionId).arg(outputId));
    }

    return Result::success();
}

Result VariantRepository::insertMooreOutput(int stateId, int outputId) {
    QSqlQuery q(m_db->handle());
    q.prepare(SQLRequests::INSERT_MOORE_OUTPUT);
    q.bindValue(":sid", stateId);
    q.bindValue(":oid", outputId);

    if (!q.exec()) {
        return makeDatabaseError(q,
            QStringLiteral("Failed to link state %1 with output %2")
            .arg(stateId).arg(outputId));
    }

    return Result::success();
}

Result VariantRepository::findMealyOutputsByVariant(int variantId) const {
    QSqlQuery q = m_db->query(
        SQLRequests::SELECT_MEALY_OUTPUTS_BY_VARIANT,
        { {":vid", variantId} });

    if (!q.isActive()) {
        return makeDatabaseError(q,
            QStringLiteral("Failed to search Mealy outputs (variant=%1)")
                .arg(variantId));
    }

    QHash<int, QList<int>> result;
    while (q.next()) {
        const int transitionId = q.value(0).toInt();
        const int outputId = q.value(1).toInt();
        result[transitionId].append(outputId);
    }
    return Result::success().withPayload(result);
}