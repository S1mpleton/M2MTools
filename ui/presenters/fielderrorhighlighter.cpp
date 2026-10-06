#include "fielderrorhighlighter.h"

namespace {
    constexpr auto kErrorStyle   = "background: #CC2222; color: white;";
    constexpr auto kErrorIconId  = "errorIcon";
    constexpr auto kIconPath     = ":/ui/resources/icons/warning.png";
}

FieldErrorHighlighter::FieldErrorHighlighter(QLineEdit* field, QStatusBar* statusBar, QObject* parent)
    : IErrorPresenter(parent), m_field(field), m_statusBar(statusBar)
{

}

void FieldErrorHighlighter::show(const Result& result) {
    if (!m_field) return;

    m_field->setStyleSheet(QString::fromLatin1(kErrorStyle));
    m_field->setToolTip(result.message());

    // Иконку добавляем только один раз
    if (!m_field->findChild<QAction*>(kErrorIconId)) {
        auto* icon = m_field->addAction(QIcon(kIconPath), QLineEdit::LeadingPosition);
        icon->setObjectName(kErrorIconId);
    }

    if (m_statusBar)
        m_statusBar->showMessage(result.message());
}

void FieldErrorHighlighter::clear(const Result& result) {
    if (!m_field) return;

    m_field->setStyleSheet("");
    m_field->setToolTip("");

    if (auto* icon = m_field->findChild<QAction*>(kErrorIconId)) {
        m_field->removeAction(icon);
        icon->deleteLater();
    }

    if (m_statusBar)
        m_statusBar->showMessage(result.message(), 2500);
}
