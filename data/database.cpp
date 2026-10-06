#include "database.h"

#include <QFileInfo>
#include <QDir>
#include <QUuid>
#include <QtSql/QSqlDatabase>

Database::Database(const QString& connectionName, const QString& path, QObject* parent)
    : QObject(parent), m_connectionName(connectionName), m_path(path)
{
}

Database* Database::open(const QString& path, QObject* parent) {
    const QFileInfo info(path);
    const QDir dir = info.absoluteDir();
    if (!dir.exists()) {
        qWarning() << "Database directory does not exist:" << dir.absolutePath();
        return nullptr;
    }

    const QString connectionName =
        QStringLiteral("db_%1")
            .arg(QUuid::createUuid().toString(QUuid::WithoutBraces));

    QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), connectionName);
    db.setDatabaseName(path);

    if (!db.open()) {
        qWarning() << "Failed to open database:" << db.lastError().text();
        QSqlDatabase::removeDatabase(connectionName);
        return nullptr;
    }

    QSqlQuery pragma(db);
    if (!pragma.exec(QStringLiteral("PRAGMA foreign_keys = ON;"))) {
        qWarning() << "Failed to enable foreign keys:" << pragma.lastError().text();
    }

    auto* database = new Database(connectionName, path, parent);

    emit database->opened();
    return database;
}

Database::~Database() {
    {
        QSqlDatabase db = QSqlDatabase::database(m_connectionName, false);
        if (db.isOpen()) {
            db.close();
            emit closed();
        }
    }

    QSqlDatabase::removeDatabase(m_connectionName);
}

QSqlDatabase Database::handle() const {
    return QSqlDatabase::database(m_connectionName, false);
}

bool Database::isOpen() const {
    return handle().isOpen();
}

bool Database::exec(const QString& sql, const QVariantMap& params, QString* error) {
    QSqlQuery q = query(sql, params, error);
    return q.isActive();
}

QSqlQuery Database::query(const QString& sql, const QVariantMap& params, QString* error) {
    QSqlDatabase db = handle();
    QSqlQuery q(db);

    if (!q.prepare(sql)) {
        m_lastError = q.lastError();
        if (error) *error = m_lastError.text();
        emit errorOccurred(m_lastError.text());
        return q;
    }

    for (auto it = params.constBegin(); it != params.constEnd(); ++it) {
        q.bindValue(it.key(), it.value());
    }

    if (!q.exec()) {
        m_lastError = q.lastError();
        if (error) *error = m_lastError.text();
        emit errorOccurred(m_lastError.text());
    }

    return q;
}
