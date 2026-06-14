# Per-OS flags, frameworks, and libraries

set(C11_PLATFORM_LIBS "")

if(APPLE)
    find_library(COCOA_FRAMEWORK Cocoa REQUIRED)
    find_library(METAL_FRAMEWORK Metal REQUIRED)
    find_library(QUARTZCORE_FRAMEWORK QuartzCore REQUIRED)
    find_library(IOSURFACE_FRAMEWORK IOSurface REQUIRED)
    find_library(CARBON_FRAMEWORK Carbon REQUIRED)
    find_library(FOUNDATION_FRAMEWORK Foundation REQUIRED)
    find_library(CORETEXT_FRAMEWORK CoreText REQUIRED)
    find_library(COREFOUNDATION_FRAMEWORK CoreFoundation REQUIRED)
    find_library(COREGRAPHICS_FRAMEWORK CoreGraphics REQUIRED)
    find_library(UNIFORMTYPEIDENTIFIERS_FRAMEWORK UniformTypeIdentifiers REQUIRED)
    list(APPEND C11_PLATFORM_LIBS
        ${COCOA_FRAMEWORK}
        ${METAL_FRAMEWORK}
        ${QUARTZCORE_FRAMEWORK}
        ${IOSURFACE_FRAMEWORK}
        ${CARBON_FRAMEWORK}
        ${FOUNDATION_FRAMEWORK}
        ${CORETEXT_FRAMEWORK}
        ${COREFOUNDATION_FRAMEWORK}
        ${COREGRAPHICS_FRAMEWORK}
        ${UNIFORMTYPEIDENTIFIERS_FRAMEWORK}
    )
    add_compile_definitions(C11_PLATFORM_MACOS)

elseif(UNIX AND NOT APPLE)
    add_compile_definitions(C11_PLATFORM_LINUX)

    # D-Bus for desktop notifications (org.freedesktop.Notifications)
    find_package(Qt6 COMPONENTS DBus QUIET)
    if(Qt6DBus_FOUND)
        list(APPEND C11_PLATFORM_LIBS Qt6::DBus)
        message(STATUS "Qt6 DBus found — desktop notifications enabled")
    else()
        message(STATUS "Qt6 DBus not found — desktop notifications disabled")
        add_compile_definitions(C11_NO_DBUS)
    endif()

    # X11 (optional, for X11 window handle)
    find_package(X11 QUIET)
    if(X11_FOUND)
        list(APPEND C11_PLATFORM_LIBS ${X11_LIBRARIES})
        add_compile_definitions(C11_HAS_X11)
        message(STATUS "X11 found: ${X11_LIBRARIES}")
    endif()

    # Wayland (detected via Qt's platform plugin)
    # No extra linking needed — Qt handles Wayland abstraction.
    # At runtime, QGuiApplication::platformName() == "wayland" indicates Wayland.

    # OpenGL for Ghostty renderer on Linux
    find_package(OpenGL QUIET)
    if(OpenGL_FOUND)
        list(APPEND C11_PLATFORM_LIBS OpenGL::GL)
        add_compile_definitions(C11_HAS_OPENGL)
        message(STATUS "OpenGL found")
    endif()

    # EGL (for Wayland/modern GL context)
    find_package(PkgConfig QUIET)
    if(PkgConfig_FOUND)
        pkg_check_modules(EGL egl QUIET)
        if(EGL_FOUND)
            list(APPEND C11_PLATFORM_LIBS ${EGL_LIBRARIES})
            add_compile_definitions(C11_HAS_EGL)
            message(STATUS "EGL found")
        endif()
    endif()

elseif(WIN32)
    add_compile_definitions(C11_PLATFORM_WINDOWS)
    # Windows libs will be added in Phase 7
endif()
