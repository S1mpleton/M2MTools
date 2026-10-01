#ifndef MOOREDELEGATE_H
#define MOOREDELEGATE_H

#include "automatondata.h"

#include <QStyledItemDelegate>
#include <QLineEdit>

class MooreDelegate : public QStyledItemDelegate
{
public:
    MooreDelegate(const AutomatonData* data, QObject* parent = nullptr);

    QWidget* createEditor(QWidget* parent, const QStyleOptionViewItem&, const QModelIndex& index) const override;

    void setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const override;

private:
    const AutomatonData* m_data;
    static QString normalizeOutputList(const QString& text);

};

#endif // MOOREDELEGATE_H
