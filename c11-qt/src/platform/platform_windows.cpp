#include "platform_windows.h"

#ifdef Q_OS_WIN

#include <QDebug>
#include <QSettings>
#include <QProcess>

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
    // Show a tray balloon via PowerShell + WinForms NotifyIcon. This works on
    // all supported Windows versions without registering an AppUserModelID.
    // A future enhancement could use WinRT's ToastNotificationManager.
    auto sanitize = [](QString s) {
        s.replace('\'', QStringLiteral("''")); // escape PowerShell single quotes
        s.remove('\r');
        s.replace('\n', ' ');
        return s;
    };

    const QString script =
        QStringLiteral(
            "Add-Type -AssemblyName System.Windows.Forms;"
            "Add-Type -AssemblyName System.Drawing;"
            "$n = New-Object System.Windows.Forms.NotifyIcon;"
            "$n.Icon = [System.Drawing.SystemIcons]::Information;"
            "$n.BalloonTipTitle = '%1';"
            "$n.BalloonTipText = '%2';"
            "$n.Visible = $true;"
            "$n.ShowBalloonTip(5000);"
            "Start-Sleep -Seconds 6;"
            "$n.Dispose()")
            .arg(sanitize(title), sanitize(body));

    QProcess::startDetached(QStringLiteral("powershell"),
                            {QStringLiteral("-NoProfile"),
                             QStringLiteral("-WindowStyle"), QStringLiteral("Hidden"),
                             QStringLiteral("-Command"), script});
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
