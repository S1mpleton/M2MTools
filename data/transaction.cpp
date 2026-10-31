#include "transaction.h"
#include "database.h"

#include <QSqlQuery>
#include <QSqlError>

Transaction::Transaction(Database* db) : m_db(db) {
    QSqlQuery q(m_db->handle());
    m_started = q.exec(QStringLiteral("BEGIN;"));
}

Transaction::~Transaction() {
    if (!m_finished) {
        rollback();
    }
}

bool Transaction::commit(QString* error) {
    if (!active()) return false;

    QSqlQuery q(m_db->handle());
    if (!q.exec(QStringLiteral("COMMIT;"))) {
        if (error) *error = q.lastError().text();
        return false;
    }
    m_finished = true;
    return true;
}

void Transaction::rollback() {
    if (!active()) return;
    m_finished = true;

    QSqlQuery q(m_db->handle());
    q.exec(QStringLiteral("ROLLBACK;"));
}