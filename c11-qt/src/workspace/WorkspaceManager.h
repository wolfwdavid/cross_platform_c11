#pragma once

#include "Workspace.h"
#include "ghostty/GhosttyRuntime.h"

#include <QObject>
#include <QUuid>
#include <QList>

namespace c11 {

// Manages the collection of workspaces (equivalent to TabManager in Swift).
class WorkspaceManager : public QObject {
    Q_OBJECT

public:
    explicit WorkspaceManager(GhosttyRuntime &runtime, QObject *parent = nullptr);
    ~WorkspaceManager() override;

    // Workspace collection
    const QList<Workspace *> &workspaces() const { return m_workspaces; }
    int count() const { return m_workspaces.size(); }
    Workspace *workspace(int index) const;
    Workspace *workspace(const QUuid &id) const;
    int indexOf(const QUuid &id) const;

    // Selection
    Workspace *selectedWorkspace() const;
    QUuid selectedWorkspaceId() const { return m_selectedId; }
    int selectedIndex() const;
    void selectWorkspace(const QUuid &id);
    void selectWorkspace(int index);
    void selectNextWorkspace();
    void selectPreviousWorkspace();

    // Add / remove
    Workspace *addWorkspace(const QString &title = "Terminal",
                            const QString &workingDirectory = {},
                            bool withInitialPanel = true);
    void removeWorkspace(const QUuid &id);
    void removeWorkspace(int index);

    // Reorder
    void moveWorkspace(int from, int to);

signals:
    void workspaceAdded(Workspace *workspace, int index);
    void workspaceRemoved(const QUuid &id, int index);
    void workspaceMoved(int from, int to);
    void selectedWorkspaceChanged(const QUuid &id);

private:
    GhosttyRuntime &m_runtime;
    QList<Workspace *> m_workspaces;
    QUuid m_selectedId;
};

} // namespace c11
