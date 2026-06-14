#pragma once

// Windows-specific platform utilities.
// Only compiled on Windows (guarded by Q_OS_WIN).

#ifdef Q_OS_WIN

#include <QString>
#include <QList>
#include <cstdint>

namespace c11::platform::win {

// Process scanning via CreateToolhelp32Snapshot
QList<qint64> findProcessesByName(const QString &name);

// Get the current user's SID for named pipe security
QString currentUserSid();

// Check if running on Windows 10 1903+ (ConPTY support)
bool hasConPtySupport();

// Show a Windows toast notification
void showToastNotification(const QString &title, const QString &body);

// Windows dark mode detection via registry
bool isSystemDarkMode();

} // namespace c11::platform::win

#endif // Q_OS_WIN
