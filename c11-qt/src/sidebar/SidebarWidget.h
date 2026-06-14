#pragma once

#include "workspace/WorkspaceManager.h"

#include <QWidget>
#include <QListWidget>
#include <QVBoxLayout>

namespace c11 {

// Sidebar showing workspace list with selection.
class SidebarWidget : public QWidget {
    Q_OBJECT

public:
    explicit SidebarWidget(WorkspaceManager &manager, QWidget *parent = nullptr);

    void setVisible(bool visible);
    bool isSidebarVisible() const { return m_visible; }
    void toggleVisibility();

private slots:
    void onWorkspaceAdded(Workspace *workspace, int index);
    void onWorkspaceRemoved(const QUuid &id, int index);
    void onSelectionChanged(const QUuid &id);
    void onItemClicked(QListWidgetItem *item);

private:
    void updateItem(int index, Workspace *workspace);

    WorkspaceManager &m_manager;
    QListWidget *m_list;
    bool m_visible = true;
    bool m_updatingSelection = false;
};

} // namespace c11
