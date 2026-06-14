#pragma once

#include "workspace/WorkspaceManager.h"

#include <QWidget>
#include <QLabel>

namespace c11 {

// Status bar at the bottom of the window showing workspace info.
class StatusBar : public QWidget {
    Q_OBJECT

public:
    explicit StatusBar(WorkspaceManager &manager, QWidget *parent = nullptr);

    void updateStatus();

private slots:
    void onSelectionChanged(const QUuid &id);

private:
    WorkspaceManager &m_manager;
    QLabel *m_workspaceLabel;
    QLabel *m_paneLabel;
    QLabel *m_infoLabel;
};

} // namespace c11
