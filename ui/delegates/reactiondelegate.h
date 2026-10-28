#ifndef REACTIONDELEGATE_H
#define REACTIONDELEGATE_H

#include <QStyledItemDelegate>

class ReactionDelegate : public QStyledItemDelegate {
    Q_OBJECT
  public:
    explicit ReactionDelegate(QObject *parent = nullptr);

    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option,
                          const QModelIndex &index) const override;
    void setModelData(QWidget *editor, QAbstractItemModel *model,
                      const QModelIndex &index) const override;
  signals:
    void nextStepRequested();

  protected:
    bool eventFilter(QObject *editor, QEvent *event) override;
};

#endif // REACTIONDELEGATE_H
