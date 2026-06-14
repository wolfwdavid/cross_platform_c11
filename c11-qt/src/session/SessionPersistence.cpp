#include "SessionPersistence.h"
#include "platform/PlatformAbstraction.h"

#include <QDir>
#include <QFile>
#include <QJsonDocument>
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

    // Remove existing workspaces (the initial one)
    while (m_manager.count() > 0) {
        m_manager.removeWorkspace(0);
    }

    for (const auto &wsVal : workspaces) {
        QJsonObject wsObj = wsVal.toObject();
        QString title = wsObj.value("title").toString("Terminal");
        auto *ws = m_manager.addWorkspace(title);
        if (wsObj.contains("custom_title")) {
            ws->setCustomTitle(wsObj.value("custom_title").toString());
        }
        ws->setPinned(wsObj.value("pinned").toBool());
    }

    int selectedIdx = snapshot.value("selected_workspace_index").toInt(0);
    m_manager.selectWorkspace(selectedIdx);

    emit restored();
    return true;
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

    // Panel types
    QJsonArray panels;
    int count = 0;
    for (auto *panel : ws->allPanels()) {
        if (count >= MaxPanelsPerWorkspace) break;
        QJsonObject p;
        p["id"] = panel->id().toString(QUuid::WithoutBraces);
        p["title"] = panel->displayTitle();
        switch (panel->panelType()) {
        case PanelType::Terminal: p["type"] = "terminal"; break;
        case PanelType::Browser:  p["type"] = "browser"; break;
        case PanelType::Markdown: p["type"] = "markdown"; break;
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

} // namespace c11
