#include "validation/reactionvalidator.h"
#include <QRegularExpression>
#include <QSet>

namespace {
bool isSingleDigit(const QString &text) {
    return text.size() == 1 && text[0] >= QLatin1Char('0') && text[0] <= QLatin1Char('9');
}
}

ReactionValidator::ReactionValidator(bool outputs, QObject *parent)
    : QValidator(parent), m_outputs(outputs) {}
QValidator::State ReactionValidator::validate(QString &input, int &position) const {
    if (!m_outputs) {
        if (isSingleDigit(input)) {
            input.prepend('x');
            if (position > 0) ++position;
        }
        if (input.isEmpty())
            return Acceptable; // Position and automaton kind are checked when saving the reaction.
        if (input == "x")
            return Intermediate;
        return QRegularExpression(QStringLiteral("\\Ax[0-9]\\z")).match(input).hasMatch()
                   ? Acceptable : Invalid;
    }
    const QString text = input.trimmed();
    if (text.isEmpty() || text == "-" || text == QStringLiteral("—"))
        return Acceptable;
    auto parts = input.split(',');
    int offset = 0;
    int cursor = position;
    State state = Acceptable;
    QSet<QString> used;
    for (int i = 0; i < parts.size(); ++i) {
        auto part = parts[i].trimmed();
        if (isSingleDigit(part)) {
            const int start = parts[i].indexOf(part);
            parts[i].insert(start, 'y');
            if (cursor > offset + start) ++cursor;
            part.prepend('y');
        }
        offset += parts[i].size() + 1;
        // The editor leaves a final separator ready for the next signal.
        if (i == parts.size() - 1 && part.isEmpty())
            break;
        if (i == parts.size() - 1 && part == "y") {
            state = Intermediate;
            break;
        }
        if (!QRegularExpression(QStringLiteral("^y[0-9]$")).match(part).hasMatch() ||
            used.contains(part))
            return Invalid;
        used.insert(part);
    }
    // QLineEdit applies validator changes while typing and pasting, including the cursor shift.
    input = parts.join(',');
    position = cursor;
    return state;
}
