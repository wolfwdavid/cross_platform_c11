#include "TranslationManager.h"

#include <QApplication>
#include <QDir>
#include <QLocale>
#include <QDebug>

namespace c11 {

TranslationManager &TranslationManager::instance()
{
    static TranslationManager mgr;
    return mgr;
}

TranslationManager::TranslationManager()
    : m_translator(new QTranslator(this))
{
}

QStringList TranslationManager::availableLocales() const
{
    return {"en", "ja", "uk", "ko", "zh_Hans", "zh_Hant", "ru"};
}

bool TranslationManager::setLocale(const QString &locale)
{
    QString resolved = locale;
    if (resolved.isEmpty()) {
        resolved = QLocale::system().name(); // e.g., "ja_JP"
        // Map to our supported locales
        if (resolved.startsWith("ja")) resolved = "ja";
        else if (resolved.startsWith("uk")) resolved = "uk";
        else if (resolved.startsWith("ko")) resolved = "ko";
        else if (resolved.startsWith("zh_Hant") || resolved.startsWith("zh_TW")
                 || resolved.startsWith("zh_HK")) resolved = "zh_Hant";
        else if (resolved.startsWith("zh")) resolved = "zh_Hans";
        else if (resolved.startsWith("ru")) resolved = "ru";
        else resolved = "en";
    }

    if (resolved == m_currentLocale) return true;

    // Remove previous translator
    QApplication::removeTranslator(m_translator);

    if (resolved == "en") {
        // English is the source language, no .qm needed
        m_currentLocale = resolved;
        emit localeChanged(resolved);
        return true;
    }

    // Try loading from multiple paths
    QStringList searchPaths = {
        QApplication::applicationDirPath() + "/../Resources/translations",
        QApplication::applicationDirPath() + "/translations",
        ":/translations",
    };

    QString filename = "c11_" + resolved;
    for (const auto &path : searchPaths) {
        if (m_translator->load(filename, path)) {
            QApplication::installTranslator(m_translator);
            m_currentLocale = resolved;
            emit localeChanged(resolved);
            return true;
        }
    }

    qWarning() << "TranslationManager: failed to load translations for" << resolved;
    m_currentLocale = "en";
    emit localeChanged("en");
    return false;
}

QString TranslationManager::localeDisplayName(const QString &locale)
{
    static const QMap<QString, QString> names = {
        {"en",      "English"},
        {"ja",      "\u65E5\u672C\u8A9E"},      // 日本語
        {"uk",      "\u0423\u043A\u0440\u0430\u0457\u043D\u0441\u044C\u043A\u0430"}, // Українська
        {"ko",      "\uD55C\uAD6D\uC5B4"},       // 한국어
        {"zh_Hans", "\u7B80\u4F53\u4E2D\u6587"},  // 简体中文
        {"zh_Hant", "\u7E41\u9AD4\u4E2D\u6587"},  // 繁體中文
        {"ru",      "\u0420\u0443\u0441\u0441\u043A\u0438\u0439"}, // Русский
    };
    return names.value(locale, locale);
}

} // namespace c11
