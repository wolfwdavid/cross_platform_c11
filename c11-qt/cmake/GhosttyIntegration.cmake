# Ghostty build integration via prebuilt GhosttyKit or ExternalProject
#
# Phase 0: Link against the prebuilt GhosttyKit.xcframework from the parent project.
# Future: ExternalProject_Add for Zig build from source.

set(GHOSTTYKIT_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../GhosttyKit.xcframework" CACHE PATH
    "Path to prebuilt GhosttyKit.xcframework")

set(GHOSTTY_SLICE "macos-arm64_x86_64")
set(GHOSTTY_SLICE_DIR "${GHOSTTYKIT_PATH}/${GHOSTTY_SLICE}")

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
else()
    # Stub mode: compile without Ghostty linking (for CI/testing)
    message(STATUS "GhosttyKit not found at ${GHOSTTYKIT_PATH}, building in stub mode")
    add_library(ghostty INTERFACE)
    target_include_directories(ghostty INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/../")
    target_compile_definitions(ghostty INTERFACE C11_GHOSTTY_STUB)
endif()
