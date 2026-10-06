#ifndef VALIDATIONRESULT_H
#define VALIDATIONRESULT_H

#include <QString>

struct ValidationResult {
    bool ok = true;
    QString message;
    QString offender;

    static ValidationResult success(const QString& msg = "Ok") { return {true, msg}; }
    static ValidationResult failure(const QString& msg, const QString& name = {}) {
        return { false, msg, name };
    }
};

#endif // VALIDATIONRESULT_H
