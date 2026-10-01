#include "editvalidator.h"

#include <QLineEdit>
#include <QDebug>

EditValidator::EditValidator(QObject *parent, QRegularExpression forbiddenChars)
    : QValidator(parent), forbiddenChars(forbiddenChars)
{

}

QValidator::State EditValidator::validate(QString &input, int &pos) const
{
    if (input.isEmpty()) {
        return Acceptable;
    }

    if (input.contains(forbiddenChars)) {
        int charsDeletedBeforeCursor = input.left(pos).count(forbiddenChars);

        input.remove(forbiddenChars);
        pos -= charsDeletedBeforeCursor;
    }

    return Acceptable;
}