#pragma once

#include <QObject>
#include <QColor>
#include <QString>
#include <QMap>
#include <QFileSystemWatcher>
#include <QStringList>

namespace c11 {

// A parsed c11 theme: chrome colors, sidebar, tab bar, divider, etc.
struct C11Theme {
    QString name;
    QString sourcePath;

    // Chrome colors
    QColor sidebarBackground;
    QColor tabBarBackground;
    QColor tabBarText;
    QColor tabBarActiveBackground;
    QColor dividerColor;
    QColor statusBarBackground;
    QColor statusBarText;

    // Window
    QColor windowBackground;
    double windowOpacity = 1.0;

    bool isValid() const { return !name.isEmpty(); }
};

// Loads and manages TOML-format themes with live reload.
class ThemeManager : public QObject {
    Q_OBJECT

public:
    static ThemeManager &instance();

    // Load themes from directories
    void loadThemes();

    // Active theme
    const C11Theme &activeTheme() const { return m_active; }
    void setActiveTheme(const QString &name);
    void clearActiveTheme();

    // Available themes
    QStringList availableThemeNames() const;
    const C11Theme *theme(const QString &name) const;

    // Theme directories
    QStringList themePaths() const;

    // Generate Qt stylesheet from theme
    QString generateStylesheet(const C11Theme &theme) const;

signals:
    void themeChanged(const C11Theme &theme);
    void themesReloaded();

private:
    ThemeManager();
    void watchDirectories();
    void onFileChanged(const QString &path);

    C11Theme parseTomlTheme(const QString &filePath) const;
    static QColor parseColor(const QString &value);

    C11Theme m_active;
    QMap<QString, C11Theme> m_themes;
    QFileSystemWatcher *m_watcher;
};

} // namespace c11
