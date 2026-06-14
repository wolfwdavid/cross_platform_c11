#!/bin/bash
# Build a .deb package for c11 on Debian/Ubuntu.
# Usage: ./build-deb.sh [build-dir]

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
BUILD_DIR="${1:-$REPO_ROOT/build-linux}"
VERSION="0.1.0"
ARCH="$(dpkg --print-architecture)"

echo "=== Building c11 .deb package ==="

# Build first
cmake -B "$BUILD_DIR" \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX=/usr \
    "$REPO_ROOT"
cmake --build "$BUILD_DIR" -j"$(nproc)"

# Create .deb structure
DEB_DIR="$BUILD_DIR/c11_${VERSION}_${ARCH}"
mkdir -p "$DEB_DIR/DEBIAN"
mkdir -p "$DEB_DIR/usr/bin"
mkdir -p "$DEB_DIR/usr/share/applications"
mkdir -p "$DEB_DIR/usr/share/metainfo"

# Control file
cat > "$DEB_DIR/DEBIAN/control" << EOF
Package: c11
Version: ${VERSION}
Section: utils
Priority: optional
Architecture: ${ARCH}
Depends: libqt6widgets6 (>= 6.7), libqt6webenginewidgets6 (>= 6.7)
Maintainer: Stage 11 <hello@stage11.com>
Description: c11 terminal multiplexer
 Terminal multiplexer for the operator:agent pair. Terminals, browsers,
 and markdown surfaces composed in one window.
EOF

# Install files
cp "$BUILD_DIR/bin/c11" "$DEB_DIR/usr/bin/"
cp "$BUILD_DIR/cli/c11" "$DEB_DIR/usr/bin/c11-cli"
cp "$SCRIPT_DIR/c11.desktop" "$DEB_DIR/usr/share/applications/"
cp "$SCRIPT_DIR/c11.appdata.xml" "$DEB_DIR/usr/share/metainfo/"

# Build .deb
dpkg-deb --build "$DEB_DIR"
echo "=== Created: ${DEB_DIR}.deb ==="
