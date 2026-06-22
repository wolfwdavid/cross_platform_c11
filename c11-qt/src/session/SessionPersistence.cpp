#include "SessionPersistence.h"
#include "platform/PlatformAbstraction.h"
#include "workspace/Workspace.h"
#include "panel/BrowserPanel.h"
#include "panel/MarkdownPanel.h"

#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QUrl>
#include <QDebug>

namespace c11 {

SessionPersistence::SessionPersistence(WorkspaceManager &manager, QObject *parent)
    : QObject(parent)
    , m_manager(manager)
{
    m_autosaveTimer.setInterval(AutosaveIntervalMs);
    connect(&m_autosaveTimer, &QTimer::timeout, this, &SessionPersistence::onAutosave);
}

SessionPersistence::~SessionPersistence()
{
    if (m_autosaveTimer.isActive()) {
        save(); // Final save on destruction
    }
}

void SessionPersistence::startAutosave()
{
    m_autosaveTimer.start();
}

void SessionPersistence::stopAutosave()
{
    m_autosaveTimer.stop();
}

bool SessionPersistence::save()
{
    QJsonObject snapshot = createSnapshot();
    QJsonDocument doc(snapshot);

    QString path = snapshotPath();
    QDir().mkpath(QFileInfo(path).absolutePath());

    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        qWarning() << "SessionPersistence: cannot write" << path;
        return false;
    }

    file.write(doc.toJson(QJsonDocument::Indented));
    emit saved();
    return true;
}

bool SessionPersistence::restore()
{
    QString path = snapshotPath();
    QFile file(path);
    if (!file.exists() || !file.open(QIODevice::ReadOnly)) {
        return false;
    }

    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &err);
    if (err.error != QJsonParseError::NoError || !doc.isObject()) {
        qWarning() << "SessionPersistence: invalid snapshot" << err.errorString();
        return false;
    }

    return restoreFromSnapshot(doc.object());
}

QJsonObject SessionPersistence::createSnapshot() const
{
    QJsonObject snapshot;
    snapshot["schema_version"] = 1;
    snapshot["selected_workspace_index"] = m_manager.selectedIndex();

    QJsonArray workspaces;
    int count = 0;
    for (auto *ws : m_manager.workspaces()) {
        if (count >= MaxWorkspacesPerSnapshot) break;
        workspaces.append(workspaceToSnapshot(ws));
        count++;
    }
    snapshot["workspaces"] = workspaces;

    return snapshot;
}

bool SessionPersistence::restoreFromSnapshot(const QJsonObject &snapshot)
{
    if (snapshot.value("schema_version").toInt() != 1) return false;

    QJsonArray workspaces = snapshot.value("workspaces").toArray();
    if (workspaces.isEmpty()) return false;

    // Remove existing workspaces (the initial one).
    while (m_manager.count() > 0) {
        m_manager.removeWorkspace(0);
    }

    for (const auto &wsVal : workspaces) {
        restoreWorkspace(wsVal.toObject());
    }

    int selectedIdx = snapshot.value("selected_workspace_index").toInt(0);
    m_manager.selectWorkspace(selectedIdx);

    emit restored();
    return true;
}

void SessionPersistence::restoreWorkspace(const QJsonObject &wsObj)
{
    const QString title = wsObj.value("title").toString("Terminal");

    // Create the workspace empty so we can rebuild exactly the saved panels +
    // layout without spawning a throwaway default terminal.
    auto *ws = m_manager.addWorkspace(title, {}, /*withInitialPanel=*/false);
    if (wsObj.contains("custom_title")) {
        ws->setCustomTitle(wsObj.value("custom_title").toString());
    }
    ws->setPinned(wsObj.value("pinned").toBool());

    // Recreate panels, recording snapshot-id -> new-id so the layout tree (which
    // references the old ids) can be remapped.
    QHash<QString, QUuid> idMap;
    const QJsonArray panels = wsObj.value("panels").toArray();
    for (const auto &pVal : panels) {
        const QJsonObject p = pVal.toObject();
        const QString oldId = p.value("id").toString();
        const QString type = p.value("type").toString("terminal");
        Panel *panel = nullptr;
        if (type == "browser") {
            panel = ws->createBrowserPanel(
                QUrl(p.value("url").toString("about:blank")));
        } else if (type == "markdown") {
            panel = ws->createMarkdownPanel(p.value("file_path").toString());
        } else {
            panel = ws->createTerminalPanel();
        }
        if (panel && !oldId.isEmpty()) idMap.insert(oldId, panel->id());
    }

    // A workspace must always have at least one panel.
    if (idMap.isEmpty()) {
        ws->createTerminalPanel();
        return;
    }

    // Rebuild the split tree from the snapshot; fall back to a single leaf on the
    // first panel if the saved layout can't be resolved.
    auto tree = layoutFromSnapshot(wsObj.value("layout").toObject(), idMap);
    if (!tree) tree = PaneLayout::makeLeaf(*idMap.cbegin());
    ws->setLayout(std::move(tree));

    const QUuid focused = idMap.value(wsObj.value("focused_panel_id").toString());
    if (!focused.isNull()) ws->setFocusedPanelId(focused);
}

std::unique_ptr<PaneLayout> SessionPersistence::layoutFromSnapshot(
    const QJsonObject &node, const QHash<QString, QUuid> &idMap)
{
    const QString type = node.value("type").toString();
    if (type == "leaf") {
        const QUuid id = idMap.value(node.value("panel_id").toString());
        if (id.isNull()) return nullptr; // panel was dropped (e.g. cap exceeded)
        return PaneLayout::makeLeaf(id);
    }
    if (type == "split") {
        auto first = layoutFromSnapshot(node.value("first").toObject(), idMap);
        auto second = layoutFromSnapshot(node.value("second").toObject(), idMap);
        // If a child is missing, collapse to the surviving side.
        if (!first) return second;
        if (!second) return first;
        const auto dir = node.value("direction").toString() == "horizontal"
                             ? PaneLayout::Direction::Horizontal
                             : PaneLayout::Direction::Vertical;
        return PaneLayout::makeSplit(dir, std::move(first), std::move(second),
                                     node.value("ratio").toDouble(0.5));
    }
    return nullptr;
}

void SessionPersistence::onAutosave()
{
    save();
}

QJsonObject SessionPersistence::workspaceToSnapshot(const Workspace *ws) const
{
    QJsonObject obj;
    obj["id"] = ws->id().toString(QUuid::WithoutBraces);
    obj["title"] = ws->effectiveTitle();
    if (!ws->customTitle().isEmpty()) {
        obj["custom_title"] = ws->customTitle();
    }
    obj["pinned"] = ws->isPinned();
    obj["panel_count"] = ws->panelCount();
    obj["focused_panel_id"] = ws->focusedPanelId().toString(QUuid::WithoutBraces);

    // Panel types (plus the per-type state needed to recreate them: a browser's
    // URL, a markdown panel's file path).
    QJsonArray panels;
    int count = 0;
    for (auto *panel : ws->allPanels()) {
        if (count >= MaxPanelsPerWorkspace) break;
        QJsonObject p;
        p["id"] = panel->id().toString(QUuid::WithoutBraces);
        p["title"] = panel->displayTitle();
        switch (panel->panelType()) {
        case PanelType::Terminal:
            p["type"] = "terminal";
            break;
        case PanelType::Browser:
            p["type"] = "browser";
            if (auto *bp = qobject_cast<const BrowserPanel *>(panel))
                p["url"] = bp->currentUrl().toString();
            break;
        case PanelType::Markdown:
            p["type"] = "markdown";
            if (auto *mp = qobject_cast<const MarkdownPanel *>(panel))
                p["file_path"] = mp->filePath();
            break;
        }
        panels.append(p);
        count++;
    }
    obj["panels"] = panels;

    // Layout tree
    if (ws->layout()) {
        obj["layout"] = layoutToSnapshot(ws->layout());
    }

    return obj;
}

QJsonObject SessionPersistence::layoutToSnapshot(const PaneLayout *layout) const
{
    QJsonObject obj;
    if (layout->isLeaf()) {
        obj["type"] = "leaf";
        obj["panel_id"] = layout->leaf().panelId.toString(QUuid::WithoutBraces);
    } else {
        const auto &s = layout->split();
        obj["type"] = "split";
        obj["direction"] = (s.direction == PaneLayout::Direction::Horizontal)
                               ? "horizontal" : "vertical";
        obj["ratio"] = s.ratio;
        obj["first"] = layoutToSnapshot(s.first.get());
        obj["second"] = layoutToSnapshot(s.second.get());
    }
    return obj;
}

QString SessionPersistence::snapshotPath()
{
    return platform::appDataDir() + "/session.json";
}

bool SessionPersistence::hasSnapshot()
{
    return QFile::exists(snapshotPath());
}

} // namespace c11
