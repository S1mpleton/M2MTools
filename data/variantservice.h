#ifndef VARIANTSERVICE_H
#define VARIANTSERVICE_H

#include "core/automatondata.h"
#include "data/database.h"
#include "data/variantrepository.h"

#include <QString>

struct ServiceResult {
    bool ok = true;
    QString message;

    static ServiceResult success() { return {}; }
    static ServiceResult failure(const QString& msg) { return { false, msg }; }
};

class VariantService : public QObject {
    Q_OBJECT
public:
    VariantService(Database* db, VariantRepository* repo, QObject* parent = nullptr);

    ServiceResult saveVariant(const AutomatonData& data);


    std::optional<AutomatonData> loadVariant(int variantNumber, ServiceResult* result = nullptr);

    ServiceResult removeVariant(int variantNumber, VariantType type);

    QList<int> availableVariantNumbers(VariantType type);

private:
    Database* m_db;
    VariantRepository* m_repo;

    ServiceResult saveMoore(
        const AutomatonData& data,
        int variantId,
        const QHash<QString, int>& stateIds,
        const QHash<QString, int>& inputIds,
        const QHash<QString, int>& outputIds
    );

    ServiceResult saveMealy(
        const AutomatonData& data,
        int variantId,
        const QHash<QString, int>& stateIds,
        const QHash<QString, int>& inputIds,
        const QHash<QString, int>& outputIds
    );

    std::optional<AutomatonData> loadMoore(int variantId, ServiceResult* result);
    std::optional<AutomatonData> loadMealy(int variantId, ServiceResult* result);

    // ServiceResult clearVariantContent(int variantId);
};

#endif // VARIANTSERVICE_H
