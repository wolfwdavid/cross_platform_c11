#pragma once

#include "MailboxEnvelope.h"

#include <QObject>
#include <QUuid>
#include <QFileSystemWatcher>
#include <QMap>
#include <functional>

namespace c11 {

// Filesystem-based NDJSON mailbox for agent-to-agent messaging.
// Each workspace has an _outbox/ directory. Agents write envelopes there;
// the dispatcher reads, routes, and delivers to recipient surfaces.
class MailboxDispatcher : public QObject {
    Q_OBJECT

public:
    using DeliveryHandler = std::function<void(const MailboxEnvelope &envelope)>;

    explicit MailboxDispatcher(const QUuid &workspaceId, QObject *parent = nullptr);
    ~MailboxDispatcher() override;

    bool start();
    void stop();

    // Register a handler for a surface
    void registerHandler(const QUuid &surfaceId, DeliveryHandler handler);
    void unregisterHandler(const QUuid &surfaceId);

    // Send a message
    bool send(const MailboxEnvelope &envelope);

    // Mailbox directory for this workspace
    QString mailboxDir() const;
    QString outboxDir() const;

signals:
    void messageDelivered(const MailboxEnvelope &envelope);
    void messageDropped(const MailboxEnvelope &envelope, const QString &reason);

private slots:
    void onOutboxChanged(const QString &path);

private:
    void processOutbox();
    void deliverEnvelope(const MailboxEnvelope &envelope);

    QUuid m_workspaceId;
    QFileSystemWatcher *m_watcher = nullptr;
    QMap<QUuid, DeliveryHandler> m_handlers;
    bool m_running = false;
};

} // namespace c11
