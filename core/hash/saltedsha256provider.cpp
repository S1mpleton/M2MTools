#include "saltedsha256provider.h"

#include <QCryptographicHash>

SaltedSha256Provider::SaltedSha256Provider(const QByteArray& salt)
    : m_salt(salt) {}

QString SaltedSha256Provider::sign(const QByteArray& canonicalData) const {
    QCryptographicHash hash(QCryptographicHash::Sha256);
    hash.addData(m_salt);
    hash.addData(canonicalData);
    return QString::fromLatin1(hash.result().toHex());
}

bool SaltedSha256Provider::verify(const QByteArray& canonicalData, const QString& signature) const {
    const QString computed = sign(canonicalData);
    return computed == signature;
}