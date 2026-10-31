#include "reactiondelegate.h"
#include "ui/feedbackstyle.h"
#include "validation/reactionvalidator.h"
#include <QKeyEvent>
#include <QLineEdit>
#include <QStyle>

ReactionDelegate::ReactionDelegate(QObject *parent) : QStyledItemDelegate(parent) {}
void ReactionDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option,
                             const QModelIndex &index) const {
    auto styled = option;
    if (index.data(ErrorRole).toBool()) {
        initStyleOption(&styled, index);
        const auto fill = UiFeedbackStyle::background(UiFeedbackLevel::Error);
        const auto ink = UiFeedbackStyle::foreground(UiFeedbackLevel::Error);
        styled.backgroundBrush = fill;
        styled.palette.setColor(QPalette::Text, ink);
        styled.palette.setColor(QPalette::Highlight, fill);
        styled.palette.setColor(QPalette::HighlightedText, ink);
        const auto *widget = option.widget;
        auto *style = widget ? widget->style() : QApplication::style();
        style->drawControl(QStyle::CE_ItemViewItem, &styled, painter, widget);
        return;
    }
    QStyledItemDelegate::paint(painter, styled, index);
}
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
