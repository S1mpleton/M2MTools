#include "validation/reactionvalidator.h"
#include <QRegularExpression>
#include <QSet>

ReactionValidator::ReactionValidator(bool outputs, QObject *parent)
    : QValidator(parent), m_outputs(outputs) {}
QValidator::State ReactionValidator::validate(QString &input, int &) const {
    const QString text = input.trimmed();
    if (!m_outputs) {
        if (text.isEmpty() || text == "x")
            return Intermediate;
        return QRegularExpression(QStringLiteral("^x[0-9]$")).match(text).hasMatch() ? Acceptable
                                                                                     : Invalid;
    }
    if (text.isEmpty() || text == "-" || text == QStringLiteral("—"))
        return Acceptable;
    const auto parts = text.split(',');
    QSet<QString> used;
    for (int i = 0; i < parts.size(); ++i) {
        const auto part = parts[i].trimmed();
        if (i == parts.size() - 1 && (part.isEmpty() || part == "y"))
            return Intermediate;
        if (!QRegularExpression(QStringLiteral("^y[0-9]$")).match(part).hasMatch() ||
            used.contains(part))
            return Invalid;
        used.insert(part);
    }
    return Acceptable;
}
