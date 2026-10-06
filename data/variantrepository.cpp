#include "variantrepository.h"
#include "data/database.h"


namespace {
    QSqlQuery runSelect(Database* db, const QString& sql, const QVariantMap& params = {}) {
        return db->query(sql, params);
    }
}

VariantRepository::VariantRepository(Database* db, QObject* parent)
    : m_db(db)
    , QObject(parent)
{}

std::optional<VariantRow> VariantRepository::findVariant(int number) const {
    QSqlQuery q = runSelect(m_db,
                            QStringLiteral("SELECT id, number, conversion "
                                           "FROM Variants "
                                           "WHERE number = :num"),
                            { {":num", number} });

    if (!q.isActive() || !q.next())
        return std::nullopt;

    VariantRow row;
    row.id         = q.value(0).toInt();
    row.number     = q.value(1).toInt();
    row.conversion = q.value(2).toString();
    return row;
}

QList<VariantRow> VariantRepository::findAllVariants() const {
    QSqlQuery q = runSelect(m_db,
                            QStringLiteral("SELECT id, number, conversion "
                                           "FROM Variants "
                                           "ORDER BY conversion, number"));

    QList<VariantRow> result;
    while (q.next()) {
        VariantRow row;
        row.id         = q.value(0).toInt();
        row.number     = q.value(1).toInt();
        row.conversion = q.value(2).toString();
        result.append(row);
    }
    return result;
}

int VariantRepository::insertVariant(int number,
                                     const QString& conversion,
                                     QString* error) {
    QSqlQuery q(m_db->handle());
    q.prepare(QStringLiteral(
        "INSERT INTO Variants (number, conversion) "
        "VALUES (:num, :conv)"));
    q.bindValue(":num", number);
    q.bindValue(":conv", conversion);

    if (!q.exec()) {
        if (error) *error = q.lastError().text();
        return -1;
    }
    return q.lastInsertId().toInt();
}

bool VariantRepository::removeVariant(int variantId, QString* error) {
    QSqlQuery q(m_db->handle());
    q.prepare(QStringLiteral("DELETE FROM Variants WHERE id = :id"));
    q.bindValue(":id", variantId);

    if (!q.exec()) {
        if (error) *error = q.lastError().text();
        return false;
    }
    return true;
}


// --- Read States / Inputs / Outputs ---
QList<StateRow> VariantRepository::findStates(int variantId) const {
    QSqlQuery q = runSelect(m_db,
                            QStringLiteral("SELECT id, variant_id, name, is_init "
                                           "FROM States "
                                           "WHERE variant_id = :vid "
                                           "ORDER BY id"),
                            { {":vid", variantId} });

    QList<StateRow> result;
    while (q.next()) {
        StateRow row;
        row.id        = q.value(0).toInt();
        row.variantId = q.value(1).toInt();
        row.name      = q.value(2).toString();
        row.isInit    = q.value(3).toInt() != 0;
        result.append(row);
    }
    return result;
}

QList<InputRow> VariantRepository::findInputs(int variantId) const {
    QSqlQuery q = runSelect(m_db,
                            QStringLiteral("SELECT id, variant_id, name "
                                           "FROM Inputs "
                                           "WHERE variant_id = :vid "
                                           "ORDER BY id"),
                            { {":vid", variantId} });

    QList<InputRow> result;
    while (q.next()) {
        InputRow row;
        row.id        = q.value(0).toInt();
        row.variantId = q.value(1).toInt();
        row.name      = q.value(2).toString();
        result.append(row);
    }
    return result;
}

QList<OutputRow> VariantRepository::findOutputs(int variantId) const {
    QSqlQuery q = runSelect(m_db,
                            QStringLiteral("SELECT id, variant_id, name "
                                           "FROM Outputs "
                                           "WHERE variant_id = :vid "
                                           "ORDER BY id"),
                            { {":vid", variantId} });

    QList<OutputRow> result;
    while (q.next()) {
        OutputRow row;
        row.id        = q.value(0).toInt();
        row.variantId = q.value(1).toInt();
        row.name      = q.value(2).toString();
        result.append(row);
    }
    return result;
}



// --- Write States / Inputs / Outputs ---

int VariantRepository::insertState(int variantId,
                                   const QString& name,
                                   bool isInit,
                                   QString* error) {
    QSqlQuery q(m_db->handle());
    q.prepare(QStringLiteral(
        "INSERT INTO States (variant_id, name, is_init) "
        "VALUES (:vid, :name, :init)"));
    q.bindValue(":vid",  variantId);
    q.bindValue(":name", name);
    q.bindValue(":init", isInit ? 1 : 0);

    if (!q.exec()) {
        if (error) *error = q.lastError().text();
        return -1;
    }
    return q.lastInsertId().toInt();
}

int VariantRepository::insertInput(int variantId,
                                   const QString& name,
                                   QString* error) {
    QSqlQuery q(m_db->handle());
    q.prepare(QStringLiteral(
        "INSERT INTO Inputs (variant_id, name) "
        "VALUES (:vid, :name)"));
    q.bindValue(":vid",  variantId);
    q.bindValue(":name", name);

    if (!q.exec()) {
        if (error) *error = q.lastError().text();
        return -1;
    }
    return q.lastInsertId().toInt();
}

int VariantRepository::insertOutput(int variantId,
                                    const QString& name,
                                    QString* error) {
    QSqlQuery q(m_db->handle());
    q.prepare(QStringLiteral(
        "INSERT INTO Outputs (variant_id, name) "
        "VALUES (:vid, :name)"));
    q.bindValue(":vid",  variantId);
    q.bindValue(":name", name);

    if (!q.exec()) {
        if (error) *error = q.lastError().text();
        return -1;
    }
    return q.lastInsertId().toInt();
}

// --- Read ---
QList<TransitionRow> VariantRepository::findTransitions(int variantId) const {
    QSqlQuery q = runSelect(m_db,
                            QStringLiteral("SELECT id, variant_id, from_state_id, "
                                           "       to_state_id, input_signal_id "
                                           "FROM Transitions "
                                           "WHERE variant_id = :vid "
                                           "ORDER BY id"),
                            { {":vid", variantId} });

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
    return result;
}

QList<MealyTransitionOutputRow> VariantRepository::findMealyOutputs(
    int transitionId) const {
    QSqlQuery q = runSelect(m_db,
                            QStringLiteral("SELECT transition_id, output_id "
                                           "FROM MealyTransitionOutputs "
                                           "WHERE transition_id = :tid "
                                           "ORDER BY output_id"),
                            { {":tid", transitionId} });

    QList<MealyTransitionOutputRow> result;
    while (q.next()) {
        MealyTransitionOutputRow row;
        row.transitionId = q.value(0).toInt();
        row.outputId     = q.value(1).toInt();
        result.append(row);
    }
    return result;
}

QList<MooreStateOutputRow> VariantRepository::findMooreOutputs(
    int variantId) const {
    QSqlQuery q = runSelect(m_db,
                            QStringLiteral(
                                "SELECT mso.state_id, mso.output_id "
                                "FROM MooreStateOutputs mso "
                                "JOIN States s ON s.id = mso.state_id "
                                "WHERE s.variant_id = :vid "
                                "ORDER BY mso.state_id, mso.output_id"),
                            { {":vid", variantId} });

    QList<MooreStateOutputRow> result;
    while (q.next()) {
        MooreStateOutputRow row;
        row.stateId  = q.value(0).toInt();
        row.outputId = q.value(1).toInt();
        result.append(row);
    }
    return result;
}

// --- Write ---
int VariantRepository::insertTransition(int variantId,
                                        int fromStateId,
                                        std::optional<int> toStateId,
                                        int inputSignalId,
                                        QString* error) {
    QSqlQuery q(m_db->handle());
    q.prepare(QStringLiteral(
        "INSERT INTO Transitions "
        "    (variant_id, from_state_id, to_state_id, input_signal_id) "
        "VALUES (:vid, :from, :to, :input)"));
    q.bindValue(":vid",   variantId);
    q.bindValue(":from",  fromStateId);
    q.bindValue(":input", inputSignalId);

    if (toStateId.has_value())
        q.bindValue(":to", *toStateId);
    else
        q.bindValue(":to", QVariant(QMetaType(QMetaType::Int)));

    if (!q.exec()) {
        if (error) *error = q.lastError().text();
        return -1;
    }
    return q.lastInsertId().toInt();
}

bool VariantRepository::insertMealyOutput(int transitionId,
                                          int outputId,
                                          QString* error) {
    QSqlQuery q(m_db->handle());
    q.prepare(QStringLiteral(
        "INSERT INTO MealyTransitionOutputs (transition_id, output_id) "
        "VALUES (:tid, :oid)"));
    q.bindValue(":tid", transitionId);
    q.bindValue(":oid", outputId);

    if (!q.exec()) {
        if (error) *error = q.lastError().text();
        return false;
    }
    return true;
}

bool VariantRepository::insertMooreOutput(int stateId,
                                          int outputId,
                                          QString* error) {
    QSqlQuery q(m_db->handle());
    q.prepare(QStringLiteral(
        "INSERT INTO MooreStateOutputs (state_id, output_id) "
        "VALUES (:sid, :oid)"));
    q.bindValue(":sid", stateId);
    q.bindValue(":oid", outputId);

    if (!q.exec()) {
        if (error) *error = q.lastError().text();
        return false;
    }
    return true;
}