#pragma once
#include "core/reactionvariant.h"
#include "data/database.h"

struct VariantRow {
    int id = -1;
    int number = 0;
    QString conversion;
    int stepCount = 0;
};

class VariantRepository : public QObject {
    Q_OBJECT
  public:
    explicit VariantRepository(Database *db, QObject *parent = nullptr);
    Result findVariant(int number) const;
    Result findAllVariants() const;
    Result insertVariant(const ReactionVariant &data);
    Result loadVariant(int number) const;
    Result updateVariantNumber(int variantId, int number);
    Result removeVariant(int variantId);

  private:
    Database *m_db;
};
