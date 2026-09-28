#ifndef EDITVALIDATOR_H
#define EDITVALIDATOR_H

#include <QValidator>

class EditValidator : public QValidator
{
    Q_OBJECT
public:
    explicit EditValidator(QObject *parent = nullptr);

    State validate(QString &input, int &pos) const override;
};

#endif // EDITVALIDATOR_H
