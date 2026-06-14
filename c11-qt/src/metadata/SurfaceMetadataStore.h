#pragma once

#include <QObject>
#include <QUuid>
#include <QJsonObject>
#include <QMap>
#include <QMutex>
#include <QString>

namespace c11 {

// 5-tier precedence metadata store for terminal surfaces.
// Thread-safe. 64KB per-surface JSON cap.
class SurfaceMetadataStore : public QObject {
    Q_OBJECT

public:
    enum class Source {
        Explicit = 0,  // Direct socket set-metadata
        Declare  = 1,  // Agent self-declaration
        Osc      = 2,  // Terminal OSC sequence
        Derived  = 3,  // Computed from process/env
        Heuristic = 4  // Guessed from behavior
    };

    enum class WriteMode {
        Merge,    // Merge keys into existing
        Replace   // Replace all keys from this source
    };

    struct WriteResult {
        bool ok = true;
        QString error;
    };

    static SurfaceMetadataStore &instance();

    WriteResult setMetadata(const QUuid &surfaceId,
                            const QJsonObject &partial,
                            Source source = Source::Explicit,
                            WriteMode mode = WriteMode::Merge);

    QJsonObject getMetadata(const QUuid &surfaceId) const;
    QJsonObject getMetadataWithSources(const QUuid &surfaceId) const;

    void clearMetadata(const QUuid &surfaceId);
    void clearMetadata(const QUuid &surfaceId, const QString &key);
    void removeSurface(const QUuid &surfaceId);

    // Canonical metadata keys
    static const QString KeyRole;
    static const QString KeyStatus;
    static const QString KeyTask;
    static const QString KeyModel;
    static const QString KeyProgress;
    static const QString KeyTerminalType;
    static const QString KeyTitle;
    static const QString KeyLifecycleState;
    static const QString KeyWorktree;
    static const QString KeyBranch;

    static constexpr int PayloadCapBytes = 64 * 1024;

signals:
    void metadataChanged(const QUuid &surfaceId);

private:
    SurfaceMetadataStore() = default;

    struct SourceRecord {
        Source source;
        qint64 timestamp;
    };

    // Resolve effective value: lowest Source enum wins (highest precedence)
    QJsonValue resolveValue(const QUuid &surfaceId, const QString &key) const;

    mutable QMutex m_mutex;
    // surfaceId -> source -> key -> value
    QMap<QUuid, QMap<Source, QJsonObject>> m_layers;
    // surfaceId -> key -> SourceRecord
    QMap<QUuid, QMap<QString, SourceRecord>> m_sources;
};

} // namespace c11
