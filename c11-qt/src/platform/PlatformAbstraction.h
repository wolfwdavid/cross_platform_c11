#pragma once

#include <QString>
#include <QStringList>

namespace c11::platform {

// Returns the OS-appropriate socket path for the c11 daemon.
QString socketPath();

// Returns the application data directory.
QString appDataDir();

// Returns the config directory.
QString configDir();

// Returns PIDs of processes matching a name substring.
QList<qint64> findProcessesByName(const QString &name);

// Shows a desktop notification.
void showNotification(const QString &title, const QString &body);

// Returns whether the system is using a dark color scheme.
bool isDarkMode();

} // namespace c11::platform
