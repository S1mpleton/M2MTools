#ifndef LISTNAMESVALIDATOR_H
#define LISTNAMESVALIDATOR_H

#include <QLineEdit>
#include <QObject>
#include <QValidator>

class ListNamesValidator : public QValidator
{
    Q_OBJECT
public:
    explicit ListNamesValidator(QObject* parent = nullptr);

    State validate(QString &input, int &pos) const override;

    void fixup(QString &input) const override;
};

#endif // LISTNAMESVALIDATOR_H
