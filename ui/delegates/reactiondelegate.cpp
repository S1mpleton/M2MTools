#include "reactiondelegate.h"
#include "validation/reactionvalidator.h"
#include <QKeyEvent>
#include <QLineEdit>

ReactionDelegate::ReactionDelegate(QObject *parent) : QStyledItemDelegate(parent) {}
QWidget *ReactionDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &,
                                        const QModelIndex &index) const {
    auto *editor = new QLineEdit(parent);
    editor->setObjectName(QStringLiteral("reactionCellEditor"));
    editor->setValidator(new ReactionValidator(index.row() == 1, editor));
    return editor;
}
void ReactionDelegate::setModelData(QWidget *editor, QAbstractItemModel *model,
                                    const QModelIndex &index) const {
    const auto *line = qobject_cast<QLineEdit *>(editor);
    if (!line || !line->hasAcceptableInput())
        return;
    QString value = line->text().trimmed();
    if (index.row() == 1 && value != "-" && value != QStringLiteral("—")) {
        auto parts = value.split(',');
        for (auto &part : parts)
            part = part.trimmed();
        value = parts.join(',');
    }
    model->setData(index, value, Qt::EditRole);
}

bool ReactionDelegate::eventFilter(QObject *object, QEvent *event) {
    if (event->type() == QEvent::KeyPress) {
        const auto *key = static_cast<QKeyEvent *>(event);
        if (key->key() == Qt::Key_Return || key->key() == Qt::Key_Enter) {
            auto *editor = qobject_cast<QLineEdit *>(object);
            if (editor && editor->hasAcceptableInput() && !key->isAutoRepeat()) {
                emit commitData(editor);
                emit closeEditor(editor, QAbstractItemDelegate::NoHint);
                emit nextStepRequested();
            }
            return true;
        }
    }
    return QStyledItemDelegate::eventFilter(object, event);
}
