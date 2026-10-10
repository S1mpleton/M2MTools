#include "translationmanager.h"

#include <QCoreApplication>
#include <QLocale>
#include <QDir>
#include <QDebug>
#include <QLibraryInfo>

namespace {
    constexpr auto kTranslationPrefix = "M2MTools_";
    constexpr auto kTranslationsPath  = ":/i18n/";
}

TranslationManager::TranslationManager(QObject* parent)
    : QObject(parent)
{
    m_availableLocales = {
        "en",
        "ru_RU",
    };
}

TranslationManager::~TranslationManager() {
}

QStringList TranslationManager::availableLocales() const {
    return m_availableLocales;
}

Result TranslationManager::setLocale(const QString& locale) {
    if (locale == m_currentLocale) {
        return Result::success(
            QStringLiteral("Language '%1' is already active").arg(locale));
    }

    if (locale == "en") {
        if (m_appTranslator) {
            QCoreApplication::removeTranslator(m_appTranslator.get());
            m_appTranslator.reset();
        }
        if (m_qtTranslator) {
            QCoreApplication::removeTranslator(m_qtTranslator.get());
            m_qtTranslator.reset();
        }
        m_currentLocale = locale;
        emit languageChanged(locale);
        return Result::success(
            QStringLiteral("Language changed to '%1'").arg(locale));
    }

    if (!m_availableLocales.contains(locale)) {
        return Result::error(
                   QStringLiteral("Locale '%1' is not supported").arg(locale),
                   ResultCategory::Translation)
            .withOffender(locale);
    }

    Result loadResult = loadTranslation(locale);

    if (!loadResult.ok())
        return loadResult;

    m_currentLocale = locale;
    emit languageChanged(locale);

    return Result::success(
               QStringLiteral("Language changed to '%1'").arg(locale))
        .withPayload(locale);
}

void TranslationManager::loadSystemLocale() {
    const QString systemLocale = QLocale::system().name();

    if (m_availableLocales.contains(systemLocale)) {

        setLocale(systemLocale);
    } else {
        setLocale("en");
    }
}

Result TranslationManager::loadTranslation(const QString& locale) {
    if (m_appTranslator) {
        QCoreApplication::removeTranslator(m_appTranslator.get());
        m_appTranslator.reset();
    }
    if (m_qtTranslator) {
        QCoreApplication::removeTranslator(m_qtTranslator.get());
        m_qtTranslator.reset();
    }

    auto appTranslator = std::make_unique<QTranslator>();
    const QString appPath = QStringLiteral("%1/%2%3")
                                .arg(kTranslationsPath, kTranslationPrefix, locale);

    if (!appTranslator->load(appPath)) {
        return Result::error(
                   QStringLiteral("Failed to load translation file for '%1'")
                       .arg(locale),
                   ResultCategory::Translation)
            .withDetails(QStringLiteral("Path: %1").arg(appPath))
            .withOffender(locale);
    }
    QCoreApplication::installTranslator(appTranslator.get());
    m_appTranslator = std::move(appTranslator);

    auto qtTranslator = std::make_unique<QTranslator>();
    const QString qtPath = QLibraryInfo::path(QLibraryInfo::TranslationsPath);
    if (qtTranslator->load(QStringLiteral("qtbase_%1").arg(locale), qtPath)) {
        QCoreApplication::installTranslator(qtTranslator.get());
        m_qtTranslator = std::move(qtTranslator);
    }

    return Result::success()
        .withDetails(QStringLiteral("Loaded translation for '%1'").arg(locale));
}