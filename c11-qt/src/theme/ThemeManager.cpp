#include "ThemeManager.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QStandardPaths>
#include <QTextStream>
#include <QRegularExpression>
#include <QDebug>

namespace c11 {

ThemeManager &ThemeManager::instance()
{
    static ThemeManager mgr;
    return mgr;
}

ThemeManager::ThemeManager()
    : m_watcher(new QFileSystemWatcher(this))
{
    connect(m_watcher, &QFileSystemWatcher::fileChanged,
            this, &ThemeManager::onFileChanged);
    connect(m_watcher, &QFileSystemWatcher::directoryChanged,
            this, [this](const QString &) { loadThemes(); });
}

void ThemeManager::loadThemes()
{
    m_themes.clear();

    for (const auto &dir : themePaths()) {
        QDir themeDir(dir);
        if (!themeDir.exists()) continue;

        for (const auto &entry : themeDir.entryInfoList({"*.toml", "*.theme"}, QDir::Files)) {
            C11Theme theme = parseTomlTheme(entry.absoluteFilePath());
            if (theme.isValid()) {
                m_themes[theme.name] = theme;
            }
        }
    }

    watchDirectories();
    emit themesReloaded();
}

void ThemeManager::setActiveTheme(const QString &name)
{
    auto it = m_themes.find(name);
    if (it != m_themes.end()) {
        m_active = *it;
        emit themeChanged(m_active);
    }
}

void ThemeManager::clearActiveTheme()
{
    m_active = C11Theme{};
    emit themeChanged(m_active);
}

QStringList ThemeManager::availableThemeNames() const
{
    return m_themes.keys();
}

const C11Theme *ThemeManager::theme(const QString &name) const
{
    auto it = m_themes.find(name);
    return it != m_themes.end() ? &(*it) : nullptr;
}

QStringList ThemeManager::themePaths() const
{
    QStringList paths;
    QString home = QDir::homePath();

    // User themes
    paths << home + "/.config/c11/themes";

#ifdef Q_OS_MACOS
    paths << home + "/Library/Application Support/c11/themes";
#endif

    // Built-in themes (bundled with app)
    QString appDir = QCoreApplication::applicationDirPath();
    paths << appDir + "/../Resources/themes";
    paths << appDir + "/themes";

    return paths;
}

void ThemeManager::watchDirectories()
{
    // Clear existing watches
    auto dirs = m_watcher->directories();
    if (!dirs.isEmpty()) m_watcher->removePaths(dirs);
    auto files = m_watcher->files();
    if (!files.isEmpty()) m_watcher->removePaths(files);

    for (const auto &dir : themePaths()) {
        if (QDir(dir).exists()) {
            m_watcher->addPath(dir);
        }
    }

    // Watch individual theme files
    for (const auto &theme : m_themes) {
        if (!theme.sourcePath.isEmpty()) {
            m_watcher->addPath(theme.sourcePath);
        }
    }
}

void ThemeManager::onFileChanged(const QString &path)
{
    // Re-parse the changed theme file
    C11Theme theme = parseTomlTheme(path);
    if (theme.isValid()) {
        m_themes[theme.name] = theme;
        if (m_active.sourcePath == path) {
            m_active = theme;
            emit themeChanged(m_active);
        }
    }

    // Re-watch (file might have been replaced via atomic save)
    if (!m_watcher->files().contains(path) && QFile::exists(path)) {
        m_watcher->addPath(path);
    }
}

C11Theme ThemeManager::parseTomlTheme(const QString &filePath) const
{
    C11Theme theme;
    theme.sourcePath = filePath;

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) return theme;

    // Derive name from filename
    QFileInfo fi(filePath);
    theme.name = fi.baseName();

    QString currentSection;
    QTextStream stream(&file);
    while (!stream.atEnd()) {
        QString line = stream.readLine().trimmed();
        if (line.isEmpty() || line.startsWith('#')) continue;

        // Section header
        if (line.startsWith('[') && line.endsWith(']')) {
            currentSection = line.mid(1, line.size() - 2).trimmed();
            continue;
        }

        // Key = value
        int eq = line.indexOf('=');
        if (eq < 0) continue;

        QString key = line.left(eq).trimmed();
        QString value = line.mid(eq + 1).trimmed();
        // Strip quotes
        if ((value.startsWith('"') && value.endsWith('"')) ||
            (value.startsWith('\'') && value.endsWith('\''))) {
            value = value.mid(1, value.size() - 2);
        }

        QString fullKey = currentSection.isEmpty() ? key : currentSection + "." + key;

        if (fullKey == "name") theme.name = value;
        else if (fullKey == "sidebar.background") theme.sidebarBackground = parseColor(value);
        else if (fullKey == "tab-bar.background") theme.tabBarBackground = parseColor(value);
        else if (fullKey == "tab-bar.text") theme.tabBarText = parseColor(value);
        else if (fullKey == "tab-bar.active-background") theme.tabBarActiveBackground = parseColor(value);
        else if (fullKey == "divider.color") theme.dividerColor = parseColor(value);
        else if (fullKey == "status-bar.background") theme.statusBarBackground = parseColor(value);
        else if (fullKey == "status-bar.text") theme.statusBarText = parseColor(value);
        else if (fullKey == "window.background") theme.windowBackground = parseColor(value);
        else if (fullKey == "window.opacity") theme.windowOpacity = value.toDouble();
    }

    return theme;
}

QColor ThemeManager::parseColor(const QString &value)
{
    QString v = value.trimmed();
    if (!v.startsWith('#')) v.prepend('#');
    return QColor::fromString(v);
}

QString ThemeManager::generateStylesheet(const C11Theme &theme) const
{
    QString css;

    if (theme.sidebarBackground.isValid()) {
        css += QString("QListWidget { background-color: %1; }\n")
                   .arg(theme.sidebarBackground.name(QColor::HexArgb));
    }
    if (theme.statusBarBackground.isValid()) {
        css += QString("#statusBar { background-color: %1; }\n")
                   .arg(theme.statusBarBackground.name(QColor::HexArgb));
    }
    if (theme.statusBarText.isValid()) {
        css += QString("#statusBar QLabel { color: %1; }\n")
                   .arg(theme.statusBarText.name(QColor::HexArgb));
    }

    return css;
}

} // namespace c11
