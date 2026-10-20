#include "reactionsynchronizer.h"

#include "core/namelistparser.h"

#include <QHash>
#include <QSet>

namespace {
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
        return QStringLiteral(
            "Некорректное имя «%1»: используйте буквы и цифры, начиная с буквы")
            .arg(result.offender());
    case ErrorCode::ADDuplicateNameInList:
        return QStringLiteral("Имя «%1» указано повторно").arg(result.offender());
    case ErrorCode::ADDuplicateNameInAllList:
        return QStringLiteral(
            "Имя «%1» уже используется в другой группе обозначений")
            .arg(result.offender());
    case ErrorCode::ADUnknownState:
        return QStringLiteral("Неизвестное состояние «%1»").arg(result.offender());
    case ErrorCode::ADUnknownInput:
        return QStringLiteral("Неизвестный входной сигнал «%1»").arg(result.offender());
    case ErrorCode::ADUnknownOutput:
        return QStringLiteral("Неизвестный выходной сигнал «%1»").arg(result.offender());
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
    report.data.setStateUsageChecker(base.getValidationLevel());
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
                     QStringLiteral("Заполните состояние, вход и следующее состояние"));
            continue;
        }

        const QString key = transitionKey(current, input);
        const QString value = transitionValue(next, stepOutputs, base.getType());
        if (transitions.contains(key) && transitions.value(key) != value) {
            const QString message = QStringLiteral(
                "Противоречие: для пары %1 + %2 указаны разные переходы")
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
                const QString message = QStringLiteral(
                    "Противоречие: состоянию %1 назначены разные выходы").arg(next);
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
    const QStringList& states = data.getStateNames();
    const QStringList& inputs = data.getInputSignalNames();
    const auto& table = data.getTransitionTable();

    for (int stateIndex = 0; stateIndex < states.size(); ++stateIndex) {
        for (int inputIndex = 0; inputIndex < inputs.size(); ++inputIndex) {
            ReactionStep step;
            step.currentState = states[stateIndex];
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

    if (result.isEmpty())
        result.append(ReactionStep{});
    return result;
}
