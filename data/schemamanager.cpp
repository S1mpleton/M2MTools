#include "schemamanager.h"
#include "database.h"
#include "sqlrequests.h"

#include <QSqlQuery>
#include <QSqlError>

SchemaManager::SchemaManager(Database* db, QObject* parent)
    : QObject(parent), m_db(db) {}

bool SchemaManager::createSchema(QString* error) {
    const QList<QString> requests = {
        SQLRequests::CREATE_VARIANT_TABLE,
        SQLRequests::CREATE_INPUT_TABLE,
        SQLRequests::CREATE_OUTPUTS_TABLE,
        SQLRequests::CREATE_STATE_TABLE,
        SQLRequests::CREATE_TRANSITION_TABLE,
        SQLRequests::CREATE_MEALY_TRANSITION_OUTPUTS_TABLE,
        SQLRequests::CREATE_MOORE_STATE_OUTPUTS_TABLE,
        SQLRequests::CREATE_UNIQUE_INIT_STATE_INDEX,
    };

    for (const QString& sql : requests) {
        QString localError;
        if (!m_db->exec(sql, {}, &localError)) {
            const QString msg = QStringLiteral("Schema creation failed:\n%1\nSQL: %2")
                .arg(localError, sql);

            if (error) *error = msg;
            emit schemaError(msg);
            return false;
        }
    }

    emit schemaCreated();
    return true;
}

bool SchemaManager::dropSchema(QString* error) {
    const QList<QString> tables = {
        "MealyTransitionOutputs",
        "MooreStateOutputs",
        "Transitions",
        "States",
        "Outputs",
        "Inputs",
        "Variants",
    };

    for (const QString& table : tables) {
        QString localError;
        if (!m_db->exec(QStringLiteral("DROP TABLE IF EXISTS %1;").arg(table),
                        {}, &localError)) {
            if (error) *error = localError;
            emit schemaError(localError);
            return false;
        }
    }
    return true;
}