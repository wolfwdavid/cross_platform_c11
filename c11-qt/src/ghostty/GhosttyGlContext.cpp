#include "GhosttyGlContext.h"

#if defined(Q_OS_WIN) || defined(Q_OS_LINUX)

#include <QOpenGLContext>
#include <QOffscreenSurface>
#include <QOpenGLFunctions>
#include <QSurfaceFormat>
#include <QWindow>
#include <QGuiApplication>
#include <QDebug>

#if defined(Q_OS_WIN)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

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
#if defined(Q_OS_WIN)
    if (m_wglContext) {
        wglMakeCurrent(nullptr, nullptr);
        wglDeleteContext(static_cast<HGLRC>(m_wglContext));
        m_wglContext = nullptr;
    }
#endif
}

bool GhosttyGlContext::isValidImpl() const
{
#if defined(Q_OS_WIN)
    if (m_wglContext) return true;
#endif
    return m_context != nullptr;
}

bool GhosttyGlContext::create(QWindow *window)
{
    if (!window) return false;
#if defined(Q_OS_WIN)
    return createWin32(window);
#else
    m_context = new QOpenGLContext();
    m_context->setFormat(ghosttyFormat());
    if (!m_context->create()) {
        delete m_context;
        m_context = nullptr;
        return false;
    }
    m_surface = window;
    return true;
#endif
}

#if defined(Q_OS_WIN)
// Set a GL-capable pixel format on the host window's HWND and create a matching
// HGLRC via raw WGL. Doing it on the widget's own HWND (rather than borrowing
// Qt's context, which is bound to an internal surface) means the pixel format
// that libghostty's GetDC()/wglMakeCurrent see is the one we set here, so they
// agree. A legacy wglCreateContext yields the driver's max compatibility
// profile (4.x on modern GPUs), which satisfies Ghostty's GL 3.3 requirement.
bool GhosttyGlContext::createWin32(QWindow *window)
{
    HWND hwnd = reinterpret_cast<HWND>(window->winId());
    if (!hwnd) {
        qCritical() << "GhosttyGlContext: window has no HWND";
        return false;
    }

    HDC hdc = GetDC(hwnd);
    if (!hdc) {
        qCritical() << "GhosttyGlContext: GetDC failed";
        return false;
    }

    PIXELFORMATDESCRIPTOR pfd{};
    pfd.nSize = sizeof(pfd);
    pfd.nVersion = 1;
    pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    pfd.iPixelType = PFD_TYPE_RGBA;
    pfd.cColorBits = 32;
    pfd.cAlphaBits = 8;
    pfd.cDepthBits = 24;
    pfd.cStencilBits = 8;
    pfd.iLayerType = PFD_MAIN_PLANE;

    const int pf = ChoosePixelFormat(hdc, &pfd);
    if (pf == 0) {
        qCritical() << "GhosttyGlContext: ChoosePixelFormat failed, err" << GetLastError();
        ReleaseDC(hwnd, hdc);
        return false;
    }
    if (!SetPixelFormat(hdc, pf, &pfd)) {
        qCritical() << "GhosttyGlContext: SetPixelFormat failed, err" << GetLastError();
        ReleaseDC(hwnd, hdc);
        return false;
    }

    HGLRC rc = wglCreateContext(hdc);
    if (!rc) {
        qCritical() << "GhosttyGlContext: wglCreateContext failed, err" << GetLastError();
        ReleaseDC(hwnd, hdc);
        return false;
    }

    // Leave the context not-current here; libghostty makes it current on its
    // own (render thread, and the main thread during surface init). The pixel
    // format we set persists on the HWND for every later GetDC(hwnd).
    ReleaseDC(hwnd, hdc);

    m_hwnd = hwnd;
    m_wglContext = rc;
    return true;
}
#endif

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
#if defined(Q_OS_WIN)
    // On-screen path: our own raw-WGL HGLRC.
    if (m_wglContext) return m_wglContext;
#endif
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
