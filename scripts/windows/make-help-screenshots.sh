#!/usr/bin/env bash
#
# Ant Renamer Qt - remakes the screenshots of the help (app/help/img: *_en.png and *_fr.png).
#
# Run it from Git Bash or MSYS2 on Windows, after building the program (MinGW build in build-mingw), with Python and Pillow
# and numpy installed. Like make-screenshots.sh it works on a copy of the program with its own portable settings, so your own
# settings and history are untouched, and it drives the program with the ANTREN_* variables (see docs/BUILDING.md). The sample
# folder is C:\arshots\Paris (the path shown in the pictures): it is recreated before each shot, delete C:\arshots when done.
# The toolbar strips (tabs_*, filestoolbar_*, batchtoolbar_*, logtoolbar_*) are cut out of wide window shots (strip_*) by
# make-help-crops.py: they are exactly as wide as their buttons.
#
# Made here: files_page, actions_page, log_page, modern_tabs, modern_bands, modern_columns, options_display, options_files,
# options_processing, options_events, options_language, filter_dialog, setdates_dialog, and the four strips.
# Not made here (they do not change): the 24 x 24 icons, icons_sets.png (every icon set), addfilesoptions.png.
#
# Usage: scripts/windows/make-help-screenshots.sh
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
BUILD="$ROOT/build-mingw/app"
[ -x "$BUILD/antrenamer-qt.exe" ] || { echo "Build first: $BUILD/antrenamer-qt.exe not found" >&2; exit 1; }
OUT="$ROOT/app/help/img"

W="$(mktemp -d)"; trap 'rm -rf "$W"' EXIT
cp -r "$BUILD" "$W/app"; rm -rf "$W"/app/*autogen "$W"/app/CMakeFiles
mkdir -p "$W/src" "$W/shots"
for f in IMG_4012.CR2 IMG_4012.JPG IMG_4013.CR2 IMG_4013.JPG IMG_4014.JPG IMG_4015.JPG IMG_4016.JPG IMG_4017.JPG IMG_4018.JPG \
         IMG_4019.JPG "Notes voyage.txt"; do : > "$W/src/$f"; done
python "$ROOT/scripts/windows/make-sample-dates.py" "$W/dates"   # photos with an EXIF date for the Set dates dialog
VERSION="$(head -n1 "$ROOT/VERSION" | tr -d '[:space:]')"
cat > "$W/log.arb" <<EOF
<?xml version="1.0" encoding="UTF-8"?>
<AntRenamer Version="$VERSION" Date="$(date +%F)">
 <Batch>
  <Enum Mask="Paris %num%%ext%" Start="1" Digits="3" Incr="1" Restart="False" SharePairs="False"/>
 </Batch>
</AntRenamer>
EOF

# shot NAME ui(0 classic/1 modern) layout(0 tabs/1 bands/2 columns) WxH [ENV=value ...]  (light theme, Modern icon set)
# SRC=folder (default $W/src) is copied to C:\arshots\Paris; GO=1 runs the batch; BATCH=file.arb loads it; ARGS="..." adds
# parameters.
shot() {
  local name=$1 ui=$2 lay=$3 size=$4; shift 4
  # [hints] showPreview=false: in the classic layout the first preview shows a modal hint, which would hold the preview back
  printf '[General]\nlanguage=%s\n[hints]\nshowPreview=false\n[options]\nnewUi=%s\npageLayout=%s\ntheme=1\n' "$LANG_NAME" \
    "$([ "$ui" = 1 ] && echo true || echo false)" "$lay" > "$W/app/antrenamer-qt.ini"
  rm -f "$W/app/antrenamer-qt-history.json"   # the undo history is kept between runs: the buttons would be active
  rm -rf /c/arshots/Paris; mkdir -p /c/arshots; cp -rp "${SRC:-$W/src}" /c/arshots/Paris
  rm -f "$W/shots/$name.png"
  local pattern="Paris/*"; [ -n "${SRC:-}" ] && pattern="Paris/*.JPG"   # the dates sample: the pictures, then the rest
  local more=(); [ -n "${SRC:-}" ] && more=("Paris/*.CR2" "Paris/*.xmp" "Paris/*.txt")
  ( cd /c/arshots && env ANTREN_SHOT="$W/shots/$name.png" ANTREN_SIZE="$size" ANTREN_DELAY=4000 "$@" \
      "$W/app/antrenamer-qt.exe" ${BATCH:+-b "$BATCH"} ${GO:+-g} ${SETDATES:+--set-dates-from-exif} -af "$pattern" "${more[@]}" ) \
      >/dev/null 2>&1 || true   # exits with a crash code
  [ -f "$W/shots/$name.png" ] || { echo "MISSING $name" >&2; exit 1; }
}

for LANG_NAME in English French; do
  s=$([ "$LANG_NAME" = English ] && echo en || echo fr)
  shot files_page_$s   0 0 680x430 ANTREN_PREVIEW=1 ANTREN_ACTION=6
  shot actions_page_$s 0 0 800x430 ANTREN_TAB=1 ANTREN_ACTION=6
  BATCH="$W/log.arb" GO=1 shot log_page_$s 0 0 680x430 ANTREN_TAB=2
  shot modern_tabs_$s    1 0 660x430  ANTREN_PREVIEW=1 ANTREN_ACTION=6
  shot modern_bands_$s   1 1 720x600  ANTREN_PREVIEW=1 ANTREN_ACTION=6
  shot modern_columns_$s 1 2 1000x440 ANTREN_PREVIEW=1 ANTREN_ACTION=6
  # wide windows for the toolbar strips: in French the first row is wider than the 680 pixels of the page shots
  shot strip_files_$s   0 0 1000x300 ANTREN_PREVIEW=1 ANTREN_ACTION=6
  shot strip_actions_$s 0 0 1000x300 ANTREN_TAB=1 ANTREN_ACTION=6
  shot strip_log_$s     0 0 1000x300 ANTREN_TAB=2
  n=0
  for tab in display files processing events language; do
    shot options_${tab}_$s 1 0 1000x600 ANTREN_DIALOG=options ANTREN_DTAB=$n; n=$((n + 1))
  done
  shot filter_dialog_$s 1 0 1000x600 ANTREN_DIALOG=filter
  SRC="$W/dates" SETDATES=1 shot setdates_dialog_$s 1 0 1000x600 ANTREN_DIALOG=setdates
done
rm -rf /c/arshots/Paris
python "$ROOT/scripts/windows/make-help-crops.py" "$W/shots" "$OUT"
echo "Done: help screenshots of version $VERSION in app/help/img (delete C:\\arshots if you do not use it)."
