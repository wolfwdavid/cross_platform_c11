#pragma once

// GhosttyQtPlatform: defines the interface for GHOSTTY_PLATFORM_QT.
//
// On Linux (and eventually Windows), Ghostty cannot use GHOSTTY_PLATFORM_MACOS.
// Instead, a new platform enum GHOSTTY_PLATFORM_QT is added to ghostty.h with
// a struct that provides the native window handle and OpenGL context.
//
// This file documents the expected C API additions and provides the
// Qt-side wrapper that would pass the right handles to Ghostty.
//
// STATUS: Stubbed. The actual Ghostty fork changes (adding the enum,
// struct, and OpenGL renderer wiring in Zig) are tracked separately.

#include "ghostty.h"
#include <cstdint>

// -- Expected additions to ghostty.h for GHOSTTY_PLATFORM_QT --
//
// typedef struct {
//     void *native_window;    // X11: Window (uint64_t), Wayland: wl_surface*
//     void *gl_context;       // EGL context or GLX context
//     void *gl_display;       // EGL display or X11 Display*
//     uint32_t width;
//     uint32_t height;
//     double scale_factor;
//     bool is_wayland;
// } ghostty_platform_qt_s;
//
// The ghostty_platform_u union gains a .qt member.
// The ghostty_platform_e enum gains GHOSTTY_PLATFORM_QT.

namespace c11 {

// Helper to configure a Ghostty surface for the Qt platform on Linux.
// Extracts the native window handle from a QWidget and sets up the
// surface config accordingly.
struct GhosttyQtPlatform {
    // Returns true if this build supports the Qt platform (Linux/Windows).
    static bool isSupported()
    {
#if defined(Q_OS_LINUX) || defined(Q_OS_WIN)
        // Will be true once the Ghostty fork adds GHOSTTY_PLATFORM_QT
        return false; // Stub: not yet available
#else
        return false;
#endif
    }

    // Configure a surface config for the Qt platform.
    // Returns false if the platform is not supported.
    static bool configureSurface(ghostty_surface_config_s &config,
                                  void *nativeWindowHandle,
                                  double scaleFactor)
    {
        Q_UNUSED(config);
        Q_UNUSED(nativeWindowHandle);
        Q_UNUSED(scaleFactor);
        // Stub: will be implemented when Ghostty adds GHOSTTY_PLATFORM_QT
        return false;
    }
};

} // namespace c11
