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

    bool isValid() const { return m_context != nullptr; }

    bool makeCurrent();
    void doneCurrent();
    void swapBuffers();

    // Native handles for ghostty_platform_qt_s.
    void *nativeContext() const;   // HGLRC (Win) / GLXContext|EGLContext (Linux)
    void *nativeDisplay() const;   // nullptr (Win) / X11 Display* (Linux/X11)

    // GL_VERSION string; makes the context current transiently. Empty on failure.
    QString glVersionString();

    QOpenGLContext *context() const { return m_context; }

private:
    QOpenGLContext *m_context = nullptr;
    QOffscreenSurface *m_offscreen = nullptr;
    QSurface *m_surface = nullptr; // window (not owned) or m_offscreen (owned)
};

} // namespace c11

#endif // Q_OS_WIN || Q_OS_LINUX
