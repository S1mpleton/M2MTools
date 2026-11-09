#include "resultmessenger.h"
#include "ui/translateresult.h"

QMessageBox::Icon ResultMessenger::iconFor(ResultSeverity severity) {
    switch (severity) {
    case ResultSeverity::Success: return QMessageBox::Information;
    case ResultSeverity::Warning: return QMessageBox::Warning;
    case ResultSeverity::Error: return QMessageBox::Critical;
    }
    return QMessageBox::Information;
}

QString ResultMessenger::titleFor(const Result& result) {
    switch (result.severity()) {
    case ResultSeverity::Success: return tr("Success");
    case ResultSeverity::Warning: return tr("Warning");
    case ResultSeverity::Error: return tr("Error");
    }
    return tr("Message");
}

QString ResultMessenger::textFor(const Result& result) {
    QString text = TranslateResult::forUi(result);

    if (result.code() == ErrorCode::None && !result.offender().isEmpty()) {
        text += QStringLiteral("\n\n") + tr("Related: %1").arg(result.offender());
    }

    if (result.isError() && !result.details().isEmpty()) {
        text += QStringLiteral("\n\n")
            + tr("Technical details are available in the event log.");
    }

    return text;
}

void ResultMessenger::show(QWidget* parent, const Result& result, bool showSuccess) {
    if (result.isSuccess() && !showSuccess)
        return;

    QMessageBox box(
        iconFor(result.severity()),
        titleFor(result), textFor(result),
        QMessageBox::Ok,
        parent);

    box.exec();
}

void ResultMessenger::showSuccess(QWidget* parent, const QString& message) {
    show(parent, Result::success(message), true);
}

void ResultMessenger::showWarning(QWidget* parent, const QString& message) {
    show(parent, Result::warning(message));
}

void ResultMessenger::showError(QWidget* parent, const QString& message) {
    show(parent, Result::error(message));
}

bool ResultMessenger::askConfirm(QWidget* parent, const QString& message, const QString& title) {
    const QMessageBox::StandardButton answer = QMessageBox::question(
        parent,
        title.isEmpty() ? tr("Confirm") : title,
        message,
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No);

    return answer == QMessageBox::Yes;
}
