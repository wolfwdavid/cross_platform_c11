# Ghostty build integration.
#
# Three paths, in priority order:
#   1. macOS: link the prebuilt GhosttyKit.xcframework (libghostty.a / framework).
#   2. Non-Apple, C11_BUILD_GHOSTTY=ON: build libghostty from the ghostty/ submodule
#      via `zig build` (ExternalProject) and link it. Requires a zig toolchain AND a
#      ghostty checkout that exposes a GHOSTTY_PLATFORM_QT renderer backend.
#   3. Otherwise: stub mode (C11_GHOSTTY_STUB) — the app compiles and runs with no
#      live terminal. This is the default off-Apple so the build stays green until
#      the GHOSTTY_PLATFORM_QT-capable libghostty is available.

option(C11_BUILD_GHOSTTY
    "Build libghostty from the ghostty/ submodule via zig (needs zig + GHOSTTY_PLATFORM_QT)"
    OFF)

set(GHOSTTYKIT_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../GhosttyKit.xcframework" CACHE PATH
    "Path to prebuilt GhosttyKit.xcframework")

set(GHOSTTY_SLICE "macos-arm64_x86_64")
set(GHOSTTY_SLICE_DIR "${GHOSTTYKIT_PATH}/${GHOSTTY_SLICE}")

# Repo root holds the embedding header (ghostty.h) and the ghostty/ submodule.
set(GHOSTTY_REPO_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/..")
set(GHOSTTY_SUBMODULE "${GHOSTTY_REPO_ROOT}/ghostty")

if(APPLE AND EXISTS "${GHOSTTY_SLICE_DIR}/libghostty.a")
    # Prebuilt static library from xcframework
    add_library(ghostty INTERFACE)
    target_include_directories(ghostty INTERFACE "${GHOSTTY_SLICE_DIR}/Headers")
    target_link_libraries(ghostty INTERFACE "${GHOSTTY_SLICE_DIR}/libghostty.a")
    message(STATUS "GhosttyKit found: ${GHOSTTY_SLICE_DIR}/libghostty.a")
elseif(APPLE AND EXISTS "${GHOSTTY_SLICE_DIR}/GhosttyKit.framework")
    # Framework variant (older builds)
    add_library(ghostty INTERFACE)
    target_include_directories(ghostty INTERFACE "${GHOSTTY_SLICE_DIR}/GhosttyKit.framework/Headers")
    target_link_libraries(ghostty INTERFACE "${GHOSTTY_SLICE_DIR}/GhosttyKit.framework/GhosttyKit")
    message(STATUS "GhosttyKit framework found: ${GHOSTTY_SLICE_DIR}/GhosttyKit.framework")
elseif(C11_BUILD_GHOSTTY AND NOT APPLE)
    # Build libghostty from source via zig.
    find_program(ZIG_EXECUTABLE zig)
    if(NOT ZIG_EXECUTABLE)
        message(FATAL_ERROR
            "C11_BUILD_GHOSTTY=ON but 'zig' was not found on PATH. Install zig, or "
            "leave C11_BUILD_GHOSTTY=OFF to build in stub mode.")
    endif()
    if(NOT EXISTS "${GHOSTTY_SUBMODULE}/build.zig")
        message(FATAL_ERROR
            "ghostty/ submodule not checked out at ${GHOSTTY_SUBMODULE} "
            "(run: git submodule update --init ghostty).")
    endif()

    if(WIN32)
        set(_ghostty_zig_target "x86_64-windows")
        set(_ghostty_libname "ghostty.lib")
    else()
        set(_ghostty_zig_target "x86_64-linux-gnu")
        set(_ghostty_libname "libghostty.a")
    endif()

    include(ExternalProject)
    set(_ghostty_prefix "${CMAKE_CURRENT_BINARY_DIR}/ghostty-zig")
    ExternalProject_Add(ghostty_zig
        SOURCE_DIR        "${GHOSTTY_SUBMODULE}"
        CONFIGURE_COMMAND ""
        BUILD_IN_SOURCE   1
        BUILD_COMMAND     "${ZIG_EXECUTABLE}" build
                          -Dtarget=${_ghostty_zig_target}
                          -Dapp-runtime=none
                          -Doptimize=ReleaseFast
        INSTALL_COMMAND   ""
        BUILD_ALWAYS      0
        BUILD_BYPRODUCTS  "${GHOSTTY_SUBMODULE}/zig-out/lib/${_ghostty_libname}")

    add_library(ghostty INTERFACE)
    add_dependencies(ghostty ghostty_zig)
    # Embedding header lives at the repo root (kept in sync with the Zig ABI).
    target_include_directories(ghostty INTERFACE "${GHOSTTY_REPO_ROOT}")
    target_link_libraries(ghostty INTERFACE
        "${GHOSTTY_SUBMODULE}/zig-out/lib/${_ghostty_libname}")
    message(STATUS "Ghostty: building libghostty from source (target ${_ghostty_zig_target})")
else()
    # Stub mode: compile without Ghostty linking (default off-Apple / CI / testing).
    message(STATUS "Ghostty: stub mode (no live terminal). "
        "Set -DC11_BUILD_GHOSTTY=ON to build libghostty from source.")
    add_library(ghostty INTERFACE)
    target_include_directories(ghostty INTERFACE "${GHOSTTY_REPO_ROOT}")
    target_compile_definitions(ghostty INTERFACE C11_GHOSTTY_STUB)
endif()
