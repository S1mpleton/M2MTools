#ifndef REACTIONSYNCHRONIZER_H
#define REACTIONSYNCHRONIZER_H

#include "core/automatondata.h"

#include <QString>
#include <QVector>

struct ReactionStep {
    QString currentState;
    QString inputSignal;
    QString nextState;
    QString outputSignals;
};

enum class ReactionIssueSeverity {
    Incomplete,
    Error
};

struct ReactionIssue {
    int column = -1;
    int row = -1;
    ReactionIssueSeverity severity = ReactionIssueSeverity::Error;
    QString message;
};

struct ReactionSyncReport {
    AutomatonData data;
    QVector<ReactionIssue> issues;

    bool hasErrors() const;
};

class ReactionSynchronizer
{
public:
    static ReactionSyncReport buildAutomaton(
        const QVector<ReactionStep>& steps,
        const AutomatonData& base);

    static QVector<ReactionStep> buildReaction(const AutomatonData& data);
};

#endif // REACTIONSYNCHRONIZER_H
