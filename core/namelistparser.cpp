#include "namelistparser.h"

#include <QRegularExpression>

NameListParser::NameListParser() {}

QStringList NameListParser::parse(const QString& text) {
    QString normalized = text;
    normalized.replace(';', ',');
    normalized.replace(QRegularExpression("\\s+"), " ");

    QStringList result;
    for (const QString& part : normalized.split(',', Qt::SkipEmptyParts)) {
        QString trimmed = part.trimmed();
        if (!trimmed.isEmpty())
            result.append(trimmed);
    }

    return result;
}