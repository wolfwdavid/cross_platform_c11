#include "WorkspaceManager.h"

namespace c11 {

WorkspaceManager::WorkspaceManager(GhosttyRuntime &runtime, QObject *parent)
    : QObject(parent)
    , m_runtime(runtime)
{
}

WorkspaceManager::~WorkspaceManager()
{
    qDeleteAll(m_workspaces);
}

Workspace *WorkspaceManager::workspace(int index) const
{
    if (index < 0 || index >= m_workspaces.size()) return nullptr;
    return m_workspaces.at(index);
}

Workspace *WorkspaceManager::workspace(const QUuid &id) const
{
    for (auto *ws : m_workspaces) {
        if (ws->id() == id) return ws;
    }
    return nullptr;
}

int WorkspaceManager::indexOf(const QUuid &id) const
{
    for (int i = 0; i < m_workspaces.size(); ++i) {
        if (m_workspaces[i]->id() == id) return i;
    }
    return -1;
}

Workspace *WorkspaceManager::selectedWorkspace() const
{
    return workspace(m_selectedId);
}

int WorkspaceManager::selectedIndex() const
{
    return indexOf(m_selectedId);
}

void WorkspaceManager::selectWorkspace(const QUuid &id)
{
    if (m_selectedId != id && workspace(id)) {
        m_selectedId = id;
        emit selectedWorkspaceChanged(id);
    }
}

void WorkspaceManager::selectWorkspace(int index)
{
    if (auto *ws = workspace(index)) {
        selectWorkspace(ws->id());
    }
}

void WorkspaceManager::selectNextWorkspace()
{
    int idx = selectedIndex();
    if (idx < 0) return;
    int next = (idx + 1) % m_workspaces.size();
    selectWorkspace(next);
}

void WorkspaceManager::selectPreviousWorkspace()
{
    int idx = selectedIndex();
    if (idx < 0) return;
    int prev = (idx - 1 + m_workspaces.size()) % m_workspaces.size();
    selectWorkspace(prev);
}

Workspace *WorkspaceManager::addWorkspace(const QString &title,
                                           const QString &workingDirectory,
                                           bool withInitialPanel)
{
    auto *ws = new Workspace(m_runtime, title, workingDirectory, this, withInitialPanel);
    int index = m_workspaces.size();
    m_workspaces.append(ws);

    emit workspaceAdded(ws, index);

    if (m_selectedId.isNull()) {
        selectWorkspace(ws->id());
    }

    return ws;
}

void WorkspaceManager::removeWorkspace(const QUuid &id)
{
    int idx = indexOf(id);
    if (idx < 0) return;
    removeWorkspace(idx);
}

void WorkspaceManager::removeWorkspace(int index)
{
    if (index < 0 || index >= m_workspaces.size()) return;

    auto *ws = m_workspaces.at(index);
    QUuid removedId = ws->id();

    m_workspaces.removeAt(index);
    emit workspaceRemoved(removedId, index);

    // Update selection
    if (m_selectedId == removedId) {
        if (!m_workspaces.isEmpty()) {
            int newIdx = qMin(index, m_workspaces.size() - 1);
            selectWorkspace(newIdx);
        } else {
            m_selectedId = QUuid();
            emit selectedWorkspaceChanged(m_selectedId);
        }
    }

    delete ws;
}

void WorkspaceManager::moveWorkspace(int from, int to)
{
    if (from < 0 || from >= m_workspaces.size()) return;
    if (to < 0 || to >= m_workspaces.size()) return;
    if (from == to) return;

    m_workspaces.move(from, to);
    emit workspaceMoved(from, to);
}

} // namespace c11
