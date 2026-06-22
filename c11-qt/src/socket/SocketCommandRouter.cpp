#include "SocketCommandRouter.h"
#include "metadata/SurfaceMetadataStore.h"
#include "theme/ThemeManager.h"
#include "panel/TerminalPanel.h"
#include "panel/BrowserPanel.h"
#include "panel/MarkdownPanel.h"

#include <QJsonArray>

namespace c11 {

SocketCommandRouter::SocketCommandRouter(WorkspaceManager &manager, QObject *parent)
    : QObject(parent)
    , m_manager(manager)
{
    registerCommands();
}

void SocketCommandRouter::registerCommands()
{
    // V1 commands
    m_v1Commands["ping"]              = [this](auto &a) { return cmdPing(a); };
    m_v1Commands["help"]              = [this](auto &a) { return cmdHelp(a); };
    m_v1Commands["list_workspaces"]   = [this](auto &a) { return cmdListWorkspaces(a); };
    m_v1Commands["current_workspace"] = [this](auto &a) { return cmdCurrentWorkspace(a); };
    m_v1Commands["new_workspace"]     = [this](auto &a) { return cmdNewWorkspace(a); };
    m_v1Commands["close_workspace"]   = [this](auto &a) { return cmdCloseWorkspace(a); };
    m_v1Commands["select_workspace"]  = [this](auto &a) { return cmdSelectWorkspace(a); };
    m_v1Commands["list_surfaces"]     = [this](auto &a) { return cmdListSurfaces(a); };
    m_v1Commands["new_pane"]          = [this](auto &a) { return cmdNewPane(a); };
    m_v1Commands["new_split"]         = [this](auto &a) { return cmdNewSplit(a); };
    m_v1Commands["close_surface"]     = [this](auto &a) { return cmdCloseSurface(a); };
    m_v1Commands["set_status"]        = [this](auto &a) { return cmdSetStatus(a); };
    m_v1Commands["clear_status"]      = [this](auto &a) { return cmdClearStatus(a); };
    m_v1Commands["set_progress"]      = [this](auto &a) { return cmdSetProgress(a); };
    m_v1Commands["clear_progress"]    = [this](auto &a) { return cmdClearProgress(a); };
    m_v1Commands["open_browser"]      = [this](auto &a) { return cmdOpenBrowser(a); };
    m_v1Commands["navigate"]          = [this](auto &a) { return cmdNavigate(a); };

    // V2 methods
    m_v2Methods["system.ping"]         = [this](auto &p) { return v2SystemPing(p); };
    m_v2Methods["system.tree"]         = [this](auto &p) { return v2SystemTree(p); };
    m_v2Methods["system.capabilities"] = [this](auto &p) { return v2SystemCapabilities(p); };
    m_v2Methods["workspace.list"]      = [this](auto &p) { return v2WorkspaceList(p); };
    m_v2Methods["workspace.current"]   = [this](auto &p) { return v2WorkspaceCurrent(p); };
    m_v2Methods["workspace.create"]    = [this](auto &p) { return v2WorkspaceCreate(p); };
    m_v2Methods["workspace.close"]     = [this](auto &p) { return v2WorkspaceClose(p); };
    m_v2Methods["workspace.select"]    = [this](auto &p) { return v2WorkspaceSelect(p); };
    m_v2Methods["workspace.next"]      = [this](auto &p) { return v2WorkspaceNext(p); };
    m_v2Methods["workspace.previous"]  = [this](auto &p) { return v2WorkspacePrevious(p); };
    m_v2Methods["surface.list"]        = [this](auto &p) { return v2SurfaceList(p); };
    m_v2Methods["surface.create"]      = [this](auto &p) { return v2SurfaceCreate(p); };
    m_v2Methods["surface.split"]       = [this](auto &p) { return v2SurfaceSplit(p); };
    m_v2Methods["surface.close"]       = [this](auto &p) { return v2SurfaceClose(p); };
    m_v2Methods["surface.send"]        = [this](auto &p) { return v2SurfaceSend(p); };
    m_v2Methods["pane.list"]           = [this](auto &p) { return v2PaneList(p); };
    m_v2Methods["browser.open_split"]  = [this](auto &p) { return v2BrowserOpen(p); };
    m_v2Methods["theme.list"]              = [this](auto &p) { return v2ThemeList(p); };
    m_v2Methods["theme.get"]               = [this](auto &p) { return v2ThemeGet(p); };
    m_v2Methods["theme.set_active"]        = [this](auto &p) { return v2ThemeSetActive(p); };
    m_v2Methods["surface.set_metadata"]   = [this](auto &p) { return v2SurfaceSetMetadata(p); };
    m_v2Methods["surface.get_metadata"]   = [this](auto &p) { return v2SurfaceGetMetadata(p); };
    m_v2Methods["surface.clear_metadata"] = [this](auto &p) { return v2SurfaceClearMetadata(p); };
}

QString SocketCommandRouter::processLine(const QString &line)
{
    auto version = SocketProtocol::detectVersion(line);

    if (version == SocketProtocol::Version::V2) {
        auto req = SocketProtocol::parseV2(line);
        if (!req) {
            return SocketProtocol::v2Error(QJsonValue(), "parse_error", "Invalid JSON-RPC");
        }

        auto it = m_v2Methods.find(req->method);
        if (it == m_v2Methods.end()) {
            return SocketProtocol::v2Error(req->id, "method_not_found",
                                            "Unknown method: " + req->method);
        }

        QJsonValue result = it.value()(req->params);
        return SocketProtocol::v2Ok(req->id, result);
    }

    // V1
    auto cmd = SocketProtocol::parseV1(line);
    if (cmd.name.isEmpty()) {
        return SocketProtocol::v1Error("Empty command");
    }

    auto it = m_v1Commands.find(cmd.name);
    if (it == m_v1Commands.end()) {
        return SocketProtocol::v1Error("Unknown command: " + cmd.name);
    }

    return it.value()(cmd.args);
}

// === V1 Command Implementations ===

QString SocketCommandRouter::cmdPing(const QStringList &) { return "pong\n"; }

QString SocketCommandRouter::cmdHelp(const QStringList &)
{
    QStringList cmds = m_v1Commands.keys();
    cmds.sort();
    return cmds.join("\n") + "\n";
}

QString SocketCommandRouter::cmdListWorkspaces(const QStringList &)
{
    QStringList lines;
    for (auto *ws : m_manager.workspaces()) {
        QString selected = (ws->id() == m_manager.selectedWorkspaceId()) ? "*" : " ";
        lines << QString("%1 %2 %3 [%4 panes]")
                     .arg(selected, ws->id().toString(QUuid::WithoutBraces),
                          ws->effectiveTitle(), QString::number(ws->panelCount()));
    }
    return lines.join("\n") + "\n";
}

QString SocketCommandRouter::cmdCurrentWorkspace(const QStringList &)
{
    auto *ws = m_manager.selectedWorkspace();
    if (!ws) return SocketProtocol::v1Error("No workspace selected");
    return ws->id().toString(QUuid::WithoutBraces) + "\n";
}

QString SocketCommandRouter::cmdNewWorkspace(const QStringList &args)
{
    QString title = args.isEmpty() ? "Terminal" : args.join(" ");
    auto *ws = m_manager.addWorkspace(title);
    m_manager.selectWorkspace(ws->id());
    return ws->id().toString(QUuid::WithoutBraces) + "\n";
}

QString SocketCommandRouter::cmdCloseWorkspace(const QStringList &args)
{
    QUuid id;
    if (!args.isEmpty()) {
        id = QUuid::fromString(args.first());
    } else {
        id = m_manager.selectedWorkspaceId();
    }
    if (id.isNull()) return SocketProtocol::v1Error("No workspace to close");
    m_manager.removeWorkspace(id);
    return SocketProtocol::v1Ok();
}

QString SocketCommandRouter::cmdSelectWorkspace(const QStringList &args)
{
    if (args.isEmpty()) return SocketProtocol::v1Error("Usage: select_workspace <id|index>");

    // Try UUID first
    QUuid id = QUuid::fromString(args.first());
    if (!id.isNull()) {
        m_manager.selectWorkspace(id);
        return SocketProtocol::v1Ok();
    }

    // Try index
    bool ok;
    int idx = args.first().toInt(&ok);
    if (ok) {
        m_manager.selectWorkspace(idx);
        return SocketProtocol::v1Ok();
    }

    return SocketProtocol::v1Error("Invalid workspace id or index");
}

QString SocketCommandRouter::cmdListSurfaces(const QStringList &)
{
    QStringList lines;
    for (auto *ws : m_manager.workspaces()) {
        for (auto *panel : ws->allPanels()) {
            QString focused = (panel->id() == ws->focusedPanelId()) ? "*" : " ";
            QString type;
            switch (panel->panelType()) {
            case PanelType::Terminal: type = "terminal"; break;
            case PanelType::Browser:  type = "browser"; break;
            case PanelType::Markdown: type = "markdown"; break;
            }
            lines << QString("%1 %2 %3 %4 [%5]")
                         .arg(focused, panel->id().toString(QUuid::WithoutBraces),
                              ws->id().toString(QUuid::WithoutBraces),
                              panel->displayTitle(), type);
        }
    }
    return lines.join("\n") + "\n";
}

QString SocketCommandRouter::cmdNewPane(const QStringList &args)
{
    auto *ws = m_manager.selectedWorkspace();
    if (!ws) return SocketProtocol::v1Error("No workspace");
    QString wd = SocketProtocol::v1Arg(args, "cwd");
    auto *panel = ws->createTerminalPanel(wd);
    return panel->id().toString(QUuid::WithoutBraces) + "\n";
}

QString SocketCommandRouter::cmdNewSplit(const QStringList &args)
{
    auto *ws = m_manager.selectedWorkspace();
    if (!ws) return SocketProtocol::v1Error("No workspace");

    QString dir = SocketProtocol::v1Arg(args, "direction", "right");
    auto direction = (dir == "down" || dir == "vertical")
                         ? PaneLayout::Direction::Vertical
                         : PaneLayout::Direction::Horizontal;
    QString wd = SocketProtocol::v1Arg(args, "cwd");
    ws->splitPanel(ws->focusedPanelId(), direction, wd);
    return SocketProtocol::v1Ok();
}

QString SocketCommandRouter::cmdCloseSurface(const QStringList &args)
{
    auto *ws = m_manager.selectedWorkspace();
    if (!ws) return SocketProtocol::v1Error("No workspace");

    QUuid panelId;
    if (!args.isEmpty()) {
        panelId = QUuid::fromString(args.first());
    } else {
        panelId = ws->focusedPanelId();
    }
    if (panelId.isNull()) return SocketProtocol::v1Error("No surface to close");
    ws->removePanel(panelId);
    return SocketProtocol::v1Ok();
}

QString SocketCommandRouter::cmdSetStatus(const QStringList &args)
{
    Q_UNUSED(args);
    return SocketProtocol::v1Ok(); // Phase 4: metadata store
}

QString SocketCommandRouter::cmdClearStatus(const QStringList &args)
{
    Q_UNUSED(args);
    return SocketProtocol::v1Ok();
}

QString SocketCommandRouter::cmdSetProgress(const QStringList &args)
{
    Q_UNUSED(args);
    return SocketProtocol::v1Ok();
}

QString SocketCommandRouter::cmdClearProgress(const QStringList &args)
{
    Q_UNUSED(args);
    return SocketProtocol::v1Ok();
}

QString SocketCommandRouter::cmdOpenBrowser(const QStringList &args)
{
    auto *ws = m_manager.selectedWorkspace();
    if (!ws) return SocketProtocol::v1Error("No workspace");

    QUrl url = QUrl("about:blank");
    if (!args.isEmpty()) {
        url = QUrl::fromUserInput(args.first());
    }
    auto *panel = ws->createBrowserPanel(url);
    if (ws->layout() && !ws->focusedPanelId().isNull()) {
        ws->layout()->splitLeaf(ws->focusedPanelId(), panel->id(),
                                 PaneLayout::Direction::Horizontal);
    }
    ws->setFocusedPanelId(panel->id());
    emit ws->layoutChanged();
    return panel->id().toString(QUuid::WithoutBraces) + "\n";
}

QString SocketCommandRouter::cmdNavigate(const QStringList &args)
{
    if (args.isEmpty()) return SocketProtocol::v1Error("Usage: navigate <url>");
    auto *ws = m_manager.selectedWorkspace();
    if (!ws) return SocketProtocol::v1Error("No workspace");
    auto *panel = dynamic_cast<BrowserPanel *>(ws->focusedPanel());
    if (!panel) return SocketProtocol::v1Error("Focused panel is not a browser");
    panel->navigate(QUrl::fromUserInput(args.first()));
    return SocketProtocol::v1Ok();
}

// === V2 Command Implementations ===

QJsonValue SocketCommandRouter::v2SystemPing(const QJsonObject &)
{
    return QJsonValue("pong");
}

QJsonValue SocketCommandRouter::v2SystemTree(const QJsonObject &)
{
    QJsonArray workspaces;
    for (auto *ws : m_manager.workspaces()) {
        QJsonObject wsObj = workspaceToJson(ws);
        QJsonArray panels;
        for (auto *panel : ws->allPanels()) {
            panels.append(panelToJson(panel));
        }
        wsObj["panels"] = panels;
        workspaces.append(wsObj);
    }
    QJsonObject result;
    result["workspaces"] = workspaces;
    return result;
}

QJsonValue SocketCommandRouter::v2SystemCapabilities(const QJsonObject &)
{
    QJsonObject caps;
    caps["v1"] = true;
    caps["v2"] = true;
    caps["browser"] = true;
    caps["markdown"] = true;
    caps["version"] = C11_VERSION;
    return caps;
}

QJsonValue SocketCommandRouter::v2WorkspaceList(const QJsonObject &)
{
    QJsonArray arr;
    for (auto *ws : m_manager.workspaces()) {
        arr.append(workspaceToJson(ws));
    }
    return arr;
}

QJsonValue SocketCommandRouter::v2WorkspaceCurrent(const QJsonObject &)
{
    auto *ws = m_manager.selectedWorkspace();
    if (!ws) return QJsonValue();
    return workspaceToJson(ws);
}

QJsonValue SocketCommandRouter::v2WorkspaceCreate(const QJsonObject &params)
{
    QString title = params.value("title").toString("Terminal");
    QString cwd = params.value("cwd").toString();
    auto *ws = m_manager.addWorkspace(title, cwd);
    if (params.value("select").toBool(true)) {
        m_manager.selectWorkspace(ws->id());
    }
    return workspaceToJson(ws);
}

QJsonValue SocketCommandRouter::v2WorkspaceClose(const QJsonObject &params)
{
    QString idStr = params.value("id").toString();
    QUuid id = idStr.isEmpty() ? m_manager.selectedWorkspaceId()
                                : QUuid::fromString(idStr);
    m_manager.removeWorkspace(id);
    return QJsonValue(true);
}

QJsonValue SocketCommandRouter::v2WorkspaceSelect(const QJsonObject &params)
{
    QString idStr = params.value("id").toString();
    if (!idStr.isEmpty()) {
        m_manager.selectWorkspace(QUuid::fromString(idStr));
    } else if (params.contains("index")) {
        m_manager.selectWorkspace(params.value("index").toInt());
    }
    return QJsonValue(true);
}

QJsonValue SocketCommandRouter::v2WorkspaceNext(const QJsonObject &)
{
    m_manager.selectNextWorkspace();
    return QJsonValue(true);
}

QJsonValue SocketCommandRouter::v2WorkspacePrevious(const QJsonObject &)
{
    m_manager.selectPreviousWorkspace();
    return QJsonValue(true);
}

QJsonValue SocketCommandRouter::v2SurfaceList(const QJsonObject &params)
{
    QJsonArray arr;
    QString wsId = params.value("workspace_id").toString();

    for (auto *ws : m_manager.workspaces()) {
        if (!wsId.isEmpty() && ws->id().toString(QUuid::WithoutBraces) != wsId) continue;
        for (auto *panel : ws->allPanels()) {
            QJsonObject obj = panelToJson(panel);
            obj["workspace_id"] = ws->id().toString(QUuid::WithoutBraces);
            arr.append(obj);
        }
    }
    return arr;
}

QJsonValue SocketCommandRouter::v2SurfaceCreate(const QJsonObject &params)
{
    auto *ws = m_manager.selectedWorkspace();
    if (!ws) return QJsonValue();
    QString cwd = params.value("cwd").toString();
    auto *panel = ws->createTerminalPanel(cwd);
    return panelToJson(panel);
}

QJsonValue SocketCommandRouter::v2SurfaceSplit(const QJsonObject &params)
{
    auto *ws = m_manager.selectedWorkspace();
    if (!ws) return QJsonValue();
    QString dir = params.value("direction").toString("right");
    auto direction = (dir == "down" || dir == "vertical")
                         ? PaneLayout::Direction::Vertical
                         : PaneLayout::Direction::Horizontal;
    QString cwd = params.value("cwd").toString();
    ws->splitPanel(ws->focusedPanelId(), direction, cwd);
    return QJsonValue(true);
}

QJsonValue SocketCommandRouter::v2SurfaceClose(const QJsonObject &params)
{
    auto *ws = m_manager.selectedWorkspace();
    if (!ws) return QJsonValue();
    QString idStr = params.value("id").toString();
    QUuid id = idStr.isEmpty() ? ws->focusedPanelId() : QUuid::fromString(idStr);
    ws->removePanel(id);
    return QJsonValue(true);
}

QJsonValue SocketCommandRouter::v2SurfaceSend(const QJsonObject &params)
{
    // text is required; submit defaults to true (type the text AND press Return).
    if (!params.contains("text")) {
        return QJsonObject{{"error", "missing_text"},
                           {"message", "send requires a 'text' param"}};
    }
    const QString text = params.value("text").toString();
    const bool submit = params.value("submit").toBool(true);

    // Target: explicit id/surface ref, else the focused panel.
    QString idStr = params.value("id").toString();
    if (idStr.isEmpty()) idStr = params.value("surface").toString();

    auto *panel = resolvePanel(idStr);
    if (!panel) {
        return QJsonObject{{"error", "not_found"},
                           {"message", "No surface matches the given ref"}};
    }
    auto *term = dynamic_cast<TerminalPanel *>(panel);
    if (!term) {
        return QJsonObject{{"error", "not_a_terminal"},
                           {"message", "Target surface is not a terminal"}};
    }
    auto *widget = term->ghosttyWidget();
    if (!widget) {
        return QJsonObject{{"error", "no_surface"},
                           {"message", "Terminal has no live surface"}};
    }

    widget->sendText(text);
    if (submit) widget->sendText(QStringLiteral("\r"));

    return QJsonObject{{"ok", true},
                       {"surface", panel->id().toString(QUuid::WithoutBraces)},
                       {"sent", text.size()},
                       {"submitted", submit}};
}

QJsonValue SocketCommandRouter::v2PaneList(const QJsonObject &)
{
    auto *ws = m_manager.selectedWorkspace();
    if (!ws) return QJsonArray();
    QJsonArray arr;
    for (const auto &id : ws->orderedPanelIds()) {
        if (auto *panel = ws->panel(id)) {
            arr.append(panelToJson(panel));
        }
    }
    return arr;
}

QJsonValue SocketCommandRouter::v2BrowserOpen(const QJsonObject &params)
{
    auto *ws = m_manager.selectedWorkspace();
    if (!ws) return QJsonValue();
    QUrl url = QUrl(params.value("url").toString("about:blank"));
    auto *panel = ws->createBrowserPanel(url);
    if (ws->layout() && !ws->focusedPanelId().isNull()) {
        ws->layout()->splitLeaf(ws->focusedPanelId(), panel->id(),
                                 PaneLayout::Direction::Horizontal);
    }
    ws->setFocusedPanelId(panel->id());
    emit ws->layoutChanged();
    return panelToJson(panel);
}

// === Theme Commands ===

QJsonValue SocketCommandRouter::v2ThemeList(const QJsonObject &)
{
    QJsonArray arr;
    for (const auto &name : ThemeManager::instance().availableThemeNames()) {
        arr.append(name);
    }
    return arr;
}

QJsonValue SocketCommandRouter::v2ThemeGet(const QJsonObject &params)
{
    QString name = params.value("name").toString();
    auto *theme = ThemeManager::instance().theme(name);
    if (!theme) return QJsonValue();

    QJsonObject obj;
    obj["name"] = theme->name;
    obj["source_path"] = theme->sourcePath;
    if (theme->windowBackground.isValid())
        obj["window_background"] = theme->windowBackground.name();
    if (theme->sidebarBackground.isValid())
        obj["sidebar_background"] = theme->sidebarBackground.name();
    return obj;
}

QJsonValue SocketCommandRouter::v2ThemeSetActive(const QJsonObject &params)
{
    QString name = params.value("name").toString();
    if (name.isEmpty()) {
        ThemeManager::instance().clearActiveTheme();
    } else {
        ThemeManager::instance().setActiveTheme(name);
    }
    return QJsonValue(true);
}

// === Metadata Commands ===

QJsonValue SocketCommandRouter::v2SurfaceSetMetadata(const QJsonObject &params)
{
    QString surfaceIdStr = params.value("id").toString();
    QJsonObject metadata = params.value("metadata").toObject();

    if (surfaceIdStr.isEmpty() || metadata.isEmpty()) {
        return QJsonValue("Missing id or metadata");
    }

    QUuid surfaceId = QUuid::fromString(surfaceIdStr);
    auto result = SurfaceMetadataStore::instance().setMetadata(surfaceId, metadata);
    if (!result.ok) {
        return QJsonValue(result.error);
    }
    return QJsonValue(true);
}

QJsonValue SocketCommandRouter::v2SurfaceGetMetadata(const QJsonObject &params)
{
    QString surfaceIdStr = params.value("id").toString();
    if (surfaceIdStr.isEmpty()) {
        // Use focused panel
        auto *ws = m_manager.selectedWorkspace();
        if (ws) surfaceIdStr = ws->focusedPanelId().toString(QUuid::WithoutBraces);
    }

    QUuid surfaceId = QUuid::fromString(surfaceIdStr);
    return SurfaceMetadataStore::instance().getMetadata(surfaceId);
}

QJsonValue SocketCommandRouter::v2SurfaceClearMetadata(const QJsonObject &params)
{
    QString surfaceIdStr = params.value("id").toString();
    QString key = params.value("key").toString();

    QUuid surfaceId = QUuid::fromString(surfaceIdStr);
    if (key.isEmpty()) {
        SurfaceMetadataStore::instance().clearMetadata(surfaceId);
    } else {
        SurfaceMetadataStore::instance().clearMetadata(surfaceId, key);
    }
    return QJsonValue(true);
}

// === Helpers ===

Panel *SocketCommandRouter::resolvePanel(const QString &idStr) const
{
    if (idStr.isEmpty()) {
        auto *ws = m_manager.selectedWorkspace();
        return ws ? ws->panel(ws->focusedPanelId()) : nullptr;
    }
    QUuid id = QUuid::fromString(idStr);
    if (id.isNull()) return nullptr;
    for (auto *ws : m_manager.workspaces()) {
        if (auto *panel = ws->panel(id)) return panel;
    }
    return nullptr;
}

QJsonObject SocketCommandRouter::workspaceToJson(const Workspace *ws) const
{
    QJsonObject obj;
    obj["id"] = ws->id().toString(QUuid::WithoutBraces);
    obj["title"] = ws->effectiveTitle();
    obj["panel_count"] = ws->panelCount();
    obj["selected"] = (ws->id() == m_manager.selectedWorkspaceId());
    obj["pinned"] = ws->isPinned();
    if (!ws->focusedPanelId().isNull()) {
        obj["focused_panel_id"] = ws->focusedPanelId().toString(QUuid::WithoutBraces);
    }
    return obj;
}

QJsonObject SocketCommandRouter::panelToJson(const Panel *panel) const
{
    QJsonObject obj;
    obj["id"] = panel->id().toString(QUuid::WithoutBraces);
    obj["title"] = panel->displayTitle();
    switch (panel->panelType()) {
    case PanelType::Terminal: obj["type"] = "terminal"; break;
    case PanelType::Browser:  obj["type"] = "browser"; break;
    case PanelType::Markdown: obj["type"] = "markdown"; break;
    }
    return obj;
}

} // namespace c11
