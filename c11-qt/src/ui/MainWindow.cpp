#include "MainWindow.h"
#include "panel/TerminalPanel.h"
#include "panel/BrowserPanel.h"
#include "panel/MarkdownPanel.h"

#include <QApplication>
#include <QCloseEvent>
#include <QClipboard>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QMenuBar>
#include <QScreen>
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
    auto *vbox = new QVBoxLayout(centralWidget);
    vbox->setContentsMargins(0, 0, 0, 0);
    vbox->setSpacing(0);

    // Main content: sidebar + workspace stack
    auto *contentWidget = new QWidget(centralWidget);
    auto *hbox = new QHBoxLayout(contentWidget);
    hbox->setContentsMargins(0, 0, 0, 0);
    hbox->setSpacing(0);

    m_sidebar = new SidebarWidget(*m_workspaceManager, contentWidget);
    hbox->addWidget(m_sidebar);

    m_workspaceStack = new WorkspaceStackWidget(*m_workspaceManager, contentWidget);
    hbox->addWidget(m_workspaceStack, 1);

    vbox->addWidget(contentWidget, 1);

    // Find overlay (floating, initially hidden)
    m_findOverlay = new FindOverlay(centralWidget);
    vbox->addWidget(m_findOverlay);

    // Status bar
    m_statusBar = new StatusBar(*m_workspaceManager, centralWidget);
    vbox->addWidget(m_statusBar);

    setCentralWidget(centralWidget);

    // Wire find overlay
    connect(m_findOverlay, &FindOverlay::searchRequested, this, [this](const QString &text) {
        auto *ws = m_workspaceManager->selectedWorkspace();
        if (!ws) return;
        auto *panel = ws->focusedPanel();
        if (!panel) return;
        if (auto *bp = dynamic_cast<BrowserPanel *>(panel)) {
            bp->findText(text);
        } else if (auto *mp = dynamic_cast<MarkdownPanel *>(panel)) {
            mp->findText(text);
        }
    });

    connect(m_findOverlay, &FindOverlay::nextRequested, this, [this]() {
        auto *ws = m_workspaceManager->selectedWorkspace();
        if (!ws) return;
        auto *panel = ws->focusedPanel();
        if (auto *bp = dynamic_cast<BrowserPanel *>(panel)) {
            bp->findText(m_findOverlay->searchText(), true);
        } else if (auto *mp = dynamic_cast<MarkdownPanel *>(panel)) {
            mp->findText(m_findOverlay->searchText(), true);
        }
    });

    connect(m_findOverlay, &FindOverlay::previousRequested, this, [this]() {
        auto *ws = m_workspaceManager->selectedWorkspace();
        if (!ws) return;
        auto *panel = ws->focusedPanel();
        if (auto *bp = dynamic_cast<BrowserPanel *>(panel)) {
            bp->findText(m_findOverlay->searchText(), false);
        } else if (auto *mp = dynamic_cast<MarkdownPanel *>(panel)) {
            mp->findText(m_findOverlay->searchText(), false);
        }
    });

    connect(m_findOverlay, &FindOverlay::closeRequested, this, [this]() {
        m_findOverlay->hide();
        auto *ws = m_workspaceManager->selectedWorkspace();
        if (ws && ws->focusedPanel()) {
            if (auto *bp = dynamic_cast<BrowserPanel *>(ws->focusedPanel())) {
                bp->clearFind();
            } else if (auto *mp = dynamic_cast<MarkdownPanel *>(ws->focusedPanel())) {
                mp->clearFind();
            }
            ws->focusedPanel()->focus();
        }
    });

    // Session persistence
    m_sessionPersistence = new SessionPersistence(*m_workspaceManager, this);
    m_sessionPersistence->startAutosave();

    // Theme manager
    ThemeManager::instance().loadThemes();
    connect(&ThemeManager::instance(), &ThemeManager::themeChanged, this, [this](const C11Theme &theme) {
        if (!theme.windowBackground.isValid()) return;
        QPalette pal = palette();
        pal.setColor(QPalette::Window, theme.windowBackground);
        setPalette(pal);
        setStyleSheet(ThemeManager::instance().generateStylesheet(theme));
    });

    // Start socket server
    m_app.startSocketServer(*m_workspaceManager);

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

void MainWindow::toggleFind()
{
    if (m_findOverlay->isVisible()) {
        m_findOverlay->hide();
    } else {
        m_findOverlay->show();
        m_findOverlay->focusSearchField();
    }
}

void MainWindow::setupMenuBar()
{
    // --- File menu ---
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

    fileMenu->addAction(tr("Open Browser"), QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_B), [this]() {
        auto *ws = m_workspaceManager->selectedWorkspace();
        if (!ws) return;
        auto *panel = ws->createBrowserPanel(QUrl("https://duckduckgo.com"));
        if (ws->layout() && !ws->focusedPanelId().isNull()) {
            ws->layout()->splitLeaf(ws->focusedPanelId(), panel->id(),
                                     PaneLayout::Direction::Horizontal);
        }
        ws->setFocusedPanelId(panel->id());
        emit ws->layoutChanged();
    });

    fileMenu->addAction(tr("Open Markdown..."), QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_M), [this]() {
        auto *ws = m_workspaceManager->selectedWorkspace();
        if (!ws) return;
        QString path = QFileDialog::getOpenFileName(this, tr("Open Markdown File"),
                                                     QString(), tr("Markdown (*.md *.markdown);;All (*)"));
        if (path.isEmpty()) return;
        auto *panel = ws->createMarkdownPanel(path);
        if (ws->layout() && !ws->focusedPanelId().isNull()) {
            ws->layout()->splitLeaf(ws->focusedPanelId(), panel->id(),
                                     PaneLayout::Direction::Horizontal);
        }
        ws->setFocusedPanelId(panel->id());
        emit ws->layoutChanged();
    });

    fileMenu->addSeparator();

    fileMenu->addAction(tr("&Quit"), QKeySequence::Quit, []() {
        QApplication::quit();
    });

    // --- Edit menu ---
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

    editMenu->addSeparator();

    editMenu->addAction(tr("&Find"), QKeySequence::Find, [this]() {
        toggleFind();
    });

    // --- View menu ---
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

    // --- Help menu ---
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
}

} // namespace c11
