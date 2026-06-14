#include "MainWindow.h"
#include "panel/TerminalPanel.h"

#include <QApplication>
#include <QCloseEvent>
#include <QClipboard>
#include <QHBoxLayout>
#include <QMenuBar>
#include <QScreen>
#include <QStatusBar>
#include <QDebug>

namespace c11 {

MainWindow::MainWindow(C11Application &app, QWidget *parent)
    : QMainWindow(parent)
    , m_app(app)
{
    setWindowTitle("c11");
    resize(1200, 800);

    if (auto *screen = QApplication::primaryScreen()) {
        auto geom = screen->availableGeometry();
        move(geom.center() - QPoint(600, 400));
    }

    applyConfig();
    setupMenuBar();

    // Create workspace manager with initial workspace
    m_workspaceManager = new WorkspaceManager(app.ghosttyRuntime(), this);
    m_workspaceManager->addWorkspace("Terminal", app.ghosttyConfig().workingDirectory);

    // Build sidebar + workspace stack layout
    auto *centralWidget = new QWidget(this);
    auto *hbox = new QHBoxLayout(centralWidget);
    hbox->setContentsMargins(0, 0, 0, 0);
    hbox->setSpacing(0);

    m_sidebar = new SidebarWidget(*m_workspaceManager, centralWidget);
    hbox->addWidget(m_sidebar);

    m_workspaceStack = new WorkspaceStackWidget(*m_workspaceManager, centralWidget);
    hbox->addWidget(m_workspaceStack, 1); // stretch factor 1

    setCentralWidget(centralWidget);

    // Focus the initial workspace's terminal
    if (auto *ws = m_workspaceManager->selectedWorkspace()) {
        if (auto *panel = ws->focusedPanel()) {
            panel->focus();
        }
    }

    connect(&app, &C11Application::configReloaded, this, &MainWindow::applyConfig);
}

MainWindow::~MainWindow() = default;

void MainWindow::closeEvent(QCloseEvent *event)
{
    // Close all workspaces
    while (m_workspaceManager->count() > 0) {
        m_workspaceManager->removeWorkspace(0);
    }
    event->accept();
}

void MainWindow::changeEvent(QEvent *event)
{
    QMainWindow::changeEvent(event);
    if (event->type() == QEvent::ActivationChange) {
        m_app.ghosttyRuntime().setFocus(isActiveWindow());
    }
}

void MainWindow::setupMenuBar()
{
    auto *fileMenu = menuBar()->addMenu(tr("&File"));

    fileMenu->addAction(tr("New Workspace"), QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_T), [this]() {
        m_workspaceManager->addWorkspace();
    });

    fileMenu->addAction(tr("Close Workspace"), QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_W), [this]() {
        auto id = m_workspaceManager->selectedWorkspaceId();
        if (!id.isNull() && m_workspaceManager->count() > 1) {
            m_workspaceManager->removeWorkspace(id);
        }
    });

    fileMenu->addSeparator();

    fileMenu->addAction(tr("&Quit"), QKeySequence::Quit, []() {
        QApplication::quit();
    });

    auto *editMenu = menuBar()->addMenu(tr("&Edit"));

    editMenu->addAction(tr("&Copy"), QKeySequence::Copy, [this]() {
#ifndef C11_GHOSTTY_STUB
        auto *ws = m_workspaceManager->selectedWorkspace();
        if (!ws) return;
        auto *panel = dynamic_cast<TerminalPanel *>(ws->focusedPanel());
        if (!panel || !panel->ghosttyWidget()->hasSurface()) return;
        ghostty_text_s text{};
        if (ghostty_surface_read_selection(panel->ghosttyWidget()->surface(), &text)) {
            if (text.text && text.text_len > 0) {
                QApplication::clipboard()->setText(
                    QString::fromUtf8(text.text, static_cast<int>(text.text_len)));
            }
            ghostty_surface_free_text(panel->ghosttyWidget()->surface(), &text);
        }
#endif
    });

    editMenu->addAction(tr("&Paste"), QKeySequence::Paste, [this]() {
        QString text = QApplication::clipboard()->text();
        if (text.isEmpty()) return;
        auto *ws = m_workspaceManager->selectedWorkspace();
        if (!ws) return;
        auto *panel = dynamic_cast<TerminalPanel *>(ws->focusedPanel());
        if (panel) {
            panel->ghosttyWidget()->sendText(text);
        }
    });

    auto *viewMenu = menuBar()->addMenu(tr("&View"));

    viewMenu->addAction(tr("Toggle Sidebar"), QKeySequence(Qt::CTRL | Qt::Key_B), [this]() {
        m_sidebar->toggleVisibility();
    });

    viewMenu->addSeparator();

    viewMenu->addAction(tr("Split Right"), QKeySequence(Qt::CTRL | Qt::Key_Backslash), [this]() {
        auto *ws = m_workspaceManager->selectedWorkspace();
        if (!ws) return;
        ws->splitPanel(ws->focusedPanelId(), PaneLayout::Direction::Horizontal);
    });

    viewMenu->addAction(tr("Split Down"), QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_Backslash), [this]() {
        auto *ws = m_workspaceManager->selectedWorkspace();
        if (!ws) return;
        ws->splitPanel(ws->focusedPanelId(), PaneLayout::Direction::Vertical);
    });

    viewMenu->addSeparator();

    viewMenu->addAction(tr("Next Workspace"), QKeySequence(Qt::CTRL | Qt::Key_Tab), [this]() {
        m_workspaceManager->selectNextWorkspace();
    });

    viewMenu->addAction(tr("Previous Workspace"), QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_Tab), [this]() {
        m_workspaceManager->selectPreviousWorkspace();
    });

    viewMenu->addSeparator();

    viewMenu->addAction(tr("Reload Config"), QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_Comma), [this]() {
        m_app.reloadConfig();
    });

    auto *helpMenu = menuBar()->addMenu(tr("&Help"));
    helpMenu->addAction(tr("About c11"), [this]() {
        qDebug() << "c11 version" << C11_VERSION;
    });
}

void MainWindow::applyConfig()
{
    const auto &config = m_app.ghosttyConfig();

    QPalette pal = palette();
    pal.setColor(QPalette::Window, config.backgroundColor);
    setPalette(pal);

    statusBar()->hide();
}

} // namespace c11
