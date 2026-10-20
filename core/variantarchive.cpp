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
const QByteArray kMagic("M2MTOOLS1");
constexpr int kSaltSize = 16;
constexpr int kNonceSize = 12;
constexpr int kTagSize = 16;
constexpr int kKeySize = 32;
constexpr int kPbkdf2Iterations = 200000;

using CipherContext = std::unique_ptr<EVP_CIPHER_CTX, decltype(&EVP_CIPHER_CTX_free)>;

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
            QStringLiteral("Архив содержит некорректную запись варианта."));
    }
    const QJsonObject object = value.toObject();
    const int number = object.value(QStringLiteral("number")).toInt(-1);
    const QString typeName = object.value(QStringLiteral("type")).toString();
    if (number <= 0 ||
        (typeName != QStringLiteral("MealyToMoore") &&
         typeName != QStringLiteral("MooreToMealy"))) {
        return Result::error(
            QStringLiteral("В архиве указан некорректный номер или тип варианта."));
    }

    QStringList states;
    QStringList inputs;
    QStringList outputs;
    if (!stringListFromJson(object.value(QStringLiteral("states")), &states) ||
        !stringListFromJson(object.value(QStringLiteral("inputs")), &inputs) ||
        !stringListFromJson(object.value(QStringLiteral("outputs")), &outputs)) {
        return Result::error(
            QStringLiteral("Архив содержит некорректные списки обозначений."));
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
            QStringLiteral("В архиве отсутствует таблица переходов."));
    }
    const QJsonArray transitions = transitionsValue.toArray();
    if (transitions.size() != inputs.size()) {
        return Result::error(
            QStringLiteral("Размер таблицы переходов в архиве не совпадает с X."));
    }
    for (int inputIndex = 0; inputIndex < inputs.size(); ++inputIndex) {
        if (!transitions[inputIndex].isArray()) {
            return Result::error(
                QStringLiteral("Строка таблицы переходов в архиве повреждена."));
        }
        const QJsonArray row = transitions[inputIndex].toArray();
        if (row.size() != states.size()) {
            return Result::error(
                QStringLiteral("Размер строки переходов в архиве не совпадает с A."));
        }
        for (int stateIndex = 0; stateIndex < states.size(); ++stateIndex) {
            if (!row[stateIndex].isObject()) {
                return Result::error(
                    QStringLiteral("Ячейка таблицы переходов в архиве повреждена."));
            }
            const QJsonObject cell = row[stateIndex].toObject();
            const QString nextState =
                cell.value(QStringLiteral("nextState")).toString();
            if (nextState.isEmpty()) {
                return Result::error(
                    QStringLiteral(
                        "Вариант №%1 в архиве содержит незаполненный переход.")
                        .arg(number));
            }
            QStringList cellOutputs;
            if (!stringListFromJson(
                    cell.value(QStringLiteral("outputs")), &cellOutputs)) {
                return Result::error(
                    QStringLiteral("Выходы перехода в архиве повреждены."));
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
                QStringLiteral("Выходы автомата Мура в архиве повреждены."));
        }
        const QJsonObject mooreOutputs = mooreValue.toObject();
        for (const QString& state : states) {
            QStringList stateOutputs;
            if (!stringListFromJson(mooreOutputs.value(state), &stateOutputs)) {
                return Result::error(
                    QStringLiteral("Выход состояния «%1» в архиве повреждён.")
                        .arg(state));
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
            QStringLiteral("Вариант №%1 в архиве не содержит изображения.")
                .arg(number));
    }
    candidate.setDiagramImage(imageData, imageName);

    result = candidate.validate();
    if (!result.ok()) {
        return Result::error(
            QStringLiteral("Вариант №%1 в архиве не прошёл проверку: %2")
                .arg(number).arg(result.message()),
            ResultCategory::Validation)
            .withCode(result.code())
            .withOffender(result.offender())
            .withDetails(result.details());
    }

    *data = candidate;
    return Result::success();
}

bool deriveKey(const QString& password, const QByteArray& salt,
               QByteArray* key) {
    if (!key) return false;
    key->resize(kKeySize);
    const QByteArray passwordBytes = password.toUtf8();
    return PKCS5_PBKDF2_HMAC(
        passwordBytes.constData(), passwordBytes.size(),
        reinterpret_cast<const unsigned char*>(salt.constData()), salt.size(),
        kPbkdf2Iterations, EVP_sha256(), key->size(),
        reinterpret_cast<unsigned char*>(key->data())) == 1;
}

bool encrypt(const QByteArray& plainText, const QString& password,
             QByteArray* salt, QByteArray* nonce,
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
    if (!deriveKey(password, *salt, &key)) return false;

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

bool decrypt(const QByteArray& cipherText, const QString& password,
             const QByteArray& salt, const QByteArray& nonce,
             const QByteArray& tag, QByteArray* plainText) {
    if (!plainText) return false;
    QByteArray key;
    if (!deriveKey(password, salt, &key)) return false;

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
                            const QList<AutomatonData>& variants,
                            const QString& password) {
    if (password.size() < 8) {
        return Result::error(
            QStringLiteral("Пароль архива должен содержать не менее 8 символов."),
            ResultCategory::Validation);
    }

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
    if (!encrypt(plainText, password, &salt, &nonce, &tag, &cipherText)) {
        return Result::error(
            QStringLiteral("Не удалось зашифровать архив BazaVar."));
    }

    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly)) {
        return Result::error(
            QStringLiteral("Не удалось открыть файл «%1» для записи.")
                .arg(QFileInfo(path).fileName()))
            .withDetails(file.errorString());
    }
    const QByteArray archive = kMagic + salt + nonce + tag + cipherText;
    if (file.write(archive) != archive.size() || !file.commit()) {
        return Result::error(
            QStringLiteral("Не удалось сохранить архив BazaVar."))
            .withDetails(file.errorString());
    }
    return Result::success(
        QStringLiteral("Архив содержит вариантов: %1").arg(variants.size()));
}

Result VariantArchive::load(const QString& path,
                            const QString& password,
                            QList<AutomatonData>* variants) {
    if (!variants) {
        return Result::error(
            QStringLiteral("Не указан получатель данных архива."));
    }
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        return Result::error(
            QStringLiteral("Не удалось открыть архив BazaVar."))
            .withDetails(file.errorString());
    }
    const QByteArray archive = file.readAll();
    const int prefixSize =
        kMagic.size() + kSaltSize + kNonceSize + kTagSize;
    if (archive.size() <= prefixSize || !archive.startsWith(kMagic)) {
        return Result::error(
            QStringLiteral("Файл не является архивом M2MTools."));
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
    if (!decrypt(cipherText, password, salt, nonce, tag, &plainText)) {
        return Result::error(
            QStringLiteral(
                "Не удалось расшифровать BazaVar: неверный пароль "
                "или файл был изменён."));
    }

    QJsonParseError parseError;
    const QJsonDocument document =
        QJsonDocument::fromJson(plainText, &parseError);
    if (parseError.error != QJsonParseError::NoError ||
        !document.isObject()) {
        return Result::error(
            QStringLiteral("Расшифрованные данные архива повреждены."));
    }
    const QJsonObject root = document.object();
    if (root.value(QStringLiteral("format")).toString() !=
            QStringLiteral("M2MTools") ||
        root.value(QStringLiteral("version")).toInt() != 1 ||
        !root.value(QStringLiteral("variants")).isArray()) {
        return Result::error(
            QStringLiteral("Версия или формат архива BazaVar не поддерживается."));
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
                QStringLiteral("Архив содержит повтор номера варианта №%1.")
                    .arg(data.getVariantNumber()));
        }
        numbers.insert(data.getVariantNumber());
        loaded.append(data);
    }
    *variants = loaded;
    return Result::success(
        QStringLiteral("Архив содержит вариантов: %1").arg(loaded.size()));
}
