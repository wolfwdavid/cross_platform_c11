#include "SurfaceMetadataStore.h"
#include <QDateTime>
#include <QJsonDocument>

namespace c11 {

const QString SurfaceMetadataStore::KeyRole = "role";
const QString SurfaceMetadataStore::KeyStatus = "status";
const QString SurfaceMetadataStore::KeyTask = "task";
const QString SurfaceMetadataStore::KeyModel = "model";
const QString SurfaceMetadataStore::KeyProgress = "progress";
const QString SurfaceMetadataStore::KeyTerminalType = "terminal_type";
const QString SurfaceMetadataStore::KeyTitle = "title";
const QString SurfaceMetadataStore::KeyLifecycleState = "lifecycle_state";
const QString SurfaceMetadataStore::KeyWorktree = "worktree";
const QString SurfaceMetadataStore::KeyBranch = "branch";

SurfaceMetadataStore &SurfaceMetadataStore::instance()
{
    static SurfaceMetadataStore store;
    return store;
}

SurfaceMetadataStore::WriteResult
SurfaceMetadataStore::setMetadata(const QUuid &surfaceId,
                                   const QJsonObject &partial,
                                   Source source,
                                   WriteMode mode)
{
    QMutexLocker lock(&m_mutex);

    // Size check
    auto &layers = m_layers[surfaceId];
    QJsonObject existing = layers.value(source);

    QJsonObject merged;
    if (mode == WriteMode::Merge) {
        merged = existing;
        for (auto it = partial.begin(); it != partial.end(); ++it) {
            merged[it.key()] = it.value();
        }
    } else {
        merged = partial;
    }

    QByteArray serialized = QJsonDocument(merged).toJson(QJsonDocument::Compact);
    if (serialized.size() > PayloadCapBytes) {
        return {false, "Payload exceeds 64KB cap"};
    }

    layers[source] = merged;

    // Update source records
    qint64 now = QDateTime::currentMSecsSinceEpoch();
    auto &sources = m_sources[surfaceId];
    for (auto it = partial.begin(); it != partial.end(); ++it) {
        auto existing_it = sources.find(it.key());
        if (existing_it == sources.end() || source <= existing_it->source) {
            sources[it.key()] = SourceRecord{source, now};
        }
    }

    lock.unlock();
    emit metadataChanged(surfaceId);
    return {true, {}};
}

QJsonObject SurfaceMetadataStore::getMetadata(const QUuid &surfaceId) const
{
    QMutexLocker lock(&m_mutex);

    QJsonObject result;
    auto layersIt = m_layers.find(surfaceId);
    if (layersIt == m_layers.end()) return result;

    // Merge all layers, lower Source values take precedence
    QMap<QString, Source> keySource;
    for (auto srcIt = layersIt->begin(); srcIt != layersIt->end(); ++srcIt) {
        Source src = srcIt.key();
        const QJsonObject &obj = srcIt.value();
        for (auto kvIt = obj.begin(); kvIt != obj.end(); ++kvIt) {
            auto existing = keySource.find(kvIt.key());
            if (existing == keySource.end() || src < *existing) {
                result[kvIt.key()] = kvIt.value();
                keySource[kvIt.key()] = src;
            }
        }
    }

    return result;
}

QJsonObject SurfaceMetadataStore::getMetadataWithSources(const QUuid &surfaceId) const
{
    QMutexLocker lock(&m_mutex);

    QJsonObject result;
    auto sourcesIt = m_sources.find(surfaceId);
    if (sourcesIt == m_sources.end()) return result;

    for (auto it = sourcesIt->begin(); it != sourcesIt->end(); ++it) {
        QJsonObject entry;
        entry["source"] = static_cast<int>(it->source);
        entry["timestamp"] = it->timestamp;
        result[it.key()] = entry;
    }
    return result;
}

void SurfaceMetadataStore::clearMetadata(const QUuid &surfaceId)
{
    QMutexLocker lock(&m_mutex);
    m_layers.remove(surfaceId);
    m_sources.remove(surfaceId);
    lock.unlock();
    emit metadataChanged(surfaceId);
}

void SurfaceMetadataStore::clearMetadata(const QUuid &surfaceId, const QString &key)
{
    QMutexLocker lock(&m_mutex);
    auto layersIt = m_layers.find(surfaceId);
    if (layersIt != m_layers.end()) {
        for (auto &layer : *layersIt) {
            layer.remove(key);
        }
    }
    auto sourcesIt = m_sources.find(surfaceId);
    if (sourcesIt != m_sources.end()) {
        sourcesIt->remove(key);
    }
    lock.unlock();
    emit metadataChanged(surfaceId);
}

void SurfaceMetadataStore::removeSurface(const QUuid &surfaceId)
{
    QMutexLocker lock(&m_mutex);
    m_layers.remove(surfaceId);
    m_sources.remove(surfaceId);
}

} // namespace c11
