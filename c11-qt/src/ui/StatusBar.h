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
    // (Re)subscribe to the currently selected workspace's panel/title signals so
    // the counter tracks splits/closes. Safe to call repeatedly.
    void connectToSelectedWorkspace();

    WorkspaceManager &m_manager;
    QLabel *m_workspaceLabel;
    QLabel *m_paneLabel;
    QLabel *m_infoLabel;
};

} // namespace c11
