#!/usr/bin/env bash
#
# Ant Renamer Qt - remakes the screenshots of docs/screenshots/English and docs/screenshots/French (22 each).
#
# Run it from Git Bash or MSYS2 on Windows, after building the program (MinGW build in build-mingw).
# It copies the program to a temporary folder (so your own settings are untouched: portable mode with its own ini
# and history), creates sample photos in C:\Photos\Brittany (the path shown in the screenshots; the folder is
# recreated before each shot, delete C:\Photos when done), and drives the program with the ANTREN_* variables
# (see docs/BUILDING.md). The Qt DLLs are taken from the build folder.
#
# Usage: scripts/windows/make-screenshots.sh [--only about]
#   --only about   remakes only the four "About" screenshots (the only ones that show the version)
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
ONLY=""; [ "${1:-}" = "--only" ] && ONLY="${2:-}"
BUILD="$ROOT/build-mingw/app"
[ -x "$BUILD/antrenamer-qt.exe" ] || { echo "Build first: $BUILD/antrenamer-qt.exe not found" >&2; exit 1; }
VERSION="$(head -n1 "$ROOT/VERSION" | tr -d '[:space:]')"

W="$(mktemp -d)"; trap 'rm -rf "$W"' EXIT
cp -r "$BUILD" "$W/app"; rm -rf "$W"/app/*autogen "$W"/app/CMakeFiles
mkdir -p "$W/src/Day1" "$W/src/Day2"
for n in 0412 0413 0414 0415 0416 0417; do
  : > "$W/src/Day1/IMG_$n.CR2"; : > "$W/src/Day1/IMG_$n.JPG"; : > "$W/src/Day1/IMG_$n.CR2.xmp"
done
for n in 0501 0502 0503 0504; do : > "$W/src/Day2/IMG_$n.CR2"; : > "$W/src/Day2/IMG_$n.JPG"; done
: > "$W/src/Day2/notes.txt"
cat > "$W/batch.arb" <<EOF
<?xml version="1.0" encoding="UTF-8"?>
<AntRenamer Version="$VERSION" Date="$(date +%F)">
 <Batch>
  <StrRepl Search="IMG_" Repl="2026_" AllOccurences="True" CaseSensitive="True" IncludeExt="False" OnlyExt="False" Wildcards="False"/>
  <StrIns Str="Brittany-" Pos="0" FromBegin="True" Ext="False"/>
 </Batch>
</AntRenamer>
EOF

# shot FILE ui(0 classic/1 modern) layout(0 tabs/1 bands/2 columns) theme(1 light/2 dark) WxH [ENV=value ...]
shot() {
  local name=$1 ui=$2 lay=$3 th=$4 size=$5; shift 5
  printf '[General]\nlanguage=%s\n[options]\nnewUi=%s\npageLayout=%s\ntheme=%s\n' "$LANG_NAME" \
    "$([ "$ui" = 1 ] && echo true || echo false)" "$lay" "$th" > "$W/app/antrenamer-qt.ini"
  rm -f "$W/app/antrenamer-qt-history.json"   # the undo history is kept between runs: the buttons would be active
  rm -rf /c/Photos/Brittany; mkdir -p /c/Photos; cp -r "$W/src" /c/Photos/Brittany
  rm -f "$OUTDIR/$name.png"
  ( cd /c/Photos && env ANTREN_SHOT="$OUTDIR/$name.png" ANTREN_SIZE="$size" ANTREN_DELAY=4000 "$@" \
      "$W/app/antrenamer-qt.exe" -b "$W/batch.arb" ${GO:+-g} -af "Brittany/Day1/*.CR2" "Brittany/Day1/*.JPG" \
      "Brittany/Day2/*.CR2" "Brittany/Day2/*.JPG" "Brittany/Day2/*.txt" ) >/dev/null 2>&1 || true   # exits with a crash code
  [ -f "$OUTDIR/$name.png" ] || { echo "MISSING $name" >&2; exit 1; }
}

for LANG_NAME in English French; do
  OUTDIR="$ROOT/docs/screenshots/$LANG_NAME"; mkdir -p "$OUTDIR"
  for th in 1 2; do
    t=$([ $th = 1 ] && echo light || echo dark)
    if [ -z "$ONLY" ]; then
      shot classic-ui-$t-files   0 0 $th 1000x600 ANTREN_PREVIEW=1
      shot classic-ui-$t-actions 0 0 $th 1000x720 ANTREN_TAB=1 ANTREN_ACTION=1
      GO=1 shot classic-ui-$t-log 0 0 $th 1000x600 ANTREN_TAB=2
      shot modern-ui-$t-tabs-files   1 0 $th 1000x600 ANTREN_PREVIEW=1
      shot modern-ui-$t-tabs-actions 1 0 $th 1000x720 ANTREN_TAB=1 ANTREN_ACTION=1
      GO=1 shot modern-ui-$t-tabs-log 1 0 $th 1000x600 ANTREN_TAB=2
      shot modern-ui-$t-single-screen-bands   1 1 $th 1100x920 ANTREN_PREVIEW=1 ANTREN_ACTION=1
      shot modern-ui-$t-single-screen-columns 1 2 $th 1700x760 ANTREN_PREVIEW=1 ANTREN_ACTION=1
      shot help-$t    1 0 $th 1000x600 ANTREN_DIALOG=help ANTREN_HELPTOPIC=actions
      shot options-$t 1 0 $th 1000x600 ANTREN_DIALOG=options ANTREN_DTAB=0
    fi
    shot about-$t 1 0 $th 1000x600 ANTREN_DIALOG=about
  done
done
rm -rf /c/Photos/Brittany
echo "Done: screenshots of version $VERSION in docs/screenshots (delete C:\\Photos if you do not use it)."
