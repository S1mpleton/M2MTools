#include "transaction.h"
#include "database.h"

#include <QSqlQuery>
#include <QSqlError>

Transaction::Transaction(Database* db) : m_db(db) {
    QSqlQuery q(m_db->handle());
    q.exec(QStringLiteral("BEGIN;"));
}

Transaction::~Transaction() {
    if (!m_finished) {
        rollback();
    }
}

bool Transaction::commit(QString* error) {
    if (m_finished) return false;
    m_finished = true;

    QSqlQuery q(m_db->handle());
    if (!q.exec(QStringLiteral("COMMIT;"))) {
        if (error) *error = q.lastError().text();
        return false;
    }
    return true;
}

void Transaction::rollback() {
    if (m_finished) return;
    m_finished = true;

    QSqlQuery q(m_db->handle());
    q.exec(QStringLiteral("ROLLBACK;"));
}