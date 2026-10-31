#include "result.h"
#include <QCoreApplication>

namespace {
QString formatMessage(QString text, const QStringList &arguments) {
    for (const auto &argument : arguments)
        text = text.arg(argument);
    return text;
}
} // namespace

Result Result::successMessage(const char *source, const QStringList &arguments) {
    auto result = success(formatMessage(QString::fromUtf8(source), arguments));
    result.m_messageSource = source;
    result.m_messageArguments = arguments;
    return result;
}

Result Result::errorMessage(const char *source, const QStringList &arguments,
                            ResultCategory category) {
    auto result = successMessage(source, arguments);
    result.m_severity = ResultSeverity::Error;
    result.m_category = category;
    return result;
}

QString Result::localizedMessage() const {
    return hasMessageTemplate()
               ? formatMessage(
                     QCoreApplication::translate("ResultMessage", m_messageSource.constData()),
                     m_messageArguments)
               : QCoreApplication::translate("ResultMessage", m_message.toUtf8().constData());
}

ResultCategory resultCategoryStringToType(QString resultCategoryString) {
    if (resultCategoryString == QString("General"))
        return ResultCategory::General;
    if (resultCategoryString == QString("Validation"))
        return ResultCategory::Validation;
    if (resultCategoryString == QString("Database"))
        return ResultCategory::Database;
    if (resultCategoryString == QString("Service"))
        return ResultCategory::Service;
    if (resultCategoryString == QString("Ui"))
        return ResultCategory::Ui;
    if (resultCategoryString == QString("Hash"))
        return ResultCategory::Hash;
    if (resultCategoryString == QString("Translation"))
        return ResultCategory::Translation;
    if (resultCategoryString == QString("Unknown"))
        return ResultCategory::Unknown;

    qDebug() << "Error: resultCategoryString to ResultCategory - Not found";
    return ResultCategory::Unknown;
}

QString resultCategoryToString(ResultCategory resultCategory) {
    switch (resultCategory) {
    case ResultCategory::Translation:
        return QStringLiteral("Translation");
    case ResultCategory::General:
        return QStringLiteral("General");
    case ResultCategory::Validation:
        return QStringLiteral("Validation");
    case ResultCategory::Database:
        return QStringLiteral("Database");
    case ResultCategory::Service:
        return QStringLiteral("Service");
    case ResultCategory::Ui:
        return QStringLiteral("Ui");
    case ResultCategory::Hash:
        return QStringLiteral("Hash");
    case ResultCategory::Unknown:
        return QStringLiteral("Unknown");
    }

    qDebug() << "Error: ResultCategory to string - Not found";
    return QStringLiteral("Unknown");
}

QString resultCategoryDisplayName(ResultCategory resultCategory) {
    switch (resultCategory) {
    case ResultCategory::Translation:
        return QCoreApplication::translate("ResultCategory", "Translation");
    case ResultCategory::General:
        return QCoreApplication::translate("ResultCategory", "General");
    case ResultCategory::Validation:
        return QCoreApplication::translate("ResultCategory", "Validation");
    case ResultCategory::Database:
        return QCoreApplication::translate("ResultCategory", "Database");
    case ResultCategory::Service:
        return QCoreApplication::translate("ResultCategory", "Service");
    case ResultCategory::Ui:
        return QCoreApplication::translate("ResultCategory", "Ui");
    case ResultCategory::Hash:
        return QCoreApplication::translate("ResultCategory", "Hash");
    case ResultCategory::Unknown:
        return QCoreApplication::translate("ResultCategory", "Unknown");
    }

    qDebug() << "Error: ResultCategory to display name - Not found";
    return QCoreApplication::translate("ResultCategory", "Unknown");
}

// ResultSeverity
ResultSeverity resultSeverityStringToType(QString resultSeverityString) {
    if (resultSeverityString == QString("Success"))
        return ResultSeverity::Success;
    if (resultSeverityString == QString("Warning"))
        return ResultSeverity::Warning;
    if (resultSeverityString == QString("Error"))
        return ResultSeverity::Error;

    qDebug() << "Error: resultSeverityString to ResultSeverity - Not found";
    return ResultSeverity::Error;
}

QString resultSeverityToString(ResultSeverity resultSeverity) {
    switch (resultSeverity) {
    case ResultSeverity::Success:
        return QStringLiteral("Success");
    case ResultSeverity::Warning:
        return QStringLiteral("Warning");
    case ResultSeverity::Error:
        return QStringLiteral("Error");
    }

    qDebug() << "Error: ResultSeverity to string - Not found";
    return QStringLiteral("Error");
}

QString resultSeverityDisplayName(ResultSeverity resultSeverity) {
    switch (resultSeverity) {
    case ResultSeverity::Success:
        return QCoreApplication::translate("ResultSeverity", "Success");
    case ResultSeverity::Warning:
        return QCoreApplication::translate("ResultSeverity", "Warning");
    case ResultSeverity::Error:
        return QCoreApplication::translate("ResultSeverity", "ERROR");
    }

    qDebug() << "Error: ResultSeverity to display name - Not found";
    return QCoreApplication::translate("ResultSeverity", "Error");
}

void Result::updateMessage(const QString &message) {
    m_messageSource.clear();
    m_messageArguments.clear();
    m_message = message;
}
