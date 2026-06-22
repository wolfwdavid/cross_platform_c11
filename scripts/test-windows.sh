#!/usr/bin/env bash
# Build and run the c11-qt unit test suite on Windows via ctest.
#
# The Windows counterpart to scripts/test-unit-local.sh. Run from Git Bash:
#
#   scripts/test-windows.sh                          # build (if a compiler env
#                                                    # is active) + run the suite
#   SKIP_BUILD=1 scripts/test-windows.sh             # just run, don't rebuild
#   CTEST_ARGS='-R StatusBarTest' scripts/test-windows.sh   # one test
#
# Paths are overridable via environment so this works on any checkout:
#   QT_PREFIX   Qt install     (default C:/Qt/6.11.1/msvc2022_64)
#   QT_TOOLS    cmake/ctest    (default C:/Qt/Tools/CMake_64/bin)
#   BUILD_DIR   cmake build    (default c11-qt/build)
#
# Building requires the MSVC environment (cl.exe on PATH) — run from a
# "Developer Command Prompt" / after vcvars64.bat, or use SKIP_BUILD=1 to run a
# tree that was already built (e.g. by the local build_msvc.bat helper).
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
build_dir="${BUILD_DIR:-${repo_root}/c11-qt/build}"
qt_prefix="${QT_PREFIX:-C:/Qt/6.11.1/msvc2022_64}"
qt_tools="${QT_TOOLS:-C:/Qt/Tools/CMake_64/bin}"

# Normalize to Unix-style paths so Git Bash hands the right PATH to the Windows
# test executables (mixed C:/… entries don't resolve their Qt runtime DLLs).
export PATH="$(cygpath -u "$qt_tools"):$(cygpath -u "$qt_prefix")/bin:${PATH}"

if [[ ! -d "$build_dir" ]]; then
    echo "error: build dir '$build_dir' not found — configure c11-qt first" >&2
    echo "       (e.g. c11-qt/build_msvc.bat configure, or set BUILD_DIR)" >&2
    exit 1
fi

if [[ "${SKIP_BUILD:-0}" != 1 ]]; then
    cmake --build "$build_dir"
fi

# Run from inside the build dir so ctest finds its test manifest without passing
# a path argument through Git Bash's path mangling.
cd "$build_dir"
# shellcheck disable=SC2086  # CTEST_ARGS is intentionally word-split
ctest --output-on-failure ${CTEST_ARGS:-}
