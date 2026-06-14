#include "PlatformAbstraction.h"

#include <QDir>
#include <QProcess>
#include <QStandardPaths>
#include <QGuiApplication>
#include <QStyleHints>

#ifdef Q_OS_WIN
#include "platform_windows.h"
#endif

#ifdef Q_OS_LINUX
#include <QDBusInterface>
#include <QDBusReply>
#include <fstream>
#include <string>
#include <dirent.h>
#include <unistd.h>
#include <signal.h>
#endif

namespace c11::platform {

QString socketPath()
{
    QByteArray envPath = qgetenv("C11_SOCKET");
    if (envPath.isEmpty()) envPath = qgetenv("CMUX_SOCKET");
    if (!envPath.isEmpty()) return QString::fromUtf8(envPath);

#ifdef Q_OS_MACOS
    return appDataDir() + "/c11.sock";
#elif defined(Q_OS_LINUX)
    QString runtimeDir = QStandardPaths::writableLocation(QStandardPaths::RuntimeLocation);
    if (runtimeDir.isEmpty()) {
        // $XDG_RUNTIME_DIR fallback
        runtimeDir = qEnvironmentVariable("XDG_RUNTIME_DIR");
    }
    if (runtimeDir.isEmpty()) runtimeDir = "/tmp";
    return runtimeDir + "/c11.sock";
#elif defined(Q_OS_WIN)
    return "\\\\.\\pipe\\c11-" + QString::fromLocal8Bit(qgetenv("USERNAME"));
#else
    return "/tmp/c11.sock";
#endif
}

QString appDataDir()
{
#ifdef Q_OS_MACOS
    return QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation)
           + "/c11";
#elif defined(Q_OS_LINUX)
    // XDG_DATA_HOME/c11 (defaults to ~/.local/share/c11)
    return QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation)
           + "/c11";
#elif defined(Q_OS_WIN)
    return QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
#else
    return QDir::homePath() + "/.c11";
#endif
}

QString configDir()
{
#ifdef Q_OS_MACOS
    return QDir::homePath() + "/Library/Application Support/com.stage11.c11";
#else
    // XDG_CONFIG_HOME/c11 (defaults to ~/.config/c11)
    return QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation) + "/c11";
#endif
}

QList<qint64> findProcessesByName(const QString &name)
{
    QList<qint64> pids;

#ifdef Q_OS_LINUX
    // Scan /proc for matching processes
    DIR *procDir = opendir("/proc");
    if (!procDir) return pids;

    struct dirent *entry;
    while ((entry = readdir(procDir)) != nullptr) {
        // Only numeric directories (PIDs)
        bool isNum = true;
        for (const char *p = entry->d_name; *p; ++p) {
            if (*p < '0' || *p > '9') { isNum = false; break; }
        }
        if (!isNum) continue;

        qint64 pid = QString::fromUtf8(entry->d_name).toLongLong();

        // Read /proc/<pid>/comm
        std::string commPath = std::string("/proc/") + entry->d_name + "/comm";
        std::ifstream commFile(commPath);
        if (!commFile) continue;

        std::string comm;
        std::getline(commFile, comm);
        if (QString::fromStdString(comm).contains(name, Qt::CaseInsensitive)) {
            pids.append(pid);
        }
    }
    closedir(procDir);
#elif defined(Q_OS_WIN)
    return win::findProcessesByName(name);
#elif defined(Q_OS_MACOS) || defined(Q_OS_UNIX)
    QProcess ps;
    ps.start("pgrep", {"-f", name});
    if (ps.waitForFinished(3000)) {
        for (const auto &line : ps.readAllStandardOutput().split('\n')) {
            QString trimmed = QString::fromUtf8(line).trimmed();
            if (!trimmed.isEmpty()) {
                bool ok;
                qint64 pid = trimmed.toLongLong(&ok);
                if (ok) pids.append(pid);
            }
        }
    }
#else
    Q_UNUSED(name);
#endif

    return pids;
}

void showNotification(const QString &title, const QString &body)
{
#ifdef Q_OS_LINUX
    // Use libnotify via D-Bus (org.freedesktop.Notifications)
    QDBusInterface iface("org.freedesktop.Notifications",
                          "/org/freedesktop/Notifications",
                          "org.freedesktop.Notifications");
    if (iface.isValid()) {
        iface.call("Notify",
                    "c11",            // app_name
                    uint(0),          // replaces_id
                    "terminal",       // app_icon
                    title,            // summary
                    body,             // body
                    QStringList(),    // actions
                    QVariantMap(),    // hints
                    int(5000));       // timeout_ms
    }
#elif defined(Q_OS_WIN)
    win::showToastNotification(title, body);
#elif defined(Q_OS_MACOS)
    Q_UNUSED(title);
    Q_UNUSED(body);
#else
    Q_UNUSED(title);
    Q_UNUSED(body);
#endif
}

bool isDarkMode()
{
    auto *hints = QGuiApplication::styleHints();
    if (hints) {
        return hints->colorScheme() == Qt::ColorScheme::Dark;
    }

#ifdef Q_OS_WIN
    return win::isSystemDarkMode();
#elif defined(Q_OS_LINUX)
    QProcess gsettings;
    gsettings.start("gsettings", {"get", "org.gnome.desktop.interface", "color-scheme"});
    if (gsettings.waitForFinished(1000)) {
        QString output = QString::fromUtf8(gsettings.readAllStandardOutput()).trimmed();
        return output.contains("dark", Qt::CaseInsensitive);
    }
#endif

    return false;
}

} // namespace c11::platform
