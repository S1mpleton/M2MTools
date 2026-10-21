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
const QByteArray kMagic("M2MTOOLS2");
constexpr int kSaltSize = 16;
constexpr int kNonceSize = 12;
constexpr int kTagSize = 16;
constexpr int kKeySize = 32;
constexpr int kPbkdf2Iterations = 200000;

using CipherContext = std::unique_ptr<EVP_CIPHER_CTX, decltype(&EVP_CIPHER_CTX_free)>;

QByteArray applicationArchiveKeyMaterial() {
    // This application-managed key is intentionally not exposed in the UI or
    // stored in an archive. A compatible trusted application must use the same
    // key material and M2MTOOLS2 container format.
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

QJsonArray stringListToJson(const QStringList& values) {
    QJsonArray array;
    for (const QString& value : values)
        array.append(value);
    return array;
}

bool stringListFromJson(const QJsonValue& value, QStringList* values) {
    if (!value.isArray() || !values) return false;
    values->clear();
    for (const QJsonValue& item : value.toArray()) {
        if (!item.isString()) return false;
        values->append(item.toString());
    }
    return true;
}

QJsonObject automatonToJson(const AutomatonData& data) {
    QJsonObject object;
    object.insert(QStringLiteral("number"), data.getVariantNumber());
    object.insert(QStringLiteral("type"), variantTypeToString(data.getType()));
    object.insert(QStringLiteral("states"), stringListToJson(data.getStateNames()));
    object.insert(QStringLiteral("inputs"), stringListToJson(data.getInputSignalNames()));
    object.insert(QStringLiteral("outputs"), stringListToJson(data.getOutputSignalNames()));
    object.insert(QStringLiteral("initialState"), data.getInitialState());
    object.insert(QStringLiteral("imageName"), data.getDiagramFileName());
    object.insert(QStringLiteral("imageData"),
                  QString::fromLatin1(data.getDiagramImage().toBase64()));

    QJsonArray transitions;
    for (const auto& row : data.getTransitionTable()) {
        QJsonArray jsonRow;
        for (const CellData& cell : row) {
            QJsonObject jsonCell;
            jsonCell.insert(QStringLiteral("nextState"), cell.nextState);
            jsonCell.insert(QStringLiteral("outputs"),
                            stringListToJson(cell.outputSignals));
            jsonRow.append(jsonCell);
        }
        transitions.append(jsonRow);
    }
    object.insert(QStringLiteral("transitions"), transitions);

    QJsonObject mooreOutputs;
    for (const QString& state : data.getStateNames())
        mooreOutputs.insert(
            state, stringListToJson(data.getMooreOutputs().value(state)));
    object.insert(QStringLiteral("mooreOutputs"), mooreOutputs);
    return object;
}

Result automatonFromJson(const QJsonValue& value, AutomatonData* data) {
    if (!value.isObject() || !data) {
        return Result::error(
            QStringLiteral("Archive contains an invalid variant entry"))
            .withCode(ErrorCode::ArchiveInvalidEntry);
    }
    const QJsonObject object = value.toObject();
    const int number = object.value(QStringLiteral("number")).toInt(-1);
    const QString typeName = object.value(QStringLiteral("type")).toString();
    if (number <= 0 ||
        (typeName != QStringLiteral("MealyToMoore") &&
         typeName != QStringLiteral("MooreToMealy"))) {
        return Result::error(
            QStringLiteral("Archive contains an invalid variant number or type"))
            .withCode(ErrorCode::ArchiveInvalidMetadata);
    }

    QStringList states;
    QStringList inputs;
    QStringList outputs;
    if (!stringListFromJson(object.value(QStringLiteral("states")), &states) ||
        !stringListFromJson(object.value(QStringLiteral("inputs")), &inputs) ||
        !stringListFromJson(object.value(QStringLiteral("outputs")), &outputs)) {
        return Result::error(
            QStringLiteral("Archive contains invalid symbol lists"))
            .withCode(ErrorCode::ArchiveInvalidSymbols);
    }

    AutomatonData candidate;
    Result result = candidate.setVariantNumber(number);
    if (!result.ok()) return result;
    result = candidate.setVariantType(variantTypeStringToType(typeName));
    if (!result.ok()) return result;
    result = candidate.setStateNames(states);
    if (!result.ok()) return result;
    result = candidate.setInputSignalNames(inputs);
    if (!result.ok()) return result;
    result = candidate.setOutputSignalNames(outputs);
    if (!result.ok()) return result;

    const QString initialState =
        object.value(QStringLiteral("initialState")).toString();
    result = candidate.setInitialState(initialState);
    if (!result.ok()) return result;

    const QJsonValue transitionsValue =
        object.value(QStringLiteral("transitions"));
    if (!transitionsValue.isArray()) {
        return Result::error(
            QStringLiteral("Archive does not contain a transition table"))
            .withCode(ErrorCode::ArchiveMissingTransitionTable);
    }
    const QJsonArray transitions = transitionsValue.toArray();
    if (transitions.size() != inputs.size()) {
        return Result::error(
            QStringLiteral("Archive transition table size does not match X"))
            .withCode(ErrorCode::ArchiveCorruptedTransitionTable);
    }
    for (int inputIndex = 0; inputIndex < inputs.size(); ++inputIndex) {
        if (!transitions[inputIndex].isArray()) {
            return Result::error(
                QStringLiteral("Archive transition table row is corrupted"))
                .withCode(ErrorCode::ArchiveCorruptedTransitionTable);
        }
        const QJsonArray row = transitions[inputIndex].toArray();
        if (row.size() != states.size()) {
            return Result::error(
                QStringLiteral("Archive transition row size does not match A"))
                .withCode(ErrorCode::ArchiveCorruptedTransitionTable);
        }
        for (int stateIndex = 0; stateIndex < states.size(); ++stateIndex) {
            if (!row[stateIndex].isObject()) {
                return Result::error(
                    QStringLiteral("Archive transition table cell is corrupted"))
                    .withCode(ErrorCode::ArchiveCorruptedTransitionTable);
            }
            const QJsonObject cell = row[stateIndex].toObject();
            const QString nextState =
                cell.value(QStringLiteral("nextState")).toString();
            if (nextState.isEmpty()) {
                return Result::error(
                    QStringLiteral(
                        "Variant #%1 in the archive contains an incomplete transition")
                        .arg(number))
                    .withCode(ErrorCode::ArchiveCorruptedTransitionTable)
                    .withOffender(QString::number(number));
            }
            QStringList cellOutputs;
            if (!stringListFromJson(
                    cell.value(QStringLiteral("outputs")), &cellOutputs)) {
                return Result::error(
                    QStringLiteral("Archive transition outputs are corrupted"))
                    .withCode(ErrorCode::ArchiveCorruptedTransitionTable);
            }
            const QString cellText =
                candidate.getType() == VariantType::MealyToMoore
                ? QStringLiteral("%1 / %2").arg(
                      nextState.isEmpty() ? QStringLiteral("—") : nextState,
                      cellOutputs.isEmpty()
                          ? QStringLiteral("—") : cellOutputs.join(", "))
                : (nextState.isEmpty() ? QStringLiteral("—") : nextState);
            result = candidate.setTransitionCellByName(
                inputs[inputIndex], states[stateIndex], cellText);
            if (!result.ok()) return result;
        }
    }

    if (candidate.getType() == VariantType::MooreToMealy) {
        const QJsonValue mooreValue =
            object.value(QStringLiteral("mooreOutputs"));
        if (!mooreValue.isObject()) {
            return Result::error(
                QStringLiteral("Archive Moore automaton outputs are corrupted"))
                .withCode(ErrorCode::ArchiveCorruptedTransitionTable);
        }
        const QJsonObject mooreOutputs = mooreValue.toObject();
        for (const QString& state : states) {
            QStringList stateOutputs;
            if (!stringListFromJson(mooreOutputs.value(state), &stateOutputs)) {
                return Result::error(
                    QStringLiteral("Archive output for state '%1' is corrupted")
                        .arg(state))
                    .withCode(ErrorCode::ArchiveCorruptedTransitionTable)
                    .withOffender(state);
            }
            result = candidate.setMooreOutputCellByName(
                state, stateOutputs.isEmpty()
                    ? QStringLiteral("—") : stateOutputs.join(", "));
            if (!result.ok()) return result;
        }
    }

    const QString imageName =
        object.value(QStringLiteral("imageName")).toString();
    const QByteArray imageData = QByteArray::fromBase64(
        object.value(QStringLiteral("imageData")).toString().toLatin1());
    if (imageName.isEmpty() || imageData.isEmpty()) {
        return Result::error(
            QStringLiteral("Variant #%1 in the archive has no image").arg(number))
            .withCode(ErrorCode::ArchiveMissingImage)
            .withOffender(QString::number(number));
    }
    candidate.setDiagramImage(imageData, imageName);

    result = candidate.validate();
    if (!result.ok()) {
        return Result::error(
            QStringLiteral("Variant #%1 in the archive failed validation: %2")
                .arg(number).arg(result.message()),
            ResultCategory::Validation)
            .withCode(result.code())
            .withOffender(result.offender())
            .withDetails(result.details());
    }

    *data = candidate;
    return Result::success(QStringLiteral("Archive variant #%1 parsed successfully")
                               .arg(number));
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
                            const QList<AutomatonData>& variants) {
    QJsonArray variantArray;
    for (const AutomatonData& variant : variants)
        variantArray.append(automatonToJson(variant));
    QJsonObject root;
    root.insert(QStringLiteral("format"), QStringLiteral("M2MTools"));
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
    return Result::success(
        QStringLiteral("Saved %1 variants to the archive").arg(variants.size()));
}

Result VariantArchive::load(const QString& path,
                            QList<AutomatonData>* variants) {
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
            QStringLiteral("The file is not an M2MTools archive"))
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
            QStringLiteral("M2MTools") ||
        root.value(QStringLiteral("version")).toInt() != 1 ||
        !root.value(QStringLiteral("variants")).isArray()) {
        return Result::error(
            QStringLiteral("BazaVar archive version or format is not supported"))
            .withCode(ErrorCode::ArchiveUnsupportedVersion);
    }

    QList<AutomatonData> loaded;
    QSet<int> numbers;
    for (const QJsonValue& value :
         root.value(QStringLiteral("variants")).toArray()) {
        AutomatonData data;
        Result result = automatonFromJson(value, &data);
        if (!result.ok()) return result;
        if (numbers.contains(data.getVariantNumber())) {
            return Result::error(
                QStringLiteral("Archive contains duplicate variant number #%1")
                    .arg(data.getVariantNumber()))
                .withCode(ErrorCode::ArchiveDuplicateVariantNumber)
                .withOffender(QString::number(data.getVariantNumber()));
        }
        numbers.insert(data.getVariantNumber());
        loaded.append(data);
    }
    *variants = loaded;
    return Result::success(
        QStringLiteral("Loaded %1 variants from the archive").arg(loaded.size()));
}
