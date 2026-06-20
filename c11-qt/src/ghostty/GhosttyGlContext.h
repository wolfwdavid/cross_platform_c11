#pragma once

#include <QtGlobal>

// Desktop-OpenGL context helper for the Qt platform (Linux/Windows).
//
// Ghostty's non-Apple renderer is OpenGL. Rather than depend on ANGLE (which Qt 6
// no longer ships on Windows), we create a desktop GL context through Qt's
// QOpenGLContext — Qt picks the correct native backend (WGL on Windows,
// GLX/EGL on Linux) — and expose the native handles needed to populate
// ghostty_platform_qt_s. The actual makeCurrent/swap on the render thread is
// performed by libghostty using these handles.
#if defined(Q_OS_WIN) || defined(Q_OS_LINUX)

#include <QString>

class QWindow;
class QOpenGLContext;
class QOffscreenSurface;
class QSurface;

namespace c11 {

class GhosttyGlContext {
public:
    GhosttyGlContext();
    ~GhosttyGlContext();

    GhosttyGlContext(const GhosttyGlContext &) = delete;
    GhosttyGlContext &operator=(const GhosttyGlContext &) = delete;

    // Create a context bound to a real, already-native QWindow (production use).
    bool create(QWindow *window);
    // Create a context bound to an internal offscreen surface (tests / headless).
    bool createOffscreen();

    bool isValid() const { return isValidImpl(); }

    bool makeCurrent();
    void doneCurrent();
    void swapBuffers();

    // Native handles for ghostty_platform_qt_s.
    void *nativeContext() const;   // HGLRC (Win) / GLXContext|EGLContext (Linux)
    void *nativeDisplay() const;   // nullptr (Win) / X11 Display* (Linux/X11)

    // GL_VERSION string; makes the context current transiently. Empty on failure.
    QString glVersionString();

    QOpenGLContext *context() const { return m_context; }

    bool isValidImpl() const;

private:
#if defined(Q_OS_WIN)
    // On Windows we drive WGL directly for the on-screen window: Qt won't set a
    // pixel format on a plain QWidget's (raster) HWND, so sharing its
    // QOpenGLContext with that HWND fails wglMakeCurrent (ERROR_INVALID_PIXEL_FORMAT).
    // Instead we ChoosePixelFormat/SetPixelFormat on the widget HWND ourselves and
    // create a matching HGLRC. m_offscreen/m_context stay for the headless test path.
    bool createWin32(QWindow *window);
    void *m_wglContext = nullptr; // HGLRC owned by us (on-screen path)
    void *m_hwnd = nullptr;       // HWND of the host window (not owned)
#endif
    QOpenGLContext *m_context = nullptr;
    QOffscreenSurface *m_offscreen = nullptr;
    QSurface *m_surface = nullptr; // window (not owned) or m_offscreen (owned)
};

} // namespace c11

#endif // Q_OS_WIN || Q_OS_LINUX
