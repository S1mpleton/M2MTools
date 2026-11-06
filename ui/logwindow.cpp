#include "logwindow.h"
#include "infrastructure/logger.h"
#include "ui/appicons.h"
#include "ui/dialogs/resultmessenger.h"
#include "ui/feedbackstyle.h"
#include "ui/translateresult.h"
#include "ui_logwindow.h"

#include <QApplication>
#include <QCheckBox>
#include <QClipboard>
#include <QCloseEvent>
#include <QComboBox>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QScrollBar>
#include <QTextStream>
#include <QVBoxLayout>

LogWindow::LogWindow(QWidget *parent) : QDialog(parent), ui(new Ui::LogWindow) {
    ui->setupUi(this);
    setWindowIcon(AppIcons::logWindowIcon());
    setAttribute(Qt::WA_DeleteOnClose, false);
    m_textEdit = ui->logTextEdit;
    m_autoScrollBox = ui->autoScrollBox;
    m_showInfoBox = ui->showInfoBox;
    m_showWarningsBox = ui->showWarningsBox;
    m_showErrorsBox = ui->showErrorsBox;
    m_copyButton = ui->copyButton;
    m_clearButton = ui->clearButton;
    m_saveButton = ui->saveButton;
    m_copyButton->setIcon(AppIcons::icon(AppIcons::Kind::Copy));
    m_clearButton->setIcon(AppIcons::icon(AppIcons::Kind::Clear));
    m_saveButton->setIcon(AppIcons::icon(AppIcons::Kind::Save));
    m_textEdit->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));

    // --- Load history ---
    for (const Result &r : Logger::instance().history())
        appendResult(r);

    // --- Connect by new ---
    connect(&Logger::instance(), &Logger::newResult, this, &LogWindow::onNewResult);

    // --- Buttons ---
    connect(m_clearButton, &QPushButton::clicked, this, &LogWindow::onClearClicked);
    connect(m_saveButton, &QPushButton::clicked, this, &LogWindow::onSaveToFileClicked);
    connect(m_copyButton, &QPushButton::clicked, this, &LogWindow::onCopySelectedClicked);

    connect(m_autoScrollBox, &QCheckBox::toggled, this, &LogWindow::onAutoScrollToggled);
    connect(m_showInfoBox, &QCheckBox::toggled, this, &LogWindow::onFilterChanged);
    connect(m_showWarningsBox, &QCheckBox::toggled, this, &LogWindow::onFilterChanged);
    connect(m_showErrorsBox, &QCheckBox::toggled, this, &LogWindow::onFilterChanged);
}

LogWindow::~LogWindow() {
    delete ui;
}

void LogWindow::onNewResult(const Result &result) {
    appendResult(result);

    if (m_autoScrollBox->isChecked()) {
        m_textEdit->verticalScrollBar()->setValue(m_textEdit->verticalScrollBar()->maximum());
    }
}

void LogWindow::appendResult(const Result &result) {
    if (!passesFilter(result))
        return;

    const UiFeedbackLevel level = result.isError()     ? UiFeedbackLevel::Error
                                  : result.isWarning() ? UiFeedbackLevel::Warning
                                                       : UiFeedbackLevel::Success;
    const QString color = UiFeedbackStyle::foreground(level).name();

    const QString timestamp = result.timestamp().toString("hh:mm:ss");

    QString text = QStringLiteral("[%1] [%2] [%3] %4")
                       .arg(timestamp, resultSeverityDisplayName(result.severity()).toHtmlEscaped(),
                            resultCategoryDisplayName(result.category()).toHtmlEscaped(),
                            TranslateResult::forUi(result).toHtmlEscaped());
    if (!result.details().isEmpty())
        text += " | " + tr("Details: %1").arg(result.details()).toHtmlEscaped();

    const QString html =
        QStringLiteral("<span style='color:%1; white-space: pre;'>%2</span>").arg(color, text);

    m_textEdit->appendHtml(html);
}

bool LogWindow::passesFilter(const Result &result) const {
    if (result.isSuccess() && !m_showInfoBox->isChecked())
        return false;
    if (result.isWarning() && !m_showWarningsBox->isChecked())
        return false;
    if (result.isError() && !m_showErrorsBox->isChecked())
        return false;
    return true;
}

void LogWindow::onFilterChanged() {
    m_textEdit->clear();
    for (const Result &r : Logger::instance().history())
        appendResult(r);
}

void LogWindow::onCopySelectedClicked() {
    const QString selected = m_textEdit->textCursor().selectedText();
    if (selected.isEmpty()) {
        QApplication::clipboard()->setText(m_textEdit->toPlainText());
    } else {
        QString fixed = selected;
        fixed.replace(QChar(0x2029), '\n');
        QApplication::clipboard()->setText(fixed);
    }
}

void LogWindow::onClearClicked() {
    m_textEdit->clear();
}

void LogWindow::onSaveToFileClicked() {
    const QString path = QFileDialog::getSaveFileName(this, tr("Save Log"), QString(),
                                                      tr("Text files (*.txt);;All files (*)"));

    if (path.isEmpty())
        return;

    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        ResultMessenger::showError(this, tr("Cannot save file: %1").arg(file.errorString()));
        return;
    }

    QTextStream out(&file);
    out << m_textEdit->toPlainText();
    file.close();
}

void LogWindow::onAutoScrollToggled(bool /*checked*/) {}

void LogWindow::closeEvent(QCloseEvent *event) {
    hide();
    event->ignore();
}

void LogWindow::changeEvent(QEvent *event) {
    if (event->type() == QEvent::LanguageChange) {
        retranslateUi();
    }
    QDialog::changeEvent(event);
}

void LogWindow::retranslateUi() {
    ui->retranslateUi(this);
    onFilterChanged();
}
