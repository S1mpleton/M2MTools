#pragma once
#include "ui/feedbackstyle.h"
#include "ui/widgets/wrappinglabel.h"
class QGraphicsOpacityEffect;
class QSequentialAnimationGroup;

// Designer owns the layout; this label owns the complete notification lifecycle.
class FeedbackLabel : public WrappingLabel {
    Q_OBJECT
  public:
    explicit FeedbackLabel(QWidget *parent = nullptr);
    void showMessage(const QString &text, UiFeedbackLevel level);
    void updateMessageText(const QString &text);
    void clearMessage();
  signals:
    void expired();
  protected:
    bool eventFilter(QObject *object, QEvent *event) override;


  private:
    void refreshStyle();
    QGraphicsOpacityEffect *m_opacity;
    QSequentialAnimationGroup *m_animation;
    UiFeedbackLevel m_level = UiFeedbackLevel::Success;
    bool m_active = false;
};
