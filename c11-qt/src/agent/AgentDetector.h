#pragma once

#include <QObject>
#include <QUuid>
#include <QTimer>
#include <QMap>
#include <QSet>
#include <QString>

namespace c11 {

// Detects agent processes (Claude Code, Codex, etc.) running inside
// terminal panes by scanning the process tree for known agent binaries.
class AgentDetector : public QObject {
    Q_OBJECT

public:
    struct AgentInfo {
        QString type;        // "claude-code", "codex", "kimi", etc.
        QString displayName; // "Claude Code", "Codex", etc.
        qint64 pid = 0;
    };

    static AgentDetector &instance();

    void registerTTY(const QUuid &workspaceId, const QUuid &panelId, const QString &ttyName);
    void unregister(const QUuid &workspaceId, const QUuid &panelId);
    void kick(const QUuid &workspaceId, const QUuid &panelId);

    AgentInfo agentForPanel(const QUuid &panelId) const;
    bool hasAgent(const QUuid &panelId) const;

signals:
    void agentDetected(const QUuid &panelId, const AgentInfo &info);
    void agentRemoved(const QUuid &panelId);

private:
    AgentDetector();

    struct PanelKey {
        QUuid workspaceId;
        QUuid panelId;
        bool operator==(const PanelKey &o) const { return workspaceId == o.workspaceId && panelId == o.panelId; }
    };

    void runScan();
    static QList<AgentInfo> scanProcesses(const QString &ttyName);
    static QString agentTypeFromBinary(const QString &binary);

    QTimer m_sweepTimer;
    QMap<QUuid, QString> m_ttyNames;        // panelId -> ttyName
    QMap<QUuid, AgentInfo> m_detectedAgents; // panelId -> detected agent
    QSet<QUuid> m_pendingKicks;
};

} // namespace c11
