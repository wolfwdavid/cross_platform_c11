#include "StatusBar.h"

#include <QHBoxLayout>

namespace c11 {

StatusBar::StatusBar(WorkspaceManager &manager, QWidget *parent)
    : QWidget(parent)
    , m_manager(manager)
{
    setFixedHeight(24);

    auto *layout = new QHBoxLayout(this);
    layout->setContentsMargins(8, 0, 8, 0);
    layout->setSpacing(16);

    m_workspaceLabel = new QLabel(this);
    layout->addWidget(m_workspaceLabel);

    m_paneLabel = new QLabel(this);
    layout->addWidget(m_paneLabel);

    layout->addStretch();

    m_infoLabel = new QLabel(this);
    layout->addWidget(m_infoLabel);

    connect(&m_manager, &WorkspaceManager::selectedWorkspaceChanged,
            this, &StatusBar::onSelectionChanged);
    connect(&m_manager, &WorkspaceManager::workspaceAdded,
            this, [this](Workspace *, int) { updateStatus(); });
    connect(&m_manager, &WorkspaceManager::workspaceRemoved,
            this, [this](const QUuid &, int) { updateStatus(); });

    updateStatus();
}

void StatusBar::updateStatus()
{
    auto *ws = m_manager.selectedWorkspace();
    if (!ws) {
        m_workspaceLabel->setText("No workspace");
        m_paneLabel->clear();
        m_infoLabel->clear();
        return;
    }

    int idx = m_manager.selectedIndex() + 1;
    int total = m_manager.count();
    m_workspaceLabel->setText(
        QString("Workspace %1/%2: %3").arg(idx).arg(total).arg(ws->effectiveTitle()));

    int panes = ws->panelCount();
    m_paneLabel->setText(QString("%1 pane%2").arg(panes).arg(panes != 1 ? "s" : ""));

    m_infoLabel->setText("c11");
}

void StatusBar::onSelectionChanged(const QUuid &id)
{
    Q_UNUSED(id);
    updateStatus();

    // Connect to the new workspace's signals (disconnect old first)
    if (auto *ws = m_manager.selectedWorkspace()) {
        // Disconnect any previous workspace connections to this
        for (auto *other : m_manager.workspaces()) {
            if (other != ws) disconnect(other, nullptr, this, nullptr);
        }
        connect(ws, &Workspace::panelAdded, this, [this](const QUuid &) { updateStatus(); });
        connect(ws, &Workspace::panelRemoved, this, [this](const QUuid &) { updateStatus(); });
        connect(ws, &Workspace::titleChanged, this, [this](const QString &) { updateStatus(); });
    }
}

} // namespace c11
