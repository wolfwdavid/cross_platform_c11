#pragma once

#include <QObject>
#include <QUuid>
#include <QJsonObject>
#include <QMap>
#include <QMutex>

namespace c11 {

// Per-pane metadata store. Thread-safe, 64KB cap per pane.
class PaneMetadataStore : public QObject {
    Q_OBJECT

public:
    static PaneMetadataStore &instance();

    bool setMetadata(const QUuid &workspaceId, const QUuid &paneId,
                     const QJsonObject &partial);
    QJsonObject getMetadata(const QUuid &workspaceId, const QUuid &paneId) const;
    void clearMetadata(const QUuid &workspaceId, const QUuid &paneId);
    void removePane(const QUuid &workspaceId, const QUuid &paneId);

    static constexpr int PayloadCapBytes = 64 * 1024;

signals:
    void metadataChanged(const QUuid &workspaceId, const QUuid &paneId);

private:
    PaneMetadataStore() = default;

    mutable QMutex m_mutex;
    // workspaceId -> paneId -> metadata
    QMap<QUuid, QMap<QUuid, QJsonObject>> m_metadata;
};

} // namespace c11
