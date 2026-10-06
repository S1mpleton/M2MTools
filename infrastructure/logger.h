#ifndef LOGGER_H
#define LOGGER_H

#include "core/result.h"

#include <QObject>
#include <QList>
#include <QTimer>

class Logger : public QObject {
    Q_OBJECT
public:
    static Logger& instance();
    static void log(const Result& result);

    static void info(const QString& message, ResultCategory category = ResultCategory::General);
    static void warning(const QString& message, ResultCategory category = ResultCategory::General);
    static void error(const QString& message, ResultCategory category = ResultCategory::General);

    static void setDebounceInterval(int ms);
    static int  debounceInterval();

    static void setDebounceWarnings(bool enabled);
    static bool debounceWarnings();

    QList<Result> history() const;
    void clearHistory();

signals:
    void newResult(const Result& result);
    void debounceIntervalChanged(int ms);

private:
    Logger();
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    void writeNow(const Result& result);
    void scheduleWrite(const Result& result);

    void flushPending();

    QList<Result> m_history;
    QTimer* m_logDebounceTimer = new QTimer(this);
    QTimer* m_debounceTimer = nullptr;
    std::optional<Result> m_pending;
    int m_debounceIntervalMs = 0;
    bool m_debounceWarnings = false;

};
#endif // LOGGER_H
