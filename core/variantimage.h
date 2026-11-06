#pragma once
#include "core/result.h"
#include <QByteArray>
#include <QJsonObject>

// An optional attachment, stored by value rather than as a path to an external
// file.
struct VariantImage {
    static constexpr qsizetype MaximumBytes = 10 * 1024 * 1024;
    QByteArray data;
    QString fileName;

    Result validate() const;
    QJsonObject toJson() const;
    static Result fromJson(const QJsonObject &object, VariantImage *image);
    static Result fromFile(const QString &path, VariantImage *image);
};
