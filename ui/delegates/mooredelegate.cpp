#include "mooredelegate.h"

#include "validation/compositevalidator.h"
#include "validation/listnamesvalidator.h"
#include "validation/editvalidator.h"

MooreDelegate::MooreDelegate(const AutomatonData* data, QObject* parent)
    : QStyledItemDelegate(parent), m_data(data)
{

}

QWidget* MooreDelegate::createEditor(QWidget* parent, const QStyleOptionViewItem&, const QModelIndex& index) const
{
    QLineEdit* editor = new QLineEdit(parent);
    editor->setAutoFillBackground(true);

    auto kind = m_data->cellKind(index.row(), index.column());
    if (kind == CellKind::MooreOutput) {
        CompositeValidator *composite = new CompositeValidator(editor);
        composite->addValidator(new EditValidator());
        composite->addValidator(new ListNamesValidator());

        editor->setValidator(composite);

    } else if (kind == CellKind::Transition) {
        editor->setValidator(new EditValidator(editor, QRegularExpression("[^a-zA-Zа-яА-ЯёЁ0-9]")));
    }

    return editor;
}

void MooreDelegate::setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const
{
    QLineEdit* lineEdit = qobject_cast<QLineEdit*>(editor);
    if (!lineEdit) return;

    QString text = lineEdit->text().trimmed();

    if (text.isEmpty() || text == "-")
        text = "—";

    auto kind = m_data->cellKind(index.row(), index.column());

    if (kind == CellKind::MooreOutput) {
        text = normalizeOutputList(text);
    }

    model->setData(index, text, Qt::EditRole);
}

QString MooreDelegate::normalizeOutputList(const QString& text)
{
    if (text == "—") return text;

    QStringList parts = text.split(',', Qt::SkipEmptyParts);
    for (QString& p : parts){
        p = p.trimmed();
    }
    return parts.join(", ");
}
