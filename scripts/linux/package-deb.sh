#!/usr/bin/env bash
#
# Ant Renamer Qt - construction d'un paquet Debian (.deb).
#
# Produit dist/AntRenamerQt-<version>-linux-<arch>.deb, installable avec
#   sudo apt install ./AntRenamerQt-<version>-linux-<arch>.deb
# Le binaire est lie dynamiquement au Qt du systeme : le paquet DECLARE ses dependances (Qt6...) pour
# qu'apt les installe. Les langues et l'aide de l'original vont dans /usr/share/antrenamer-qt.
#
# Binaire attendu apres une compilation (voir CMakePresets.json) :
#   cmake --preset linux             (x86_64, natif)        -> build/app/antrenamer-qt
#   cmake --preset linux-arm64       (ARM64, natif, ex. Pi) -> build-arm64/app/antrenamer-qt
#   cmake --preset linux-arm64-cross (ARM64 depuis x86_64)  -> build-arm64-cross/app/antrenamer-qt
#
# Compilation croisee arm64 : l'architecture du paquet se lit dans l'ELF (pas dans l'hote) et les
# dependances sont resolues dans le sysroot (variable ARM64_SYSROOT, il doit contenir la base dpkg).
#
# Usage : scripts/linux/package-deb.sh [--build <dossier>] [--maintainer "Nom <email>"] [--depends "pkg, ..."]
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"

CMD="antrenamer-qt"
BUILD_DIR=""
MAINTAINER="${DEB_MAINTAINER:-Frederic Biron <morfredus@users.noreply.github.com>}"
DEPENDS_OVERRIDE=""
BUILD_CANDIDATES=("$ROOT/build" "$ROOT/build-arm64" "$ROOT/build-arm64-cross")

die() { printf 'Erreur : %s\n' "$1" >&2; exit 1; }

# Architecture reelle d'un ELF (e_machine, offset 18, little-endian) : 0xb7 = aarch64, 0x3e = x86-64.
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
        *) die "option inconnue : $1 (voir --help)" ;;
    esac
    shift
done

[ "$(uname -s)" = "Linux" ] || die "ce script doit etre execute sous Linux."
command -v dpkg-deb >/dev/null 2>&1 || die "dpkg-deb introuvable (paquet 'dpkg')."

# --build <dossier> : le dossier de build ou son sous-dossier app/ (les deux sont acceptes).
find_binary() {
    local d="$1"
    for c in "$d/app/antrenamer-qt" "$d/antrenamer-qt"; do [ -x "$c" ] && { echo "$c"; return 0; }; done
    return 1
}
if [ -n "$BUILD_DIR" ]; then
    BINARY="$(find_binary "$BUILD_DIR")" || die "binaire introuvable dans $BUILD_DIR (compile d'abord)."
else
    for dir in "${BUILD_CANDIDATES[@]}"; do
        if BINARY="$(find_binary "$dir")"; then BUILD_DIR="$dir"; break; fi
    done
    [ -n "${BINARY:-}" ] || die "binaire introuvable dans : ${BUILD_CANDIDATES[*]}
  Compile d'abord :  cmake --preset linux && cmake --build --preset linux"
fi
APP_DIR="$(dirname "$BINARY")"
echo "Binaire : $BINARY"

VERSION="$(head -n1 "$ROOT/VERSION" | tr -d '[:space:]')"
[ -n "$VERSION" ] || die "fichier VERSION vide ou absent."
# Debian : le suffixe de pre-version se note avec ~ (3.0.0~rc.1 < 3.0.0) ; le '-' est la revision du paquet.
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

ICON_SRC="$ROOT/app/antrenamer-qt.png"
[ -f "$ICON_SRC" ] || die "icone introuvable : $ICON_SRC"
for d in Languages help; do [ -d "$APP_DIR/$d" ] || die "dossier $d absent de $APP_DIR (le build le copie apres l'edition de liens)."; done

# Dependances d'un binaire natif : uniquement les bibliotheques DIRECTEMENT demandees (NEEDED), resolues avec
# ldconfig puis converties en paquets avec dpkg -S (readlink -f : sur un systeme "usr-merged", dpkg -S ne
# connait que le chemin reel /usr/lib/...). Les dependances transitives viennent des paquets eux-memes.
detect_depends() {
    local soname path
    objdump -p "$BINARY" 2>/dev/null | awk '$1=="NEEDED"{print $2}' | sort -u | while read -r soname; do
        path="$(ldconfig -p 2>/dev/null | awk -v s="$soname" '$1==s{print $NF; exit}')"
        [ -n "$path" ] || continue
        dpkg -S "$(readlink -f "$path")" 2>/dev/null | cut -d: -f1 | cut -d, -f1
    done | sort -u | sed 's/:[a-z0-9]*$//' | paste -sd, - | sed 's/,/, /g'
}

# Dependances d'un binaire croise : sonames NEEDED (objdump de la cible) resolus dans le sysroot, puis
# convertis en paquets avec la base dpkg DU SYSROOT (dpkg --admindir).
cross_depends() {
    local sysroot="$1" soname path
    command -v aarch64-linux-gnu-objdump >/dev/null 2>&1 || die "objdump cible manquant : installe binutils-aarch64-linux-gnu."
    [ -d "$sysroot/var/lib/dpkg" ] || die "pas de base dpkg dans le sysroot : $sysroot"
    aarch64-linux-gnu-objdump -p "$BINARY" | awk '$1=="NEEDED"{print $2}' | sort -u | while read -r soname; do
        path="$(find "$sysroot/lib" "$sysroot/usr/lib" -name "$soname" -print -quit 2>/dev/null || true)"
        [ -n "$path" ] || continue
        dpkg --admindir "$sysroot/var/lib/dpkg" -S "${path#"$sysroot"}" 2>/dev/null | cut -d: -f1 | cut -d, -f1
    done | sort -u | sed 's/:arm64$//' | paste -sd, - | sed 's/,/, /g'
}

if [ -n "$DEPENDS_OVERRIDE" ]; then
    DEPENDS="$DEPENDS_OVERRIDE"
elif [ "$CROSS_ARM64" = "1" ]; then
    [ -n "${ARM64_SYSROOT:-}" ] && [ -d "$ARM64_SYSROOT" ] || die "ARM64_SYSROOT absent : sysroot arm64 requis pour les dependances."
    DEPENDS="$(cross_depends "$ARM64_SYSROOT")"
    [ -n "$DEPENDS" ] || die "dependances croisees vides (sysroot sans base dpkg ?)."
else
    DEPENDS="$(detect_depends || true)"
    [ -n "$DEPENDS" ] || DEPENDS="libc6, libqt6core6, libqt6gui6, libqt6widgets6, libqt6network6"
fi
# Le plugin xcb (charge par dlopen) exige libxcb-cursor0 depuis Qt 6.5.
case ", $DEPENDS," in
    *", libxcb-cursor0,"*) : ;;
    *) DEPENDS="$DEPENDS, libxcb-cursor0" ;;
esac

# Arborescence preparee dans le systeme de fichiers NATIF (jamais sur /mnt/c : DrvFs force le mode 777 et
# dpkg-deb refuse un DEBIAN/ en 777).
STAGE_ROOT="$(mktemp -d)"
trap 'rm -rf "$STAGE_ROOT"' EXIT
PKGDIR="$STAGE_ROOT/${CMD}_${DEB_VERSION}_${ARCH}"
install -Dm755 "$BINARY" "$PKGDIR/usr/bin/$CMD"

# Langues et aide de l'original : le programme les cherche dans ../share/antrenamer-qt a cote du binaire.
install -d "$PKGDIR/usr/share/antrenamer-qt"
cp -r "$APP_DIR/Languages" "$APP_DIR/help" "$PKGDIR/usr/share/antrenamer-qt/"
find "$PKGDIR/usr/share/antrenamer-qt" -type d -exec chmod 755 {} + -o -type f -exec chmod 644 {} +

install -d "$PKGDIR/usr/share/applications"
sed 's/\r$//' "$SCRIPT_DIR/antrenamer-qt.desktop" > "$PKGDIR/usr/share/applications/$CMD.desktop"
chmod 644 "$PKGDIR/usr/share/applications/$CMD.desktop"

install -Dm644 "$ICON_SRC" "$PKGDIR/usr/share/icons/hicolor/48x48/apps/$CMD.png"

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
Section: utils
Priority: optional
Homepage: https://www.antp.be/software/renamer/
Description: Batch file renamer (Qt port of Ant Renamer)
 Ant Renamer Qt renames many files and folders at once: replace, insert, move or
 delete text, enumerate, change case, regular expressions, names from a list,
 file dates, MP3 (ID3v1/v2) and EXIF tags.
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
echo "Dependances : $DEPENDS"
echo "Installation :  sudo apt install \"$OUT\""
