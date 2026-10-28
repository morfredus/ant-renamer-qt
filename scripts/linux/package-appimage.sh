#!/usr/bin/env bash
#
# Ant Renamer Qt - builds a self-contained AppImage (x86_64).
#
# Produces dist/AntRenamerQt-<version>-linux-x86_64.AppImage: a single portable file that bundles Qt.
# The user makes it executable and runs it, with no build and no Qt installation.
#
# Requirements: the program built (cmake --preset linux && cmake --build --preset linux) and, on the first
# run, an internet connection (linuxdeploy + its Qt plugin, pinned versions checked by SHA-256, cached
# in .cache/appimage-tools).
#
# Usage: scripts/linux/package-appimage.sh [--build <folder>]
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"

CMD="antrenamer-qt"
ARCH="x86_64"
BUILD_DIR="$ROOT/build"

die() { printf 'Error: %s\n' "$1" >&2; exit 1; }

while [ $# -gt 0 ]; do
    case "$1" in
        --build) BUILD_DIR="${2:-}"; shift ;;
        -h|--help) echo "Usage: scripts/linux/package-appimage.sh [--build <folder>]"; exit 0 ;;
        *) die "unknown option: $1 (see --help)" ;;
    esac
    shift
done

[ "$(uname -s)" = "Linux" ] || die "this script must be run on Linux."
[ "$(uname -m)" = "x86_64" ] || die "the AppImage is only built on x86_64 (host: $(uname -m))."

APP_DIR="$BUILD_DIR/app"
[ -x "$APP_DIR/antrenamer-qt" ] || APP_DIR="$BUILD_DIR"
BINARY="$APP_DIR/antrenamer-qt"
[ -x "$BINARY" ] || die "binary not found: $BINARY
  Build first:  cmake --preset linux && cmake --build --preset linux"
for d in Languages help; do [ -d "$APP_DIR/$d" ] || die "folder $d missing from $APP_DIR."; done

VERSION="$(head -n1 "$ROOT/VERSION" | tr -d '[:space:]')"
[ -n "$VERSION" ] || die "VERSION file empty or missing."
ICON_SRC="$ROOT/app/antrenamer-qt.png"
[ -f "$ICON_SRC" ] || die "icon not found: $ICON_SRC"

TOOLS_DIR="$ROOT/.cache/appimage-tools"
mkdir -p "$TOOLS_DIR"
LINUXDEPLOY="$TOOLS_DIR/linuxdeploy-$ARCH.AppImage"
PLUGIN_QT="$TOOLS_DIR/linuxdeploy-plugin-qt-$ARCH.AppImage"
# Pinned tools: an exact version and its SHA-256 (the "continuous" channel changes without notice and cannot be checked).
# To change the version: update both tags, download the files, then recompute with sha256sum
# (linuxdeploy: the release page also shows the checksum).
LD_TAG="1-alpha-20251107-1"
LD_SHA256="c20cd71e3a4e3b80c3483cef793cda3f4e990aca14014d23c544ca3ce1270b4d"
QT_TAG="1-alpha-20250213-1"
QT_SHA256="15106be885c1c48a021198e7e1e9a48ce9d02a86dd0a1848f00bdbf3c1c92724"
BASE_URL="https://github.com/linuxdeploy/linuxdeploy/releases/download/$LD_TAG"
QT_URL="https://github.com/linuxdeploy/linuxdeploy-plugin-qt/releases/download/$QT_TAG"

# A tool is only used if its checksum is exactly the expected one (a truncated or modified download is refused).
sha_ok() { [ -s "$1" ] && [ "$(sha256sum "$1" | cut -d' ' -f1)" = "$2" ]; }
fetch() {
    local url="$1" dst="$2" sha="$3"
    [ -f "$dst" ] && sha_ok "$dst" "$sha" && return 0
    rm -f "$dst"
    echo "Downloading: $(basename "$dst")"
    if command -v wget >/dev/null 2>&1; then wget -q -O "$dst" "$url" || die "download failed: $url"
    elif command -v curl >/dev/null 2>&1; then curl -fsSL -o "$dst" "$url" || die "download failed: $url"
    else die "neither wget nor curl is available."; fi
    sha_ok "$dst" "$sha" || { rm -f "$dst"; die "unexpected SHA-256 for $(basename "$dst") (corrupted download or modified file)."; }
    chmod +x "$dst"
}
fetch "$BASE_URL/linuxdeploy-$ARCH.AppImage"          "$LINUXDEPLOY" "$LD_SHA256"
fetch "$QT_URL/linuxdeploy-plugin-qt-$ARCH.AppImage"  "$PLUGIN_QT"   "$QT_SHA256"

# Staging happens on the native file system: under WSL, /mnt/c (DrvFs) ignores permissions.
WORK="$(mktemp -d)"
trap 'rm -rf "$WORK"' EXIT
APPDIR="$WORK/AppDir"
install -Dm755 "$BINARY" "$APPDIR/usr/bin/$CMD"
mkdir -p "$APPDIR/usr/share/antrenamer-qt"
install -Dm644 "$ROOT/LICENSE" "$APPDIR/usr/share/doc/antrenamer-qt/copyright"  # GPL v2 or later
cp -r "$APP_DIR/Languages" "$APP_DIR/help" "$APPDIR/usr/share/antrenamer-qt/"

# .desktop in LF: with CRLF, linuxdeploy would look for an icon literally named "antrenamer-qt".
DESKTOP="$APPDIR/usr/share/applications/$CMD.desktop"
mkdir -p "$(dirname "$DESKTOP")"
sed 's/\r$//' "$SCRIPT_DIR/antrenamer-qt.desktop" > "$DESKTOP"
chmod 644 "$DESKTOP"

# linuxdeploy needs the REAL size of the icon to match the hicolor/<size> folder: it is read from
# the PNG header (IHDR, big-endian) rather than assumed.
png_dim() { printf '%d' "0x$(od -An -tx1 -j"$1" -N4 "$2" | tr -d ' \n')"; }
ICON_W="$(png_dim 16 "$ICON_SRC")"; ICON_H="$(png_dim 20 "$ICON_SRC")"
ICON_MAIN="$APPDIR/usr/share/icons/hicolor/${ICON_W}x${ICON_H}/apps/$CMD.png"
install -Dm644 "$ICON_SRC" "$ICON_MAIN"
install -Dm644 "$ICON_SRC" "$APPDIR/usr/share/pixmaps/$CMD.png"

export APPIMAGE_EXTRACT_AND_RUN=1   # no FUSE needed (WSL, containers)
export VERSION
if command -v qmake6 >/dev/null 2>&1; then export QMAKE="$(command -v qmake6)"
elif command -v qmake >/dev/null 2>&1; then export QMAKE="$(command -v qmake)"; fi

echo "Building the AppImage (Qt bundled)..."
( cd "$WORK" && "$LINUXDEPLOY" \
    --appdir "$APPDIR" \
    --executable "$APPDIR/usr/bin/$CMD" \
    --desktop-file "$DESKTOP" \
    --icon-file "$ICON_MAIN" \
    --plugin qt \
    --output appimage )

RESULT="$(ls -t "$WORK"/*.AppImage 2>/dev/null | head -n1)"
[ -n "$RESULT" ] || die "AppImage not produced."
install -d "$ROOT/dist"
FINAL="$ROOT/dist/AntRenamerQt-${VERSION}-linux-${ARCH}.AppImage"
cp -f "$RESULT" "$FINAL"
chmod +x "$FINAL"
echo "OK - $FINAL ($(du -h "$FINAL" | cut -f1))"
