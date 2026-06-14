#include "KeyboardShortcutSettings.h"
#include "platform/PlatformAbstraction.h"

#include <QFile>
#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>

namespace c11 {

KeyboardShortcutSettings &KeyboardShortcutSettings::instance()
{
    static KeyboardShortcutSettings settings;
    return settings;
}

KeyboardShortcutSettings::KeyboardShortcutSettings()
{
    m_shortcuts = defaultShortcuts();
    load();
}

QKeySequence KeyboardShortcutSettings::shortcut(const QString &action) const
{
    return m_shortcuts.value(action);
}

void KeyboardShortcutSettings::setShortcut(const QString &action, const QKeySequence &seq)
{
    m_shortcuts[action] = seq;
    save();
    emit shortcutsChanged();
}

void KeyboardShortcutSettings::resetToDefaults()
{
    m_shortcuts = defaultShortcuts();
    save();
    emit shortcutsChanged();
}

QMap<QString, QKeySequence> KeyboardShortcutSettings::defaultShortcuts() const
{
    return {
        {"new_workspace",      QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_T)},
        {"close_workspace",    QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_W)},
        {"next_workspace",     QKeySequence(Qt::CTRL | Qt::Key_Tab)},
        {"prev_workspace",     QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_Tab)},
        {"toggle_sidebar",     QKeySequence(Qt::CTRL | Qt::Key_B)},
        {"split_right",        QKeySequence(Qt::CTRL | Qt::Key_Backslash)},
        {"split_down",         QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_Backslash)},
        {"open_browser",       QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_B)},
        {"open_markdown",      QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_M)},
        {"find",               QKeySequence::Find},
        {"copy",               QKeySequence::Copy},
        {"paste",              QKeySequence::Paste},
        {"reload_config",      QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_Comma)},
        {"quit",               QKeySequence::Quit},
    };
}

void KeyboardShortcutSettings::load()
{
    QString path = platform::configDir() + "/shortcuts.json";
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) return;

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    if (!doc.isObject()) return;

    QJsonObject obj = doc.object();
    for (auto it = obj.begin(); it != obj.end(); ++it) {
        m_shortcuts[it.key()] = QKeySequence(it.value().toString());
    }
}

void KeyboardShortcutSettings::save() const
{
    QString dir = platform::configDir();
    QDir().mkpath(dir);

    QJsonObject obj;
    for (auto it = m_shortcuts.begin(); it != m_shortcuts.end(); ++it) {
        obj[it.key()] = it.value().toString();
    }

    QFile file(dir + "/shortcuts.json");
    if (file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        file.write(QJsonDocument(obj).toJson(QJsonDocument::Indented));
    }
}

} // namespace c11
