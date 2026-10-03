#include "listnamesvalidator.h"



ListNamesValidator::ListNamesValidator(QObject *parent)
    : QValidator(parent) {

}

QValidator::State ListNamesValidator::validate(QString &input, int &pos) const {
    if (input.isEmpty()) {
        return Acceptable;
    }

    // Inserting a comma
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
    }

    if (pos > input.length()) {
        pos = input.length();
    } else if (pos < 0) {
        pos = 0;
    }

    return Acceptable;
}

void ListNamesValidator::fixup(QString &input) const {

    // input.remove(trailingSeparators);
}

