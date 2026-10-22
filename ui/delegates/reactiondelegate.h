#ifndef REACTIONDELEGATE_H
#define REACTIONDELEGATE_H

#include <QStyledItemDelegate>

class ReactionDelegate : public QStyledItemDelegate
{
public:
    explicit ReactionDelegate(QObject* parent = nullptr);

    QWidget* createEditor(QWidget* parent,
                          const QStyleOptionViewItem& option,
                          const QModelIndex& index) const override;
    void setModelData(QWidget* editor, QAbstractItemModel* model,
                      const QModelIndex& index) const override;
};

#endif // REACTIONDELEGATE_H
