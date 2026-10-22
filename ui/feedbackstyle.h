#ifndef FEEDBACKSTYLE_H
#define FEEDBACKSTYLE_H

#include <QColor>
#include <QString>

enum class UiFeedbackLevel {
    Error,
    Warning,
    Success,
};

namespace UiFeedbackStyle {

inline QColor background(UiFeedbackLevel level) {
    switch (level) {
    case UiFeedbackLevel::Error: return QColor(QStringLiteral("#FDECEC"));
    case UiFeedbackLevel::Warning: return QColor(QStringLiteral("#FFF6D8"));
    case UiFeedbackLevel::Success: return QColor(QStringLiteral("#E8F5E9"));
    }
    return QColor();
}

inline QColor foreground(UiFeedbackLevel level) {
    switch (level) {
    case UiFeedbackLevel::Error: return QColor(QStringLiteral("#8A1C13"));
    case UiFeedbackLevel::Warning: return QColor(QStringLiteral("#714B00"));
    case UiFeedbackLevel::Success: return QColor(QStringLiteral("#1B5E20"));
    }
    return QColor();
}

inline QColor border(UiFeedbackLevel level) {
    switch (level) {
    case UiFeedbackLevel::Error: return QColor(QStringLiteral("#D92D20"));
    case UiFeedbackLevel::Warning: return QColor(QStringLiteral("#D99A00"));
    case UiFeedbackLevel::Success: return QColor(QStringLiteral("#2E7D32"));
    }
    return QColor();
}

inline QString fieldStyle(UiFeedbackLevel level, int alpha = 255) {
    QColor fill = background(level);
    QColor outline = border(level);
    fill.setAlpha(alpha);
    outline.setAlpha(alpha);
    return QStringLiteral(
        "background-color: rgba(%1, %2, %3, %4); "
        "border: 1px solid rgba(%5, %6, %7, %8); color: %9;")
        .arg(fill.red()).arg(fill.green()).arg(fill.blue()).arg(fill.alpha())
        .arg(outline.red()).arg(outline.green()).arg(outline.blue()).arg(outline.alpha())
        .arg(foreground(level).name());
}

inline QString bannerStyle(UiFeedbackLevel level) {
    return QStringLiteral(
        "background-color: %1; color: %2; border: 1px solid %3; "
        "border-radius: 3px; padding: 5px 8px;")
        .arg(background(level).name(), foreground(level).name(), border(level).name());
}

} // namespace UiFeedbackStyle

#endif // FEEDBACKSTYLE_H
