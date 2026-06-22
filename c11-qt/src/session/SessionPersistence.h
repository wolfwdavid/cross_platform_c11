#pragma once

#include "workspace/WorkspaceManager.h"

#include "workspace/PaneLayout.h"

#include <QObject>
#include <QTimer>
#include <QJsonObject>
#include <QJsonArray>
#include <QHash>
#include <QString>
#include <QUuid>
#include <memory>

namespace c11 {

// Autosaves workspace state and restores on launch.
// Snapshot format: JSON file with workspace titles, panel types, and layout.
class SessionPersistence : public QObject {
    Q_OBJECT

public:
    static constexpr int AutosaveIntervalMs = 8000;
    static constexpr int MaxWorkspacesPerSnapshot = 128;
    static constexpr int MaxPanelsPerWorkspace = 512;

    explicit SessionPersistence(WorkspaceManager &manager, QObject *parent = nullptr);
    ~SessionPersistence() override;

    void startAutosave();
    void stopAutosave();

    // Manual save/restore
    bool save();
    bool restore();

    // Snapshot
    QJsonObject createSnapshot() const;
    bool restoreFromSnapshot(const QJsonObject &snapshot);

    // File path
    static QString snapshotPath();
    // Whether a session snapshot exists on disk (so the caller can decide whether
    // to seed a default workspace before restoring).
    static bool hasSnapshot();

signals:
    void saved();
    void restored();

private:
    void onAutosave();
    QJsonObject workspaceToSnapshot(const Workspace *ws) const;
    QJsonObject layoutToSnapshot(const PaneLayout *layout) const;

    // Restore one workspace (panels + layout) from its snapshot object.
    void restoreWorkspace(const QJsonObject &wsObj);
    // Rebuild a layout tree from its snapshot, mapping snapshot panel ids to the
    // ids of the freshly recreated panels. Returns null if no leaf resolves.
    static std::unique_ptr<PaneLayout> layoutFromSnapshot(
        const QJsonObject &node, const QHash<QString, QUuid> &idMap);

    WorkspaceManager &m_manager;
    QTimer m_autosaveTimer;
};

} // namespace c11
