#!/bin/sh

set -eu

VERSION="${1:-dev}"
DIST_DIR="dist"
PKG_DIR="$DIST_DIR/tcurl-$VERSION"

mkdir -p "$PKG_DIR/bin" "$PKG_DIR/config"

cp tcurl "$PKG_DIR/bin/tcurl"
chmod 755 "$PKG_DIR/bin/tcurl"
cp config/keymap.conf "$PKG_DIR/config/keymap.conf"
cp config/layout.conf "$PKG_DIR/config/layout.conf"
cp config/themes.conf "$PKG_DIR/config/themes.conf"
cp config/envs.json "$PKG_DIR/config/envs.json"
cp config/headers.txt "$PKG_DIR/config/headers.txt"
cp config/history.conf "$PKG_DIR/config/history.conf"
cp README.md "$PKG_DIR/README.md"
cp LICENSE.md "$PKG_DIR/LICENSE.md"

tar -C "$DIST_DIR" -czf "$DIST_DIR/tcurl-$VERSION.tar.gz" "tcurl-$VERSION"
echo "Release package created: $DIST_DIR/tcurl-$VERSION.tar.gz"
