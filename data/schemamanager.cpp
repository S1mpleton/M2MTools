#include "schemamanager.h"
#include "data/transaction.h"
#include "database.h"

SchemaManager::SchemaManager(Database *db, QObject *parent) : QObject(parent), m_db(db) {}

bool SchemaManager::createSchema(QString *error) {
    // Refuse full-edition databases: their original trace order was not stored.
    auto old = m_db->query(
        "SELECT name FROM sqlite_master WHERE type='table' AND name IN ('States','Transitions');");
    if (!old.isActive() || old.next()) {
        if (error)
            *error =
                tr("This is a full-edition database. A separate reaction database is required.");
        return false;
    }
    Transaction transaction(m_db);
    if (!transaction.active())
        return false;
    const QString reactionDefinition =
        "(variant_id INTEGER NOT NULL REFERENCES Variants(id) ON DELETE CASCADE, "
        "position INTEGER NOT NULL CHECK(position>=0), "
        "input TEXT NOT NULL CHECK(input='' OR input GLOB 'x[0-9]'), "
        "outputs_json TEXT NOT NULL, PRIMARY KEY(variant_id,position));";
    const QStringList sql = {
        "CREATE TABLE IF NOT EXISTS LiteMetadata (version INTEGER NOT NULL CHECK(version=2));",
        "INSERT INTO LiteMetadata SELECT 2 WHERE NOT EXISTS (SELECT 1 FROM LiteMetadata);",
        "CREATE TABLE IF NOT EXISTS Variants (id INTEGER PRIMARY KEY AUTOINCREMENT, number INTEGER "
        "NOT NULL UNIQUE, "
        "conversion TEXT NOT NULL CHECK(conversion IN ('MealyToMoore','MooreToMealy')), "
        "mealy_states INTEGER NOT NULL CHECK(mealy_states BETWEEN 1 AND 999999), "
        "mealy_inputs INTEGER NOT NULL CHECK(mealy_inputs BETWEEN 1 AND 10), "
        "mealy_outputs INTEGER NOT NULL CHECK(mealy_outputs BETWEEN 1 AND 10), "
        "moore_states INTEGER NOT NULL CHECK(moore_states BETWEEN 1 AND 999999), "
        "moore_inputs INTEGER NOT NULL CHECK(moore_inputs BETWEEN 1 AND 10), "
        "moore_outputs INTEGER NOT NULL CHECK(moore_outputs BETWEEN 1 AND 10));",
        "CREATE TABLE IF NOT EXISTS ReactionSteps " + reactionDefinition,
        // Additive migration: existing Lite variants keep their reactions and have no attachment.
        "CREATE TABLE IF NOT EXISTS VariantImages (variant_id INTEGER PRIMARY KEY REFERENCES "
        "Variants(id) ON DELETE CASCADE, data BLOB NOT NULL, file_name TEXT NOT NULL);"};
    for (const auto &request : sql)
        if (!m_db->exec(request, {}, error))
            return false;
    auto version = m_db->query("SELECT version FROM LiteMetadata;");
    const int schemaVersion = version.next() ? version.value(0).toInt() : -1;
    if ((schemaVersion != 1 && schemaVersion != 2) || version.next()) {
        if (error)
            *error = tr("Unknown reaction database version.");
        return false;
    }
    version.finish();
    if (schemaVersion == 1) {
        // SQLite cannot alter a CHECK constraint. Copy every stored step in the
        // same transaction before replacing the old table and schema version.
        const QStringList migration = {
            "CREATE TABLE ReactionStepsUpgrade " + reactionDefinition,
            "INSERT INTO ReactionStepsUpgrade SELECT variant_id,position,input,outputs_json FROM ReactionSteps;",
            "DROP TABLE ReactionSteps;",
            "ALTER TABLE ReactionStepsUpgrade RENAME TO ReactionSteps;",
            "DROP TABLE LiteMetadata;",
            "CREATE TABLE LiteMetadata (version INTEGER NOT NULL CHECK(version=2));",
            "INSERT INTO LiteMetadata VALUES(2);"};
        for (const auto &request : migration)
            if (!m_db->exec(request, {}, error))
                return false;
    }
    if (!transaction.commit(error))
        return false;
    emit schemaCreated();
    return true;
}

bool SchemaManager::dropSchema(QString *error) {
    Transaction transaction(m_db);
    if (!transaction.active())
        return false;
    for (const auto &table : {"VariantImages", "ReactionSteps", "Variants", "LiteMetadata"})
        if (!m_db->exec(QStringLiteral("DROP TABLE IF EXISTS %1;").arg(table), {}, error))
            return false;
    return transaction.commit(error);
}
