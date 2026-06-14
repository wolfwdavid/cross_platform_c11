#include "platform_windows.h"

#ifdef Q_OS_WIN

#include <QDebug>
#include <QSettings>

#include <windows.h>
#include <tlhelp32.h>
#include <sddl.h>
#include <versionhelpers.h>

namespace c11::platform::win {

QList<qint64> findProcessesByName(const QString &name)
{
    QList<qint64> pids;

    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot == INVALID_HANDLE_VALUE) return pids;

    PROCESSENTRY32W pe;
    pe.dwSize = sizeof(pe);

    if (Process32FirstW(snapshot, &pe)) {
        do {
            QString exeName = QString::fromWCharArray(pe.szExeFile);
            if (exeName.contains(name, Qt::CaseInsensitive)) {
                pids.append(static_cast<qint64>(pe.th32ProcessID));
            }
        } while (Process32NextW(snapshot, &pe));
    }

    CloseHandle(snapshot);
    return pids;
}

QString currentUserSid()
{
    HANDLE token = nullptr;
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &token)) {
        return {};
    }

    DWORD size = 0;
    GetTokenInformation(token, TokenUser, nullptr, 0, &size);
    if (size == 0) {
        CloseHandle(token);
        return {};
    }

    QByteArray buffer(static_cast<int>(size), '\0');
    auto *tokenUser = reinterpret_cast<TOKEN_USER *>(buffer.data());
    if (!GetTokenInformation(token, TokenUser, tokenUser, size, &size)) {
        CloseHandle(token);
        return {};
    }

    LPWSTR sidString = nullptr;
    QString result;
    if (ConvertSidToStringSidW(tokenUser->User.Sid, &sidString)) {
        result = QString::fromWCharArray(sidString);
        LocalFree(sidString);
    }

    CloseHandle(token);
    return result;
}

bool hasConPtySupport()
{
    // ConPTY requires Windows 10 1903 (build 18362) or later
    return IsWindows10OrGreater();
}

void showToastNotification(const QString &title, const QString &body)
{
    // Use PowerShell for toast notifications as a simple cross-version approach.
    // A production implementation would use WinRT's ToastNotificationManager.
    Q_UNUSED(title);
    Q_UNUSED(body);
    // TODO: Implement via WinRT ToastNotificationManager
}

bool isSystemDarkMode()
{
    QSettings reg(
        "HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize",
        QSettings::NativeFormat);
    // AppsUseLightTheme: 0 = dark, 1 = light
    return reg.value("AppsUseLightTheme", 1).toInt() == 0;
}

} // namespace c11::platform::win

#endif // Q_OS_WIN
