#include "AgentChipWidget.h"

#include <QHBoxLayout>
#include <QFont>

namespace c11 {

// --- AgentChipWidget ---

AgentChipWidget::AgentChipWidget(QWidget *parent)
    : QWidget(parent)
{
    auto *layout = new QHBoxLayout(this);
    layout->setContentsMargins(4, 1, 4, 1);
    layout->setSpacing(0);

    m_label = new QLabel(this);
    m_label->setStyleSheet(
        "QLabel { background: #2d5aa0; color: white; border-radius: 4px; "
        "padding: 1px 6px; font-size: 10px; }");
    layout->addWidget(m_label);

    hide();
}

void AgentChipWidget::setAgentInfo(const AgentDetector::AgentInfo &info)
{
    m_label->setText(info.displayName);
    show();
}

void AgentChipWidget::clear()
{
    m_label->clear();
    hide();
}

// --- WorktreeChipWidget ---

WorktreeChipWidget::WorktreeChipWidget(QWidget *parent)
    : QWidget(parent)
{
    auto *layout = new QHBoxLayout(this);
    layout->setContentsMargins(4, 1, 4, 1);
    layout->setSpacing(0);

    m_label = new QLabel(this);
    m_label->setStyleSheet(
        "QLabel { background: #2d7a2d; color: white; border-radius: 4px; "
        "padding: 1px 6px; font-size: 10px; }");
    layout->addWidget(m_label);

    hide();
}

void WorktreeChipWidget::setBranch(const QString &branch)
{
    m_branch = branch;
    updateDisplay();
}

void WorktreeChipWidget::setWorktree(const QString &worktree)
{
    m_worktree = worktree;
    updateDisplay();
}

void WorktreeChipWidget::clear()
{
    m_branch.clear();
    m_worktree.clear();
    m_label->clear();
    hide();
}

void WorktreeChipWidget::updateDisplay()
{
    if (m_branch.isEmpty() && m_worktree.isEmpty()) {
        hide();
        return;
    }

    QString text;
    if (!m_worktree.isEmpty()) {
        text = m_worktree;
        if (!m_branch.isEmpty()) text += ":" + m_branch;
    } else {
        text = m_branch;
    }

    m_label->setText(text);
    show();
}

} // namespace c11
