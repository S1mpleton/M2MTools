#ifndef DATABASE_H
#define DATABASE_H

#include <QObject>
#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlError>
#include <QtSql/QSqlQuery>

class Database : public QObject {
    Q_OBJECT
public:
    static Database* open(const QString& path, QObject* parent = nullptr);

    ~Database() override;

    Database(const Database&) = delete;
    Database& operator=(const Database&) = delete;

    QSqlDatabase handle() const;

    bool exec(const QString& sql, const QVariantMap& params = {}, QString* error = nullptr);
    QSqlQuery query(const QString& sql, const QVariantMap& params = {}, QString* error = nullptr);

    QString path() const { return m_path; }
    QString connectionName() const { return m_connectionName; }
    bool isOpen() const;

    QSqlError lastError() const { return m_lastError; }

    bool createSchema(QString* error = nullptr);

signals:
    void opened();
    void closed();
    void errorOccurred(const QString& message);

private:
    explicit Database(const QString& connectionName, const QString& path, QObject* parent = nullptr);

    QString m_connectionName;
    QString m_path;
    mutable QSqlError m_lastError;
};
#endif // DATABASE_H
