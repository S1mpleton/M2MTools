#ifndef EDITVALIDATOR_H
#define EDITVALIDATOR_H

#include <QValidator>

class EditValidator : public QValidator
{
    Q_OBJECT
public:
    explicit EditValidator(QObject *parent = nullptr, QRegularExpression forbiddenChars = QRegularExpression("[^a-zA-Zа-яА-ЯёЁ0-9, ]"));

    State validate(QString &input, int &pos) const override;

private:
    QRegularExpression forbiddenChars;
};

#endif // EDITVALIDATOR_H
