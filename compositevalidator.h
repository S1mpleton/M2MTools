#ifndef COMPOSITEVALIDATOR_H
#define COMPOSITEVALIDATOR_H

#include <QValidator>

class CompositeValidator : public QValidator
{
    Q_OBJECT
public:
    explicit CompositeValidator(QObject *parent = nullptr);

    void addValidator(QValidator* v);
    void fixup(QString& input) const override;

    State validate(QString &input, int &pos) const override;

private:
    QList<QValidator*> m_validators;
};

#endif // COMPOSITEVALIDATOR_H
