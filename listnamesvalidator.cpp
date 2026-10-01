#include "listnamesvalidator.h"



ListNamesValidator::ListNamesValidator(QObject *parent)
    : QValidator(parent)
{

}

QValidator::State ListNamesValidator::validate(QString &input, int &pos) const
{
    if (input.isEmpty()) {
        return Acceptable;
    }

    // Update position
    if (pos > 0 && input[pos - 1] == ' ') {
        if (pos > 1 && input[pos - 2] != ',') {
            input.insert(pos - 1, ',');
            pos++;
        }
    }

    // Delete dublicate
    const QRegularExpression validateSeparators("[, ]{2,}");
    if (input.contains(validateSeparators)) {
        input.replace(validateSeparators, ", ");
        if (pos > input.length()) {
            pos = input.length();
        }
    }

    return Acceptable;
}

