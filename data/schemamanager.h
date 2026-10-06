#ifndef SCHEMAMANAGER_H
#define SCHEMAMANAGER_H

#include <QObject>
#include <QString>

class Database;

class SchemaManager : public QObject {
    Q_OBJECT
public:
    explicit SchemaManager(Database* db, QObject* parent = nullptr);

    bool createSchema(QString* error = nullptr);

    bool dropSchema(QString* error = nullptr);

    // int currentVersion(QString* error = nullptr) const;
    // bool migrateTo(int targetVersion, QString* error = nullptr);

signals:
    void schemaCreated();
    void schemaError(const QString& message);

private:
    Database* m_db;
};
#endif // SCHEMAMANAGER_H
