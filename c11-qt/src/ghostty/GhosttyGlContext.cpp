#include "GhosttyGlContext.h"

#if defined(Q_OS_WIN) || defined(Q_OS_LINUX)

#include <QOpenGLContext>
#include <QOffscreenSurface>
#include <QOpenGLFunctions>
#include <QSurfaceFormat>
#include <QWindow>
#include <QGuiApplication>

namespace c11 {

static QSurfaceFormat ghosttyFormat()
{
    // Ghostty's OpenGL renderer targets a 3.3 core context.
    QSurfaceFormat fmt;
    fmt.setRenderableType(QSurfaceFormat::OpenGL);
    fmt.setProfile(QSurfaceFormat::CoreProfile);
    fmt.setVersion(3, 3);
    fmt.setSwapBehavior(QSurfaceFormat::DoubleBuffer);
    return fmt;
}

GhosttyGlContext::GhosttyGlContext() = default;

GhosttyGlContext::~GhosttyGlContext()
{
    if (m_context) m_context->doneCurrent();
    delete m_context;
    delete m_offscreen; // QSurface alias into m_offscreen when offscreen mode
}

bool GhosttyGlContext::create(QWindow *window)
{
    if (!window) return false;
    m_context = new QOpenGLContext();
    m_context->setFormat(ghosttyFormat());
    if (!m_context->create()) {
        delete m_context;
        m_context = nullptr;
        return false;
    }
    m_surface = window;
    return true;
}

bool GhosttyGlContext::createOffscreen()
{
    m_context = new QOpenGLContext();
    m_context->setFormat(ghosttyFormat());
    if (!m_context->create()) {
        delete m_context;
        m_context = nullptr;
        return false;
    }
    m_offscreen = new QOffscreenSurface();
    m_offscreen->setFormat(m_context->format());
    m_offscreen->create();
    if (!m_offscreen->isValid()) {
        delete m_offscreen;
        m_offscreen = nullptr;
        delete m_context;
        m_context = nullptr;
        return false;
    }
    m_surface = m_offscreen;
    return true;
}

bool GhosttyGlContext::makeCurrent()
{
    return m_context && m_surface && m_context->makeCurrent(m_surface);
}

void GhosttyGlContext::doneCurrent()
{
    if (m_context) m_context->doneCurrent();
}

void GhosttyGlContext::swapBuffers()
{
    if (m_context && m_surface) m_context->swapBuffers(m_surface);
}

void *GhosttyGlContext::nativeContext() const
{
    if (!m_context) return nullptr;
#if defined(Q_OS_WIN)
    if (auto *wgl = m_context->nativeInterface<QNativeInterface::QWGLContext>())
        return reinterpret_cast<void *>(wgl->nativeContext());
#elif defined(Q_OS_LINUX)
    if (auto *glx = m_context->nativeInterface<QNativeInterface::QGLXContext>())
        return reinterpret_cast<void *>(glx->nativeContext());
    if (auto *egl = m_context->nativeInterface<QNativeInterface::QEGLContext>())
        return reinterpret_cast<void *>(egl->nativeContext());
#endif
    return nullptr;
}

void *GhosttyGlContext::nativeDisplay() const
{
#if defined(Q_OS_LINUX)
    // X11 Display* via the platform integration; null under Wayland/EGL where
    // ghostty derives the display from the native window instead.
    if (auto *app = qGuiApp) {
        if (auto *x11 = app->nativeInterface<QNativeInterface::QX11Application>())
            return reinterpret_cast<void *>(x11->display());
    }
#endif
    return nullptr; // Windows: WGL has no display handle
}

QString GhosttyGlContext::glVersionString()
{
    if (!makeCurrent()) return {};
    QString version;
    if (auto *f = m_context->functions()) {
        if (const GLubyte *s = f->glGetString(GL_VERSION))
            version = QString::fromLatin1(reinterpret_cast<const char *>(s));
    }
    doneCurrent();
    return version;
}

} // namespace c11

#endif // Q_OS_WIN || Q_OS_LINUX
