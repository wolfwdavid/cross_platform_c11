#include "NotificationsWidget.h"

#include <QVBoxLayout>
#include <QDateTime>

namespace c11 {

NotificationsWidget::NotificationsWidget(QWidget *parent)
    : QWidget(parent)
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    m_list = new QListWidget(this);
    m_list->setFrameShape(QFrame::NoFrame);
    m_list->setMaximumHeight(150);
    layout->addWidget(m_list);

    connect(m_list, &QListWidget::itemClicked, this, [this](QListWidgetItem *item) {
        int idx = m_list->row(item);
        if (idx >= 0 && idx < m_notifications.size()) {
            const auto &n = m_notifications[idx];
            emit notificationClicked(n.workspaceId, n.surfaceId);
        }
    });

    hide();
}

void NotificationsWidget::addNotification(const Notification &notif)
{
    m_notifications.prepend(notif);

    auto *item = new QListWidgetItem();
    QString timeStr = QDateTime::fromMSecsSinceEpoch(notif.timestamp)
                          .toString("hh:mm:ss");
    item->setText(QString("[%1] %2").arg(timeStr, notif.title));
    item->setToolTip(notif.body);
    m_list->insertItem(0, item);

    // Cap at 50 notifications
    while (m_notifications.size() > 50) {
        m_notifications.removeLast();
        delete m_list->takeItem(m_list->count() - 1);
    }

    m_unread++;
    emit unreadCountChanged(m_unread);
    show();
}

void NotificationsWidget::clearNotifications()
{
    m_notifications.clear();
    m_list->clear();
    m_unread = 0;
    emit unreadCountChanged(0);
    hide();
}

void NotificationsWidget::clearNotification(const QUuid &id)
{
    for (int i = 0; i < m_notifications.size(); ++i) {
        if (m_notifications[i].id == id) {
            m_notifications.removeAt(i);
            delete m_list->takeItem(i);
            if (m_unread > 0) {
                m_unread--;
                emit unreadCountChanged(m_unread);
            }
            break;
        }
    }
    if (m_notifications.isEmpty()) hide();
}

} // namespace c11
