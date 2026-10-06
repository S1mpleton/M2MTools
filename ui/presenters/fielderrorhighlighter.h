#ifndef FIELDERRORHIGHLIGHTER_H
#define FIELDERRORHIGHLIGHTER_H

#include "ierrorpresenter.h"
#include "core/Result.h"

#include <QObject>
#include <QStatusBar>
#include <QLineEdit>
#include <QPointer>

class FieldErrorHighlighter : public IErrorPresenter
{
    Q_OBJECT
public:
    explicit  FieldErrorHighlighter(QLineEdit* field, QStatusBar* statusBar = nullptr, QObject* parent = nullptr);

    void show(const Result& result);
    void clear(const Result& result);

private:
    QPointer<QLineEdit> m_field;
    QPointer<QStatusBar> m_statusBar;

signals:
};

#endif // FIELDERRORHIGHLIGHTER_H
