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
    auto fail = [](const QString &text) { return Result::error(text, ResultCategory::Validation); };
    if (number < 1 || number > 999999)
        return fail(QStringLiteral("Номер варианта: от 1 до 999999."));
    if (type != VariantType::MealyToMoore && type != VariantType::MooreToMealy)
        return fail(QStringLiteral("Неизвестное направление преобразования."));
    for (const auto &counts : {mealy, moore}) {
        if (counts.states < 1 || counts.states > 999999 || counts.inputs < 1 ||
            counts.inputs > 10 || counts.outputs < 1 || counts.outputs > 10)
            return fail(QStringLiteral("Для обоих автоматов задайте число состояний от 1 до 999999 "
                                       "и число сигналов от 1 до 10."));
    }
    if (reaction.isEmpty())
        return fail(QStringLiteral("Введите хотя бы один такт реакции."));
    const QRegularExpression inputPattern(QStringLiteral("^x[0-9]$"));
    const QRegularExpression outputPattern(QStringLiteral("^y[0-9]$"));
    QSet<QString> inputs, outputs;
    for (int i = 0; i < reaction.size(); ++i) {
        const auto &step = reaction[i];
        if (!inputPattern.match(step.input).hasMatch())
            return fail(QStringLiteral(
                            "Такт %1: нужен один вход x0–x9; пропуски между тактами недопустимы.")
                            .arg(i + 1))
                .withOffender(QStringLiteral("0:%1").arg(i));
        inputs.insert(step.input);
        QSet<QString> unique;
        for (const auto &output : step.outputs) {
            if (!outputPattern.match(output).hasMatch() || unique.contains(output))
                return fail(QStringLiteral("Такт %1: выходы y0–y9 через запятую, без повторов.")
                                .arg(i + 1))
                    .withOffender(QStringLiteral("1:%1").arg(i));
            unique.insert(output);
            outputs.insert(output);
        }
    }
    // A trace need not exercise the entire alphabet; suffix numbers are labels,
    // not indexes into the declared counts (one signal may be called x9).
    if (inputs.size() > mealy.inputs || inputs.size() > moore.inputs ||
        outputs.size() > mealy.outputs || outputs.size() > moore.outputs)
        return fail(QStringLiteral(
            "В реакции больше различных сигналов, чем указано в количествах Мили или Мура."));
    return Result::success();
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
    auto invalid = [] { return Result::error(QStringLiteral("Повреждены данные варианта Lite.")); };
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
