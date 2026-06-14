#pragma once

#include "WorkspaceManager.h"
#include "PaneLayoutWidget.h"

#include <QStackedWidget>
#include <QMap>
#include <QUuid>

namespace c11 {

// QStackedWidget that shows one workspace's PaneLayoutWidget at a time.
class WorkspaceStackWidget : public QStackedWidget {
    Q_OBJECT

public:
    explicit WorkspaceStackWidget(WorkspaceManager &manager, QWidget *parent = nullptr);

private slots:
    void onWorkspaceAdded(Workspace *workspace, int index);
    void onWorkspaceRemoved(const QUuid &id, int index);
    void onSelectedWorkspaceChanged(const QUuid &id);

private:
    void rebuildLayout(Workspace *workspace);

    WorkspaceManager &m_manager;
    QMap<QUuid, PaneLayoutWidget *> m_layoutWidgets;
};

} // namespace c11
