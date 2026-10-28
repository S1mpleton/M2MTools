#include "variantservice.h"

#include "data/database.h"
#include "data/transaction.h"
#include "core/reactionvariant.h"
#include "core/variantarchive.h"

#include <QSet>
#include <algorithm>

VariantService::VariantService(Database* db, VariantRepository* repo, QObject* parent)
    : QObject(parent)
    , m_db(db)
    , m_repo(repo)
{}


Result VariantService::saveVariant(const ReactionVariant& data) {
    const auto validation = data.validate();
    if (!validation.ok()) return validation;
    Result findResult = m_repo->findVariant(data.number);
    if (!findResult.ok()) return findResult;

    Transaction tx(m_db);
    if (!tx.active()) return Result::error(QStringLiteral("Не удалось начать транзакцию."));

    if (findResult.hasPayload()) {
        return Result::error(
                   QStringLiteral("Variant №%1 already exists").arg(data.number),
                   ResultCategory::Service)
            .withCode(ErrorCode::VariantAlreadyExists)
            .withOffender(QString::number(data.number))
            .withDetails(findResult.details());
    }

    Result insertResult = insertVariantData(data);
    if (!insertResult.ok()) return insertResult;

    // 4. COMMIT
    if (!tx.commit()) {
        return Result::error(
            QString("Failed to commit transaction"),
            ResultCategory::Database);
    }

    return Result::success(
        QString("Variant №%1 saved successfully").arg(data.number));

}

Result VariantService::replaceVariant(const ReactionVariant& data) {
    const auto validation = data.validate();
    if (!validation.ok()) return validation;
    Result existing = m_repo->findVariant(data.number);
    if (!existing.ok()) return existing;

    Transaction transaction(m_db);
    if (!transaction.active()) return Result::error(QStringLiteral("Не удалось начать транзакцию."));
    if (existing.hasPayload()) {
        Result removeResult =
            m_repo->removeVariant(existing.payloadAs<VariantRow>().id);
        if (!removeResult.ok()) return removeResult;
    }
    Result insertResult = insertVariantData(data);
    if (!insertResult.ok()) return insertResult;

    QString error;
    if (!transaction.commit(&error)) {
        return Result::error(
            QStringLiteral("Failed to commit variant replacement"),
            ResultCategory::Database)
            .withCode(ErrorCode::DatabaseCommitFailed)
            .withDetails(error);
    }
    return Result::success(
        QStringLiteral("Variant №%1 replaced successfully")
            .arg(data.number));
}

Result VariantService::insertVariantData(const ReactionVariant& data) {
    return m_repo->insertVariant(data);
}

Result VariantService::removeVariant(int variantNumber) {
    Result existing = m_repo->findVariant(variantNumber);
    if (!existing.ok()) return existing;
    if (!existing.hasPayload()) {
        return Result::warning(
            QStringLiteral("Variant №%1 not found").arg(variantNumber),
            ResultCategory::Service)
            .withCode(ErrorCode::VariantNotFound)
            .withOffender(QString::number(variantNumber));
    }
    VariantRow variant = existing.payloadAs<VariantRow>();

    Transaction tx(m_db);
    if (!tx.active()) return Result::error(QStringLiteral("Не удалось начать транзакцию."));

    if (existing.hasPayload()) {
        Result clearResult = m_repo->removeVariant(variant.id);
        if (!clearResult.ok()) return clearResult;
    }

    if (!tx.commit()) {
        return Result::error(
                QString("Failed to commit remove variant"),
                ResultCategory::Database)
            .withCode(ErrorCode::DatabaseCommitFailed);
    }

    return Result::success(
        QString("Variant №%1 deleted successfully").arg(variant.number));
}

Result VariantService::swapVariants(int firstNumber, int secondNumber) {
    if (firstNumber <= 0 || secondNumber <= 0) {
        return Result::error(QStringLiteral("Variant numbers must be positive"),
                             ResultCategory::Validation);
    }
    if (firstNumber == secondNumber)
        return Result::success(QStringLiteral("Variant numbers are equal"));

    Result firstResult = m_repo->findVariant(firstNumber);
    if (!firstResult.ok()) return firstResult;
    Result secondResult = m_repo->findVariant(secondNumber);
    if (!secondResult.ok()) return secondResult;
    if (!firstResult.hasPayload() && !secondResult.hasPayload()) {
        return Result::error(
            QStringLiteral("Neither variant №%1 nor №%2 exists")
                .arg(firstNumber).arg(secondNumber),
            ResultCategory::Service)
            .withCode(ErrorCode::VariantNotFound);
    }

    Transaction tx(m_db);
    if (!tx.active()) return Result::error(QStringLiteral("Не удалось начать транзакцию."));
    if (firstResult.hasPayload() && secondResult.hasPayload()) {
        const VariantRow first = firstResult.payloadAs<VariantRow>();
        const VariantRow second = secondResult.payloadAs<VariantRow>();

        int temporaryNumber = -1;
        while (m_repo->findVariant(temporaryNumber).hasPayload())
            --temporaryNumber;

        Result r = m_repo->updateVariantNumber(first.id, temporaryNumber);
        if (!r.ok()) return r;
        r = m_repo->updateVariantNumber(second.id, firstNumber);
        if (!r.ok()) return r;
        r = m_repo->updateVariantNumber(first.id, secondNumber);
        if (!r.ok()) return r;
    } else {
        const VariantRow existing = firstResult.hasPayload()
            ? firstResult.payloadAs<VariantRow>()
            : secondResult.payloadAs<VariantRow>();
        const int target = firstResult.hasPayload() ? secondNumber : firstNumber;
        Result r = m_repo->updateVariantNumber(existing.id, target);
        if (!r.ok()) return r;
    }

    QString error;
    if (!tx.commit(&error)) {
        return Result::error(QStringLiteral("Failed to commit variant swap"),
                             ResultCategory::Database)
            .withCode(ErrorCode::DatabaseCommitFailed)
            .withDetails(error);
    }
    return Result::success(
        QStringLiteral("Variants №%1 and №%2 swapped")
            .arg(firstNumber).arg(secondNumber));
}

Result VariantService::compactVariantNumbers() {
    Result allResult = m_repo->findAllVariants();
    if (!allResult.ok()) return allResult;
    QList<VariantRow> variants = allResult.payloadAs<QList<VariantRow>>();
    std::sort(variants.begin(), variants.end(),
              [](const VariantRow& left, const VariantRow& right) {
                  return left.number < right.number;
              });

    bool needsCompaction = false;
    for (int i = 0; i < variants.size(); ++i) {
        if (variants[i].number != i + 1) {
            needsCompaction = true;
            break;
        }
    }
    if (!needsCompaction)
        return Result::success(QStringLiteral("Variant numbers are already compact"));

    Transaction tx(m_db);
    if (!tx.active()) return Result::error(QStringLiteral("Не удалось начать транзакцию."));
    for (int i = 0; i < variants.size(); ++i) {
        Result result = m_repo->updateVariantNumber(
            variants[i].id, -1000000 - i);
        if (!result.ok()) return result;
    }
    for (int i = 0; i < variants.size(); ++i) {
        Result result = m_repo->updateVariantNumber(
            variants[i].id, i + 1);
        if (!result.ok()) return result;
    }

    QString error;
    if (!tx.commit(&error)) {
        return Result::error(
            QStringLiteral("Failed to commit variant renumbering"),
            ResultCategory::Database)
            .withCode(ErrorCode::DatabaseCommitFailed)
            .withDetails(error);
    }
    return Result::success(QStringLiteral("Variant numbers compacted"));
}

Result VariantService::exportArchive(const QString& path) {
    Result allResult = m_repo->findAllVariants();
    if (!allResult.ok()) return allResult;

    QList<ReactionVariant> variants;
    for (const VariantRow& row :
         allResult.payloadAs<QList<VariantRow>>()) {
        Result loadResult = loadVariant(row.number);
        if (!loadResult.ok() || !loadResult.hasPayload())
            return loadResult;
        ReactionVariant data = loadResult.payloadAs<ReactionVariant>();
        Result validation = data.validate();
        if (!validation.ok()) {
            return Result::error(
                QStringLiteral("Variant #%1 is incomplete: %2")
                    .arg(row.number).arg(validation.message()),
                ResultCategory::Validation)
                .withCode(validation.code())
                .withOffender(validation.offender())
                .withDetails(validation.details());
        }
        variants.append(data);
    }
    return VariantArchive::save(path, variants);
}

Result VariantService::importArchive(const QString& path) {
    QList<ReactionVariant> variants;
    Result loadResult = VariantArchive::load(path, &variants);
    if (!loadResult.ok()) return loadResult;

    Transaction transaction(m_db);
    if (!transaction.active()) return Result::error(QStringLiteral("Не удалось начать транзакцию."));
    Result allResult = m_repo->findAllVariants();
    if (!allResult.ok()) return allResult;
    for (const VariantRow& row :
         allResult.payloadAs<QList<VariantRow>>()) {
        Result removeResult = m_repo->removeVariant(row.id);
        if (!removeResult.ok()) return removeResult;
    }
    for (const ReactionVariant& data : variants) {
        Result insertResult = insertVariantData(data);
        if (!insertResult.ok()) return insertResult;
    }

    QString error;
    if (!transaction.commit(&error)) {
        return Result::error(
            QStringLiteral("Failed to apply the BazaVar archive"),
            ResultCategory::Database)
            .withCode(ErrorCode::DatabaseCommitFailed)
            .withDetails(error);
    }
    return Result::success(
        QStringLiteral("Imported %1 variants from the archive").arg(variants.size()));
}

Result VariantService::loadVariant(int variantNumber) {
    return m_repo->loadVariant(variantNumber);
}

Result VariantService::isExist(int variantNumber) {
    Result findResult = m_repo->findVariant(variantNumber);
    if (!findResult.ok()) {
        return findResult;
    }

    const bool exists = findResult.hasPayload();
    return Result::success(QStringLiteral("Variant #%1 existence check completed: %2")
                               .arg(variantNumber)
                               .arg(exists ? QStringLiteral("exists")
                                           : QStringLiteral("not found")))
        .withPayload(exists);
}

Result VariantService::findAllVariants() const {
    return m_repo->findAllVariants();
}

int VariantService::firstAvailableVariantNumber() const {
    Result result = m_repo->findAllVariants();
    if (!result.ok() || !result.hasPayload()) return 1;

    QSet<int> used;
    for (const VariantRow& row : result.payloadAs<QList<VariantRow>>())
        if (row.number > 0) used.insert(row.number);
    int number = 1;
    while (used.contains(number)) ++number;
    return number;
}
