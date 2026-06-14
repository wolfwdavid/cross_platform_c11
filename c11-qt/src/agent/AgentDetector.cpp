#include "AgentDetector.h"

#include <QProcess>
#include <QRegularExpression>
#include <QDebug>

namespace c11 {

AgentDetector &AgentDetector::instance()
{
    static AgentDetector detector;
    return detector;
}

AgentDetector::AgentDetector()
{
    m_sweepTimer.setInterval(5000); // Sweep every 5 seconds
    connect(&m_sweepTimer, &QTimer::timeout, this, &AgentDetector::runScan);
}

void AgentDetector::registerTTY(const QUuid &workspaceId, const QUuid &panelId,
                                 const QString &ttyName)
{
    Q_UNUSED(workspaceId);
    m_ttyNames[panelId] = ttyName;
    kick(workspaceId, panelId);
    if (!m_sweepTimer.isActive()) m_sweepTimer.start();
}

void AgentDetector::unregister(const QUuid &workspaceId, const QUuid &panelId)
{
    Q_UNUSED(workspaceId);
    m_ttyNames.remove(panelId);
    if (m_detectedAgents.remove(panelId)) {
        emit agentRemoved(panelId);
    }
    if (m_ttyNames.isEmpty()) m_sweepTimer.stop();
}

void AgentDetector::kick(const QUuid &workspaceId, const QUuid &panelId)
{
    Q_UNUSED(workspaceId);
    m_pendingKicks.insert(panelId);
    QTimer::singleShot(200, this, &AgentDetector::runScan);
}

AgentDetector::AgentInfo AgentDetector::agentForPanel(const QUuid &panelId) const
{
    return m_detectedAgents.value(panelId);
}

bool AgentDetector::hasAgent(const QUuid &panelId) const
{
    return m_detectedAgents.contains(panelId);
}

void AgentDetector::runScan()
{
    QSet<QUuid> toScan = m_pendingKicks;
    m_pendingKicks.clear();

    // If no specific kicks, scan all registered TTYs
    if (toScan.isEmpty()) {
        toScan = QSet<QUuid>(m_ttyNames.keyBegin(), m_ttyNames.keyEnd());
    }

    for (const auto &panelId : toScan) {
        auto ttyIt = m_ttyNames.find(panelId);
        if (ttyIt == m_ttyNames.end()) continue;

        auto agents = scanProcesses(ttyIt.value());
        if (!agents.isEmpty()) {
            auto &best = agents.first();
            bool isNew = !m_detectedAgents.contains(panelId)
                         || m_detectedAgents[panelId].type != best.type;
            m_detectedAgents[panelId] = best;
            if (isNew) emit agentDetected(panelId, best);
        } else if (m_detectedAgents.remove(panelId)) {
            emit agentRemoved(panelId);
        }
    }
}

QList<AgentDetector::AgentInfo> AgentDetector::scanProcesses(const QString &ttyName)
{
    QList<AgentInfo> results;

#ifdef Q_OS_WIN
    Q_UNUSED(ttyName);
    return results;
#else
    // Use ps to find processes on this TTY
    QProcess ps;
    ps.start("ps", {"-o", "pid,comm", "-t", ttyName});
    if (!ps.waitForFinished(2000)) return results;

    QString output = QString::fromUtf8(ps.readAllStandardOutput());
    for (const auto &line : output.split('\n')) {
        QString trimmed = line.trimmed();
        if (trimmed.isEmpty() || trimmed.startsWith("PID")) continue;

        auto parts = trimmed.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
        if (parts.size() < 2) continue;

        qint64 pid = parts[0].toLongLong();
        QString comm = parts[1];

        QString type = agentTypeFromBinary(comm);
        if (!type.isEmpty()) {
            AgentInfo info;
            info.type = type;
            info.pid = pid;
            if (type == "claude-code") info.displayName = "Claude Code";
            else if (type == "codex") info.displayName = "Codex";
            else if (type == "kimi") info.displayName = "Kimi";
            else if (type == "opencode") info.displayName = "OpenCode";
            else if (type == "grok") info.displayName = "Grok";
            else if (type == "copilot") info.displayName = "GitHub Copilot";
            else info.displayName = type;
            results.append(info);
        }
    }
#endif

    return results;
}

QString AgentDetector::agentTypeFromBinary(const QString &binary)
{
    QString name = binary.toLower();
    // Strip path
    int lastSlash = name.lastIndexOf('/');
    if (lastSlash >= 0) name = name.mid(lastSlash + 1);

    if (name == "claude" || name.contains("claude-code")) return "claude-code";
    if (name == "codex" || name.contains("codex")) return "codex";
    if (name == "kimi") return "kimi";
    if (name == "opencode") return "opencode";
    if (name == "grok") return "grok";
    if (name.contains("copilot")) return "copilot";
    return {};
}

} // namespace c11
