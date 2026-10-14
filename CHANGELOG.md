# Changelog

All notable changes to the project are recorded in this file.
The project follows [Semantic Versioning](https://semver.org/) (the `VERSION` file at the repository root).

Original program: Antoine Potten ([Ant Renamer](https://www.antp.be/software/renamer/)).
C++/Qt 6 port and Linux port: Frédéric Biron ([morfredus](https://github.com/morfredus)).

## [Unreleased]

Changes committed but not yet published; they get a version number and date when a release is made.

## [3.0.0-rc.8] - 2026-10-14

### Added

- Modern interface layout, selectable in Options > Display ("Modern interface layout", used at the next start; the classic layout stays the default): one main bar with page tabs showing the number of files and planned actions, a prominent Go button, and the Undo, Options, Help, On top, About and Exit buttons; each page has its own slim toolbar; the Files list shows the new name (Preview) column by default; on the Actions page the plan is always visible next to the editor with a fixed preview strip under both. Uses the Fusion style and the system palette (light or dark). The modern layout keeps its own window size, columns and splitter settings. English and French texts.
- Modern layout: edit an action of the plan in place. Double-click an action of the plan to load its settings into the editor (all 14 action types), change them, then click Modify to replace the action (same position in the batch). The "On top", "About" and "Exit" buttons are visible in the main bar again instead of a menu.
- Modern layout: while an action is being edited, a bar under the editor offers "Validate the modification" and "Cancel the modification" (in addition to the Modify button of the toolbar).
- Modern layout: "Pages layout" option (Options > Display, used at the next start): tabs (default), or a single screen with Files, Actions and Log all visible, either in bands (stacked) or in columns (Files on the left, Actions over Log on the right). The splitters are movable, each pane has a minimum size, the sizes and the hidden panes are remembered per arrangement, and the columns give way to bands when the window gets narrow (below about 900 px). In a single screen the page buttons of the main bar show or hide a pane. English and French texts. Idea: Antoine Potten.
- Test `panel` (CTest, no window): loads every action type into the editor and reads it back (`ActionPanel::setFromAction` / `makeAction`); it fails when an action gets a new setting or a new type that the editor does not handle.
- Help: new chapter "Modern interface layout" (English, French; English text in the Croatian and Tamil help) with screenshots of the three page layouts and the editing of an action of the batch; the news page, the Options, Files, Batch and Main window pages and the table of contents mention the modern layout and the natural sort.

### Changed

- Internal: the toolbars of the classic and the modern layouts are now built from one description (`MainWindow::createToolbars`), so a button is added once for both layouts. No visible change.

### Fixed

- Translations: a text that appears twice in the language files with two different translations (for example the caption "Ajouter" and the hint "Ajouter au job" of the batch button) got one or the other at random, depending on the run (the captions and tooltips of the classic layout too). The choice is now fixed (the caption wins).
- Modern layout: the title of the "Batch contents" and "Preview" boxes was glued to the left edge of the box.

## [3.0.0-rc.7] - 2026-10-14

### Added

- Option "Natural sort in files list (2 before 10) instead of alphabetical" (Options > Display), enabled by default: sorting the Files list by text columns now puts `fichier 2` before `fichier 10`. Untick it to get the alphabetical order of the original program. English and French texts.

### Fixed

- Files list: after the first click on a column header, clicking any header no longer sorted the list (the sort direction was read from the header, which Qt had already changed).

## [3.0.0-rc.6] - 2026-10-14

### Changed

- Copyright now reads 2000-2026 Antoine Potten - Frédéric Biron (About window, installer, Debian package, presentation documents).

## [3.0.0-rc.5] - 2026-10-13

### Changed

- The e-mail address of the original author is removed from the About window and the help, so that requests about Ant Renamer Qt do not reach him (his web site link stays, as the source of the original program).

## [3.0.0-rc.4] - 2026-10-13

### Fixed

- Build error with the Qt 6.4 of Ubuntu 24.04 in the About window (missing `_s` on a string literal added in rc.3).

## [3.0.0-rc.3] - 2026-10-13

### Added

- Help page "Ant Renamer Qt: news and support" (novelties compared with 2.13, fixes, support on GitHub Issues and Discussions), new command-line switches in the help, support link in the About window and the help contact section.

### Fixed

- Help (button, F1 and the "Help" links of the action notes) opened the pages in an external browser through a file path,
  which failed depending on the installation (packaged browsers cannot read the program folder, Linux packages keep
  the pages in `/usr/share/antrenamer-qt`). The help now opens in a window of the program, with the table of contents of
  the original help on the left, and reads the pages from the `help` folder found next to the program or in
  `<prefix>/share/antrenamer-qt/help`. If the folder is missing, the message tells where the program looked.

## [3.0.0-rc.2] - 2026-10-13

Second release candidate: Linux packages.

### Changed

- Official name: **Ant Renamer Qt** (agreed with the author of the original program), developed in parallel with the
  original Ant Renamer so that the two never get mixed up: executable `antrenamer-qt`, installation folder, start menu
  group, settings (`antrenamer-qt.ini`, `%APPDATA%\AntRenamerQt`), Explorer menu key and label, Linux package
  and desktop entry (`antrenamer-qt`), asset names (`AntRenamerQt-<version>-...`). Batch files (`.arb`) keep the format
  of the original program, so both programs read each other's batches.

### Added

- Linux packages: `scripts/linux/package-deb.sh` builds the `.deb` (amd64, arm64) and `scripts/linux/package-appimage.sh`
  the x86_64 AppImage. The arm64 package can be cross-compiled from x86_64 (set `ARM64_SYSROOT`) or built natively.
- The program finds `Languages` and `help` in `<prefix>/share/antrenamer-qt` as well as next to the executable, which is
  where the Linux packages install them.
- Desktop entry and icon for Linux (`scripts/linux/antrenamer-qt.desktop`, `app/antrenamer-qt.png`).

## [3.0.0-rc.1] - 2026-10-13

First release candidate of the C++/Qt 6 port of Ant Renamer 2.13 (Delphi). The project is self-contained: it
depends only on Qt 6 and a C++20 compiler.

### Added

- Help: the pages of the original help (English, French, Croatian, Tamil) ship in `help/` and open in the
  browser (F1, and the "Help" links of the action notes open the mask / regular expression pages).
- Windows Explorer context menu "Add to Ant Renamer" on files and folders (Options > Files && Folders, or the
  installer task). Selected items are sent to the window that is already open (`--shell <path>`); `--install-shell`
  and `--remove-shell` register / remove the entry from a script.
- `docs/PRESENTATION.md` (and `_fr`): presentation of the port for the author of the original program.
- Windows packaging: `scripts/windows/package-win.ps1` builds a portable zip and, when Inno Setup 6 is
  installed, an installer (`installer/antrenamer.iss`).
- Portable core library (`core/`, QtCore only): the 14 renaming actions, batch files (`.arb`) read and
  written in the original XML format, preview, rename and undo.
- Qt Widgets interface mirroring the original (Files / Actions / Log pages, toolbars, sample preview,
  batch list, options, drag and drop, command line switches `-b -g -x -a`).
- The 27 language files of the original program are used as is (`Languages/`).
- ID3v1 and ID3v2.2/2.3/2.4 tags for the MP3 action; EXIF reader and a `%num%` counter for the EXIF action.
- Settings in an INI file; portable mode when `antrenamer.ini` sits next to the program.
- CMake presets `linux`, `linux-arm64`, `linux-arm64-cross` and `mingw`; on Windows the Qt and MinGW
  DLLs are deployed next to the executable after each build.

### Credits

- Frédéric Biron ([morfredus](https://github.com/morfredus)) is credited for the C++/Qt port and the Linux port: About box (General and Credits tabs),
  help pages (`about_*`), installer, and a 3.0.0-rc.1 entry in the version history of the help (`history_*`).

### Fixed

- Compiler warnings on Linux (GCC 13, `-Wall -Wextra`): a discarded `QString::trimmed()` result in the notes of the
  EXIF page (the trailing whitespace was never trimmed) and a bounds-check false positive in the EXIF reader.
- `-x` (exit) on the command line could leave the program running: the window is now closed from the event loop.
