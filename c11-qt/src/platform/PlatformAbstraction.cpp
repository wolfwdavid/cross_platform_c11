#include "PlatformAbstraction.h"

#include <QDir>
#include <QStandardPaths>
#include <QGuiApplication>
#include <QStyleHints>

#ifdef Q_OS_MACOS
#include <cstdlib>
#endif

namespace c11::platform {

QString socketPath()
{
    // Check env overrides first
    QByteArray envPath = qgetenv("C11_SOCKET");
    if (envPath.isEmpty()) envPath = qgetenv("CMUX_SOCKET");
    if (!envPath.isEmpty()) return QString::fromUtf8(envPath);

#ifdef Q_OS_MACOS
    return appDataDir() + "/c11.sock";
#elif defined(Q_OS_LINUX)
    QString runtimeDir = QStandardPaths::writableLocation(QStandardPaths::RuntimeLocation);
    if (runtimeDir.isEmpty()) runtimeDir = "/tmp";
    return runtimeDir + "/c11.sock";
#elif defined(Q_OS_WIN)
    // Windows uses named pipes, not file paths
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
    return QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation) + "/c11";
#endif
}

QList<qint64> findProcessesByName(const QString &name)
{
    Q_UNUSED(name);
    // Phase 4 implements full process scanning
    return {};
}

void showNotification(const QString &title, const QString &body)
{
    Q_UNUSED(title);
    Q_UNUSED(body);
    // Phase 4 implements desktop notifications
}

bool isDarkMode()
{
    auto *hints = QGuiApplication::styleHints();
    return hints && hints->colorScheme() == Qt::ColorScheme::Dark;
}

} // namespace c11::platform
