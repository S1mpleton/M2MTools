#include "reactionsynchronizer.h"

#include "core/namelistparser.h"

#include <QCoreApplication>
#include <QHash>
#include <QSet>

namespace {
[[maybe_unused]] const char* const kReactionTranslationSources[] = {
    QT_TRANSLATE_NOOP("ReactionSynchronizer",
                      "Invalid name '%1': use letters and digits, starting with a letter"),
    QT_TRANSLATE_NOOP("ReactionSynchronizer",
                      "Name '%1' is specified more than once"),
    QT_TRANSLATE_NOOP("ReactionSynchronizer",
                      "Name '%1' is already used in another symbol group"),
    QT_TRANSLATE_NOOP("ReactionSynchronizer", "Unknown state '%1'"),
    QT_TRANSLATE_NOOP("ReactionSynchronizer", "Unknown input signal '%1'"),
    QT_TRANSLATE_NOOP("ReactionSynchronizer", "Unknown output signal '%1'"),
    QT_TRANSLATE_NOOP("ReactionSynchronizer",
                      "Fill in the state, input and next state"),
    QT_TRANSLATE_NOOP("ReactionSynchronizer",
                      "Conflict: different transitions are specified for %1 + %2"),
    QT_TRANSLATE_NOOP("ReactionSynchronizer",
                      "Conflict: state %1 is assigned different outputs"),
};

QString trReaction(const char* text) {
    return QCoreApplication::translate("ReactionSynchronizer", text);
}

QString normalizedName(const QString& value) {
    const QString result = value.trimmed();
    return (result == QStringLiteral("—") || result == QStringLiteral("-"))
        ? QString()
        : result;
}

QStringList normalizedOutputs(const QString& value) {
    const QString normalized = normalizedName(value);
    return normalized.isEmpty() ? QStringList() : NameListParser::parse(normalized);
}

void appendUnique(QStringList& values, const QString& value) {
    const QString normalized = normalizedName(value);
    if (!normalized.isEmpty() && !values.contains(normalized))
        values.append(normalized);
}

void appendUnique(QStringList& values, const QStringList& additions) {
    for (const QString& value : additions)
        appendUnique(values, value);
}

void addIssue(QVector<ReactionIssue>& issues, int column, int row,
              ReactionIssueSeverity severity, const QString& message) {
    issues.append({column, row, severity, message});
}

QString friendlyValidationMessage(const Result& result) {
    switch (result.code()) {
    case ErrorCode::ADInvalidName:
        return trReaction(
            "Invalid name '%1': use letters and digits, starting with a letter")
            .arg(result.offender());
    case ErrorCode::ADDuplicateNameInList:
        return trReaction("Name '%1' is specified more than once").arg(result.offender());
    case ErrorCode::ADDuplicateNameInAllList:
        return trReaction(
            "Name '%1' is already used in another symbol group")
            .arg(result.offender());
    case ErrorCode::ADUnknownState:
        return trReaction("Unknown state '%1'").arg(result.offender());
    case ErrorCode::ADUnknownInput:
        return trReaction("Unknown input signal '%1'").arg(result.offender());
    case ErrorCode::ADUnknownOutput:
        return trReaction("Unknown output signal '%1'").arg(result.offender());
    default:
        return result.message();
    }
}

QString transitionKey(const QString& state, const QString& input) {
    return state + QChar(0x1f) + input;
}

QString transitionValue(const QString& nextState, const QStringList& outputs,
                        VariantType type) {
    QStringList normalized = outputs;
    normalized.sort(Qt::CaseSensitive);
    return nextState + QChar(0x1f)
        + (type == VariantType::MealyToMoore ? normalized.join(QChar(0x1e)) : QString());
}
}

bool ReactionSyncReport::hasErrors() const {
    for (const ReactionIssue& issue : issues) {
        if (issue.severity == ReactionIssueSeverity::Error)
            return true;
    }
    return false;
}

ReactionSyncReport ReactionSynchronizer::buildAutomaton(
    const QVector<ReactionStep>& steps,
    const AutomatonData& base) {
    ReactionSyncReport report;
    report.data.setVariantNumber(base.getVariantNumber());
    report.data.setVariantType(base.getType());
    if (base.hasDiagramImage())
        report.data.setDiagramImage(base.getDiagramImage(), base.getDiagramFileName());

    // Keep manually entered dictionaries and append names discovered in the
    // reaction. This makes both entry paths additive instead of unexpectedly
    // deleting a name while the user is still filling a reaction column.
    QStringList states = base.getStateNames();
    QStringList inputs = base.getInputSignalNames();
    QStringList outputs = base.getOutputSignalNames();

    for (const ReactionStep& step : steps) {
        appendUnique(states, step.currentState);
        appendUnique(states, step.nextState);
        appendUnique(inputs, step.inputSignal);
        appendUnique(outputs, normalizedOutputs(step.outputSignals));
    }

    Result result = report.data.setStateNames(states);
    if (!result.ok()) {
        for (int column = 0; column < steps.size(); ++column) {
            if (steps[column].currentState.trimmed() == result.offender())
                addIssue(report.issues, column, 0, ReactionIssueSeverity::Error,
                         friendlyValidationMessage(result));
            if (steps[column].nextState.trimmed() == result.offender())
                addIssue(report.issues, column, 2, ReactionIssueSeverity::Error,
                         friendlyValidationMessage(result));
        }
        return report;
    }

    result = report.data.setInputSignalNames(inputs);
    if (!result.ok()) {
        for (int column = 0; column < steps.size(); ++column) {
            if (steps[column].inputSignal.trimmed() == result.offender())
                addIssue(report.issues, column, 1, ReactionIssueSeverity::Error,
                         friendlyValidationMessage(result));
        }
        return report;
    }

    result = report.data.setOutputSignalNames(outputs);
    if (!result.ok()) {
        for (int column = 0; column < steps.size(); ++column) {
            if (normalizedOutputs(steps[column].outputSignals).contains(result.offender()))
                addIssue(report.issues, column, 3, ReactionIssueSeverity::Error,
                         friendlyValidationMessage(result));
        }
        return report;
    }

    QString initialState = base.getInitialState();
    if (!states.contains(initialState)) {
        initialState.clear();
        for (const ReactionStep& step : steps) {
            initialState = normalizedName(step.currentState);
            if (!initialState.isEmpty()) break;
        }
    }
    if (!initialState.isEmpty())
        report.data.setInitialState(initialState);

    QHash<QString, QString> transitions;
    QHash<QString, int> firstTransitionColumn;
    QHash<QString, QString> mooreOutputs;
    QHash<QString, int> firstMooreColumn;

    for (int column = 0; column < steps.size(); ++column) {
        const ReactionStep& step = steps[column];
        const QString current = normalizedName(step.currentState);
        const QString input = normalizedName(step.inputSignal);
        const QString next = normalizedName(step.nextState);
        const QStringList stepOutputs = normalizedOutputs(step.outputSignals);

        if (current.isEmpty() || input.isEmpty() || next.isEmpty()) {
            addIssue(report.issues, column, -1, ReactionIssueSeverity::Incomplete,
                     trReaction("Fill in the state, input and next state"));
            continue;
        }

        const QString key = transitionKey(current, input);
        const QString value = transitionValue(next, stepOutputs, base.getType());
        if (transitions.contains(key) && transitions.value(key) != value) {
            const QString message = trReaction(
                "Conflict: different transitions are specified for %1 + %2")
                .arg(current, input);
            addIssue(report.issues, firstTransitionColumn.value(key), -1,
                     ReactionIssueSeverity::Error, message);
            addIssue(report.issues, column, -1, ReactionIssueSeverity::Error, message);
            continue;
        }
        transitions.insert(key, value);
        if (!firstTransitionColumn.contains(key))
            firstTransitionColumn.insert(key, column);

        if (base.getType() == VariantType::MooreToMealy) {
            QStringList sortedOutputs = stepOutputs;
            sortedOutputs.sort(Qt::CaseSensitive);
            const QString outputValue = sortedOutputs.join(QChar(0x1e));
            if (mooreOutputs.contains(next) && mooreOutputs.value(next) != outputValue) {
                const QString message = trReaction(
                    "Conflict: state %1 is assigned different outputs").arg(next);
                addIssue(report.issues, firstMooreColumn.value(next), 3,
                         ReactionIssueSeverity::Error, message);
                addIssue(report.issues, column, 3, ReactionIssueSeverity::Error, message);
                continue;
            }
            mooreOutputs.insert(next, outputValue);
            if (!firstMooreColumn.contains(next))
                firstMooreColumn.insert(next, column);
        }

        const QString transitionText = base.getType() == VariantType::MealyToMoore
            ? QStringLiteral("%1 / %2").arg(
                  next, stepOutputs.isEmpty() ? QStringLiteral("—") : stepOutputs.join(", "))
            : next;
        result = report.data.setTransitionCellByName(input, current, transitionText);
        if (!result.ok()) {
            addIssue(report.issues, column, -1, ReactionIssueSeverity::Error,
                     friendlyValidationMessage(result) + (result.details().isEmpty()
                         ? QString() : QStringLiteral(": ") + result.details()));
            continue;
        }

        if (base.getType() == VariantType::MooreToMealy) {
            result = report.data.setMooreOutputCellByName(
                next, stepOutputs.isEmpty() ? QStringLiteral("—") : stepOutputs.join(", "));
            if (!result.ok())
                addIssue(report.issues, column, 3, ReactionIssueSeverity::Error,
                         friendlyValidationMessage(result));
        }
    }

    return report;
}

QVector<ReactionStep> ReactionSynchronizer::buildReaction(const AutomatonData& data) {
    QVector<ReactionStep> result;
    QStringList states = data.getStateNames();
    const QString initialState = data.getInitialState();
    if (states.removeOne(initialState)) states.prepend(initialState);
    const QStringList& inputs = data.getInputSignalNames();
    const auto& table = data.getTransitionTable();

    for (const QString& state : states) {
        const int stateIndex = data.getStateNames().indexOf(state);
        for (int inputIndex = 0; inputIndex < inputs.size(); ++inputIndex) {
            ReactionStep step;
            step.currentState = state;
            step.inputSignal = inputs[inputIndex];

            if (inputIndex < table.size() && stateIndex < table[inputIndex].size()) {
                const CellData& cell = table[inputIndex][stateIndex];
                step.nextState = cell.nextState;
                const QStringList outputs = data.getType() == VariantType::MealyToMoore
                    ? cell.outputSignals
                    : data.getMooreOutputs().value(cell.nextState);
                step.outputSignals = outputs.join(QStringLiteral(", "));
            }
            result.append(step);
        }
    }

    if (result.isEmpty()) {
        ReactionStep firstStep;
        firstStep.currentState = initialState;
        result.append(firstStep);
    }
    return result;
}
