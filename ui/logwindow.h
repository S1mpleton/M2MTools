#ifndef LOGWINDOW_H
#define LOGWINDOW_H

#include "core/result.h"

#include <QDialog>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QCheckBox>

namespace Ui {
class LogWindow;
}

class LogWindow : public QDialog
{
    Q_OBJECT

public:
    explicit LogWindow(QWidget* parent = nullptr);
    ~LogWindow() override;

protected:
    // void changeEvent(QEvent* event) override;
    void closeEvent(QCloseEvent* event) override;
    void changeEvent(QEvent* event) override;

private slots:
    void onNewResult(const Result& result);
    void onClearClicked();
    void onSaveToFileClicked();
    void onCopySelectedClicked();
    void onAutoScrollToggled(bool checked);
    void onFilterChanged();


private:
    void retranslateUi();
    void appendResult(const Result& result);
    bool passesFilter(const Result& result) const;

    QPlainTextEdit* m_textEdit = nullptr;
    QCheckBox* m_autoScrollBox = nullptr;
    QCheckBox* m_showInfoBox = nullptr;
    QCheckBox* m_showWarningsBox = nullptr;
    QCheckBox* m_showErrorsBox = nullptr;
    QPushButton* m_clearButton = nullptr;
    QPushButton* m_saveButton = nullptr;
    QPushButton* m_copyButton = nullptr;

    static constexpr int kMaxBlocks = 1000;
};

#endif // LOGWINDOW_H
