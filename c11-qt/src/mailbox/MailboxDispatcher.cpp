#include "MailboxDispatcher.h"
#include "platform/PlatformAbstraction.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QDebug>

namespace c11 {

MailboxDispatcher::MailboxDispatcher(const QUuid &workspaceId, QObject *parent)
    : QObject(parent)
    , m_workspaceId(workspaceId)
{
}

MailboxDispatcher::~MailboxDispatcher()
{
    stop();
}

bool MailboxDispatcher::start()
{
    if (m_running) return true;

    // Create mailbox directories
    QDir().mkpath(outboxDir());

    m_watcher = new QFileSystemWatcher(this);
    m_watcher->addPath(outboxDir());
    connect(m_watcher, &QFileSystemWatcher::directoryChanged,
            this, &MailboxDispatcher::onOutboxChanged);

    m_running = true;

    // Process any existing outbox messages
    processOutbox();
    return true;
}

void MailboxDispatcher::stop()
{
    if (!m_running) return;
    m_running = false;

    delete m_watcher;
    m_watcher = nullptr;
}

void MailboxDispatcher::registerHandler(const QUuid &surfaceId, DeliveryHandler handler)
{
    m_handlers[surfaceId] = std::move(handler);
}

void MailboxDispatcher::unregisterHandler(const QUuid &surfaceId)
{
    m_handlers.remove(surfaceId);
}

bool MailboxDispatcher::send(const MailboxEnvelope &envelope)
{
    if (!envelope.isValid()) return false;

    QByteArray data = envelope.encode();
    if (data.size() > MailboxEnvelope::MaxBodyBytes * 2) return false;

    QString filename = envelope.id + ".json";
    QString filepath = outboxDir() + "/" + filename;

    QFile file(filepath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        qWarning() << "MailboxDispatcher: cannot write" << filepath;
        return false;
    }

    file.write(data);
    file.write("\n");
    return true;
}

QString MailboxDispatcher::mailboxDir() const
{
    return platform::appDataDir() + "/mailbox/"
           + m_workspaceId.toString(QUuid::WithoutBraces);
}

QString MailboxDispatcher::outboxDir() const
{
    return mailboxDir() + "/_outbox";
}

void MailboxDispatcher::onOutboxChanged(const QString &path)
{
    Q_UNUSED(path);
    processOutbox();
}

void MailboxDispatcher::processOutbox()
{
    QDir outbox(outboxDir());
    auto entries = outbox.entryInfoList({"*.json"}, QDir::Files, QDir::Time);

    for (const auto &entry : entries) {
        QFile file(entry.absoluteFilePath());
        if (!file.open(QIODevice::ReadOnly)) continue;

        QByteArray data = file.readAll();
        file.close();

        MailboxEnvelope envelope = MailboxEnvelope::decode(data);
        if (!envelope.isValid()) {
            file.remove();
            continue;
        }

        deliverEnvelope(envelope);

        // Remove processed message
        QFile::remove(entry.absoluteFilePath());
    }
}

void MailboxDispatcher::deliverEnvelope(const MailboxEnvelope &envelope)
{
    if (!envelope.to.isEmpty()) {
        // Targeted delivery
        QUuid recipientId = QUuid::fromString(envelope.to);
        auto it = m_handlers.find(recipientId);
        if (it != m_handlers.end()) {
            it.value()(envelope);
            emit messageDelivered(envelope);
        } else {
            emit messageDropped(envelope, "No handler for recipient " + envelope.to);
        }
    } else {
        // Broadcast to all handlers
        bool delivered = false;
        for (auto it = m_handlers.begin(); it != m_handlers.end(); ++it) {
            // Don't deliver to sender
            if (it.key().toString(QUuid::WithoutBraces) != envelope.from) {
                it.value()(envelope);
                delivered = true;
            }
        }
        if (delivered) {
            emit messageDelivered(envelope);
        } else {
            emit messageDropped(envelope, "No recipients");
        }
    }
}

} // namespace c11
