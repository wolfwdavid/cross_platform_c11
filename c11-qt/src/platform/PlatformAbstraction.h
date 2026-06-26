#pragma once

#include <QString>
#include <QStringList>

namespace c11::platform {

// Returns the OS-appropriate socket path for the c11 daemon.
QString socketPath();

// Returns the command a new terminal pane should spawn when the caller gives no
// explicit command. On Windows this prefers PowerShell (pwsh.exe, else
// powershell.exe) over ghostty's bare cmd.exe default so `ls` and PATH-installed
// tools behave like the macOS login shell. On macOS/Linux it returns an empty
// string, letting ghostty pick the user's login shell / $SHELL.
QString defaultShellCommand();

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
