#include "GhosttyRuntime.h"

#include <QApplication>
#include <QClipboard>
#include <QMimeData>
#include <QDebug>

namespace c11 {

GhosttyRuntime::GhosttyRuntime(QObject *parent)
    : QObject(parent)
{
    connect(&m_tickTimer, &QTimer::timeout, this, &GhosttyRuntime::tick);
    connect(this, &GhosttyRuntime::wakeupRequested, this, &GhosttyRuntime::tick,
            Qt::QueuedConnection);
}

GhosttyRuntime::~GhosttyRuntime()
{
    shutdown();
}

bool GhosttyRuntime::initialize(const GhosttyConfig &config)
{
#ifdef C11_GHOSTTY_STUB
    qWarning() << "GhosttyRuntime: stub mode, no terminal rendering";
    return true;
#else
    // Initialize the Ghostty library
    if (ghostty_init(0, nullptr) != GHOSTTY_SUCCESS) {
        qCritical() << "Failed to initialize Ghostty";
        return false;
    }

    // Create Ghostty config
    m_ghosttyConfig = ghostty_config_new();
    if (!m_ghosttyConfig) {
        qCritical() << "Failed to create Ghostty config";
        return false;
    }

    ghostty_config_load_default_files(m_ghosttyConfig);
    ghostty_config_load_recursive_files(m_ghosttyConfig);
    ghostty_config_finalize(m_ghosttyConfig);

    // Set up runtime callbacks
    ghostty_runtime_config_s rtConfig{};
    rtConfig.userdata = this;
    rtConfig.supports_selection_clipboard = false;
    rtConfig.wakeup_cb = &GhosttyRuntime::onWakeup;
    rtConfig.action_cb = &GhosttyRuntime::onAction;
    rtConfig.read_clipboard_cb = &GhosttyRuntime::onReadClipboard;
    rtConfig.confirm_read_clipboard_cb = &GhosttyRuntime::onConfirmReadClipboard;
    rtConfig.write_clipboard_cb = &GhosttyRuntime::onWriteClipboard;
    rtConfig.close_surface_cb = &GhosttyRuntime::onCloseSurface;

    m_app = ghostty_app_new(&rtConfig, m_ghosttyConfig);
    if (!m_app) {
        qCritical() << "Failed to create Ghostty app";
        ghostty_config_free(m_ghosttyConfig);
        m_ghosttyConfig = nullptr;
        return false;
    }

    // Ghostty's event loop is driven by its wakeup callback (onWakeup ->
    // wakeupRequested -> tick): libghostty calls it whenever it has work pending
    // (PTY output, cursor blink, its own libxev timers). That is the correct,
    // event-driven path — the rendering happens on ghostty's own render thread.
    //
    // We keep only a slow, coarse heartbeat as a safety net in case a wakeup is
    // ever missed during a reparent/resize. A free-running 16ms PreciseTimer was
    // wrong here: it ticked 60x/sec doing nothing most frames, and PreciseTimer
    // forces the global Windows timer resolution to 1ms (timeBeginPeriod), which
    // raises power draw and adds scheduling jitter felt as lag.
    m_tickTimer.setTimerType(Qt::CoarseTimer);
    m_tickTimer.start(100);

    qDebug() << "GhosttyRuntime initialized";
    return true;
#endif
}

void GhosttyRuntime::shutdown()
{
    m_tickTimer.stop();

#ifndef C11_GHOSTTY_STUB
    if (m_app) {
        ghostty_app_free(m_app);
        m_app = nullptr;
    }
    if (m_ghosttyConfig) {
        ghostty_config_free(m_ghosttyConfig);
        m_ghosttyConfig = nullptr;
    }
#endif
}

void GhosttyRuntime::updateConfig(const GhosttyConfig &config)
{
#ifndef C11_GHOSTTY_STUB
    if (!m_app) return;

    auto newConfig = ghostty_config_new();
    if (!newConfig) return;

    ghostty_config_load_default_files(newConfig);
    ghostty_config_load_recursive_files(newConfig);
    ghostty_config_finalize(newConfig);

    ghostty_app_update_config(m_app, newConfig);

    if (m_ghosttyConfig) {
        ghostty_config_free(m_ghosttyConfig);
    }
    m_ghosttyConfig = newConfig;
#endif
}

void GhosttyRuntime::setFocus(bool focused)
{
#ifndef C11_GHOSTTY_STUB
    if (m_app) {
        ghostty_app_set_focus(m_app, focused);
    }
#endif
}

void GhosttyRuntime::tick()
{
#ifndef C11_GHOSTTY_STUB
    if (m_app) {
        ghostty_app_tick(m_app);
    }
#endif
}

// --- Static callback trampolines ---

void GhosttyRuntime::onWakeup(void *userdata)
{
    auto *self = static_cast<GhosttyRuntime *>(userdata);
    // Signal is thread-safe via QueuedConnection
    emit self->wakeupRequested();
}

bool GhosttyRuntime::onAction(ghostty_app_t app, ghostty_target_s target, ghostty_action_s action)
{
#ifndef C11_GHOSTTY_STUB
    auto *self = static_cast<GhosttyRuntime *>(ghostty_app_userdata(app));
    if (!self) return false;
    return self->handleAction(target, action);
#else
    Q_UNUSED(app); Q_UNUSED(target); Q_UNUSED(action);
    return false;
#endif
}

void GhosttyRuntime::onReadClipboard(void *userdata, ghostty_clipboard_e location, void *state)
{
    auto *self = static_cast<GhosttyRuntime *>(userdata);
    self->handleReadClipboard(location, state);
}

void GhosttyRuntime::onConfirmReadClipboard(void *userdata, const char *contents,
                                             void *state, ghostty_clipboard_request_e request)
{
    Q_UNUSED(request);
    auto *self = static_cast<GhosttyRuntime *>(userdata);
    // Auto-confirm clipboard reads
    self->handleReadClipboard(GHOSTTY_CLIPBOARD_STANDARD, state);
}

void GhosttyRuntime::onWriteClipboard(void *userdata, ghostty_clipboard_e location,
                                       const ghostty_clipboard_content_s *content,
                                       size_t count, bool confirm)
{
    Q_UNUSED(confirm);
    auto *self = static_cast<GhosttyRuntime *>(userdata);
    self->handleWriteClipboard(location, content, count);
}

void GhosttyRuntime::onCloseSurface(void *userdata, bool processAlive)
{
    Q_UNUSED(userdata);
    Q_UNUSED(processAlive);
    // Will be wired in Phase 1 when we have surface tracking
}

// --- Instance handlers ---

bool GhosttyRuntime::handleAction(ghostty_target_s target, ghostty_action_s action)
{
    emit actionReceived(target, action);

    switch (action.tag) {
    case GHOSTTY_ACTION_SET_TITLE:
    case GHOSTTY_ACTION_PWD:
    case GHOSTTY_ACTION_MOUSE_SHAPE:
    case GHOSTTY_ACTION_MOUSE_VISIBILITY:
    case GHOSTTY_ACTION_CELL_SIZE:
    case GHOSTTY_ACTION_RENDERER_HEALTH:
    case GHOSTTY_ACTION_RENDER:
        return true;

    case GHOSTTY_ACTION_NEW_WINDOW:
    case GHOSTTY_ACTION_NEW_TAB:
    case GHOSTTY_ACTION_NEW_SPLIT:
    case GHOSTTY_ACTION_CLOSE_TAB:
        // Will be handled in Phase 1
        return false;

    default:
        return false;
    }
}

void GhosttyRuntime::handleReadClipboard(ghostty_clipboard_e location, void *state)
{
    Q_UNUSED(location);
    Q_UNUSED(state);
#ifndef C11_GHOSTTY_STUB
    auto *clipboard = QApplication::clipboard();
    if (!clipboard) return;

    QString text = clipboard->text();
    if (text.isEmpty()) return;

    QByteArray utf8 = text.toUtf8();
    // The surface that requested the clipboard read is encoded in state.
    // Complete the request by sending the clipboard content back.
    auto surface = static_cast<ghostty_surface_t>(state);
    if (surface) {
        ghostty_surface_complete_clipboard_request(surface, utf8.constData(), nullptr, true);
    }
#endif
}

void GhosttyRuntime::handleWriteClipboard(ghostty_clipboard_e location,
                                            const ghostty_clipboard_content_s *content,
                                            size_t count)
{
    Q_UNUSED(location);
    if (count == 0 || !content) return;

    auto *clipboard = QApplication::clipboard();
    if (!clipboard) return;

    // Use the first content entry's data
    if (content[0].data) {
        clipboard->setText(QString::fromUtf8(content[0].data));
    }
}

} // namespace c11
