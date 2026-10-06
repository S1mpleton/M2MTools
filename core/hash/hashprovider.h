#ifndef HASHPROVIDER_H
#define HASHPROVIDER_H

#include <QByteArray>
#include <QString>

class HashProvider {
public:
    virtual ~HashProvider() = default;

    // Подписать канонические данные. Возвращает hex-строку.
    virtual QString sign(const QByteArray& canonicalData) const = 0;

    // Проверить, что signature соответствует canonicalData.
    virtual bool verify(const QByteArray& canonicalData, const QString& signature) const = 0;

    // Короткое имя алгоритма — для отладки и логов.
    virtual QString algorithmName() const = 0;
};

using HashProviderPtr = std::shared_ptr<HashProvider>;

#endif // HASHPROVIDER_H
