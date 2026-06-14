#pragma once

#include "agent/AgentDetector.h"
#include <QWidget>
#include <QLabel>

namespace c11 {

// Small badge showing the detected agent type for a panel.
class AgentChipWidget : public QWidget {
    Q_OBJECT

public:
    explicit AgentChipWidget(QWidget *parent = nullptr);

    void setAgentInfo(const AgentDetector::AgentInfo &info);
    void clear();

private:
    QLabel *m_label;
};

// Badge showing git worktree/branch info for a workspace.
class WorktreeChipWidget : public QWidget {
    Q_OBJECT

public:
    explicit WorktreeChipWidget(QWidget *parent = nullptr);

    void setBranch(const QString &branch);
    void setWorktree(const QString &worktree);
    void clear();

private:
    void updateDisplay();
    QLabel *m_label;
    QString m_branch;
    QString m_worktree;
};

} // namespace c11
