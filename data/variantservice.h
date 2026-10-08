#ifndef VARIANTSERVICE_H
#define VARIANTSERVICE_H

#include "core/automatondata.h"
#include "data/database.h"
#include "data/variantrepository.h"

#include <QString>


class VariantService : public QObject {
    Q_OBJECT
public:
    VariantService(Database* db, VariantRepository* repo, QObject* parent = nullptr);

    Result saveVariant(const AutomatonData& data);

    Result loadVariant(int variantNumber);

    Result removeVariant(int variantNumber);

    Result isExist(int variantNumber);

    QList<int> availableVariantNumbers(VariantType type);

private:
    Database* m_db;
    VariantRepository* m_repo;

    Result saveMoore(
        const AutomatonData& data,
        int variantId,
        const QHash<QString, int>& stateIds,
        const QHash<QString, int>& inputIds,
        const QHash<QString, int>& outputIds
    );

    Result saveMealy(
        const AutomatonData& data,
        int variantId,
        const QHash<QString, int>& stateIds,
        const QHash<QString, int>& inputIds,
        const QHash<QString, int>& outputIds
    );

    Result loadMoore(int variantId, int variantNumber);
    Result loadMealy(int variantId, int variantNumber);

    Result insertStates(int variantId,
        const QStringList& names,
        const QString& initialState,
        QHash<QString, int>& stateIds);

    Result insertInputs(int variantId,
        const QStringList& names,
        QHash<QString, int>& inputIds);

    Result insertOutputs(int variantId,
         const QStringList& names,
         QHash<QString, int>& outputIds);

    // Result clearVariantContent(int variantId);
};

#endif // VARIANTSERVICE_H
