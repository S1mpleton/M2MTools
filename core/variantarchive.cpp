#include "variantarchive.h"

#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>
#include <QSet>

#include <openssl/evp.h>
#include <openssl/rand.h>

#include <memory>

namespace {
const QByteArray kMagic("M2MLITE1");
constexpr int kSaltSize = 16;
constexpr int kNonceSize = 12;
constexpr int kTagSize = 16;
constexpr int kKeySize = 32;
constexpr int kPbkdf2Iterations = 200000;

using CipherContext = std::unique_ptr<EVP_CIPHER_CTX, decltype(&EVP_CIPHER_CTX_free)>;

QByteArray applicationArchiveKeyMaterial() {
    // This application-managed key is intentionally not exposed in the UI or
    // stored in an archive. A compatible trusted application must use the same
    // key material and M2MLITE1 container format.
    static const QByteArray partA = QByteArray::fromHex(
        "e758d0ffb756c99a9fed62dbb728a1e4"
        "cb06939e6d0d017dbb321b89a262c6e8");
    static const QByteArray partB = QByteArray::fromHex(
        "91ed552426a3a7bc31a8c75f11d2ec96"
        "aec327628668c5aa074e99718add5531");
    QByteArray material(kKeySize, Qt::Uninitialized);
    for (int i = 0; i < material.size(); ++i)
        material[i] = partA[i] ^ partB[i];
    return material;
}

bool deriveKey(const QByteArray& salt, QByteArray* key) {
    if (!key) return false;
    key->resize(kKeySize);
    QByteArray keyMaterial = applicationArchiveKeyMaterial();
    return PKCS5_PBKDF2_HMAC(
        keyMaterial.constData(), keyMaterial.size(),
        reinterpret_cast<const unsigned char*>(salt.constData()), salt.size(),
        kPbkdf2Iterations, EVP_sha256(), key->size(),
        reinterpret_cast<unsigned char*>(key->data())) == 1;
}

bool encrypt(const QByteArray& plainText, QByteArray* salt, QByteArray* nonce,
             QByteArray* tag, QByteArray* cipherText) {
    if (!salt || !nonce || !tag || !cipherText) return false;
    salt->resize(kSaltSize);
    nonce->resize(kNonceSize);
    if (RAND_bytes(reinterpret_cast<unsigned char*>(salt->data()),
                   salt->size()) != 1 ||
        RAND_bytes(reinterpret_cast<unsigned char*>(nonce->data()),
                   nonce->size()) != 1) {
        return false;
    }
    QByteArray key;
    if (!deriveKey(*salt, &key)) return false;

    CipherContext context(EVP_CIPHER_CTX_new(), &EVP_CIPHER_CTX_free);
    if (!context) return false;
    if (EVP_EncryptInit_ex(context.get(), EVP_aes_256_gcm(),
                           nullptr, nullptr, nullptr) != 1 ||
        EVP_CIPHER_CTX_ctrl(context.get(), EVP_CTRL_GCM_SET_IVLEN,
                            nonce->size(), nullptr) != 1 ||
        EVP_EncryptInit_ex(
            context.get(), nullptr, nullptr,
            reinterpret_cast<const unsigned char*>(key.constData()),
            reinterpret_cast<const unsigned char*>(nonce->constData())) != 1) {
        return false;
    }

    int outputLength = 0;
    if (EVP_EncryptUpdate(
            context.get(), nullptr, &outputLength,
            reinterpret_cast<const unsigned char*>(kMagic.constData()),
            kMagic.size()) != 1) {
        return false;
    }

    cipherText->resize(plainText.size() + EVP_MAX_BLOCK_LENGTH);
    int encryptedLength = 0;
    if (EVP_EncryptUpdate(
            context.get(),
            reinterpret_cast<unsigned char*>(cipherText->data()),
            &outputLength,
            reinterpret_cast<const unsigned char*>(plainText.constData()),
            plainText.size()) != 1) {
        return false;
    }
    encryptedLength = outputLength;
    if (EVP_EncryptFinal_ex(
            context.get(),
            reinterpret_cast<unsigned char*>(cipherText->data()) + encryptedLength,
            &outputLength) != 1) {
        return false;
    }
    encryptedLength += outputLength;
    cipherText->resize(encryptedLength);

    tag->resize(kTagSize);
    return EVP_CIPHER_CTX_ctrl(
        context.get(), EVP_CTRL_GCM_GET_TAG, tag->size(), tag->data()) == 1;
}

bool decrypt(const QByteArray& cipherText, const QByteArray& salt,
             const QByteArray& nonce,
             const QByteArray& tag, QByteArray* plainText) {
    if (!plainText) return false;
    QByteArray key;
    if (!deriveKey(salt, &key)) return false;

    CipherContext context(EVP_CIPHER_CTX_new(), &EVP_CIPHER_CTX_free);
    if (!context) return false;
    if (EVP_DecryptInit_ex(context.get(), EVP_aes_256_gcm(),
                           nullptr, nullptr, nullptr) != 1 ||
        EVP_CIPHER_CTX_ctrl(context.get(), EVP_CTRL_GCM_SET_IVLEN,
                            nonce.size(), nullptr) != 1 ||
        EVP_DecryptInit_ex(
            context.get(), nullptr, nullptr,
            reinterpret_cast<const unsigned char*>(key.constData()),
            reinterpret_cast<const unsigned char*>(nonce.constData())) != 1) {
        return false;
    }

    int outputLength = 0;
    if (EVP_DecryptUpdate(
            context.get(), nullptr, &outputLength,
            reinterpret_cast<const unsigned char*>(kMagic.constData()),
            kMagic.size()) != 1) {
        return false;
    }

    plainText->resize(cipherText.size());
    int plainLength = 0;
    if (EVP_DecryptUpdate(
            context.get(),
            reinterpret_cast<unsigned char*>(plainText->data()),
            &outputLength,
            reinterpret_cast<const unsigned char*>(cipherText.constData()),
            cipherText.size()) != 1) {
        return false;
    }
    plainLength = outputLength;
    if (EVP_CIPHER_CTX_ctrl(
            context.get(), EVP_CTRL_GCM_SET_TAG, tag.size(),
            const_cast<char*>(tag.constData())) != 1 ||
        EVP_DecryptFinal_ex(
            context.get(),
            reinterpret_cast<unsigned char*>(plainText->data()) + plainLength,
            &outputLength) != 1) {
        return false;
    }
    plainLength += outputLength;
    plainText->resize(plainLength);
    return true;
}
}

Result VariantArchive::save(const QString& path,
                            const QList<ReactionVariant>& variants) {
    QJsonArray variantArray;
    QSet<int> numbers;
    for (const ReactionVariant& variant : variants) {
        const auto valid = variant.validateForSave();
        if (!valid.ok()) return valid;
        if (numbers.contains(variant.number))
            return Result::error(QStringLiteral("Duplicate variant number in archive"))
                .withCode(ErrorCode::ArchiveDuplicateVariantNumber)
                .withOffender(QString::number(variant.number));
        numbers.insert(variant.number);
        variantArray.append(variant.toJson());
    }
    QJsonObject root;
    root.insert(QStringLiteral("format"), QStringLiteral("M2MToolsLite"));
    root.insert(QStringLiteral("version"), 1);
    root.insert(QStringLiteral("variants"), variantArray);
    const QByteArray plainText =
        QJsonDocument(root).toJson(QJsonDocument::Compact);

    QByteArray salt;
    QByteArray nonce;
    QByteArray tag;
    QByteArray cipherText;
    if (!encrypt(plainText, &salt, &nonce, &tag, &cipherText)) {
        return Result::error(
            QStringLiteral("Failed to protect the BazaVar file"))
            .withCode(ErrorCode::ArchiveEncryptionFailed);
    }

    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly)) {
        return Result::error(
            QStringLiteral("Failed to open file '%1' for writing")
                .arg(QFileInfo(path).fileName()))
            .withCode(ErrorCode::ArchiveFileOpenFailed)
            .withDetails(file.errorString());
    }
    const QByteArray archive = kMagic + salt + nonce + tag + cipherText;
    if (file.write(archive) != archive.size() || !file.commit()) {
        return Result::error(
            QStringLiteral("Failed to save the BazaVar archive"))
            .withCode(ErrorCode::ArchiveSaveFailed)
            .withDetails(file.errorString());
    }
    return Result::successMessage(
        QT_TRANSLATE_NOOP("ResultMessage", "Saved %1 variants to the archive"),
        {QString::number(variants.size())});
}

Result VariantArchive::load(const QString& path,
                            QList<ReactionVariant>* variants) {
    if (!variants) {
        return Result::error(
            QStringLiteral("Archive data destination was not provided"))
            .withCode(ErrorCode::ArchiveInvalidTarget);
    }
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        return Result::error(
            QStringLiteral("Failed to open the BazaVar archive"))
            .withCode(ErrorCode::ArchiveFileOpenFailed)
            .withDetails(file.errorString());
    }
    const QByteArray archive = file.readAll();
    const int prefixSize =
        kMagic.size() + kSaltSize + kNonceSize + kTagSize;
    if (archive.size() <= prefixSize || !archive.startsWith(kMagic)) {
        return Result::error(
            QStringLiteral("Нужен архив M2MTools Lite. Архив полной версии не хранит исходный порядок реакции."))
            .withCode(ErrorCode::ArchiveInvalidFormat);
    }

    int offset = kMagic.size();
    const QByteArray salt = archive.mid(offset, kSaltSize);
    offset += kSaltSize;
    const QByteArray nonce = archive.mid(offset, kNonceSize);
    offset += kNonceSize;
    const QByteArray tag = archive.mid(offset, kTagSize);
    offset += kTagSize;
    const QByteArray cipherText = archive.mid(offset);

    QByteArray plainText;
    if (!decrypt(cipherText, salt, nonce, tag, &plainText)) {
        return Result::error(
            QStringLiteral(
                "Failed to open BazaVar: the file was modified or was "
                "created by an incompatible application"))
            .withCode(ErrorCode::ArchiveDecryptionFailed);
    }

    QJsonParseError parseError;
    const QJsonDocument document =
        QJsonDocument::fromJson(plainText, &parseError);
    if (parseError.error != QJsonParseError::NoError ||
        !document.isObject()) {
        return Result::error(
            QStringLiteral("BazaVar file data is corrupted"))
            .withCode(ErrorCode::ArchiveCorruptedPayload);
    }
    const QJsonObject root = document.object();
    if (root.value(QStringLiteral("format")).toString() !=
            QStringLiteral("M2MToolsLite") ||
        root.value(QStringLiteral("version")).toInt() != 1 ||
        !root.value(QStringLiteral("variants")).isArray()) {
        return Result::error(
            QStringLiteral("BazaVar archive version or format is not supported"))
            .withCode(ErrorCode::ArchiveUnsupportedVersion);
    }

    QList<ReactionVariant> loaded;
    QSet<int> numbers;
    for (const QJsonValue& value :
         root.value(QStringLiteral("variants")).toArray()) {
        ReactionVariant data;
        Result result = ReactionVariant::fromJson(value.toObject(), &data);
        if (!result.ok()) return result;
        if (numbers.contains(data.number)) {
            return Result::error(
                QStringLiteral("Archive contains duplicate variant number #%1")
                    .arg(data.number))
                .withCode(ErrorCode::ArchiveDuplicateVariantNumber)
                .withOffender(QString::number(data.number));
        }
        numbers.insert(data.number);
        loaded.append(data);
    }
    *variants = loaded;
    return Result::successMessage(
        QT_TRANSLATE_NOOP("ResultMessage", "Loaded %1 variants from the archive"),
        {QString::number(loaded.size())});
}
