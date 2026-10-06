#ifndef RSAPROVIDER_H
#define RSAPROVIDER_H

#include "hashprovider.h"

class RsaProvider : public HashProvider {
public:
    explicit RsaProvider(const QByteArray& key, bool isPrivate);

    // QString sign(const QByteArray& canonicalData) const override;   // Only private
    // bool verify(const QByteArray& canonicalData, const QString& signature) const override;
    // QString algorithmName() const override { return "RSA-SHA256"; }

private:
    QByteArray m_key;
    bool m_isPrivate;
};

#endif // RSAPROVIDER_H
