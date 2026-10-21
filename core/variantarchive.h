#ifndef VARIANTARCHIVE_H
#define VARIANTARCHIVE_H

#include "core/automatondata.h"

#include <QList>
#include <QString>

class VariantArchive
{
public:
    static Result save(const QString& path,
                       const QList<AutomatonData>& variants);

    static Result load(const QString& path,
                       QList<AutomatonData>* variants);
};

#endif // VARIANTARCHIVE_H
