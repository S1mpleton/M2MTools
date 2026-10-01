#include "compositevalidator.h"

CompositeValidator::CompositeValidator(QObject *parent)
    : QValidator(parent)
{

}

void CompositeValidator::addValidator(QValidator* v) {
    v->setParent(this);
    m_validators.append(v);
}

void CompositeValidator::fixup(QString& input) const {
    for (QValidator* v : m_validators)
        v->fixup(input);
}

QValidator::State CompositeValidator::validate(QString &input, int &pos) const
{
    State worst = Acceptable;

    for (QValidator* v : m_validators) {
        State s = v->validate(input, pos);
        if (s == Invalid)
            return Invalid;
        if (s == Intermediate)
            worst = Intermediate;
    }
    return worst;

}
