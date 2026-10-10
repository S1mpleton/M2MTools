#include "logger.h"
#include "core/result.h"

#include <QDebug>
#include <QFile>
#include <QTextStream>


Logger& Logger::instance() {
    static Logger logger;
    return logger;
}

Logger::Logger() {
    m_debounceTimer = new QTimer(this);
    m_debounceTimer->setSingleShot(true);
    m_debounceIntervalMs = 800;

    connect(m_debounceTimer, &QTimer::timeout,
        this, &Logger::flushPending);
}

// Public ---
void Logger::log(const Result& result) {
    Logger& self = instance();

    bool useDebounce = false;
    if (self.m_debounceIntervalMs > 0) {
        if (result.isSuccess()) {
            useDebounce = true;
        } else if (result.isWarning() && self.m_debounceWarnings) {
            useDebounce = true;
        }
    }

    if (useDebounce)
        self.scheduleWrite(result);
    else
        self.writeNow(result);
}

void Logger::setDebounceInterval(int ms) {
    Logger& self = instance();
    if (self.m_debounceIntervalMs == ms)
        return;

    self.m_debounceIntervalMs = qMax(0, ms);

    if (self.m_debounceIntervalMs == 0) {
        self.m_debounceTimer->stop();
        self.flushPending();
    } else {
        if (self.m_pending.has_value())
            self.m_debounceTimer->start(self.m_debounceIntervalMs);
    }

    emit self.debounceIntervalChanged(self.m_debounceIntervalMs);
}

int Logger::debounceInterval() {
    return instance().m_debounceIntervalMs;
}

void Logger::setDebounceWarnings(bool enabled) {
    instance().m_debounceWarnings = enabled;
}

bool Logger::debounceWarnings() {
    return instance().m_debounceWarnings;
}

QList<Result> Logger::history() const {
    return m_history;
}

void Logger::clearHistory() {
    m_history.clear();
}

void Logger::info(const QString& message, ResultCategory category) {
    Logger& self = instance();
    self.writeNow(Result::success(message).withCategory(category));
}

void Logger::warning(const QString& message, ResultCategory category) {
    log(Result::warning(message, category));
}

void Logger::error(const QString& message, ResultCategory category) {
    log(Result::error(message, category));
}

// Inner logic ---
void Logger::scheduleWrite(const Result& result) {
    m_pending = result;

    m_debounceTimer->start(m_debounceIntervalMs);
}

void Logger::flushPending() {
    if (!m_pending.has_value())
        return;

    writeNow(*m_pending);
    m_pending.reset();
}

void Logger::writeNow(const Result& result) {
    m_history.append(result);
    emit newResult(result);

    if (result.isSuccess()) return;

    const char* level =
        result.isError()   ? "ERROR" :
        result.isWarning() ? "WARN " : "INFO ";

    QString line = QStringLiteral("[%1] [%2] %3")
                       .arg(QString::fromLatin1(level))
                       .arg(static_cast<int>(result.category()))
                       .arg(result.message());

    if (result.code() != ErrorCode::None) {
        line += QStringLiteral(" | code=%1").arg(errorCodeToString(result.code()));
    }
    if (!result.offender().isEmpty()) {
        line += QStringLiteral(" | offender=%1").arg(result.offender());
    }
    if (!result.details().isEmpty()) {
        line += QStringLiteral(" | details=%1").arg(result.details());
    }

    qDebug().noquote() << line;
}

