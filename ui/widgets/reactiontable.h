#pragma once
#include "core/reactionvariant.h"
#include <QTableWidget>

class ReactionTable : public QTableWidget {
    Q_OBJECT
  public:
    explicit ReactionTable(QWidget *parent = nullptr);
    ~ReactionTable() override;
    QVector<ReactionStep> steps() const;
    void setSteps(const QVector<ReactionStep> &steps);
    void insertStep();
    void removeStep();
    void markError(const QString &offender, const QString &message);
    void markRowError(int row, const QString &message);
    void clearCellFeedback();
    void retranslate();
    QSize minimumSizeHint() const override;
    QSize sizeHint() const override;

  protected:
    void keyPressEvent(QKeyEvent *event) override;
    void changeEvent(QEvent *event) override;

  private:
    void prepareColumn(int column);
    void updateTableSize();
};
