#include "PortScanner.h"

#include <QProcess>
#include <QRegularExpression>

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
#endif

    return result;
}

} // namespace c11
