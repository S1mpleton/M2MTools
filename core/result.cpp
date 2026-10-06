#include "result.h"

ResultCategory resultCategoryStringToType(QString resultCategoryString) {
    if (resultCategoryString == QString("General")) return ResultCategory::General;
    if (resultCategoryString == QString("Validation")) return ResultCategory::Validation;
    if (resultCategoryString == QString("Database")) return ResultCategory::Database;
    if (resultCategoryString == QString("Service")) return ResultCategory::Service;
    if (resultCategoryString == QString("Ui")) return ResultCategory::Ui;
    if (resultCategoryString == QString("Hash")) return ResultCategory::Hash;
    if (resultCategoryString == QString("Unknown")) return ResultCategory::Unknown;

    qDebug() << "Error: resultCategoryString to ResultCategory - Not found";
    return ResultCategory::Unknown;
}

QString resultCategoryToString(ResultCategory resultCategory) {
    switch (resultCategory) {
    case ResultCategory::General: return QStringLiteral("General");
    case ResultCategory::Validation: return QStringLiteral("Validation");
    case ResultCategory::Database: return QStringLiteral("Database");
    case ResultCategory::Service: return QStringLiteral("Service");
    case ResultCategory::Ui: return QStringLiteral("Ui");
    case ResultCategory::Hash: return QStringLiteral("Hash");
    case ResultCategory::Unknown: return QStringLiteral("Unknown");
    }

    qDebug() << "Error: ResultCategory to string - Not found";
    return QStringLiteral("Unknown");
}

QString resultCategoryDisplayName(ResultCategory resultCategory) {
    switch (resultCategory) {
    case ResultCategory::General: return QStringLiteral("General");
    case ResultCategory::Validation: return QStringLiteral("Validation");
    case ResultCategory::Database: return QStringLiteral("Database");
    case ResultCategory::Service: return QStringLiteral("Service");
    case ResultCategory::Ui: return QStringLiteral("Ui");
    case ResultCategory::Hash: return QStringLiteral("Hash");
    case ResultCategory::Unknown: return QStringLiteral("Unknown");
    }

    qDebug() << "Error: ResultCategory to display name - Not found";
    return QStringLiteral("Unknown");
}


// ResultSeverity
ResultSeverity resultSeverityStringToType(QString resultSeverityString) {
    if (resultSeverityString == QString("Success")) return ResultSeverity::Success;
    if (resultSeverityString == QString("Warning")) return ResultSeverity::Warning;
    if (resultSeverityString == QString("Error")) return ResultSeverity::Error;

    qDebug() << "Error: resultSeverityString to ResultSeverity - Not found";
    return ResultSeverity::Error;
}

QString resultSeverityToString(ResultSeverity resultSeverity) {
    switch (resultSeverity) {
    case ResultSeverity::Success: return QStringLiteral("Success");
    case ResultSeverity::Warning: return QStringLiteral("Warning");
    case ResultSeverity::Error: return QStringLiteral("Error");
    }

    qDebug() << "Error: ResultSeverity to string - Not found";
    return QStringLiteral("Error");
}

QString resultSeverityDisplayName(ResultSeverity resultSeverity) {
    switch (resultSeverity) {
    case ResultSeverity::Success: return QStringLiteral("Success");
    case ResultSeverity::Warning: return QStringLiteral("Warning");
    case ResultSeverity::Error: return QStringLiteral("ERROR");
    }

    qDebug() << "Error: ResultSeverity to display name - Not found";
    return QStringLiteral("Error");
}




void Result::updateMessage(const QString& message){
    m_message = message;
}