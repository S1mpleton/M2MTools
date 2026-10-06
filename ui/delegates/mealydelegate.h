#ifndef MEALYDELEGATE_H
#define MEALYDELEGATE_H

#include "core/automatondata.h"

#include <QStyledItemDelegate>
#include <QWidget>

class MealyDelegate : public QStyledItemDelegate
{
public:
    MealyDelegate(const AutomatonData* data, QObject* parent);

    QWidget* createEditor(QWidget* parent, const QStyleOptionViewItem&, const QModelIndex& index) const override;

    void setEditorData(QWidget* editor, const QModelIndex& index) const override;

    void setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const override;

    static QString normalizeOutputList(const QString& text);

    static QWidget* createContainer(QWidget* parent);

private:
    const AutomatonData* m_data;

protected:
    bool eventFilter(QObject *object, QEvent *event) override;
};

#endif // MEALYDELEGATE_H
