#pragma once

#include "ghostty.h"
#include <QWidget>
#include <QGuiApplication>

#if defined(Q_OS_LINUX) || defined(Q_OS_WIN)
#include <QOpenGLContext>
#include <QOpenGLWidget>
#endif

namespace c11 {

// Configures a Ghostty surface for the Qt platform (Linux/Windows).
// Uses GHOSTTY_PLATFORM_QT with the OpenGL renderer.
// On Windows, ANGLE (bundled with Qt WebEngine) provides OpenGL->DirectX.
struct GhosttyQtPlatform {
    static bool isSupported()
    {
#if defined(Q_OS_LINUX) || defined(Q_OS_WIN)
        return true;
#else
        return false;
#endif
    }

    // Configure a surface config for the Qt platform.
    static bool configureSurface(ghostty_surface_config_s &config,
                                  void *nativeWindowHandle,
                                  double scaleFactor)
    {
#if defined(Q_OS_LINUX) || defined(Q_OS_WIN)
        config.platform_tag = GHOSTTY_PLATFORM_QT;
        config.platform.qt.native_window = nativeWindowHandle;
        config.platform.qt.scale_factor = scaleFactor;
        config.platform.qt.is_wayland = false;
        config.platform.qt.width = 0;
        config.platform.qt.height = 0;
        config.platform.qt.gl_context = nullptr;
        config.platform.qt.gl_display = nullptr;

#ifdef Q_OS_LINUX
        // Detect Wayland
        QString platform = QGuiApplication::platformName();
        config.platform.qt.is_wayland = (platform == "wayland");
#endif

        // Get the OpenGL context from Qt
        QOpenGLContext *ctx = QOpenGLContext::currentContext();
        if (ctx) {
            config.platform.qt.gl_context = ctx->nativeInterface<QNativeInterface::QGLXContext>();
            if (!config.platform.qt.gl_context) {
                config.platform.qt.gl_context = ctx->nativeInterface<QNativeInterface::QEGLContext>();
            }
        }

        return true;
#else
        Q_UNUSED(config);
        Q_UNUSED(nativeWindowHandle);
        Q_UNUSED(scaleFactor);
        return false;
#endif
    }
};

} // namespace c11
