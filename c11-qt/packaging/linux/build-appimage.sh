#!/bin/bash
# Build an AppImage for c11 on Linux.
# Usage: ./build-appimage.sh [build-dir]
#
# Prerequisites:
#   - cmake, Qt 6.7+, linuxdeployqt or linuxdeploy
#   - GhosttyKit built for Linux (via Zig cross-compile)

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
BUILD_DIR="${1:-$REPO_ROOT/build-linux}"

echo "=== Building c11 for Linux (AppImage) ==="

# Configure
cmake -B "$BUILD_DIR" \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX=/usr \
    "$REPO_ROOT"

# Build
cmake --build "$BUILD_DIR" -j"$(nproc)"

# Install to AppDir
APPDIR="$BUILD_DIR/AppDir"
mkdir -p "$APPDIR/usr/bin" "$APPDIR/usr/share/applications" \
         "$APPDIR/usr/share/metainfo" "$APPDIR/usr/share/icons/hicolor/256x256/apps"

cp "$BUILD_DIR/bin/c11" "$APPDIR/usr/bin/"
cp "$BUILD_DIR/cli/c11" "$APPDIR/usr/bin/c11-cli"
cp "$SCRIPT_DIR/c11.desktop" "$APPDIR/usr/share/applications/"
cp "$SCRIPT_DIR/c11.appdata.xml" "$APPDIR/usr/share/metainfo/"

# Icon (placeholder — replace with actual icon)
if [ -f "$REPO_ROOT/resources/c11.png" ]; then
    cp "$REPO_ROOT/resources/c11.png" "$APPDIR/usr/share/icons/hicolor/256x256/apps/"
fi

# Create AppImage using linuxdeploy
if command -v linuxdeploy &>/dev/null; then
    linuxdeploy \
        --appdir "$APPDIR" \
        --plugin qt \
        --output appimage
    echo "AppImage created successfully"
elif command -v linuxdeployqt &>/dev/null; then
    linuxdeployqt "$APPDIR/usr/share/applications/c11.desktop" -appimage
    echo "AppImage created successfully"
else
    echo "WARNING: linuxdeploy/linuxdeployqt not found. AppDir prepared at $APPDIR"
    echo "Install linuxdeploy to create the AppImage."
fi

echo "=== Done ==="
