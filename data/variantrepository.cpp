#include "data/variantrepository.h"
#include <QJsonArray>
#include <QJsonDocument>

namespace {
Result error(const QSqlQuery &q) {
    return Result::error(QStringLiteral("Database query failed."), ResultCategory::Database)
        .withCode(ErrorCode::DatabaseQueryFailed)
        .withDetails(q.lastError().text());
}
VariantRow row(const QSqlQuery &q) {
    return {q.value(0).toInt(), q.value(1).toInt(), q.value(2).toString(), q.value(3).toInt()};
}
const QString selectRows =
    QStringLiteral("SELECT v.id,v.number,v.conversion,(SELECT COUNT(*) FROM ReactionSteps r WHERE "
                   "r.variant_id=v.id) FROM Variants v ");
} // namespace
VariantRepository::VariantRepository(Database *db, QObject *parent) : QObject(parent), m_db(db) {}
Result VariantRepository::findVariant(int number) const {
    auto q = m_db->query(selectRows + "WHERE v.number=:n", {{":n", number}});
    if (!q.isActive())
        return error(q);
    if (!q.next())
        return Result::success();
    return Result::success().withPayload(row(q));
}
Result VariantRepository::findAllVariants() const {
    auto q = m_db->query(selectRows + "ORDER BY v.number");
    if (!q.isActive())
        return error(q);
    QList<VariantRow> rows;
    while (q.next())
        rows.append(row(q));
    return Result::success().withPayload(rows);
}
Result VariantRepository::insertVariant(const ReactionVariant &data) {
    auto valid = data.validate();
    if (!valid.ok())
        return valid;
    auto q = m_db->query("INSERT INTO "
                         "Variants(number,conversion,mealy_states,mealy_inputs,mealy_outputs,moore_"
                         "states,moore_inputs,moore_outputs) "
                         "VALUES(:n,:t,:ms,:mi,:mo,:rs,:ri,:ro)",
                         {{":n", data.number},
                          {":t", variantTypeToString(data.type)},
                          {":ms", data.mealy.states},
                          {":mi", data.mealy.inputs},
                          {":mo", data.mealy.outputs},
                          {":rs", data.moore.states},
                          {":ri", data.moore.inputs},
                          {":ro", data.moore.outputs}});
    if (!q.isActive())
        return error(q);
    const int id = q.lastInsertId().toInt();
    if (!data.image.data.isEmpty()) {
        q = m_db->query("INSERT INTO VariantImages(variant_id,data,file_name) VALUES(:v,:d,:f)",
                        {{":v", id}, {":d", data.image.data}, {":f", data.image.fileName}});
        if (!q.isActive())
            return error(q);
    }
    for (int i = 0; i < data.reaction.size(); ++i) {
        const auto &step = data.reaction[i];
        q = m_db->query(
            "INSERT INTO ReactionSteps(variant_id,position,input,outputs_json) VALUES(:v,:p,:i,:o)",
            {{":v", id},
             {":p", i},
             {":i", step.input},
             {":o", QString::fromUtf8(QJsonDocument(QJsonArray::fromStringList(step.outputs))
                                          .toJson(QJsonDocument::Compact))}});
        if (!q.isActive())
            return error(q);
    }
    return Result::success().withPayload(id);
}
Result VariantRepository::loadVariant(int number) const {
    auto q = m_db->query("SELECT "
                         "id,conversion,mealy_states,mealy_inputs,mealy_outputs,moore_states,moore_"
                         "inputs,moore_outputs "
                         "FROM Variants WHERE number=:n",
                         {{":n", number}});
    if (!q.isActive())
        return error(q);
    if (!q.next())
        return Result::error(QStringLiteral("Variant not found."))
            .withCode(ErrorCode::VariantNotFound)
            .withOffender(QString::number(number));
    const int id = q.value(0).toInt();
    QJsonObject object{{"number", number},
                       {"type", q.value(1).toString()},
                       {"mealy", QJsonObject{{"states", q.value(2).toInt()},
                                             {"inputs", q.value(3).toInt()},
                                             {"outputs", q.value(4).toInt()}}},
                       {"moore", QJsonObject{{"states", q.value(5).toInt()},
                                             {"inputs", q.value(6).toInt()},
                                             {"outputs", q.value(7).toInt()}}}};
    q = m_db->query("SELECT position,input,outputs_json FROM ReactionSteps WHERE variant_id=:v "
                    "ORDER BY position ASC",
                    {{":v", id}});
    if (!q.isActive())
        return error(q);
    QJsonArray steps;
    while (q.next()) {
        const auto outputs = QJsonDocument::fromJson(q.value(2).toString().toUtf8());
        if (q.value(0).toInt() != steps.size() || !outputs.isArray())
            return Result::errorMessage(QT_TRANSLATE_NOOP(
                "ResultMessage", "The stored step order or output list is corrupted."));
        steps.append(QJsonObject{{"input", q.value(1).toString()}, {"outputs", outputs.array()}});
    }
    object.insert("reaction", steps);
    ReactionVariant data;
    auto valid = ReactionVariant::fromJson(object, &data);
    if (!valid.ok())
        return valid;
    q = m_db->query("SELECT data,file_name FROM VariantImages WHERE variant_id=:v", {{":v", id}});
    if (!q.isActive())
        return error(q);
    if (q.next()) {
        data.image = {q.value(0).toByteArray(), q.value(1).toString()};
        const auto imageValid = data.image.validate();
        if (!imageValid.ok())
            return imageValid;
    }
    return Result::success().withPayload(data);
}
Result VariantRepository::updateVariantNumber(int id, int number) {
    auto q =
        m_db->query("UPDATE Variants SET number=:n WHERE id=:id", {{":n", number}, {":id", id}});
    return q.isActive() ? Result::success() : error(q);
}
Result VariantRepository::removeVariant(int id) {
    auto q = m_db->query("DELETE FROM Variants WHERE id=:id", {{":id", id}});
    return q.isActive() ? Result::success() : error(q);
}
