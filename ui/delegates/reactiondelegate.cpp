#include "reactiondelegate.h"

#include "core/namelistparser.h"
#include "validation/compositevalidator.h"
#include "validation/editvalidator.h"
#include "validation/listnamesvalidator.h"

#include <QLineEdit>
#include <QRegularExpression>

namespace {
const QRegularExpression kSingleNameForbiddenCharacters(
    QStringLiteral("[^a-zA-Zа-яА-ЯёЁ0-9]"));
}

ReactionDelegate::ReactionDelegate(QObject* parent)
    : QStyledItemDelegate(parent)
{
}

QWidget* ReactionDelegate::createEditor(
    QWidget* parent, const QStyleOptionViewItem&, const QModelIndex& index) const {
    auto* editor = new QLineEdit(parent);
    editor->setObjectName(QStringLiteral("reactionCellEditor"));
    editor->setAutoFillBackground(true);

    if (index.row() == 3) {
        auto* validator = new CompositeValidator(editor);
        validator->addValidator(new EditValidator());
        validator->addValidator(new ListNamesValidator());
        editor->setValidator(validator);
    } else {
        editor->setValidator(
            new EditValidator(editor, kSingleNameForbiddenCharacters));
    }
    return editor;
}

void ReactionDelegate::setModelData(
    QWidget* editor, QAbstractItemModel* model,
    const QModelIndex& index) const {
    const auto* lineEdit = qobject_cast<QLineEdit*>(editor);
    if (!lineEdit) return;

    QString text = lineEdit->text().trimmed();
    if (index.row() == 3)
        text = NameListParser::parse(text).join(QStringLiteral(", "));
    model->setData(index, text, Qt::EditRole);
}
