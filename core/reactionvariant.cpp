#include "core/reactionvariant.h"
#include <QCoreApplication>
#include <QJsonArray>
#include <QRegularExpression>
#include <QSet>

QString variantTypeToString(VariantType type) {
    return type == VariantType::MealyToMoore ? QStringLiteral("MealyToMoore")
                                             : QStringLiteral("MooreToMealy");
}
VariantType variantTypeStringToType(const QString &text) {
    return text == QStringLiteral("MooreToMealy") ? VariantType::MooreToMealy
                                                  : VariantType::MealyToMoore;
}
QString variantTypeDisplayName(VariantType type) {
    return type == VariantType::MealyToMoore
               ? QCoreApplication::translate("VariantTypeDisplay", "Mealy → Moore")
               : QCoreApplication::translate("VariantTypeDisplay", "Moore → Mealy");
}

Result ReactionVariant::validate() const {
    const auto imageValidation = image.validate();
    if (!imageValidation.ok())
        return imageValidation;
    auto fail = [](const char *source, const QStringList &arguments = {}) {
        return Result::errorMessage(source, arguments, ResultCategory::Validation);
    };
    if (number < 1 || number > 999999)
        return fail(
            QT_TRANSLATE_NOOP("ResultMessage", "Variant number must be between 1 and 999999."));
    if (type != VariantType::MealyToMoore && type != VariantType::MooreToMealy)
        return fail(QT_TRANSLATE_NOOP("ResultMessage", "Unknown conversion direction."));
    for (const auto &counts : {mealy, moore}) {
        if (counts.states < 1 || counts.states > 999999 || counts.inputs < 1 ||
            counts.inputs > 10 || counts.outputs < 1 || counts.outputs > 10)
            return fail(QT_TRANSLATE_NOOP(
                "ResultMessage",
                "For both automata, specify 1–999999 states and 1–10 signals of each kind."));
    }
    if (reaction.isEmpty())
        return fail(QT_TRANSLATE_NOOP("ResultMessage", "Enter at least one reaction step."));
    const QRegularExpression inputPattern(QStringLiteral("^x[0-9]$"));
    const QRegularExpression outputPattern(QStringLiteral("^y[0-9]$"));
    QSet<QString> inputs, outputs;
    for (int i = 0; i < reaction.size(); ++i) {
        const auto &step = reaction[i];
        if (!inputPattern.match(step.input).hasMatch())
            return fail(QT_TRANSLATE_NOOP(
                            "ResultMessage",
                            "Step %1: enter one input x0–x9; gaps between steps are not allowed."),
                        {QString::number(i + 1)})
                .withOffender(QStringLiteral("0:%1").arg(i));
        inputs.insert(step.input);
        QSet<QString> unique;
        for (const auto &output : step.outputs) {
            if (!outputPattern.match(output).hasMatch() || unique.contains(output))
                return fail(QT_TRANSLATE_NOOP("ResultMessage",
                                              "Step %1: enter outputs y0–y9 separated by commas, "
                                              "without duplicates."),
                            {QString::number(i + 1)})
                    .withOffender(QStringLiteral("1:%1").arg(i));
            unique.insert(output);
            outputs.insert(output);
        }
    }
    // A trace need not exercise the entire alphabet; suffix numbers are labels,
    // not indexes into the declared counts (one signal may be called x9).
    if (inputs.size() > mealy.inputs || inputs.size() > moore.inputs ||
        outputs.size() > mealy.outputs || outputs.size() > moore.outputs)
        return fail(QT_TRANSLATE_NOOP(
            "ResultMessage",
            "The reaction uses more distinct signals than declared for Mealy or Moore."));
    return Result::success();
}

Result ReactionVariant::validateForSave() const {
    const auto valid = validate();
    if (!valid.ok())
        return valid;
    if (image.data.isEmpty())
        return Result::error(QStringLiteral("A task image is required."),
                             ResultCategory::Validation)
            .withCode(ErrorCode::VariantMissingImage)
            .withOffender(QString::number(number));
    return Result::success();
}

bool ReactionVariant::operator==(const ReactionVariant &other) const {
    return number == other.number && type == other.type && mealy == other.mealy &&
           moore == other.moore && reaction == other.reaction && image.data == other.image.data &&
           image.fileName == other.image.fileName;
}

QJsonObject ReactionVariant::toJson() const {
    auto counts = [](const AutomatonCounts &c) {
        return QJsonObject{{"states", c.states}, {"inputs", c.inputs}, {"outputs", c.outputs}};
    };
    QJsonArray steps;
    for (const auto &step : reaction)
        steps.append(QJsonObject{{"input", step.input},
                                 {"outputs", QJsonArray::fromStringList(step.outputs)}});
    QJsonObject object{{"number", number},
                       {"type", variantTypeToString(type)},
                       {"mealy", counts(mealy)},
                       {"moore", counts(moore)},
                       {"reaction", steps}};
    if (!image.data.isEmpty())
        object.insert("image", image.toJson());
    return object;
}

Result ReactionVariant::fromJson(const QJsonObject &object, ReactionVariant *result) {
    auto invalid = [] {
        return Result::errorMessage(
            QT_TRANSLATE_NOOP("ResultMessage", "The variant data is corrupted."));
    };
    if (!result)
        return invalid();
    ReactionVariant data;
    if (object.contains("image")) {
        if (!object.value("image").isObject())
            return invalid();
        const auto loaded = VariantImage::fromJson(object.value("image").toObject(), &data.image);
        if (!loaded.ok())
            return loaded;
    }
    data.number = object.value("number").toInt(-1);
    const auto type = object.value("type").toString();
    if (type != "MealyToMoore" && type != "MooreToMealy")
        return invalid();
    data.type = variantTypeStringToType(type);
    auto readCounts = [](const QJsonValue &value) {
        const auto c = value.toObject();
        return AutomatonCounts{c.value("states").toInt(-1), c.value("inputs").toInt(-1),
                               c.value("outputs").toInt(-1)};
    };
    data.mealy = readCounts(object.value("mealy"));
    data.moore = readCounts(object.value("moore"));
    if (!object.value("reaction").isArray())
        return invalid();
    for (const auto &value : object.value("reaction").toArray()) {
        const auto step = value.toObject();
        if (!step.value("input").isString() || !step.value("outputs").isArray())
            return invalid();
        ReactionStep parsed{step.value("input").toString(), {}};
        for (const auto &output : step.value("outputs").toArray()) {
            if (!output.isString())
                return invalid();
            parsed.outputs.append(output.toString());
        }
        data.reaction.append(parsed);
    }
    const auto validation = data.validate();
    if (!validation.ok())
        return validation;
    *result = data;
    return Result::success();
}
