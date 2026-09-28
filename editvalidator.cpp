#include "editvalidator.h"

#include <QLineEdit>
#include <QDebug>

EditValidator::EditValidator(QObject *parent)
    : QValidator(parent)
{

}

QValidator::State EditValidator::validate(QString &input, int &pos) const
{
    if (input.isEmpty()) {
        return Acceptable;
    }

    const QRegularExpression forbiddenChars("[^a-zA-Zа-яА-ЯёЁ0-9, ]");
    if (input.contains(forbiddenChars)) {
        int charsDeletedBeforeCursor = input.left(pos).count(forbiddenChars);

        input.remove(forbiddenChars);
        pos -= charsDeletedBeforeCursor;
    }

    // Update position
    if (pos > 0 && input[pos - 1] == ' ') {
        if (pos > 1 && input[pos - 2] != ',') {
            input.insert(pos - 1, ',');
            pos++;
        }
    }

    if (pos > 0 && input[pos - 1] == ',') {
        if (pos == input.length() || input[pos] != ' ') {
            input.insert(pos, ' ');
            pos++;
        }
    }

    // Delete dublicate
    const QRegularExpression validateSeparators("[, ]{1,}");
    if (input.contains(validateSeparators)) {
        input.replace(validateSeparators, ", ");
        if (pos > input.length()) {
            pos = input.length();
        }
    }

    return Acceptable;
}