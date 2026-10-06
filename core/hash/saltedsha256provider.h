#ifndef SALTEDSHA256PROVIDER_H
#define SALTEDSHA256PROVIDER_H

#include "hashprovider.h"

#include <QString>

class SaltedSha256Provider : public HashProvider {
public:
    explicit SaltedSha256Provider(const QByteArray& salt);

    QString sign(const QByteArray& canonicalData) const override;
    bool verify(const QByteArray& canonicalData, const QString& signature) const override;
    QString algorithmName() const override { return "SHA256+salt"; }

private:
    QByteArray m_salt;
};

#endif // SALTEDSHA256PROVIDER_H
