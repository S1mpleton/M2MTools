#ifndef RESULTMESSENGER_H
#define RESULTMESSENGER_H

#include "core/result.h"

#include <QObject>
#include <QMessageBox>
#include <qcoreapplication.h>

class ResultMessenger
{
    Q_DECLARE_TR_FUNCTIONS(ResultMessenger)
public:
    static void show(QWidget* parent, const Result& result, bool showSuccess = false);

    static void showSuccess(QWidget* parent, const QString& message);
    static void showWarning(QWidget* parent, const QString& message);
    static void showError(QWidget* parent, const QString& message);

    static bool askConfirm(QWidget* parent, const QString& message, const QString& title = {});

private:
    ResultMessenger() = delete;

    static QMessageBox::Icon iconFor(ResultSeverity severity);
    static QString titleFor(const Result& result);
    static QString textFor(const Result& result);
};

#endif // RESULTMESSENGER_H
