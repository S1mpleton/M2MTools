#ifndef RESULT_H
#define RESULT_H

#include <QObject>
#include <QString>
#include <QDateTime>

// Q_DECLARE_METATYPE(bool)

enum class ResultCategory {
    General,
    Validation,
    Database,
    Service,
    Ui,
    Hash,
    Unknown,
};

enum class ResultSeverity {
    Success,
    Warning,
    Error,
};

ResultCategory resultCategoryStringToType(QString resultCategoryString);
QString resultCategoryToString(ResultCategory resultCategory);
QString resultCategoryDisplayName(ResultCategory resultCategory);

ResultSeverity resultSeverityStringToType(QString resultSeverityString);
QString resultSeverityToString(ResultSeverity resultSeverity);
QString resultSeverityDisplayName(ResultSeverity resultSeverity);

class Result
{
public:
    static Result success(const QString& message = {}) {
        return Result(ResultSeverity::Success, ResultCategory::General, message);
    }

    static Result warning(const QString& message, ResultCategory category = ResultCategory::General) {
        return Result(ResultSeverity::Warning, category, message);
    }

    static Result error(const QString& message, ResultCategory category = ResultCategory::General) {
        return Result(ResultSeverity::Error, category, message);
    }

    void updateMessage(const QString& message);

    bool isSuccess() const { return m_severity == ResultSeverity::Success; }
    bool isWarning() const { return m_severity == ResultSeverity::Warning; }
    bool isError() const { return m_severity == ResultSeverity::Error; }

    bool ok() const { return !isError(); }

    ResultSeverity severity() const { return m_severity; }
    ResultCategory category() const { return m_category; }
    QString message() const { return m_message; }

    QString details() const { return m_details; }

    QString offender() const { return m_offender; }

    int code() const { return m_code; }

    QDateTime timestamp() const { return m_timestamp; }

    QVariant payload() const { return m_payload; }
    bool hasPayload() const { return m_payload.isValid(); }

    template<typename T>
    T payloadAs() const {
        return m_payload.value<T>();
    }

    // --- chainable ---
    template<typename T>
    Result& withPayload(const T value) {
        m_payload = QVariant::fromValue(value);
        return *this;
    }

    Result& withOffender(const QString& offender) {
        m_offender = offender;
        return *this;
    }

    Result& withDetails(const QString& details) {
        m_details = details;
        return *this;
    }

    Result& withCode(int code) {
        m_code = code;
        return *this;
    }

    Result& withCategory(ResultCategory category) {
        m_category = category;
        return *this;
    }

private:
    Result(ResultSeverity severity, ResultCategory category, const QString& message)
        : m_severity(severity)
        , m_category(category)
        , m_message(message)
        , m_timestamp(QDateTime::currentDateTime())
    {}

    QVariant m_payload;

    ResultSeverity m_severity;
    ResultCategory m_category;
    QString m_message;
    QString m_details;
    QString m_offender;
    int m_code = 200;
    QDateTime m_timestamp;

};

#endif // RESULT_H
