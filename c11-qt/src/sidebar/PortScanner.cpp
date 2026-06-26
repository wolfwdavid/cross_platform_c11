#include "PortScanner.h"

#include <QProcess>
#include <QRegularExpression>

#ifdef Q_OS_WIN
#include <QHash>
#include <QSet>
#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>   // AF_INET / AF_INET6 (must precede iphlpapi.h)
#include <ws2tcpip.h>
#include <windows.h>
#include <iphlpapi.h>
#include <tlhelp32.h>
#endif

namespace c11 {

PortScanner &PortScanner::instance()
{
    static PortScanner scanner;
    return scanner;
}

PortScanner::PortScanner()
{
    connect(&m_timer, &QTimer::timeout, this, &PortScanner::runScan);
}

void PortScanner::startScanning(int intervalMs)
{
    m_timer.start(intervalMs);
    runScan();
}

void PortScanner::stopScanning()
{
    m_timer.stop();
}

void PortScanner::kick()
{
    QTimer::singleShot(500, this, &PortScanner::runScan);
}

QList<PortEntry> PortScanner::portsForPid(qint64 pid) const
{
    QList<PortEntry> result;
    for (const auto &entry : m_ports) {
        if (entry.pid == pid) result.append(entry);
    }
    return result;
}

QList<PortEntry> PortScanner::allPorts() const
{
    return m_ports;
}

void PortScanner::runScan()
{
    auto newPorts = scanListeningPorts();
    if (newPorts != m_ports) {
        m_ports = newPorts;
        emit portsChanged();
    }
}

QList<PortEntry> PortScanner::scanListeningPorts()
{
    QList<PortEntry> result;

#if defined(Q_OS_MACOS) || defined(Q_OS_LINUX)
    QProcess lsof;
    lsof.start("lsof", {"-iTCP", "-sTCP:LISTEN", "-nP", "-Fn"});
    if (!lsof.waitForFinished(5000)) return result;

    QString output = QString::fromUtf8(lsof.readAllStandardOutput());
    PortEntry current;
    current.pid = 0;

    for (const auto &line : output.split('\n')) {
        if (line.startsWith('p')) {
            current.pid = line.mid(1).toLongLong();
        } else if (line.startsWith('c')) {
            current.processName = line.mid(1);
        } else if (line.startsWith('n')) {
            // e.g., "n*:8080" or "n127.0.0.1:3000"
            QString addr = line.mid(1);
            int colonIdx = addr.lastIndexOf(':');
            if (colonIdx >= 0) {
                bool ok;
                int port = addr.mid(colonIdx + 1).toInt(&ok);
                if (ok && current.pid > 0) {
                    PortEntry entry;
                    entry.port = port;
                    entry.protocol = "tcp";
                    entry.pid = current.pid;
                    entry.processName = current.processName;
                    result.append(entry);
                }
            }
        }
    }
#elif defined(Q_OS_WIN)
    // Map pid -> executable name once via Toolhelp32.
    QHash<DWORD, QString> nameByPid;
    if (HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        snap != INVALID_HANDLE_VALUE) {
        PROCESSENTRY32W pe;
        pe.dwSize = sizeof(pe);
        if (Process32FirstW(snap, &pe)) {
            do {
                nameByPid.insert(pe.th32ProcessID, QString::fromWCharArray(pe.szExeFile));
            } while (Process32NextW(snap, &pe));
        }
        CloseHandle(snap);
    }

    // dwLocalPort is the port in network byte order in the low 16 bits.
    auto netPort = [](DWORD v) -> int {
        unsigned p = v & 0xFFFFu;
        return static_cast<int>(((p & 0xFFu) << 8) | ((p >> 8) & 0xFFu));
    };

    QSet<QString> seen;
    auto addRow = [&](int port, DWORD pid) {
        const QString key = QStringLiteral("%1/%2").arg(port).arg(pid);
        if (seen.contains(key)) return;
        seen.insert(key);
        PortEntry entry;
        entry.port = port;
        entry.protocol = "tcp";
        entry.pid = static_cast<qint64>(pid);
        entry.processName = nameByPid.value(pid);
        result.append(entry);
    };

    // IPv4 listeners.
    {
        DWORD size = 0;
        GetExtendedTcpTable(nullptr, &size, TRUE, AF_INET, TCP_TABLE_OWNER_PID_LISTENER, 0);
        QByteArray buf(static_cast<int>(size), '\0');
        if (size > 0 && GetExtendedTcpTable(buf.data(), &size, TRUE, AF_INET,
                                            TCP_TABLE_OWNER_PID_LISTENER, 0) == NO_ERROR) {
            auto *t = reinterpret_cast<const MIB_TCPTABLE_OWNER_PID *>(buf.constData());
            for (DWORD i = 0; i < t->dwNumEntries; ++i)
                addRow(netPort(t->table[i].dwLocalPort), t->table[i].dwOwningPid);
        }
    }

    // IPv6 listeners.
    {
        DWORD size = 0;
        GetExtendedTcpTable(nullptr, &size, TRUE, AF_INET6, TCP_TABLE_OWNER_PID_LISTENER, 0);
        QByteArray buf(static_cast<int>(size), '\0');
        if (size > 0 && GetExtendedTcpTable(buf.data(), &size, TRUE, AF_INET6,
                                            TCP_TABLE_OWNER_PID_LISTENER, 0) == NO_ERROR) {
            auto *t = reinterpret_cast<const MIB_TCP6TABLE_OWNER_PID *>(buf.constData());
            for (DWORD i = 0; i < t->dwNumEntries; ++i)
                addRow(netPort(t->table[i].dwLocalPort), t->table[i].dwOwningPid);
        }
    }
#endif

    return result;
}

} // namespace c11
