#pragma once

#include <QObject>
#include <QMimeData>
#include <QUuid>
#include <QString>

namespace c11 {

// MIME types for c11 internal drag-and-drop.
namespace DragMimeTypes {
    inline const QString WorkspaceReorder = "application/x-c11-workspace-reorder";
    inline const QString PanelMove        = "application/x-c11-panel-move";
    inline const QString TabTransfer      = "application/x-c11-tab-transfer";
}

// Creates QMimeData for dragging a workspace (sidebar reorder).
class WorkspaceDragData : public QMimeData {
    Q_OBJECT

public:
    explicit WorkspaceDragData(const QUuid &workspaceId, int sourceIndex);

    QUuid workspaceId() const { return m_workspaceId; }
    int sourceIndex() const { return m_sourceIndex; }

    static bool canDecode(const QMimeData *data);
    static QUuid decodeWorkspaceId(const QMimeData *data);
    static int decodeSourceIndex(const QMimeData *data);

private:
    QUuid m_workspaceId;
    int m_sourceIndex;
};

// Creates QMimeData for dragging a panel between panes/workspaces.
class PanelDragData : public QMimeData {
    Q_OBJECT

public:
    explicit PanelDragData(const QUuid &panelId, const QUuid &sourceWorkspaceId);

    QUuid panelId() const { return m_panelId; }
    QUuid sourceWorkspaceId() const { return m_sourceWorkspaceId; }

    static bool canDecode(const QMimeData *data);
    static QUuid decodePanelId(const QMimeData *data);
    static QUuid decodeSourceWorkspaceId(const QMimeData *data);

private:
    QUuid m_panelId;
    QUuid m_sourceWorkspaceId;
};

} // namespace c11
