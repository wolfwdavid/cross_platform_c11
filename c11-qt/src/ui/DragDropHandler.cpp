#include "DragDropHandler.h"

#include <QByteArray>
#include <QDataStream>
#include <QIODevice>

namespace c11 {

// --- WorkspaceDragData ---

WorkspaceDragData::WorkspaceDragData(const QUuid &workspaceId, int sourceIndex)
    : m_workspaceId(workspaceId)
    , m_sourceIndex(sourceIndex)
{
    QByteArray data;
    QDataStream stream(&data, QIODevice::WriteOnly);
    stream << workspaceId.toString() << sourceIndex;
    setData(DragMimeTypes::WorkspaceReorder, data);
}

bool WorkspaceDragData::canDecode(const QMimeData *data)
{
    return data && data->hasFormat(DragMimeTypes::WorkspaceReorder);
}

QUuid WorkspaceDragData::decodeWorkspaceId(const QMimeData *data)
{
    QByteArray raw = data->data(DragMimeTypes::WorkspaceReorder);
    QDataStream stream(&raw, QIODevice::ReadOnly);
    QString idStr;
    stream >> idStr;
    return QUuid::fromString(idStr);
}

int WorkspaceDragData::decodeSourceIndex(const QMimeData *data)
{
    QByteArray raw = data->data(DragMimeTypes::WorkspaceReorder);
    QDataStream stream(&raw, QIODevice::ReadOnly);
    QString idStr;
    int index;
    stream >> idStr >> index;
    return index;
}

// --- PanelDragData ---

PanelDragData::PanelDragData(const QUuid &panelId, const QUuid &sourceWorkspaceId)
    : m_panelId(panelId)
    , m_sourceWorkspaceId(sourceWorkspaceId)
{
    QByteArray data;
    QDataStream stream(&data, QIODevice::WriteOnly);
    stream << panelId.toString() << sourceWorkspaceId.toString();
    setData(DragMimeTypes::PanelMove, data);
}

bool PanelDragData::canDecode(const QMimeData *data)
{
    return data && data->hasFormat(DragMimeTypes::PanelMove);
}

QUuid PanelDragData::decodePanelId(const QMimeData *data)
{
    QByteArray raw = data->data(DragMimeTypes::PanelMove);
    QDataStream stream(&raw, QIODevice::ReadOnly);
    QString idStr;
    stream >> idStr;
    return QUuid::fromString(idStr);
}

QUuid PanelDragData::decodeSourceWorkspaceId(const QMimeData *data)
{
    QByteArray raw = data->data(DragMimeTypes::PanelMove);
    QDataStream stream(&raw, QIODevice::ReadOnly);
    QString panelStr, wsStr;
    stream >> panelStr >> wsStr;
    return QUuid::fromString(wsStr);
}

} // namespace c11
