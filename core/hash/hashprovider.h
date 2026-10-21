#ifndef HASHPROVIDER_H
#define HASHPROVIDER_H

#include <QByteArray>
#include <QString>

class HashProvider {
public:
    virtual ~HashProvider() = default;

    // Sign canonical data and return a hexadecimal string.
    virtual QString sign(const QByteArray& canonicalData) const = 0;

    // Verify that signature matches canonicalData.
    virtual bool verify(const QByteArray& canonicalData, const QString& signature) const = 0;

    // Short algorithm name for diagnostics and logs.
    virtual QString algorithmName() const = 0;
};

using HashProviderPtr = std::shared_ptr<HashProvider>;

#endif // HASHPROVIDER_H
