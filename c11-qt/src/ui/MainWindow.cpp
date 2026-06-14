#include "MainWindow.h"

#include <QApplication>
#include <QCloseEvent>
#include <QClipboard>
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

    // Center on primary screen to avoid spawning offscreen
    if (auto *screen = QApplication::primaryScreen()) {
        auto geom = screen->availableGeometry();
        move(geom.center() - QPoint(600, 400));
    }

    applyConfig();
    setupMenuBar();

    // Create the terminal widget as the central widget
    m_terminalWidget = new GhosttyWidget(app.ghosttyRuntime(), this);
    setCentralWidget(m_terminalWidget);

    // Create the Ghostty surface
    const auto &config = app.ghosttyConfig();
    m_terminalWidget->createSurface(config.workingDirectory);
    m_terminalWidget->setFocus();

    connect(&app, &C11Application::configReloaded, this, &MainWindow::applyConfig);
}

MainWindow::~MainWindow() = default;

void MainWindow::closeEvent(QCloseEvent *event)
{
    m_terminalWidget->destroySurface();
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

    auto *newWindowAction = fileMenu->addAction(tr("New Window"), QKeySequence::New, [this]() {
        // Phase 1: proper multi-window support
        qDebug() << "New window requested";
    });
    Q_UNUSED(newWindowAction);

    fileMenu->addSeparator();

    fileMenu->addAction(tr("&Quit"), QKeySequence::Quit, []() {
        QApplication::quit();
    });

    auto *editMenu = menuBar()->addMenu(tr("&Edit"));

    editMenu->addAction(tr("&Copy"), QKeySequence::Copy, [this]() {
#ifndef C11_GHOSTTY_STUB
        if (m_terminalWidget && m_terminalWidget->hasSurface()) {
            ghostty_text_s text{};
            if (ghostty_surface_read_selection(m_terminalWidget->surface(), &text)) {
                if (text.text && text.text_len > 0) {
                    QApplication::clipboard()->setText(
                        QString::fromUtf8(text.text, static_cast<int>(text.text_len)));
                }
                ghostty_surface_free_text(m_terminalWidget->surface(), &text);
            }
        }
#endif
    });

    editMenu->addAction(tr("&Paste"), QKeySequence::Paste, [this]() {
        QString text = QApplication::clipboard()->text();
        if (!text.isEmpty() && m_terminalWidget) {
            m_terminalWidget->sendText(text);
        }
    });

    auto *viewMenu = menuBar()->addMenu(tr("&View"));

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

    // Apply background color to the window
    QPalette pal = palette();
    pal.setColor(QPalette::Window, config.backgroundColor);
    setPalette(pal);

    statusBar()->hide(); // Phase 2 adds a status bar
}

} // namespace c11
