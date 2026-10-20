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
    Result replaceVariant(const AutomatonData& data);

    Result loadVariant(int variantNumber);

    Result removeVariant(int variantNumber);

    Result swapVariants(int firstNumber, int secondNumber);
    Result compactVariantNumbers();

    Result exportArchive(const QString& path, const QString& password);
    Result importArchive(const QString& path, const QString& password);

    Result isExist(int variantNumber);

    Result findAllVariants() const;

    QList<int> availableVariantNumbers(VariantType type);

    int firstAvailableVariantNumber() const;

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

    Result insertVariantData(const AutomatonData& data);

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
