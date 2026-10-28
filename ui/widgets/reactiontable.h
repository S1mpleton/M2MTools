#pragma once
#include "core/reactionvariant.h"
#include <QTableWidget>

class ReactionTable : public QTableWidget {
    Q_OBJECT
  public:
    explicit ReactionTable(QWidget *parent = nullptr);
    QVector<ReactionStep> steps() const;
    void setSteps(const QVector<ReactionStep> &steps);
    void insertStep();
    void removeStep();
    void markError(const QString &offender, const QString &message);
    void retranslate();

  private:
    void prepareColumn(int column);
    void ensureTrailingSpace();
};
