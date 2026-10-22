#include "fielderrorhighlighter.h"

#include "ui/feedbackstyle.h"
#include "ui/translateresult.h"

namespace {
    constexpr auto kErrorIconId  = "errorIcon";
    constexpr auto kIconPath     = ":/ui/resources/icons/warning.png";
}

FieldErrorHighlighter::FieldErrorHighlighter(QLineEdit* field, QStatusBar* statusBar, QObject* parent)
    : IErrorPresenter(parent), m_field(field), m_statusBar(statusBar)
{

}

void FieldErrorHighlighter::show(const Result& result) {
    if (!m_field) return;

    m_field->setStyleSheet(UiFeedbackStyle::fieldStyle(UiFeedbackLevel::Error));
    m_field->setToolTip(translateResult(result));

    // Add the icon only once.
    if (!m_field->findChild<QAction*>(kErrorIconId)) {
        auto* icon = m_field->addAction(QIcon(kIconPath), QLineEdit::LeadingPosition);
        icon->setObjectName(kErrorIconId);
    }

    if (m_statusBar)
        m_statusBar->showMessage(translateResult(result));
}

void FieldErrorHighlighter::clear(const Result& result) {
    if (!m_field) return;

    m_field->setStyleSheet("");
    m_field->setToolTip("");

    if (auto* icon = m_field->findChild<QAction*>(kErrorIconId)) {
        m_field->removeAction(icon);
        icon->deleteLater();
    }

    Q_UNUSED(result);
}
