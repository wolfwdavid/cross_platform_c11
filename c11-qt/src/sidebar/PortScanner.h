#pragma once

#include <QObject>
#include <QTimer>
#include <QMap>
#include <QList>
#include <QUuid>

namespace c11 {

struct PortEntry {
    int port;
    QString protocol; // "tcp" or "udp"
    qint64 pid;
    QString processName;

    bool operator==(const PortEntry &o) const {
        return port == o.port && pid == o.pid && protocol == o.protocol;
    }
};

// Scans for listening ports associated with workspace terminal processes.
class PortScanner : public QObject {
    Q_OBJECT

public:
    static PortScanner &instance();

    void startScanning(int intervalMs = 10000);
    void stopScanning();
    void kick();

    QList<PortEntry> portsForPid(qint64 pid) const;
    QList<PortEntry> allPorts() const;

signals:
    void portsChanged();

private:
    PortScanner();
    void runScan();
    static QList<PortEntry> scanListeningPorts();

    QTimer m_timer;
    QList<PortEntry> m_ports;
};

} // namespace c11
