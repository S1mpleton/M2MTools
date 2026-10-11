#include "logwindow.h"
#include "infrastructure/logger.h"
#include "ui/dialogs/resultmessenger.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPlainTextEdit>
#include <QCheckBox>
#include <QPushButton>
#include <QComboBox>
#include <QLabel>
#include <QFileDialog>
#include <QMessageBox>
#include <QTextStream>
#include <QCloseEvent>
#include <QApplication>
#include <QClipboard>
#include <QScrollBar>


LogWindow::LogWindow(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Application Log"));
    setAttribute(Qt::WA_DeleteOnClose, false);

    auto* controlsLayout = new QHBoxLayout;

    m_showInfoBox = new QCheckBox(this);
    m_showInfoBox->setChecked(true);
    m_showWarningsBox = new QCheckBox(this);
    m_showWarningsBox->setChecked(true);
    m_showErrorsBox = new QCheckBox(this);
    m_showErrorsBox->setChecked(true);

    m_autoScrollBox = new QCheckBox(this);
    m_autoScrollBox->setChecked(true);

    m_copyButton = new QPushButton(this);
    m_clearButton = new QPushButton(this);
    m_saveButton = new QPushButton(this);

    retranslateUi();

    controlsLayout->addWidget(m_showInfoBox);
    controlsLayout->addWidget(m_showWarningsBox);
    controlsLayout->addWidget(m_showErrorsBox);
    controlsLayout->addStretch();
    controlsLayout->addWidget(m_autoScrollBox);
    controlsLayout->addWidget(m_copyButton);
    controlsLayout->addWidget(m_clearButton);
    controlsLayout->addWidget(m_saveButton);

    m_textEdit = new QPlainTextEdit(this);
    m_textEdit->setReadOnly(true);
    m_textEdit->setMaximumBlockCount(kMaxBlocks);
    m_textEdit->setLineWrapMode(QPlainTextEdit::NoWrap);
    m_textEdit->setTextInteractionFlags(
        Qt::TextSelectableByMouse | Qt::TextSelectableByKeyboard);

    QFont monoFont = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    m_textEdit->setFont(monoFont);

    // --- Layout ---
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->addLayout(controlsLayout);
    mainLayout->addWidget(m_textEdit);

    resize(900, 500);

    // --- Load history ---
    for (const Result& r : Logger::instance().history())
        appendResult(r);

    // --- Connect by new ---
    connect(&Logger::instance(), &Logger::newResult,
            this, &LogWindow::onNewResult);

    // --- Buttons ---
    connect(m_clearButton, &QPushButton::clicked,
            this, &LogWindow::onClearClicked);
    connect(m_saveButton, &QPushButton::clicked,
            this, &LogWindow::onSaveToFileClicked);
    connect(m_copyButton, &QPushButton::clicked,
            this, &LogWindow::onCopySelectedClicked);

    connect(m_autoScrollBox, &QCheckBox::toggled,
            this, &LogWindow::onAutoScrollToggled);
    connect(m_showInfoBox, &QCheckBox::toggled,
            this, &LogWindow::onFilterChanged);
    connect(m_showWarningsBox, &QCheckBox::toggled,
            this, &LogWindow::onFilterChanged);
    connect(m_showErrorsBox, &QCheckBox::toggled,
            this, &LogWindow::onFilterChanged);
}

LogWindow::~LogWindow() = default;


void LogWindow::onNewResult(const Result& result) {
    appendResult(result);

    if (m_autoScrollBox->isChecked()) {
        m_textEdit->verticalScrollBar()->setValue(
            m_textEdit->verticalScrollBar()->maximum());
    }
}

void LogWindow::appendResult(const Result& result) {
    if (!passesFilter(result))
        return;

    const QString color = result.isError() ?
                QStringLiteral("#CC2222") :
                result.isWarning() ?
                    QStringLiteral("#CC8822") :
                    QStringLiteral("#228822");

    const QString timestamp = result.timestamp().toString("hh:mm:ss");

    QString text = QStringLiteral("[%1] [%2] [%3] %4")
                       .arg(timestamp,
                            resultSeverityToString(result.severity()).toHtmlEscaped(),
                            resultCategoryToString(result.category()).toHtmlEscaped(),
                            result.message().toHtmlEscaped());

    if (!result.offender().isEmpty()) {
        text += QStringLiteral(" <i>(%1: %2)</i>")
        .arg("offender", result.offender().toHtmlEscaped());
    }

    if (!result.details().isEmpty()) {
        text += QStringLiteral("<br/><span style='color:#666666;'>    %1</span>")
        .arg(result.details().toHtmlEscaped());
    }

    const QString html = QStringLiteral(
                             "<span style='color:%1; white-space: pre;'>%2</span>")
                             .arg(color, text);

    m_textEdit->appendHtml(html);
}

bool LogWindow::passesFilter(const Result& result) const {
    if (result.isSuccess() && !m_showInfoBox->isChecked()) return false;
    if (result.isWarning() && !m_showWarningsBox->isChecked()) return false;
    if (result.isError() && !m_showErrorsBox->isChecked()) return false;
    return true;
}

void LogWindow::onFilterChanged() {
    m_textEdit->clear();
    for (const Result& r : Logger::instance().history())
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
    const QString path = QFileDialog::getSaveFileName(
        this, tr("Save Log"), QString(),
        tr("Text files (*.txt);;All files (*)"));

    if (path.isEmpty()) return;

    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        ResultMessenger::showError(this, tr("Cannot save file: %1").arg(file.errorString()));
        return;
    }

    QTextStream out(&file);
    out << m_textEdit->toPlainText();
    file.close();
}

void LogWindow::onAutoScrollToggled(bool /*checked*/) {
}


void LogWindow::closeEvent(QCloseEvent* event) {
    hide();
    event->ignore();
}

void LogWindow::changeEvent(QEvent* event) {
    if (event->type() == QEvent::LanguageChange) {
        retranslateUi();
    }
    QDialog::changeEvent(event);
}

void LogWindow::retranslateUi() {
    setWindowTitle(tr("Application Log"));

    m_showInfoBox->setText(tr("Info"));
    m_showWarningsBox->setText(tr("Warnings"));
    m_showErrorsBox->setText(tr("Errors"));

    m_autoScrollBox->setText(tr("Auto-scroll"));

    m_copyButton->setText(tr("Copy Selected"));
    m_clearButton->setText(tr("Clear View"));
    m_saveButton->setText(tr("Save to File..."));
}