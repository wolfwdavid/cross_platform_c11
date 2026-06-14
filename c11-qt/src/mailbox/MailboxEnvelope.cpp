#include "MailboxEnvelope.h"

#include <QJsonDocument>
#include <QDateTime>
#include <QRandomGenerator>

namespace c11 {

QJsonObject MailboxEnvelope::toJson() const
{
    QJsonObject obj;
    obj["version"] = SchemaVersion;
    obj["id"] = id;
    obj["from"] = from;
    obj["ts"] = timestamp;
    obj["body"] = body;

    if (!to.isEmpty()) obj["to"] = to;
    if (!topic.isEmpty()) obj["topic"] = topic;
    if (!replyTo.isEmpty()) obj["reply_to"] = replyTo;
    if (!inReplyTo.isEmpty()) obj["in_reply_to"] = inReplyTo;
    if (urgent) obj["urgent"] = true;
    if (ttlSeconds > 0) obj["ttl_seconds"] = ttlSeconds;

    return obj;
}

MailboxEnvelope MailboxEnvelope::fromJson(const QJsonObject &obj)
{
    MailboxEnvelope env;
    env.id = obj.value("id").toString();
    env.from = obj.value("from").toString();
    env.timestamp = static_cast<qint64>(obj.value("ts").toDouble());
    env.body = obj.value("body").toString();
    env.to = obj.value("to").toString();
    env.topic = obj.value("topic").toString();
    env.replyTo = obj.value("reply_to").toString();
    env.inReplyTo = obj.value("in_reply_to").toString();
    env.urgent = obj.value("urgent").toBool();
    env.ttlSeconds = obj.value("ttl_seconds").toInt();
    return env;
}

QByteArray MailboxEnvelope::encode() const
{
    return QJsonDocument(toJson()).toJson(QJsonDocument::Compact);
}

MailboxEnvelope MailboxEnvelope::decode(const QByteArray &data)
{
    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (!doc.isObject()) return {};
    return fromJson(doc.object());
}

bool MailboxEnvelope::isValid() const
{
    return !id.isEmpty() && !from.isEmpty() && !body.isEmpty() && timestamp > 0;
}

QString MailboxEnvelope::generateId()
{
    // Simple ULID-like: timestamp (hex) + random
    qint64 ms = QDateTime::currentMSecsSinceEpoch();
    quint64 rand = QRandomGenerator::global()->generate64();
    return QString("%1%2")
        .arg(ms, 12, 16, QChar('0'))
        .arg(rand, 16, 16, QChar('0'));
}

} // namespace c11
