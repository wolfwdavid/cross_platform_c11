#pragma once

// GhosttyQtPlatform: configures a Ghostty surface for GHOSTTY_PLATFORM_QT.
//
// On Linux/Windows, Ghostty cannot use GHOSTTY_PLATFORM_MACOS (NSView/Metal).
// Instead it uses GHOSTTY_PLATFORM_QT with the OpenGL renderer, fed a native
// window handle + a host-created GL context. The context is owned by a
// GhosttyGlContext (see GhosttyGlContext.h); this header just marshals its
// native handles into the ghostty_surface_config_s.
//
// NOTE: this is the host (Qt) side of the contract. It is fully implemented and
// compiles today, but a working terminal additionally requires a libghostty
// built with a GHOSTTY_PLATFORM_QT renderer backend (tracked on the Ghostty
// fork). Until that exists, GhosttyIntegration.cmake keeps the app in stub mode.

#include "ghostty.h"
#include <QtGlobal>

#if defined(Q_OS_LINUX) || defined(Q_OS_WIN)
#include "GhosttyGlContext.h"
#include <QGuiApplication>
#include <cstdint>
#endif

namespace c11 {

struct GhosttyQtPlatform {
    // True on platforms that use GHOSTTY_PLATFORM_QT (Linux/Windows).
    static bool isSupported()
    {
#if defined(Q_OS_LINUX) || defined(Q_OS_WIN)
        return true;
#else
        return false;
#endif
    }

#if defined(Q_OS_LINUX) || defined(Q_OS_WIN)
    // Fill `config` for the Qt platform from a live GL context + native window.
    // `nativeWindow` is the HWND (Windows) or X11 Window / wl_surface* (Linux),
    // typically reinterpret_cast from QWidget::winId().
    static bool configureSurface(ghostty_surface_config_s &config,
                                 GhosttyGlContext &glContext,
                                 void *nativeWindow,
                                 uint32_t width,
                                 uint32_t height,
                                 double scaleFactor)
    {
        if (!glContext.isValid()) return false;

        config.platform_tag = GHOSTTY_PLATFORM_QT;
        config.platform.qt.native_window = nativeWindow;
        config.platform.qt.gl_context = glContext.nativeContext();
        config.platform.qt.gl_display = glContext.nativeDisplay();
        config.platform.qt.width = width;
        config.platform.qt.height = height;
        config.platform.qt.scale_factor = scaleFactor;
        config.platform.qt.is_wayland =
            (QGuiApplication::platformName() == QLatin1String("wayland"));

        // A native GL context handle is required for the renderer to attach.
        return config.platform.qt.gl_context != nullptr;
    }
#endif
};

} // namespace c11
