#!/usr/bin/env bash
#
# Ant Renamer Qt - builds a Debian package (.deb).
#
# Produces dist/AntRenamerQt-<version>-linux-<arch>.deb, installable with
#   sudo apt install ./AntRenamerQt-<version>-linux-<arch>.deb
# The binary is linked dynamically to the system Qt: the package DECLARES its dependencies (Qt6...) so
# that apt installs them. The language files and the help of the original go to /usr/share/antrenamer-qt.
#
# Binary expected after a build (see CMakePresets.json):
#   cmake --preset linux             (x86_64, native)         -> build/app/antrenamer-qt
#   cmake --preset linux-arm64       (ARM64, native, e.g. Pi) -> build-arm64/app/antrenamer-qt
#   cmake --preset linux-arm64-cross (ARM64 from x86_64)      -> build-arm64-cross/app/antrenamer-qt
#
# arm64 cross build: the architecture of the package is read from the ELF (not from the host) and the
# dependencies are resolved in the sysroot (variable ARM64_SYSROOT, which must contain the dpkg database).
#
# Usage: scripts/linux/package-deb.sh [--build <folder>] [--maintainer "Name <email>"] [--depends "pkg, ..."]
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"

CMD="antrenamer-qt"
BUILD_DIR=""
MAINTAINER="${DEB_MAINTAINER:-Frederic Biron <morfredus@users.noreply.github.com>}"
DEPENDS_OVERRIDE=""
BUILD_CANDIDATES=("$ROOT/build" "$ROOT/build-arm64" "$ROOT/build-arm64-cross")

die() { printf 'Error: %s\n' "$1" >&2; exit 1; }

# Real architecture of an ELF (e_machine, offset 18, little-endian) : 0xb7 = aarch64, 0x3e = x86-64.
_elf_arch() {
    case "$(od -An -tx1 -j18 -N2 "$1" 2>/dev/null | tr -d ' ')" in
        b700) echo arm64 ;;
        3e00) echo x86_64 ;;
        *)    echo "" ;;
    esac
}

while [ $# -gt 0 ]; do
    case "$1" in
        --build)      BUILD_DIR="${2:-}"; shift ;;
        --maintainer) MAINTAINER="${2:-}"; shift ;;
        --depends)    DEPENDS_OVERRIDE="${2:-}"; shift ;;
        -h|--help)
            sed -n '2,19p' "${BASH_SOURCE[0]}" | sed 's/^# \{0,1\}//'
            exit 0 ;;
        *) die "unknown option: $1 (see --help)" ;;
    esac
    shift
done

[ "$(uname -s)" = "Linux" ] || die "this script must be run on Linux."
command -v dpkg-deb >/dev/null 2>&1 || die "dpkg-deb not found (package 'dpkg')."

# --build <folder>: the build folder or its app/ subfolder (both are accepted).
find_binary() {
    local d="$1"
    for c in "$d/app/antrenamer-qt" "$d/antrenamer-qt"; do [ -x "$c" ] && { echo "$c"; return 0; }; done
    return 1
}
if [ -n "$BUILD_DIR" ]; then
    BINARY="$(find_binary "$BUILD_DIR")" || die "binary not found in $BUILD_DIR (build first)."
else
    for dir in "${BUILD_CANDIDATES[@]}"; do
        if BINARY="$(find_binary "$dir")"; then BUILD_DIR="$dir"; break; fi
    done
    [ -n "${BINARY:-}" ] || die "binary not found in: ${BUILD_CANDIDATES[*]}
  Build first:  cmake --preset linux && cmake --build --preset linux"
fi
APP_DIR="$(dirname "$BINARY")"
echo "Binary: $BINARY"

VERSION="$(head -n1 "$ROOT/VERSION" | tr -d '[:space:]')"
[ -n "$VERSION" ] || die "VERSION file empty or missing."
# Debian: a pre-release suffix is written with ~ (3.0.0~rc.1 < 3.0.0); the '-' is the package revision.
DEB_VERSION="${VERSION/-/\~}"

CROSS_ARM64=0
if [ "$(_elf_arch "$BINARY")" = "arm64" ] && [ "$(uname -m)" != "aarch64" ]; then CROSS_ARM64=1; fi
if [ "$CROSS_ARM64" = "1" ]; then
    ARCH="arm64"
elif [ "$(_elf_arch "$BINARY")" = "arm64" ]; then
    ARCH="arm64"
else
    ARCH="$(dpkg --print-architecture)"
fi

ICONS_SRC="$ROOT/app/icons/hicolor"   # the SVG and the PNG sizes 16 to 256, in the layout of the icon theme
[ -f "$ICONS_SRC/scalable/apps/$CMD.svg" ] || die "icons not found in $ICONS_SRC"
for d in Languages help; do [ -d "$APP_DIR/$d" ] || die "folder $d missing from $APP_DIR (the build copies it after linking)."; done

# Dependencies of a native binary: only the libraries it asks for DIRECTLY (NEEDED), resolved with
# ldconfig then converted to packages with dpkg -S (readlink -f: on a "usr-merged" system, dpkg -S only
# knows the real path /usr/lib/...). Transitive dependencies come from the packages themselves.
detect_depends() {
    local soname path
    objdump -p "$BINARY" 2>/dev/null | awk '$1=="NEEDED"{print $2}' | sort -u | while read -r soname; do
        path="$(ldconfig -p 2>/dev/null | awk -v s="$soname" '$1==s{print $NF; exit}')"
        [ -n "$path" ] || continue
        dpkg -S "$(readlink -f "$path")" 2>/dev/null | cut -d: -f1 | cut -d, -f1
    done | sort -u | sed 's/:[a-z0-9]*$//' | paste -sd, - | sed 's/,/, /g'
}

# Dependencies of a cross-built binary: NEEDED sonames (objdump of the target) resolved in the sysroot, then
# converted to packages with the dpkg database OF THE SYSROOT (dpkg --admindir).
cross_depends() {
    local sysroot="$1" soname path
    command -v aarch64-linux-gnu-objdump >/dev/null 2>&1 || die "target objdump missing: install binutils-aarch64-linux-gnu."
    [ -d "$sysroot/var/lib/dpkg" ] || die "no dpkg database in the sysroot: $sysroot"
    aarch64-linux-gnu-objdump -p "$BINARY" | awk '$1=="NEEDED"{print $2}' | sort -u | while read -r soname; do
        path="$(find "$sysroot/lib" "$sysroot/usr/lib" -name "$soname" -print -quit 2>/dev/null || true)"
        [ -n "$path" ] || continue
        dpkg --admindir "$sysroot/var/lib/dpkg" -S "${path#"$sysroot"}" 2>/dev/null | cut -d: -f1 | cut -d, -f1
    done | sort -u | sed 's/:arm64$//' | paste -sd, - | sed 's/,/, /g'
}

if [ -n "$DEPENDS_OVERRIDE" ]; then
    DEPENDS="$DEPENDS_OVERRIDE"
elif [ "$CROSS_ARM64" = "1" ]; then
    [ -n "${ARM64_SYSROOT:-}" ] && [ -d "$ARM64_SYSROOT" ] || die "ARM64_SYSROOT not set: an arm64 sysroot is needed for the dependencies."
    DEPENDS="$(cross_depends "$ARM64_SYSROOT")"
    [ -n "$DEPENDS" ] || die "cross dependencies empty (sysroot without a dpkg database?)."
else
    DEPENDS="$(detect_depends || true)"
    [ -n "$DEPENDS" ] || DEPENDS="libc6, libqt6core6, libqt6gui6, libqt6widgets6, libqt6network6"
fi
# Ubuntu 24.04 calls libqt6gui6t64 what Debian 13 calls libqt6gui6 (and the other way round for some packages):
# every "xxxt64" dependency becomes "xxxt64 | xxx" so that it installs on both.
DEPENDS="$(printf '%s' "$DEPENDS" | sed -E 's/([A-Za-z0-9.+-]+)t64\b/\1t64 | \1/g')"
# The xcb plugin (loaded with dlopen) needs libxcb-cursor0 since Qt 6.5.
case ", $DEPENDS," in
    *", libxcb-cursor0,"*) : ;;
    *) DEPENDS="$DEPENDS, libxcb-cursor0" ;;
esac

# Tree prepared on the NATIVE file system (never on /mnt/c: DrvFs forces mode 777 and
# dpkg-deb refuses a DEBIAN/ in 777).
STAGE_ROOT="$(mktemp -d)"
trap 'rm -rf "$STAGE_ROOT"' EXIT
PKGDIR="$STAGE_ROOT/${CMD}_${DEB_VERSION}_${ARCH}"
install -Dm755 "$BINARY" "$PKGDIR/usr/bin/$CMD"

# Language files and help of the original: the program looks for them in ../share/antrenamer-qt next to the binary.
install -d "$PKGDIR/usr/share/antrenamer-qt"
cp -r "$APP_DIR/Languages" "$APP_DIR/help" "$PKGDIR/usr/share/antrenamer-qt/"
find "$PKGDIR/usr/share/antrenamer-qt" -type d -exec chmod 755 {} + -o -type f -exec chmod 644 {} +

install -d "$PKGDIR/usr/share/applications"
sed 's/\r$//' "$SCRIPT_DIR/antrenamer-qt.desktop" > "$PKGDIR/usr/share/applications/$CMD.desktop"
chmod 644 "$PKGDIR/usr/share/applications/$CMD.desktop"

install -d "$PKGDIR/usr/share/icons"
cp -r "$ICONS_SRC" "$PKGDIR/usr/share/icons/"
find "$PKGDIR/usr/share/icons" -type d -exec chmod 755 {} + -o -type f -exec chmod 644 {} +

install -d "$PKGDIR/usr/share/doc/$CMD"
cat > "$PKGDIR/usr/share/doc/$CMD/copyright" <<EOF
Format: https://www.debian.org/doc/packaging-manuals/copyright-format/1.0/
Upstream-Name: Ant Renamer Qt
Source: https://www.antp.be/software/renamer/

Files: *
Copyright: 2000-2026 Antoine Potten - Frédéric Biron
           C++/Qt port: Frederic Biron
License: GPL-2+
 This program is free software; you can redistribute it and/or modify it under
 the terms of the GNU General Public License as published by the Free Software
 Foundation; either version 2 of the License, or (at your option) any later
 version.
 .
 On Debian systems, the full text is in /usr/share/common-licenses/GPL-2.
EOF
printf '%s (%s) unstable; urgency=low\n\n  * Ant Renamer Qt %s.\n\n -- %s  %s\n' \
    "$CMD" "$DEB_VERSION" "$VERSION" "$MAINTAINER" "$(date -R)" \
    | gzip -9n > "$PKGDIR/usr/share/doc/$CMD/changelog.Debian.gz"

INSTALLED_KB="$(du -sk "$PKGDIR" | cut -f1)"
install -d "$PKGDIR/DEBIAN"
cat > "$PKGDIR/DEBIAN/control" <<EOF
Package: $CMD
Version: $DEB_VERSION
Architecture: $ARCH
Maintainer: $MAINTAINER
Installed-Size: $INSTALLED_KB
Depends: $DEPENDS
Recommends: qt6-wayland
Section: utils
Priority: optional
Homepage: https://www.antp.be/software/renamer/
Description: Batch file renamer (Qt port of Ant Renamer)
 Ant Renamer Qt renames many files and folders at once: replace, insert, move or
 delete text, enumerate, change case, regular expressions, names from a list,
 file dates, MP3 (ID3v1/v2) tags and EXIF, IPTC and XMP metadata.
 .
 C++/Qt 6 port of Ant Renamer by Antoine Potten, developed in parallel with the
 original program, with its language files and help.
EOF

OUT="$ROOT/dist/AntRenamerQt-${VERSION}-linux-${ARCH}.deb"
install -d "$ROOT/dist"
if dpkg-deb --help 2>&1 | grep -q -- '--root-owner-group'; then
    dpkg-deb --build --root-owner-group "$PKGDIR" "$OUT"
else
    dpkg-deb --build "$PKGDIR" "$OUT"
fi

echo "OK - $OUT ($(du -h "$OUT" | cut -f1), $ARCH)"
echo "Dependencies: $DEPENDS"
echo "Install:  sudo apt install \"$OUT\""
