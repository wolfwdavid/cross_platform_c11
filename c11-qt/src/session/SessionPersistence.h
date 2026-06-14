#pragma once

#include "workspace/WorkspaceManager.h"

#include <QObject>
#include <QTimer>
#include <QJsonObject>
#include <QJsonArray>
#include <QString>

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

signals:
    void saved();
    void restored();

private:
    void onAutosave();
    QJsonObject workspaceToSnapshot(const Workspace *ws) const;
    QJsonObject layoutToSnapshot(const PaneLayout *layout) const;

    WorkspaceManager &m_manager;
    QTimer m_autosaveTimer;
};

} // namespace c11
