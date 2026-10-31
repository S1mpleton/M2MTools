#include "ui/widgets/feedbacklabel.h"
#include <QEvent>
#include <QGraphicsOpacityEffect>
#include <QPropertyAnimation>
#include <QSequentialAnimationGroup>

FeedbackLabel::FeedbackLabel(QWidget *parent) : WrappingLabel(parent) {
    setTextFormat(Qt::PlainText);
    m_opacity = new QGraphicsOpacityEffect(this);
    m_opacity->setOpacity(0);
    setGraphicsEffect(m_opacity);
    m_animation = new QSequentialAnimationGroup(this);
    // ApplicationPaletteChange is delivered to top-level windows, not every label.
    qApp->installEventFilter(this);
    auto fade = [this](qreal from, qreal to) {
        auto *animation = new QPropertyAnimation(m_opacity, "opacity");
        animation->setDuration(250);
        animation->setStartValue(from);
        animation->setEndValue(to);
        animation->setEasingCurve(QEasingCurve::InOutQuad);
        m_animation->addAnimation(animation);
    };
    fade(0, 1);
    m_animation->addPause(4000);
    fade(1, 0);
    connect(m_animation, &QSequentialAnimationGroup::finished, this, [this] {
        clearMessage();
        emit expired();
    });
}

void FeedbackLabel::showMessage(const QString &text, UiFeedbackLevel level) {
    m_animation->stop();
    m_level = level;
    m_active = true;
    m_opacity->setOpacity(0);
    setText(text);
    refreshStyle();
    refreshHeight();
    m_animation->start();
}

void FeedbackLabel::updateMessageText(const QString &text) {
    if (!m_active)
        return;
    setText(text);
    refreshHeight();
}

void FeedbackLabel::clearMessage() {
    m_animation->stop();
    m_active = false;
    m_opacity->setOpacity(0);
    clear();
    setStyleSheet({});
    refreshHeight();
}

void FeedbackLabel::refreshStyle() {
    if (m_active)
        setStyleSheet(UiFeedbackStyle::bannerStyle(m_level));
}

bool FeedbackLabel::eventFilter(QObject *object, QEvent *event) {
    if (object == window() && event->type() == QEvent::ApplicationPaletteChange) {
        refreshStyle();
        refreshHeight();
        update();
    }
    return WrappingLabel::eventFilter(object, event);
}
