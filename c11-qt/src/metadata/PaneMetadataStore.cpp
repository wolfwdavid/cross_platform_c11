#include "PaneMetadataStore.h"
#include <QJsonDocument>

namespace c11 {

PaneMetadataStore &PaneMetadataStore::instance()
{
    static PaneMetadataStore store;
    return store;
}

bool PaneMetadataStore::setMetadata(const QUuid &workspaceId, const QUuid &paneId,
                                     const QJsonObject &partial)
{
    QMutexLocker lock(&m_mutex);

    auto &paneMap = m_metadata[workspaceId];
    QJsonObject existing = paneMap.value(paneId);

    for (auto it = partial.begin(); it != partial.end(); ++it) {
        existing[it.key()] = it.value();
    }

    QByteArray serialized = QJsonDocument(existing).toJson(QJsonDocument::Compact);
    if (serialized.size() > PayloadCapBytes) return false;

    paneMap[paneId] = existing;

    lock.unlock();
    emit metadataChanged(workspaceId, paneId);
    return true;
}

QJsonObject PaneMetadataStore::getMetadata(const QUuid &workspaceId, const QUuid &paneId) const
{
    QMutexLocker lock(&m_mutex);
    return m_metadata.value(workspaceId).value(paneId);
}

void PaneMetadataStore::clearMetadata(const QUuid &workspaceId, const QUuid &paneId)
{
    QMutexLocker lock(&m_mutex);
    auto it = m_metadata.find(workspaceId);
    if (it != m_metadata.end()) {
        it->remove(paneId);
    }
    lock.unlock();
    emit metadataChanged(workspaceId, paneId);
}

void PaneMetadataStore::removePane(const QUuid &workspaceId, const QUuid &paneId)
{
    QMutexLocker lock(&m_mutex);
    auto it = m_metadata.find(workspaceId);
    if (it != m_metadata.end()) {
        it->remove(paneId);
    }
}

} // namespace c11
