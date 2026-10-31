#ifndef VARIANTARCHIVE_H
#define VARIANTARCHIVE_H

#include "core/reactionvariant.h"

#include <QList>
#include <QString>

class VariantArchive
{
public:
    static Result save(const QString& path,
                       const QList<ReactionVariant>& variants);

    static Result load(const QString& path,
                       QList<ReactionVariant>* variants);
};

#endif // VARIANTARCHIVE_H
