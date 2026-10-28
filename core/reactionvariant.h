#pragma once
#include "core/result.h"
#include "core/variantimage.h"
#include <QJsonObject>
#include <QStringList>
#include <QVector>

enum class VariantType { MealyToMoore, MooreToMealy };
QString variantTypeToString(VariantType type);
VariantType variantTypeStringToType(const QString &text);
QString variantTypeDisplayName(VariantType type);

struct AutomatonCounts {
    int states = 1;
    int inputs = 1;
    int outputs = 1;
};

// Order is part of the task: neither steps nor output lists may be sorted.
struct ReactionStep {
    QString input;
    QStringList outputs;
};

struct ReactionVariant {
    int number = 1;
    VariantType type = VariantType::MealyToMoore;
    AutomatonCounts mealy;
    AutomatonCounts moore;
    QVector<ReactionStep> reaction;
    VariantImage image;

    Result validate() const;
    QJsonObject toJson() const;
    static Result fromJson(const QJsonObject &object, ReactionVariant *result);
};
Q_DECLARE_METATYPE(ReactionVariant)
