#pragma once

#include "SocketProtocol.h"
#include "workspace/WorkspaceManager.h"

#include <QObject>
#include <QString>
#include <QMap>
#include <functional>

namespace c11 {

// Routes socket commands to handler functions.
// Supports both V1 text commands and V2 JSON-RPC methods.
class SocketCommandRouter : public QObject {
    Q_OBJECT

public:
    explicit SocketCommandRouter(WorkspaceManager &manager, QObject *parent = nullptr);

    // Process a raw line (auto-detects V1/V2)
    QString processLine(const QString &line);

private:
    // V1 handlers return response string
    using V1Handler = std::function<QString(const QStringList &args)>;
    // V2 handlers return JSON result value
    using V2Handler = std::function<QJsonValue(const QJsonObject &params)>;

    void registerCommands();

    // V1 command implementations
    QString cmdPing(const QStringList &args);
    QString cmdHelp(const QStringList &args);
    QString cmdListWorkspaces(const QStringList &args);
    QString cmdCurrentWorkspace(const QStringList &args);
    QString cmdNewWorkspace(const QStringList &args);
    QString cmdCloseWorkspace(const QStringList &args);
    QString cmdSelectWorkspace(const QStringList &args);
    QString cmdListSurfaces(const QStringList &args);
    QString cmdNewPane(const QStringList &args);
    QString cmdNewSplit(const QStringList &args);
    QString cmdCloseSurface(const QStringList &args);
    QString cmdSetStatus(const QStringList &args);
    QString cmdClearStatus(const QStringList &args);
    QString cmdSetProgress(const QStringList &args);
    QString cmdClearProgress(const QStringList &args);
    QString cmdOpenBrowser(const QStringList &args);
    QString cmdNavigate(const QStringList &args);

    // V2 command implementations
    QJsonValue v2SystemPing(const QJsonObject &params);
    QJsonValue v2SystemTree(const QJsonObject &params);
    QJsonValue v2SystemCapabilities(const QJsonObject &params);
    QJsonValue v2WorkspaceList(const QJsonObject &params);
    QJsonValue v2WorkspaceCurrent(const QJsonObject &params);
    QJsonValue v2WorkspaceCreate(const QJsonObject &params);
    QJsonValue v2WorkspaceClose(const QJsonObject &params);
    QJsonValue v2WorkspaceSelect(const QJsonObject &params);
    QJsonValue v2WorkspaceNext(const QJsonObject &params);
    QJsonValue v2WorkspacePrevious(const QJsonObject &params);
    QJsonValue v2SurfaceList(const QJsonObject &params);
    QJsonValue v2SurfaceCreate(const QJsonObject &params);
    QJsonValue v2SurfaceSplit(const QJsonObject &params);
    QJsonValue v2SurfaceClose(const QJsonObject &params);
    QJsonValue v2SurfaceSend(const QJsonObject &params);
    QJsonValue v2SurfaceSendKey(const QJsonObject &params);
    QJsonValue v2SurfaceReadScreen(const QJsonObject &params);
    QJsonValue v2PaneList(const QJsonObject &params);
    QJsonValue v2BrowserOpen(const QJsonObject &params);

    // Theme V2 commands
    QJsonValue v2ThemeList(const QJsonObject &params);
    QJsonValue v2ThemeGet(const QJsonObject &params);
    QJsonValue v2ThemeSetActive(const QJsonObject &params);

    // Metadata V2 commands
    QJsonValue v2SurfaceSetMetadata(const QJsonObject &params);
    QJsonValue v2SurfaceGetMetadata(const QJsonObject &params);
    QJsonValue v2SurfaceClearMetadata(const QJsonObject &params);

    // Helpers
    QJsonObject workspaceToJson(const Workspace *ws) const;
    QJsonObject panelToJson(const Panel *panel) const;

    // Resolve a surface/panel ref to a live Panel. Empty ref → the focused panel
    // of the selected workspace; a UUID → the matching panel in any workspace;
    // otherwise nullptr. Shared targeting seam for send / send-key / read-screen.
    Panel *resolvePanel(const QString &idStr) const;

    WorkspaceManager &m_manager;
    QMap<QString, V1Handler> m_v1Commands;
    QMap<QString, V2Handler> m_v2Methods;
};

} // namespace c11
