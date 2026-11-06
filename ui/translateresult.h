#ifndef TRANSLATERESULT_H
#define TRANSLATERESULT_H

#include "core/result.h"

#include <QString>

class TranslateResult final
{
public:
    // Localized, concise text intended for dialogs, status bars and tooltips.
    static QString forUi(const Result& result);

    // English diagnostic text intended for logs. It retains technical context.
    static QString forLog(const Result& result);
};

QString translateResult(const Result& r);

#endif // TRANSLATERESULT_H
