#include "ui/fieldfeedback.h"
#include "ui/feedbackstyle.h"
#include <QEvent>

FieldFeedback::FieldFeedback(QObject *parent) : QObject(parent) {
    parent->installEventFilter(this);
}
bool FieldFeedback::eventFilter(QObject *object, QEvent *event) {
    if (event->type() == QEvent::ApplicationPaletteChange)
        for (const auto &field : m_fields)
            apply(field);
    return QObject::eventFilter(object, event);
}

void FieldFeedback::apply(const Field &field) {
    if (field.widget)
        field.widget->setStyleSheet(field.style + "\nQWidget#" + field.widget->objectName() + " {" +
                                    UiFeedbackStyle::fieldStyle(UiFeedbackLevel::Error) + "}");
}
void FieldFeedback::mark(QWidget *widget, const QString &message) {
    for (const auto &field : m_fields)
        if (field.widget == widget)
            return;
    m_fields.append({widget, widget->styleSheet(), widget->toolTip()});
    widget->setProperty("feedbackInvalid", true);
    widget->setToolTip(message);
    apply(m_fields.back());
}
void FieldFeedback::clear() {
    for (const auto &field : m_fields) {
        if (!field.widget)
            continue;
        field.widget->setProperty("feedbackInvalid", false);
        field.widget->setStyleSheet(field.style);
        field.widget->setToolTip(field.tooltip);
    }
    m_fields.clear();
}
