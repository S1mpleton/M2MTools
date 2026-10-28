#include "core/variantimage.h"
#include <QBuffer>
#include <QCoreApplication>
#include <QFile>
#include <QFileInfo>
#include <QImageReader>

namespace {
Result invalidImage() {
    return Result::error(
        QCoreApplication::translate("VariantImage", "The selected file is not a supported image."),
        ResultCategory::Validation);
}
Result invalidSize() {
    return Result::error(
        QCoreApplication::translate("VariantImage", "The image size must not exceed 10 MB."),
        ResultCategory::Validation);
}
} // namespace

Result VariantImage::validate() const {
    if (data.isEmpty())
        return fileName.isEmpty() ? Result::success() : invalidImage();
    if (data.size() > MaximumBytes)
        return invalidSize();
    if (fileName.trimmed().isEmpty())
        return invalidImage();
    QBuffer buffer;
    buffer.setData(data);
    buffer.open(QIODevice::ReadOnly);
    QImageReader reader(&buffer);
    if (reader.read().isNull())
        return invalidImage();
    return Result::success();
}

QJsonObject VariantImage::toJson() const {
    return {{"fileName", fileName}, {"data", QString::fromLatin1(data.toBase64())}};
}

Result VariantImage::fromJson(const QJsonObject &object, VariantImage *image) {
    if (!image || !object.value("fileName").isString() || !object.value("data").isString())
        return invalidImage();
    const auto encoded = object.value("data").toString();
    if (encoded.size() > ((MaximumBytes + 2) / 3) * 4)
        return invalidSize();
    const auto decoded =
        QByteArray::fromBase64Encoding(encoded.toLatin1(), QByteArray::AbortOnBase64DecodingErrors);
    if (!decoded || decoded.decoded.isEmpty())
        return invalidImage();
    VariantImage loaded{decoded.decoded, object.value("fileName").toString()};
    const auto valid = loaded.validate();
    if (valid.ok())
        *image = loaded;
    return valid;
}

Result VariantImage::fromFile(const QString &path, VariantImage *image) {
    if (!image)
        return invalidImage();
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly))
        return Result::error(
                   QCoreApplication::translate("VariantImage", "Could not open the image file."))
            .withDetails(file.errorString());
    if (file.size() > MaximumBytes)
        return invalidSize();
    // Also bound the read if a file changes after querying its size.
    VariantImage loaded{file.read(MaximumBytes + 1), QFileInfo(path).fileName()};
    const auto valid = loaded.validate();
    if (valid.ok())
        *image = loaded;
    return valid;
}
