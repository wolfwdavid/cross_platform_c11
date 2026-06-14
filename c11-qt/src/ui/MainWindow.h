#pragma once

#include "app/C11Application.h"
#include "ghostty/GhosttyWidget.h"

#include <QMainWindow>
#include <QVBoxLayout>

namespace c11 {

// Phase 0 MainWindow: a single GhosttyWidget filling the window.
// Phase 1 adds sidebar, workspace stack, and split panes.
class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(C11Application &app, QWidget *parent = nullptr);
    ~MainWindow() override;

protected:
    void closeEvent(QCloseEvent *event) override;
    void changeEvent(QEvent *event) override;

private:
    void setupMenuBar();
    void applyConfig();

    C11Application &m_app;
    GhosttyWidget *m_terminalWidget = nullptr;
};

} // namespace c11
