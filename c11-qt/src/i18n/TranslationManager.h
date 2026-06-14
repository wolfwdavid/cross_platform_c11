#pragma once

#include <QObject>
#include <QTranslator>
#include <QString>
#include <QStringList>
#include <QMap>

namespace c11 {

// Manages Qt Linguist translations for 7 languages.
// Loads .qm files from the resources/translations/ directory.
class TranslationManager : public QObject {
    Q_OBJECT

public:
    static TranslationManager &instance();

    // Available locales: en, ja, uk, ko, zh_Hans, zh_Hant, ru
    QStringList availableLocales() const;
    QString currentLocale() const { return m_currentLocale; }

    // Load a locale's translations. Empty string = system default.
    bool setLocale(const QString &locale);

    // Display name for a locale code
    static QString localeDisplayName(const QString &locale);

signals:
    void localeChanged(const QString &locale);

private:
    TranslationManager();

    QString m_currentLocale = "en";
    QTranslator *m_translator = nullptr;
};

} // namespace c11
