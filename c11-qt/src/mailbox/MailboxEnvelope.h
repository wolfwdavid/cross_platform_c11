#pragma once

#include <QJsonObject>
#include <QString>
#include <QUuid>

namespace c11 {

// NDJSON envelope for agent-to-agent messaging.
struct MailboxEnvelope {
    static constexpr int SchemaVersion = 1;
    static constexpr int MaxBodyBytes = 4096;

    // Required fields
    QString id;       // ULID
    QString from;     // sender surface ID
    qint64 timestamp; // ms since epoch
    QString body;

    // Optional fields
    QString to;       // recipient surface ID (empty = broadcast)
    QString topic;
    QString replyTo;
    QString inReplyTo;
    bool urgent = false;
    int ttlSeconds = 0;

    // Serialization
    QJsonObject toJson() const;
    static MailboxEnvelope fromJson(const QJsonObject &obj);
    QByteArray encode() const;
    static MailboxEnvelope decode(const QByteArray &data);

    bool isValid() const;

    // Generate a unique ID
    static QString generateId();
};

} // namespace c11
