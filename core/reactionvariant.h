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
    int states = 4;
    int inputs = 3;
    int outputs = 4;
    bool operator==(const AutomatonCounts &other) const {
        return states == other.states && inputs == other.inputs && outputs == other.outputs;
    }
};

// Order is part of the task: neither steps nor output lists may be sorted.
struct ReactionStep {
    QString input;
    QStringList outputs;
    bool operator==(const ReactionStep &other) const {
        return input == other.input && outputs == other.outputs;
    }
};

struct ReactionVariant {
    int number = 1;
    VariantType type = VariantType::MealyToMoore;
    AutomatonCounts mealy;
    AutomatonCounts moore;
    QVector<ReactionStep> reaction;
    VariantImage image;

    Result validate() const;
    Result validateInputLimits() const;
    Result validateForSave() const;
    bool operator==(const ReactionVariant &other) const;
    QJsonObject toJson() const;
    static Result fromJson(const QJsonObject &object, ReactionVariant *result);
};
Q_DECLARE_METATYPE(ReactionVariant)
