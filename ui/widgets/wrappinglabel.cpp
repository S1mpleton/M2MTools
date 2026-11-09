#include "ui/widgets/wrappinglabel.h"
#include <QEvent>
#include <QResizeEvent>

WrappingLabel::WrappingLabel(QWidget *parent) : QLabel(parent) {
    setWordWrap(true);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
}
void WrappingLabel::refreshHeight() {
    // QLabel::heightForWidth also respects minimumHeight. Drop the previous
    // constraint first so a wider label or smaller font can reduce its height.
    setMinimumHeight(0);
    const int required = text().isEmpty() ? 0 : heightForWidth(qMax(1, width()));
    setMinimumHeight(qMax(0, required));
}
void WrappingLabel::resizeEvent(QResizeEvent *event) {
    QLabel::resizeEvent(event);
    refreshHeight();
}
void WrappingLabel::changeEvent(QEvent *event) {
    QLabel::changeEvent(event);
    if (event->type() == QEvent::FontChange || event->type() == QEvent::StyleChange ||
        event->type() == QEvent::LanguageChange)
        refreshHeight();
}
