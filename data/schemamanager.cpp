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
                QStringLiteral("Это база полной версии. Для Lite нужна отдельная база реакций.");
        return false;
    }
    Transaction transaction(m_db);
    if (!transaction.active())
        return false;
    const QStringList sql = {
        "CREATE TABLE IF NOT EXISTS LiteMetadata (version INTEGER NOT NULL CHECK(version=1));",
        "INSERT INTO LiteMetadata SELECT 1 WHERE NOT EXISTS (SELECT 1 FROM LiteMetadata);",
        "CREATE TABLE IF NOT EXISTS Variants (id INTEGER PRIMARY KEY AUTOINCREMENT, number INTEGER "
        "NOT NULL UNIQUE, "
        "conversion TEXT NOT NULL CHECK(conversion IN ('MealyToMoore','MooreToMealy')), "
        "mealy_states INTEGER NOT NULL CHECK(mealy_states BETWEEN 1 AND 999999), "
        "mealy_inputs INTEGER NOT NULL CHECK(mealy_inputs BETWEEN 1 AND 10), "
        "mealy_outputs INTEGER NOT NULL CHECK(mealy_outputs BETWEEN 1 AND 10), "
        "moore_states INTEGER NOT NULL CHECK(moore_states BETWEEN 1 AND 999999), "
        "moore_inputs INTEGER NOT NULL CHECK(moore_inputs BETWEEN 1 AND 10), "
        "moore_outputs INTEGER NOT NULL CHECK(moore_outputs BETWEEN 1 AND 10));",
        "CREATE TABLE IF NOT EXISTS ReactionSteps (variant_id INTEGER NOT NULL REFERENCES "
        "Variants(id) ON DELETE CASCADE, "
        "position INTEGER NOT NULL CHECK(position>=0), input TEXT NOT NULL CHECK(input GLOB "
        "'x[0-9]'), "
        "outputs_json TEXT NOT NULL, PRIMARY KEY(variant_id,position));"};
    for (const auto &request : sql)
        if (!m_db->exec(request, {}, error))
            return false;
    auto version = m_db->query("SELECT version FROM LiteMetadata;");
    if (!version.next() || version.value(0).toInt() != 1 || version.next()) {
        if (error)
            *error = QStringLiteral("Неизвестная версия базы Lite.");
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
    for (const auto &table : {"ReactionSteps", "Variants", "LiteMetadata"})
        if (!m_db->exec(QStringLiteral("DROP TABLE IF EXISTS %1;").arg(table), {}, error))
            return false;
    return transaction.commit(error);
}
