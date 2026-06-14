#pragma once

#include "app/C11Application.h"
#include "workspace/WorkspaceManager.h"
#include "workspace/WorkspaceStackWidget.h"
#include "sidebar/SidebarWidget.h"

#include <QMainWindow>

namespace c11 {

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(C11Application &app, QWidget *parent = nullptr);
    ~MainWindow() override;

    WorkspaceManager &workspaceManager() { return *m_workspaceManager; }

protected:
    void closeEvent(QCloseEvent *event) override;
    void changeEvent(QEvent *event) override;

private:
    void setupMenuBar();
    void applyConfig();

    C11Application &m_app;
    WorkspaceManager *m_workspaceManager = nullptr;
    SidebarWidget *m_sidebar = nullptr;
    WorkspaceStackWidget *m_workspaceStack = nullptr;
};

} // namespace c11
