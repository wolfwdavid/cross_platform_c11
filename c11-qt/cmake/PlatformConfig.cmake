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
elseif(UNIX)
    add_compile_definitions(C11_PLATFORM_LINUX)
    # X11/Wayland will be added in Phase 6
elseif(WIN32)
    add_compile_definitions(C11_PLATFORM_WINDOWS)
    # Windows libs will be added in Phase 7
endif()
