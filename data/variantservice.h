#ifndef VARIANTSERVICE_H
#define VARIANTSERVICE_H

#include "core/reactionvariant.h"
#include "data/database.h"
#include "data/variantrepository.h"

#include <QString>


class VariantService : public QObject {
    Q_OBJECT
public:
    VariantService(Database* db, VariantRepository* repo, QObject* parent = nullptr);

    Result saveVariant(const ReactionVariant& data);
    Result replaceVariant(const ReactionVariant& data);

    Result loadVariant(int variantNumber);

    Result removeVariant(int variantNumber);

    Result swapVariants(int firstNumber, int secondNumber);
    Result compactVariantNumbers();

    Result exportArchive(const QString& path);
    Result importArchive(const QString& path);

    Result isExist(int variantNumber);

    Result findAllVariants() const;


    int firstAvailableVariantNumber() const;

private:
    Database* m_db;
    VariantRepository* m_repo;

    Result insertVariantData(const ReactionVariant& data);
};
#endif
