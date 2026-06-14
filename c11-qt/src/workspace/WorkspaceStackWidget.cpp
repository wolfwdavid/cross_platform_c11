#include "WorkspaceStackWidget.h"

namespace c11 {

WorkspaceStackWidget::WorkspaceStackWidget(WorkspaceManager &manager, QWidget *parent)
    : QStackedWidget(parent)
    , m_manager(manager)
{
    connect(&m_manager, &WorkspaceManager::workspaceAdded,
            this, &WorkspaceStackWidget::onWorkspaceAdded);
    connect(&m_manager, &WorkspaceManager::workspaceRemoved,
            this, &WorkspaceStackWidget::onWorkspaceRemoved);
    connect(&m_manager, &WorkspaceManager::selectedWorkspaceChanged,
            this, &WorkspaceStackWidget::onSelectedWorkspaceChanged);

    // Populate existing
    for (int i = 0; i < m_manager.count(); ++i) {
        onWorkspaceAdded(m_manager.workspace(i), i);
    }
    onSelectedWorkspaceChanged(m_manager.selectedWorkspaceId());
}

void WorkspaceStackWidget::onWorkspaceAdded(Workspace *workspace, int index)
{
    Q_UNUSED(index);

    auto resolver = [workspace](const QUuid &panelId) -> QWidget * {
        auto *panel = workspace->panel(panelId);
        return panel ? panel->contentWidget() : nullptr;
    };

    auto *layoutWidget = new PaneLayoutWidget(resolver, this);
    m_layoutWidgets.insert(workspace->id(), layoutWidget);
    addWidget(layoutWidget);

    rebuildLayout(workspace);

    connect(workspace, &Workspace::layoutChanged, this, [this, workspace]() {
        rebuildLayout(workspace);
    });
}

void WorkspaceStackWidget::onWorkspaceRemoved(const QUuid &id, int index)
{
    Q_UNUSED(index);
    if (auto *widget = m_layoutWidgets.value(id)) {
        widget->clear();
        removeWidget(widget);
        m_layoutWidgets.remove(id);
        delete widget;
    }
}

void WorkspaceStackWidget::onSelectedWorkspaceChanged(const QUuid &id)
{
    if (auto *widget = m_layoutWidgets.value(id)) {
        setCurrentWidget(widget);
    }
}

void WorkspaceStackWidget::rebuildLayout(Workspace *workspace)
{
    if (auto *widget = m_layoutWidgets.value(workspace->id())) {
        if (workspace->layout()) {
            widget->setLayout(*workspace->layout());
        }
    }
}

} // namespace c11
