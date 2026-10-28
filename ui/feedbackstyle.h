#pragma once
#include <QApplication>
#include <QColor>
#include <QPalette>
#include <QString>

enum class UiFeedbackLevel { Error, Warning, Success };

namespace UiFeedbackStyle {
inline bool darkPalette() {
    return QApplication::palette().color(QPalette::Window).lightness() < 128;
}
inline QColor background(UiFeedbackLevel level) {
    const bool dark = darkPalette();
    switch (level) {
    case UiFeedbackLevel::Error:
        return QColor(dark ? "#472e30" : "#f8e9e8");
    case UiFeedbackLevel::Warning:
        return QColor(dark ? "#443e28" : "#fbf3dc");
    case UiFeedbackLevel::Success:
        return QColor(dark ? "#263c30" : "#e9f3ed");
    }
    return {};
}
inline QColor foreground(UiFeedbackLevel level) {
    const bool dark = darkPalette();
    switch (level) {
    case UiFeedbackLevel::Error:
        return QColor(dark ? "#efb7b2" : "#8e3632");
    case UiFeedbackLevel::Warning:
        return QColor(dark ? "#e9d38b" : "#735812");
    case UiFeedbackLevel::Success:
        return QColor(dark ? "#b6dec3" : "#285d3e");
    }
    return {};
}
inline QColor border(UiFeedbackLevel level) {
    const bool dark = darkPalette();
    switch (level) {
    case UiFeedbackLevel::Error:
        return QColor(dark ? "#ae716e" : "#bd7873");
    case UiFeedbackLevel::Warning:
        return QColor(dark ? "#9e884b" : "#b69a50");
    case UiFeedbackLevel::Success:
        return QColor(dark ? "#597d65" : "#769e83");
    }
    return {};
}
inline QString fieldStyle(UiFeedbackLevel level) {
    return QStringLiteral("background-color: %1; color: %2; border: 1px solid %3;")
        .arg(background(level).name(), foreground(level).name(), border(level).name());
}
inline QString bannerStyle(UiFeedbackLevel level) {
    return fieldStyle(level) + "border-radius: 4px; padding: 5px 8px;";
}
} // namespace UiFeedbackStyle
