#include "GhosttyWidget.h"

#include <QResizeEvent>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QInputMethodEvent>
#include <QApplication>
#include <QDebug>
#include <QTimer>

#ifdef Q_OS_MACOS
#include "GhosttyNSViewBridge.h"
#else
#include "GhosttyQtPlatform.h"
#endif

namespace c11 {

GhosttyWidget::GhosttyWidget(GhosttyRuntime &runtime, QWidget *parent)
    : QWidget(parent)
    , m_runtime(runtime)
{
    setFocusPolicy(Qt::StrongFocus);
    setAttribute(Qt::WA_InputMethodEnabled, true);
    setAttribute(Qt::WA_NativeWindow, true);
    setMouseTracking(true);
    setMinimumSize(80, 40);
}

GhosttyWidget::~GhosttyWidget()
{
    destroySurface();
}

bool GhosttyWidget::createSurface(const QString &workingDirectory,
                                    const QString &command,
                                    ghostty_surface_context_e context)
{
#ifdef C11_GHOSTTY_STUB
    qDebug() << "GhosttyWidget: stub mode, no surface created";
    return true;
#else
    if (!m_runtime.isInitialized()) {
        qWarning() << "Cannot create surface: runtime not initialized";
        return false;
    }

    if (m_surface) {
        qWarning() << "Surface already exists";
        return false;
    }

    // Ensure the native window is created so winId() returns the NSView
    winId();

    ghostty_surface_config_s surfConfig = ghostty_surface_config_new();
    surfConfig.context = context;

#ifdef Q_OS_MACOS
    // Create a child NSView for Ghostty's Metal rendering.
    // Qt's QNSView already has wantsLayer=YES with a QContainerLayer,
    // so Ghostty can't replace it. The child NSView starts without a
    // layer, letting Ghostty set layer=CAMetalLayer before wantsLayer=YES.
    void *parentView = reinterpret_cast<void *>(winId());
    m_childNSView = ghostty_bridge_create_child_nsview(parentView);
    if (!m_childNSView) {
        qCritical() << "Failed to create child NSView for Ghostty";
        return false;
    }

    surfConfig.platform_tag = GHOSTTY_PLATFORM_MACOS;
    surfConfig.platform.macos.nsview = m_childNSView;
    surfConfig.userdata = this;
#elif defined(Q_OS_LINUX) || defined(Q_OS_WIN)
    // Linux/Windows: OpenGL renderer via GHOSTTY_PLATFORM_QT, fed a host-created
    // desktop-GL context (WGL on Windows, GLX/EGL on Linux) — no ANGLE needed.
    m_glContext = std::make_unique<GhosttyGlContext>();
    if (!m_glContext->create(windowHandle())) {
        qCritical() << "Failed to create OpenGL context for Ghostty Qt surface";
        m_glContext.reset();
        return false;
    }
    {
        const double dpr = devicePixelRatioF();
        if (!GhosttyQtPlatform::configureSurface(
                surfConfig, *m_glContext,
                reinterpret_cast<void *>(winId()),
                static_cast<uint32_t>(width() * dpr),
                static_cast<uint32_t>(height() * dpr),
                dpr)) {
            qWarning() << "Ghostty Qt platform surface configuration failed";
            m_glContext.reset();
            return false;
        }
    }
    surfConfig.userdata = this;
#else
    qWarning() << "Ghostty surface not supported on this platform";
    return false;
#endif

    surfConfig.scale_factor = devicePixelRatioF();
    surfConfig.font_size = 0; // Use config default

    QByteArray wdBytes;
    if (!workingDirectory.isEmpty()) {
        wdBytes = workingDirectory.toUtf8();
        surfConfig.working_directory = wdBytes.constData();
    }

    QByteArray cmdBytes;
    if (!command.isEmpty()) {
        cmdBytes = command.toUtf8();
        surfConfig.command = cmdBytes.constData();
    }

    m_surface = ghostty_surface_new(m_runtime.app(), &surfConfig);
    if (!m_surface) {
        qCritical() << "Failed to create Ghostty surface";
        return false;
    }

    updateSurfaceSize();
    ghostty_surface_set_focus(m_surface, true);
    ghostty_surface_refresh(m_surface);

    // Kick a deferred resize+refresh after the layout engine has sized us.
    // At creation time the widget is often still at minimum size.
    QTimer::singleShot(100, this, [this]() {
        if (m_surface) {
            updateSurfaceSize();
            ghostty_surface_refresh(m_surface);
        }
    });

    emit surfaceCreated();
    return true;
#endif
}

void GhosttyWidget::destroySurface()
{
#ifndef C11_GHOSTTY_STUB
    if (m_surface) {
        ghostty_surface_free(m_surface);
        m_surface = nullptr;
    }
#ifdef Q_OS_MACOS
    if (m_childNSView) {
        ghostty_bridge_destroy_child_nsview(m_childNSView);
        m_childNSView = nullptr;
    }
#endif
#if defined(Q_OS_WIN) || defined(Q_OS_LINUX)
    m_glContext.reset();
#endif
    emit surfaceDestroyed();
#endif
}

void GhosttyWidget::setFocused(bool focused)
{
    m_focused = focused;
#ifndef C11_GHOSTTY_STUB
    if (m_surface) {
        ghostty_surface_set_focus(m_surface, focused);
    }
#endif
}

void GhosttyWidget::sendText(const QString &text)
{
#ifndef C11_GHOSTTY_STUB
    if (m_surface && !text.isEmpty()) {
        QByteArray utf8 = text.toUtf8();
        ghostty_surface_text(m_surface, utf8.constData(), utf8.size());
    }
#endif
}

ghostty_surface_size_s GhosttyWidget::surfaceSize() const
{
#ifndef C11_GHOSTTY_STUB
    if (m_surface) {
        return ghostty_surface_size(m_surface);
    }
#endif
    return {};
}

bool GhosttyWidget::processExited() const
{
#ifndef C11_GHOSTTY_STUB
    if (m_surface) {
        return ghostty_surface_process_exited(m_surface);
    }
#endif
    return false;
}

void GhosttyWidget::updateSurfaceSize()
{
#ifndef C11_GHOSTTY_STUB
    if (!m_surface) return;

#ifdef Q_OS_MACOS
    if (m_childNSView) {
        ghostty_bridge_resize_child_nsview(m_childNSView, width(), height());
    }
#endif

    double dpr = devicePixelRatioF();
    uint32_t w = static_cast<uint32_t>(width() * dpr);
    uint32_t h = static_cast<uint32_t>(height() * dpr);
    ghostty_surface_set_size(m_surface, w, h);
    ghostty_surface_set_content_scale(m_surface, dpr, dpr);
    ghostty_surface_refresh(m_surface);
#endif
}

// --- Event handlers ---

void GhosttyWidget::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    updateSurfaceSize();
}

void GhosttyWidget::focusInEvent(QFocusEvent *event)
{
    QWidget::focusInEvent(event);
    setFocused(true);
}

void GhosttyWidget::focusOutEvent(QFocusEvent *event)
{
    QWidget::focusOutEvent(event);
    setFocused(false);
}

void GhosttyWidget::keyPressEvent(QKeyEvent *event)
{
#ifndef C11_GHOSTTY_STUB
    if (!m_surface) return;

    ghostty_input_key_s key = m_keyMapper.mapKeyEvent(event, GHOSTTY_ACTION_PRESS);
    ghostty_surface_key(m_surface, key);

    // Also send text for printable characters
    QString text = event->text();
    if (!text.isEmpty() && !event->modifiers().testFlag(Qt::ControlModifier)
        && !event->modifiers().testFlag(Qt::AltModifier)) {
        QByteArray utf8 = text.toUtf8();
        ghostty_surface_text(m_surface, utf8.constData(), utf8.size());
    }
#else
    Q_UNUSED(event);
#endif
}

void GhosttyWidget::keyReleaseEvent(QKeyEvent *event)
{
#ifndef C11_GHOSTTY_STUB
    if (!m_surface) return;

    ghostty_input_key_s key = m_keyMapper.mapKeyEvent(event, GHOSTTY_ACTION_RELEASE);
    ghostty_surface_key(m_surface, key);
#else
    Q_UNUSED(event);
#endif
}

void GhosttyWidget::mousePressEvent(QMouseEvent *event)
{
#ifndef C11_GHOSTTY_STUB
    if (!m_surface) return;

    ghostty_surface_mouse_button(
        m_surface,
        GHOSTTY_MOUSE_PRESS,
        qtButtonToGhostty(event->button()),
        qtModsToGhostty(event->modifiers()));

    auto pos = event->position();
    double dpr = devicePixelRatioF();
    ghostty_surface_mouse_pos(m_surface, pos.x() * dpr, pos.y() * dpr,
                               qtModsToGhostty(event->modifiers()));
#else
    Q_UNUSED(event);
#endif
}

void GhosttyWidget::mouseReleaseEvent(QMouseEvent *event)
{
#ifndef C11_GHOSTTY_STUB
    if (!m_surface) return;

    ghostty_surface_mouse_button(
        m_surface,
        GHOSTTY_MOUSE_RELEASE,
        qtButtonToGhostty(event->button()),
        qtModsToGhostty(event->modifiers()));
#else
    Q_UNUSED(event);
#endif
}

void GhosttyWidget::mouseMoveEvent(QMouseEvent *event)
{
#ifndef C11_GHOSTTY_STUB
    if (!m_surface) return;

    auto pos = event->position();
    double dpr = devicePixelRatioF();
    ghostty_surface_mouse_pos(m_surface, pos.x() * dpr, pos.y() * dpr,
                               qtModsToGhostty(event->modifiers()));
#else
    Q_UNUSED(event);
#endif
}

void GhosttyWidget::wheelEvent(QWheelEvent *event)
{
#ifndef C11_GHOSTTY_STUB
    if (!m_surface) return;

    QPoint delta = event->angleDelta();
    double dx = delta.x() / 120.0;
    double dy = delta.y() / 120.0;

    ghostty_input_scroll_mods_t scrollMods = 0;
    ghostty_surface_mouse_scroll(m_surface, dx, dy, scrollMods);
#else
    Q_UNUSED(event);
#endif
}

void GhosttyWidget::inputMethodEvent(QInputMethodEvent *event)
{
#ifndef C11_GHOSTTY_STUB
    if (!m_surface) {
        event->ignore();
        return;
    }

    // Handle preedit (composing) text
    QString preedit = event->preeditString();
    if (!preedit.isEmpty()) {
        QByteArray utf8 = preedit.toUtf8();
        ghostty_surface_preedit(m_surface, utf8.constData(), utf8.size());
    } else {
        ghostty_surface_preedit(m_surface, nullptr, 0);
    }

    // Handle committed text
    QString commit = event->commitString();
    if (!commit.isEmpty()) {
        QByteArray utf8 = commit.toUtf8();
        ghostty_surface_text(m_surface, utf8.constData(), utf8.size());
    }

    event->accept();
#else
    QWidget::inputMethodEvent(event);
#endif
}

QVariant GhosttyWidget::inputMethodQuery(Qt::InputMethodQuery query) const
{
#ifndef C11_GHOSTTY_STUB
    if (m_surface && query == Qt::ImCursorRectangle) {
        double x = 0, y = 0, w = 0, h = 0;
        ghostty_surface_ime_point(m_surface, &x, &y, &w, &h);
        double dpr = devicePixelRatioF();
        return QRectF(x / dpr, y / dpr, w / dpr, h / dpr);
    }
#endif
    return QWidget::inputMethodQuery(query);
}

bool GhosttyWidget::event(QEvent *event)
{
    // Handle DPI changes
    if (event->type() == QEvent::DevicePixelRatioChange) {
        updateSurfaceSize();
    }
    return QWidget::event(event);
}

ghostty_input_mods_e GhosttyWidget::qtModsToGhostty(Qt::KeyboardModifiers mods) const
{
    int result = GHOSTTY_MODS_NONE;
    if (mods & Qt::ShiftModifier)   result |= GHOSTTY_MODS_SHIFT;
    if (mods & Qt::ControlModifier) {
#ifdef Q_OS_MACOS
        // On macOS, Qt::ControlModifier = Cmd, Qt::MetaModifier = Ctrl
        result |= GHOSTTY_MODS_SUPER;
#else
        result |= GHOSTTY_MODS_CTRL;
#endif
    }
    if (mods & Qt::AltModifier) result |= GHOSTTY_MODS_ALT;
    if (mods & Qt::MetaModifier) {
#ifdef Q_OS_MACOS
        result |= GHOSTTY_MODS_CTRL;
#else
        result |= GHOSTTY_MODS_SUPER;
#endif
    }
    return static_cast<ghostty_input_mods_e>(result);
}

ghostty_input_mouse_button_e GhosttyWidget::qtButtonToGhostty(Qt::MouseButton button) const
{
    switch (button) {
    case Qt::LeftButton:   return GHOSTTY_MOUSE_LEFT;
    case Qt::RightButton:  return GHOSTTY_MOUSE_RIGHT;
    case Qt::MiddleButton: return GHOSTTY_MOUSE_MIDDLE;
    case Qt::BackButton:   return GHOSTTY_MOUSE_FOUR;
    case Qt::ForwardButton: return GHOSTTY_MOUSE_FIVE;
    default:               return GHOSTTY_MOUSE_UNKNOWN;
    }
}

} // namespace c11
