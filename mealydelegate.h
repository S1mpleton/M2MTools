#ifndef MEALYDELEGATE_H
#define MEALYDELEGATE_H

#include "automatondata.h"

#include <QStyledItemDelegate>

class MealyDelegate : public QStyledItemDelegate
{
public:
    MealyDelegate(const AutomatonData* data, QObject* parent);

    QWidget* createEditor(QWidget* parent, const QStyleOptionViewItem&, const QModelIndex& index) const override;

    void setEditorData(QWidget* editor, const QModelIndex& index) const override;

    void setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const override;

private:
    const AutomatonData* m_data;
    static QString normalizeOutputList(const QString& text);

protected:
    bool eventFilter(QObject *object, QEvent *event) override;
};

#endif // MEALYDELEGATE_H
