#!/usr/bin/env bash
#
# Ant Renamer Qt - construction d'une AppImage autonome (x86_64).
#
# Produit dist/AntRenamerQt-<version>-linux-x86_64.AppImage : un fichier unique et portable embarquant Qt.
# L'utilisateur le rend executable et le lance, sans compilation ni installation de Qt.
#
# Prerequis : le programme compile (cmake --preset linux && cmake --build --preset linux) et, au premier
# appel, une connexion internet (linuxdeploy + son greffon Qt, versions epinglees et verifiees par SHA-256, mis en cache
# dans .cache/appimage-tools).
#
# Usage : scripts/linux/package-appimage.sh [--build <dossier>]
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"

CMD="antrenamer-qt"
ARCH="x86_64"
BUILD_DIR="$ROOT/build"

die() { printf 'Erreur : %s\n' "$1" >&2; exit 1; }

while [ $# -gt 0 ]; do
    case "$1" in
        --build) BUILD_DIR="${2:-}"; shift ;;
        -h|--help) echo "Usage : scripts/linux/package-appimage.sh [--build <dossier>]"; exit 0 ;;
        *) die "option inconnue : $1 (voir --help)" ;;
    esac
    shift
done

[ "$(uname -s)" = "Linux" ] || die "ce script doit etre execute sous Linux."
[ "$(uname -m)" = "x86_64" ] || die "l'AppImage n'est produite que sur x86_64 (hote : $(uname -m))."

APP_DIR="$BUILD_DIR/app"
[ -x "$APP_DIR/antrenamer-qt" ] || APP_DIR="$BUILD_DIR"
BINARY="$APP_DIR/antrenamer-qt"
[ -x "$BINARY" ] || die "binaire introuvable : $BINARY
  Compile d'abord :  cmake --preset linux && cmake --build --preset linux"
for d in Languages help; do [ -d "$APP_DIR/$d" ] || die "dossier $d absent de $APP_DIR."; done

VERSION="$(head -n1 "$ROOT/VERSION" | tr -d '[:space:]')"
[ -n "$VERSION" ] || die "fichier VERSION vide ou absent."
ICON_SRC="$ROOT/app/antrenamer-qt.png"
[ -f "$ICON_SRC" ] || die "icone introuvable : $ICON_SRC"

TOOLS_DIR="$ROOT/.cache/appimage-tools"
mkdir -p "$TOOLS_DIR"
LINUXDEPLOY="$TOOLS_DIR/linuxdeploy-$ARCH.AppImage"
PLUGIN_QT="$TOOLS_DIR/linuxdeploy-plugin-qt-$ARCH.AppImage"
# Outils epingles : une version precise et son SHA-256 (le canal "continuous" change sans prevenir et n'est pas verifiable).
# Pour changer de version : mettre les deux tags a jour, telecharger les fichiers, puis recalculer avec sha256sum
# (linuxdeploy : la page de la release affiche aussi l'empreinte).
LD_TAG="1-alpha-20251107-1"
LD_SHA256="c20cd71e3a4e3b80c3483cef793cda3f4e990aca14014d23c544ca3ce1270b4d"
QT_TAG="1-alpha-20250213-1"
QT_SHA256="15106be885c1c48a021198e7e1e9a48ce9d02a86dd0a1848f00bdbf3c1c92724"
BASE_URL="https://github.com/linuxdeploy/linuxdeploy/releases/download/$LD_TAG"
QT_URL="https://github.com/linuxdeploy/linuxdeploy-plugin-qt/releases/download/$QT_TAG"

# Un outil n'est utilise que si son empreinte est exactement celle attendue (telechargement tronque ou modifie : refuse).
sha_ok() { [ -s "$1" ] && [ "$(sha256sum "$1" | cut -d' ' -f1)" = "$2" ]; }
fetch() {
    local url="$1" dst="$2" sha="$3"
    [ -f "$dst" ] && sha_ok "$dst" "$sha" && return 0
    rm -f "$dst"
    echo "Telechargement : $(basename "$dst")"
    if command -v wget >/dev/null 2>&1; then wget -q -O "$dst" "$url" || die "echec du telechargement de $url"
    elif command -v curl >/dev/null 2>&1; then curl -fsSL -o "$dst" "$url" || die "echec du telechargement de $url"
    else die "ni wget ni curl disponible."; fi
    sha_ok "$dst" "$sha" || { rm -f "$dst"; die "empreinte SHA-256 inattendue pour $(basename "$dst") (telechargement corrompu ou fichier modifie)."; }
    chmod +x "$dst"
}
fetch "$BASE_URL/linuxdeploy-$ARCH.AppImage"          "$LINUXDEPLOY" "$LD_SHA256"
fetch "$QT_URL/linuxdeploy-plugin-qt-$ARCH.AppImage"  "$PLUGIN_QT"   "$QT_SHA256"

# Le staging se fait dans le systeme de fichiers natif : sous WSL, /mnt/c (DrvFs) ignore les droits.
WORK="$(mktemp -d)"
trap 'rm -rf "$WORK"' EXIT
APPDIR="$WORK/AppDir"
install -Dm755 "$BINARY" "$APPDIR/usr/bin/$CMD"
mkdir -p "$APPDIR/usr/share/antrenamer-qt"
install -Dm644 "$ROOT/LICENSE" "$APPDIR/usr/share/doc/antrenamer-qt/copyright"  # GPL v2 or later
cp -r "$APP_DIR/Languages" "$APP_DIR/help" "$APPDIR/usr/share/antrenamer-qt/"

# .desktop en LF : un CRLF ferait chercher a linuxdeploy une icone litteralement nommee « antrenamer-qt\r ».
DESKTOP="$APPDIR/usr/share/applications/$CMD.desktop"
mkdir -p "$(dirname "$DESKTOP")"
sed 's/\r$//' "$SCRIPT_DIR/antrenamer-qt.desktop" > "$DESKTOP"
chmod 644 "$DESKTOP"

# linuxdeploy exige que la taille REELLE de l'icone corresponde au dossier hicolor/<taille> : on la lit dans
# l'en-tete PNG (IHDR, gros-boutiste) plutot que de la supposer.
png_dim() { printf '%d' "0x$(od -An -tx1 -j"$1" -N4 "$2" | tr -d ' \n')"; }
ICON_W="$(png_dim 16 "$ICON_SRC")"; ICON_H="$(png_dim 20 "$ICON_SRC")"
ICON_MAIN="$APPDIR/usr/share/icons/hicolor/${ICON_W}x${ICON_H}/apps/$CMD.png"
install -Dm644 "$ICON_SRC" "$ICON_MAIN"
install -Dm644 "$ICON_SRC" "$APPDIR/usr/share/pixmaps/$CMD.png"

export APPIMAGE_EXTRACT_AND_RUN=1   # pas de FUSE requis (WSL, conteneurs)
export VERSION
if command -v qmake6 >/dev/null 2>&1; then export QMAKE="$(command -v qmake6)"
elif command -v qmake >/dev/null 2>&1; then export QMAKE="$(command -v qmake)"; fi

echo "Construction de l'AppImage (Qt embarque)..."
( cd "$WORK" && "$LINUXDEPLOY" \
    --appdir "$APPDIR" \
    --executable "$APPDIR/usr/bin/$CMD" \
    --desktop-file "$DESKTOP" \
    --icon-file "$ICON_MAIN" \
    --plugin qt \
    --output appimage )

RESULT="$(ls -t "$WORK"/*.AppImage 2>/dev/null | head -n1)"
[ -n "$RESULT" ] || die "AppImage non produite."
install -d "$ROOT/dist"
FINAL="$ROOT/dist/AntRenamerQt-${VERSION}-linux-${ARCH}.AppImage"
cp -f "$RESULT" "$FINAL"
chmod +x "$FINAL"
echo "OK - $FINAL ($(du -h "$FINAL" | cut -f1))"
