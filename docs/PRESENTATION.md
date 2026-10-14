# Ant Renamer Qt: a port of Ant Renamer 2.13

*A presentation of this C++/Qt 6 port of Ant Renamer 2.13, the program by Antoine Potten.*
French version: [PRESENTATION_fr.md](PRESENTATION_fr.md).

## About this document

Ant Renamer 2 is a Delphi 7 program, built in 2002 and last released as 2.13 (December 2024). I
(Frédéric Biron, morfredus) wanted to keep using it on recent Windows and to have a version that can be built
with today's free tools. I ported it to C++ with Qt 6. The port is released under the same licence as
the original sources (GPL v2 or later) and reuses several of the original files unchanged. This document lists
what was done, what was kept, what was changed and why, so that anyone can judge the work.

The port was written from the sources published by Antoine
Potten (`antrenamer2_sources.zip`).

Releases, changelog and source code: [github.com/morfredus/ant-renamer-qt](https://github.com/morfredus/ant-renamer-qt).

## Name and coexistence

By agreement with the author of the original program, the official name is **Ant Renamer Qt**. The port is developed in parallel with the original
program, so nothing is shared at run time: own executable (`antrenamer-qt`), installation folder, start menu group,
settings file (`antrenamer-qt.ini`), Explorer menu entry and package names. Only the batch file format (`.arb`) is
common, on purpose: both programs read each other's batches.

## In short

| | Original (Delphi 7) | Port (C++20 / Qt 6) |
|---|---|---|
| Size | about 14,700 lines of Pascal | about 2,000 lines (engine) + 3,300 lines (interface) + tests |
| Needs | Delphi 7 + 5 third-party component packs | a C++20 compiler and Qt 6, nothing else |
| Platforms | Windows | Windows (MinGW, tested), Linux x86_64 (installed from the packages and run, tested), ARM64 (presets, untested) |
| Unicode | WideString + Tnt controls | `QString` (UTF-16), same semantics for positions |
| Distribution | Inno Setup installer | portable zip + Inno Setup 6 installer |

## What was kept (compatibility)

- **The 14 renaming actions**, with the same names, order, settings, defaults and descriptions: change
  extension, string replacement, multiple string replacement, string insertion, move string, characters
  deletion, enumeration, MP3 tags, date and time, random names, change case, names from a list, regular
  expression, EXIF.
- **The mask language**: `%name%`, `%ext%`, `%folderN%` (positive and negative levels), `%count*%`, `%%`,
  quotes and `$if(cond,then,else)` with `$$`, ported from `functions_strformat.pas` with the same rules.
- **Batch files** (`.arb`): read and written in the same XML format with the same element and attribute
  names, so batches saved by version 2 load in the port and the reverse.
- **Command line**: `-b`, `-g`, `-x`, `-a[fFr]` with wildcards, and the version 2.07 compatibility.
- **The interface layout**: the three pages (Files / Actions / Log), the toolbars and their icons, the
  action list with its settings page, the live preview of the selected file, the collapsible batch list,
  drag and drop with the options window, the Options dialog with its tabs, the log filter, the MRU list of
  batches, the status bar with progress, the same keyboard shortcuts.
- **The original language files**: the 27 `.lng` files are used as they are (each with its own encoding), and the
  language can be changed in Options. The French interface is built entirely from the original `French.lng`.
- **The original help**: the HTML help pages (English, French, Croatian, Tamil) and the version history are shipped
  and opened from F1 and from the "Help" links of the action notes.
- **The original graphics**: toolbar icon sets (Scrows, Windows XP, Windows XP Large), `renamer.ico`, `ant.bmp`,
  and the predefined "Remove west-european accents" set from `default.xml`.
- **Behaviour details**: renaming runs in a worker thread with Stop and Undo, folders can be renamed with the
  list following the move, "force directories", absolute path detection, "copy instead of rename", folder
  extension rule, DST option for file dates, log saving, events at start and end of a renaming.

## Mapping of the Delphi code

| Original | Port |
|---|---|
| `Actions.pas`, `Files.pas`, `thread.pas` | `core/ren_actions.*`, `core/ren_files.*`, `RenWorker` in `app/mainwindow.*` |
| `functions_strformat.pas` | `core/ren_format.*` (also a Delphi-compatible `FormatDateTime`) |
| `ExtractID3.pas` | `core/ren_id3.*` |
| dEXIF / dIPTC | `core/ren_exif.*` (own minimal readers: EXIF, IPTC, XMP) |
| JvSimpleXml | `QXmlStreamReader` / `QXmlStreamWriter` |
| TRegExpr | `QRegularExpression` (PCRE2) |
| Tnt Unicode Controls, `WideString` | `QString` |
| VirtualTreeView | `QTreeWidget` (flat lists, sorting, drag reordering) |
| Toolbar 2000 + TBX | `QToolBar` with a style sheet |
| `ProgramSettings.pas` (`Renamer.xml`) | INI file, see "Portable mode" |
| `fMain`, `fOptions`, `fSelectDirectory`, `fDragdropOptions`, `fAbout` | `MainWindow`, `ActionPanel`, `OptionsDialog`, `AddFoldersDialog`, `DragDropDialog`, `AboutDialog` |

The engine (`core/`) depends on QtCore only, has no interface code and has its own test program
(`tests/test_core.cpp`, run by `ctest`).

## Differences from the original, on purpose

These are the places where the port does not do exactly what version 2.13 does. They are listed so that they are explicit; if one of them looks like a mistake, please report it in the
[Issues](https://github.com/morfredus/ant-renamer-qt/issues).

1. **Lone `$` in an `$if` mask** (`AdvancedFormat`): the original loops forever on a `$` that is neither
   `$$` nor `$if(`. The port skips it and drops it, as the other branches do.
2. **Empty tags in MP3 / EXIF masks**: `ValidateFileName('')` returns the Windows tick count, so an empty
   `%title%` became a number and `$if(%title%,...)` was always true, which contradicts the help. The port
   keeps an empty tag empty, as the help describes (`Vivaldi.mp3`, not `Vivaldi - .mp3`).
3. **Enumeration, "Restart"**: `SaveToFile` wrote `Settings.Increment` in the `Restart` attribute. The
   port writes the real value. Old batches that contain the wrong value will load it as is.
4. **MP3 default mask**: `%author% - %title%%ext` (missing final `%`) is `%author% - %title%%ext%`.
5. **Case-only renames** (`a.txt` to `A.TXT`): done with `MoveFileW` directly, because `QFile::rename`
   refuses them on case-insensitive volumes.
6. **Metadata actions in the preview**: the date, MP3 and EXIF actions read the real file (the one still
   on disk) instead of the name produced by the previous action, so a preview after another action no
   longer fails with "could not get file date".
7. **Change case**: without the "locale" option the original uses Delphi `UpperCase`, which only maps
   ASCII; with it, full Unicode mapping. The port keeps exactly this split, one character at a time so
   the length never changes.
8. **Regular expressions**: PCRE2 instead of TRegExpr. Same defaults as TRegExpr (case-insensitive, `.`
   matches newlines) and the same replacement syntax (`$1`, `${1}`, `$&`, `\n`, `\t`), but exotic TRegExpr
   constructs can differ.
9. **Dates in `FormatDateTime`**: a re-implementation of the subset the help documents (d, m, y, h, n, s,
   z, `ampm`, quotes, "m means minutes after h or before s"). Locale names use the system locale.
10. **EXIF values** are formatted simply (text as is, rationals as decimals, `ExposureTime` as `1/250`);
    dEXIF decodes more tags and formats some of them differently. Only about forty common tags are known.
11. **Settings**: an INI file instead of `Renamer.xml`. Old preferences are not imported.
12. **Toolbars**: the "Color type" and "Office XP style" options of the TBX themes have no equivalent and
    were left out; hover, pressed and checked states are drawn with a style sheet instead.
13. **Folder selection**: no "Network..." button (the Qt tree shows what the system shows).
14. **Packaging**: the installer installs for the current user by default and does not use
    `{commonappdata}`; user settings are kept at uninstall.

## What was added

- **ID3v2.2, 2.3 and 2.4** for the MP3 action (the original reads ID3v1 only), with the usual text
  encodings, unsynchronisation and the `(17)` / `Rock` genre forms; ID3v1 fills what v2 lacks.
- **`%num%` counter in the EXIF action** (start, digits, increment, restart per folder), the same
  settings as the Enumeration action, saved in the batch file.
- **Windows Explorer context menu** "Add to Ant Renamer" on files and folders (Options, or installer task).
  Items selected together arrive in the window that is already open (single instance through a local
  socket and a lock file). It is the classic menu entry: on Windows 11 it sits under "Show more options".
- **Portable mode**: an empty `antrenamer-qt.ini` next to the program keeps everything in that folder;
  otherwise settings go to `%APPDATA%`. A checkbox moves the settings from one to the other.
- **Windows 10/11 readiness**: application manifest with long paths (over 260 characters), UTF-8 code
  page, per-monitor DPI v2 and common controls v6; Qt handles the dark theme and high DPI.
- **Icon set, font and "auto-resize columns"** applied immediately, toolbars that can be moved and
  remembered, languages list with version, authors and comments.
- **Build system**: CMake presets for Windows (MinGW), Linux x86_64, Linux ARM64 and ARM64 cross
  compilation; Qt and MinGW DLLs are deployed automatically next to the executable on Windows.
- **Packaging**: `scripts/windows/package-win.ps1` builds a portable zip and, with Inno Setup 6, an
  installer (`installer/antrenamer-qt.iss`, English and French).
- **Hidden helpers** for scripts: `--install-shell` and `--remove-shell`.

## Quality and testing

- The engine has an automated test program: paths, mask formatting and `$if`, date formatting, every
  action's preview on sample names, real renames on disk (enumeration, case-only, revert), batch XML
  round trip, ID3v2.3 with fallback to v1, EXIF counter settings.
- By hand: batches with string replacement and EXIF + enumeration were run on real files; the interface
  was compared page by page with screenshots of version 2.13; the installer was installed and removed
  silently; the portable zip was run with a minimal `PATH`; the Explorer menu "Add to Ant Renamer" was
  clicked in Explorer on one or several files and on one or several folders (contents included); the Linux
  packages were installed and the program runs.
- Not covered yet: very large lists, network drives, a real `.arb` file saved by the Delphi version,
  ID3v2.2/2.4 files, ARM64 interface runs, administrator (all users) installation.

## Known limits

- About thirty-five EXIF tags (IFD0 and Exif IFD only, no MakerNote decoding); IPTC is read from JPEG only; XMP
  sidecar files are read and renamed with their picture (IMG.xmp, IMG.CR2.xmp and DxO `.dop`; other sidecar types are not).
- Only ID3 tags; no other audio tag formats.
- The Windows 11 short context menu needs a packaged COM component and a signed package, not done.
- The executable is not code-signed, so SmartScreen may warn on the first download.
- The interface is translated through the original `.lng` files; strings that do not exist there (About text,
  portable mode, Explorer menu) are translated by hand for French only.

## Licence and credits

- GPL v2 or later, like the original; `LICENSE` is the original `license.txt`.
- The original copyright is kept in the About box and in the installer (Copyright 2000-2026 Antoine Potten - Frédéric Biron); the
  About box states that this is a C++/Qt port and links to the original site.
- Third-party code used by the port: Qt 6 (LGPL v3 / GPL), nothing else. The Delphi components of the
  original are not needed.
- Reused from the original, unchanged: `Languages/*.lng`, `help/` (HTML, CSS, images), toolbar bitmaps,
  `renamer.ico`, `ant.bmp`, `default.xml`, texts of the action notes. Translators are credited in the
  Credits tab as in the original About box.

## Feedback and support

Questions, bug reports and ideas about Ant Renamer Qt go to
[Issues](https://github.com/morfredus/ant-renamer-qt/issues) and
[Discussions](https://github.com/morfredus/ant-renamer-qt/discussions). Support for the port is separate from the
original program: please do not report its problems to the author of the original Ant Renamer.

Thanks to Antoine Potten for the program and for publishing its sources: without them this port would not exist.

*Frédéric Biron (morfredus), October 2026*
