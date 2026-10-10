#ifndef TRANSLATIONMANAGER_H
#define TRANSLATIONMANAGER_H

#include "core/result.h"

#include <QObject>
#include <QTranslator>

class TranslationManager : public QObject
{
    Q_OBJECT
public:
    explicit TranslationManager(QObject* parent = nullptr);
    ~TranslationManager() override;

    QStringList availableLocales() const;

    QString currentLocale() const { return m_currentLocale; }

    Result setLocale(const QString& locale);

    void loadSystemLocale();

signals:
    void languageChanged(const QString& locale);

private:
    Result loadTranslation(const QString& locale);

    std::unique_ptr<QTranslator> m_appTranslator;
    std::unique_ptr<QTranslator> m_qtTranslator;
    QString m_currentLocale;
    QStringList m_availableLocales;
};

#endif // TRANSLATIONMANAGER_H
