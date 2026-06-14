#pragma once

#include <QObject>
#include <QKeySequence>
#include <QMap>
#include <QString>

namespace c11 {

// Manages user-configurable keyboard shortcuts.
// Persists to config dir as JSON. Falls back to built-in defaults.
class KeyboardShortcutSettings : public QObject {
    Q_OBJECT

public:
    static KeyboardShortcutSettings &instance();

    QKeySequence shortcut(const QString &action) const;
    void setShortcut(const QString &action, const QKeySequence &seq);
    void resetToDefaults();

    QMap<QString, QKeySequence> allShortcuts() const { return m_shortcuts; }
    QMap<QString, QKeySequence> defaultShortcuts() const;

    void load();
    void save() const;

signals:
    void shortcutsChanged();

private:
    KeyboardShortcutSettings();

    QMap<QString, QKeySequence> m_shortcuts;
};

} // namespace c11
