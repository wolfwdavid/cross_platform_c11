#pragma once

#include <QWidget>
#include <QListWidget>
#include <QUuid>

namespace c11 {

struct Notification {
    QUuid id;
    QString title;
    QString body;
    QUuid workspaceId;
    QUuid surfaceId;
    qint64 timestamp;
};

// Displays a list of notifications in the sidebar.
class NotificationsWidget : public QWidget {
    Q_OBJECT

public:
    explicit NotificationsWidget(QWidget *parent = nullptr);

    void addNotification(const Notification &notif);
    void clearNotifications();
    void clearNotification(const QUuid &id);
    int unreadCount() const { return m_unread; }

signals:
    void notificationClicked(const QUuid &workspaceId, const QUuid &surfaceId);
    void unreadCountChanged(int count);

private:
    QListWidget *m_list;
    QList<Notification> m_notifications;
    int m_unread = 0;
};

} // namespace c11
