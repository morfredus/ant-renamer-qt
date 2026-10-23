# Changelog

All notable changes to the project are recorded in this file.
The project follows [Semantic Versioning](https://semver.org/) (the `VERSION` file at the repository root).

Original program: Antoine Potten ([Ant Renamer](https://www.antp.be/software/renamer/)).
C++/Qt 6 port and Linux port: Frédéric Biron ([morfredus](https://github.com/morfredus)).

## [Unreleased]

### Changed

- TESTED_PLATFORMS (EN, FR): 3.0.0 tried on the four machines (published packages, AppImage included on Mint; offered by the new-version check); the rc.16 entries are no longer marked as drafts.

## [3.0.0] - 2026-10-23

First stable version: the code is the code of 3.0.0-rc.16 (five days of use of the candidate, no feedback and no open bug); only the version number, a few texts of the language files and the documentation change.

### Added

- `docs/release-notes/3.0.0.md` (EN, FR): presentation of the 3.0.0 release.
- `SECURITY.md` and `CODE_OF_CONDUCT.md` (EN, FR) at the repository root, linked from the README (EN, FR). Security problems are reported through the private reporting of GitHub; only the latest version receives fixes. The code of conduct is a short form of the Contributor Covenant 2.1.

### Changed

- TODO (EN, FR): 3.1 update check also lists a choice offered by the installer (final versions only, or also beta and rc); 3.1 also lists the modern layout (single screen in bands) as the default of a new installation, with an optional banner at the first start. The modern layout as the default of a new installation, with a one-time offer to switch at the first start of that version, is noted for a later version (not 3.0.0, whose code stays the code of rc.16).
- WHATS_NEW (EN, FR): now a version-by-version list (the current content is the "3.0.0" section; changes to come go under an "Upcoming" section, so the presentation of the project is not repeated at each release); CONTRIBUTING (EN, FR) follows. PRESENTATION (EN, FR): new section "Requests from users of the original program" (not presented as answers from the original author).
- TODO (EN, FR): ideas after 3.0.0, not promises, sorted by urgency (from the 120 topics of the original program's forum) and spread over possible versions 3.1 to 3.6, one theme each so that each can be tried before the next: 3.1 pre-releases checkbox for the update check, silent command line, EXIF duplicate suffix, exportable preview; 3.2 file dates, enumeration by letters, copy per action; 3.3 presets, size and checksum tags, encoding fix; 3.4 FLAC/OGG/M4A and MP3 duration; 3.5 video; 3.6 GPS, more EXIF, PDF and Office.
- Presentation (EN, FR): the "What was added" section now lists the modern layout (with the single screen) and the renaming history; the table of sizes is updated, the layout and help bullets mention the modern layout and the Markdown help, the language note says English and French carry the `<QtPort>` texts.
- Documentation pass against the code: ARCHITECTURE (EN, FR) lists `ren_update`, `theme`, `updatecheck` and `update_check.h` and says why Qt Network is linked; CONTRIBUTING (EN, FR) no longer says the undo goes back one level only; README (EN, FR) describes the `tests/` folder; WHATS_NEW (EN, FR) gets the new-version check; PRESENTATION gives the right page for the Undo button option.
- User guide (EN, FR): the 14 actions checked setting by setting against `core/ren_actions.cpp`; the only gap was the String insertion option, which applies to the extension instead of the name (it was described as "count within the extension").
- `English.lng` and `French.lng`, section `<QtPort>`: two more texts, "Back" and "Forward" (tool tips of the help window), found by running the program in French in the three modern page layouts.
- Quality pass (no code change): build with `-Wall -Wextra -Wpedantic -Wshadow -Wconversion` (only `qsizetype` to `int` narrowing and a few shadowed names, none that can matter for file names), `ctest` and the program (three modern layouts, Options, About and Help windows, French) under AddressSanitizer and UndefinedBehaviorSanitizer on Ubuntu 24.04 (WSL): no error.
: five texts that were still English in the French interface are added ("Open", "Save log", the two text-file filters of the file dialogs, "Version %1" of the About window).

## [3.0.0-rc.16] - 2026-10-18

### Added

- Renaming history for the session: every action of the batch run by "Go" is a pass kept in a journal (a job of three actions leaves three passes, shown as 1/3, 2/3, 3/3) (the moves, with their sidecars). Undo (F12) now goes back pass after pass, and a new *History* button (Shift+F12) lists the passes and undoes the chosen one and all those made after it. The undo is strictly linear (no undoing a pass in the middle). A file moved outside the program stops the undo and the pass stays in the journal. The last 50 passes are kept.

- Documentation of the renaming history (EN, FR): help (main window, options, news, limitations, modern layout), USER_GUIDE, USER_RECIPES, WHATS_NEW, ARCHITECTURE, TESTED_PLATFORMS (rc.16: tried on the four machines), draft release notes rc.16. The Croatian and Tamil help pages are not updated.

- Options > List behaviour: the Undo button either undoes the last pass or opens the history window. Both texts are translated into French.

- "Changed only" and "Hidden files" are icon buttons (a sheet with a pencil, a dashed sheet with an eye) instead of text, which took too much room in the toolbar; the name stays in the tool tip. Drawn in code, so they are the same in every icon set.

- Drag and drop: the import options window is opened once the drop is over (not inside the drop handler, which kept it behind the file manager) and forces the focus on Windows.

- The "Add folders" window and the drag and drop window have an *Include hidden and system files* checkbox (same setting as in the Options, remembered).

- On Windows too, a name starting with a dot (`.git`, `.config`, `.ScreamingFrogSEOSpider`) counts as hidden, like on Linux, besides the hidden and system attributes; a hidden folder is not entered when adding a folder.

- **Hidden files** button in the files list: shows or hides the hidden and system files of the list (off by default). While hidden they are skipped by every operation (`File::hidden` and `ren::options().skipHidden` in the engine, so an enumeration does not count them either); the choice is remembered. The warning before a renaming only counts them when they are shown.

- Before a rename, a warning (Yes/No, default No) appears when the list contains hidden or system files, with their count.

- *Options > Files & Folders > Include hidden and system files when adding a folder* (off by default): adding a folder no longer lists `desktop.ini`, `.git` and the like unless this is ticked; files picked or dropped one by one are always added.

- New-version check: *Options > Display > Look for a new version at start* (off by default) asks GitHub once a day for the list of releases of `morfredus/ant-renamer-qt` and offers to open the download page of a newer one; the "Check for updates" button of the About window does it on demand. Nothing but an ordinary request (program name and version as user agent) is sent. A release candidate is told about newer candidates and about the final version; a final version is not told about candidates. Version order and the choice of the release are in the engine (`ren_update`) and tested; the network part was checked against the real repository, and with a GitHub that cannot be reached (refused connection, lost packets): the answer is a silent failure after 10 seconds at most while the interface keeps running, so the program never depends on it.

## [3.0.0-rc.15] - 2026-10-18

### Fixed

- Theme: on a desktop that is dark (Linux Mint with the `.deb` and the system Qt), "Follow the system" is dark and the button asked for light but the window stayed dark: the light palette came from the desktop theme through Fusion. It is written out now. Found by reproducing it on the Mint with the environment of its session; the links in the notes under the actions also keep their colour (dark blue on dark) when the theme changes while the program runs: fixed.
- Theme: the tool tips kept the palette of the desktop theme (white text on a pale yellow background on a Raspberry Pi when the program was set to light); they follow the program's light or dark theme now.

### Changed

- AppImage: `linuxdeploy` (1-alpha-20251107-1) and its Qt plugin (1-alpha-20250213-1) are pinned to a release and checked by SHA-256 instead of downloading the moving "continuous" build; a cached or downloaded file with another hash is refused and downloaded again. Built and checked in WSL (licence file inside, a corrupted cache is replaced); the window itself was not started there (no display): test the new AppImage on a desktop.
- Windows build script: a failed build is tried a second time (the CI sometimes failed with "ld: Permission denied" while linking an executable, a file lock on the runner; the second pass links only what is missing).
- Languages: the 49 texts that only French had (a table in the code) are now in the language files, section `<QtPort>` of `English.lng` and `French.lng`, so every translator can translate them; the French interface is unchanged (checked text by text). `TRANSLATING` (EN, FR) explains the section; a `test_panel` check loads the real `French.lng`.
- `TESTED_PLATFORMS` (EN, FR): rc.13 and rc.14 tested from the packages on the four machines, the AppImage and the light / dark switch included.

## [3.0.0-rc.14] - 2026-10-18

### Added

- Theme: a "Light / dark" button in the toolbar (half-filled circle) switches at once between the light and the dark theme, and *Options > Display > Theme* offers "Follow the system" (default), "Light" and "Dark". Light and dark use the Fusion style with their own palette, so they work on every system, including an AppImage that cannot detect the desktop theme.

### Fixed

- Theme: switching with the button left part of the window in the old colours (the widgets under the window's style sheet kept their palette); the style sheet is applied again after each change. New developer aid `ANTREN_THEME_TOGGLE=n`.
- Dark theme: the highlight of a toolbar button under the mouse was light grey with light text (unreadable): the dark palette now defines the shades the toolbar style sheet uses for hover, pressed and borders.
- Log: in a dark theme (Linux, or the new dark theme) the navy and dark red lines were hard to read; lighter shades are used on a dark background, and the log is redrawn when the theme changes.
- User guide: the history size is 1 to 200 (it said "0 = none").
- CI (Windows): the interface test blocked the job for more than 20 minutes (no Qt platform plugin to run it without a screen); only the engine tests run on Windows now, the interface test stays on Linux, and every job has a time limit.

## [3.0.0-rc.13] - 2026-10-18

### Changed

- Licence: "GPL v2 or later", like the original program, everywhere (`README`, `CONTRIBUTING`, About box, help, `.deb`); the AppImage now carries the licence text (`usr/share/doc/antrenamer-qt/copyright`).
- CI (Linux): the `.deb` package is built, installed with apt and checked (binary, libraries, language files) at each run, and kept as an artifact. Not tested on GitHub yet.
- Documentation: `ARCHITECTURE`, `CONTRIBUTING`, `PRESENTATION`, `README` (EN, FR) corrected against the code (Linux path rules, ID3v2.2/2.4, line counts, tested ARM64, Linux packages, number of language files, 36 EXIF tags, "Add to Ant Renamer Qt", natural sort option in Files & Folders), a Download section and the missing folders in the README; the English guide, recipes and README say "actions" and "batch" like the English interface; `ANTREN_SIZE` documented; CHANGELOG sections of rc.10 and rc.11 in the usual order; `docs/TODO_fr.md` added.
- Issue templates: ASCII file names, English and French in the same template, the bug report asks for version, installation and system.
- Linux packaging text (`.desktop`, `.deb`): mentions IPTC and XMP.
- Tests: the NUL-character test no longer uses a file named `nul.txt` (a reserved device name on Windows Server, the cause of the Windows CI failure).
- Tests: no more fixed file names in the shared temp folder (the two test programs could clash when `ctest` runs them in parallel); the date action's `f` / `e` letters are tested.
- Windows package: the `*_autogen` folders of the test programs (with paths of the build machine) are no longer copied into the portable zip and the installer. CI: the tests also run on Windows; the workflow comment no longer names the private release scripts.

### Fixed

- Help (English and French, copied to Croatian and Tamil), corrected against the code: the log has blue lines for every file not renamed (identical name, missing ID3/EXIF information, empty line, source not found), red for the errors of the system, grey for the messages of the program (always shown, never saved); the auto-saved log has a space for the files not renamed, CR+LF and a byte order mark; names `%name%`, `%ext%`, `%folderN%`... are case-sensitive (only EXIF/IPTC/XMP tags are not); EXIF `%DateTime%` is the date written by the editing software; the history keeps 1 to 200 values; the button is "Add to batch"; dropping files does not look at Ctrl/Shift; a case change is applied on Windows too; in the date mask `f` and `e` are replaced even between single quotes; the French EXIF message is translated.
- Help: "associés" and "Opérations" everywhere in French (the table of contents said "Actions"), a few English spelling mistakes, wrong language codes in the Tamil links, a mojibake `.arb` in the Croatian batch page, the Tamil and Croatian index pages say Ant Renamer Qt.
- `German.lng` was not valid XML (an entity without its semicolon on line 199): the loader dropped silently every text after it, so about 57 % of the German interface was in English (86 % of the keys are translated now). It was the only invalid file of the 28.
- The language loader no longer fails silently: an XML error is reported on the error output with the file, the line and the number of texts read before it; a UTF-8 byte order mark is accepted (such a file used to be ignored altogether); a file declared UTF-8 that holds windows-1252 text is read as windows-1252; the English text decides about the spaces at the start and the end of a translation (the log lines built from several pieces were glued together in a dozen languages). Tested in `test_panel`.
- Notes under the actions: the `<LINK>` that names another action must equal the translated name in the actions list, or the click does nothing. Fixed in Italian, Polish, Portuguese (Brazil), Hungarian, Japanese, Slovak, Czech, Tamil (both files) and the two Spanish (Argentina, Colombia) files. Greek and Polish had translated the tags themselves (`%autor%`, `$jeżeli(`, `%τιτλος%`, `<λινκ>`): the program only knows the English ones, they are restored; the Greek "Generate an enumeration" text lost its fourth placeholder.
- Log descriptions that no language could translate: "Generate namees with MP3 tags" (typo in the code, none in English.lng) and "Generate names with EXIF info" (the reverse), "with %1 min offset" (the offset is in seconds, as the editor says: "s offset" now) and "Could not get file date" (absent from the language files: added to English and French).
- Credits: the About window now lists every translator named in the language files or in the help credits (13 names were missing); the help gives the Czech translator as GVG (was "CVG"), the French page translates the language names and the licence text.
- Interface, during a renaming: adding files or folders, sorting, the header menu and saving the batch could modify the lists the worker thread was reading (undefined behaviour). Now the buttons and shortcuts that modify the list, the batch or the options (add, remove, move, batch commands, Options, Undo, Preview, "Changed only") are disabled until the run ends, and the handlers refuse to act. The rows of the list are refreshed from a copy made by the worker, not from the files it is still editing. The progress bar of an Undo now counts one pass instead of one pass per action of the batch.
- Command line: `-af folder\*.*` skipped the files without extension and `-aF folder\*.*` the folders without a dot, because Qt's `*.*` only matches names that contain a dot. `*.*` now means everything.
- Multiple string replacement: "Delete" with an empty set name removed every saved set (and the predefined accents set never came back). A "/" in a set name made the set invisible: "/" and "\" are now replaced by a dash.
- The auto-saved log (Options > Events) contained the entries of the previous runs again when "Clear log" was off, and a write failure was silent. It now holds only the run that just ended and a red line says when the file cannot be written.
- Settings: the one-time switch to the "Modern" icon set only replaces the stale value 0; a deliberate choice of "Windows XP" or "XP Large" is kept. Values read from the INI file (icon set, history size, font size, page switches, layout) are clamped to their valid range.
- The label next to a check box toggled it on any mouse button, even on a release outside the label; only the left button released over the label does now.
- A file whose ID3v1 year is blank (NUL padded) lost its extension when `%year%` was in the mask: the system cut the new name at the NUL. The year is read like the other ID3v1 fields, NUL characters are dropped from tag values and the other control characters become a dash; a new name that still contains a NUL is refused ("Could not rename") instead of being truncated.
- An ID3v2.3 file with a corrupt extended header (huge size field) could crash the MP3 action (read before the buffer). The skipped size is now computed in 64 bits and bounded. ID3v1.0 tags (comment text up to the last byte) no longer give a bogus track number: only ID3v1.1 (byte 125 = 0) has one.
- EXIF / IPTC readers: every offset and size read from the file is 64-bit and bounds-checked (signed overflow was undefined behaviour and could crash on a corrupt picture); a file that merely starts with "MM" is no longer taken for a TIFF; an APP13 segment shorter than 16 bytes is ignored.
- Regular expressions: an invalid expression used to become the new file name (on Linux the files were really renamed to `<RegExp Error> ...`). The preview still shows the error, but a real run renames nothing and the log says why. An empty expression no longer inserts the replacement between every character.
- EXIF date offset is applied to the calendar fields, not through the local time zone: no more wrong result around the daylight-saving changes (2024:10:27 02:30 + 1 h gave 02:30, now 03:30).
- "Apply to extension" (String replacement, String insertion, Characters deletion) no longer leaves a dot at the end of the name when the extension ends up empty (`Makefile` became `Makefile.`).
- Date and time mask: a repeated `a` (`aab`) kept only one letter.
- Batch files: loading an XML file that is not a batch is refused instead of emptying the current batch; saving writes a temporary file and replaces the old one only on success; numbers read from a batch file are clamped to the ranges of the editor (a corrupt `Digits="2000000000"` froze the program).

## [3.0.0-rc.12] - 2026-10-17

### Added

- New toolbar icon set "Modern" (Options, Display, Icon set): 30 line icons drawn in code with `QPainter` on a 24 x 24 grid (`QIconEngine` in `app/icons.cpp`), so they are sharp at any size and screen scaling, follow the light or dark colours of the system and are greyed when disabled; green and red badges for add / remove, green Go, red Stop, black Help and About (the bubble "i"). No new dependency and no image file. It is the default set for everyone who has not chosen one yet (settings saved by an older version are switched once, because every option is saved, so "Scrows" was stored without anybody having chosen it; after that, the choice made in Options is kept); "Scrows" and the Windows XP sets stay available.

### Changed

- User guide (`docs/USER_GUIDE.md`) and recipes (`docs/USER_RECIPES.md`), English and French, brought up to date: every option of every operation, tags table, Windows / Linux differences, icon sets, help and languages; `/` as the folder separator in the examples, with the Linux notes; `WHATS_NEW` and the READMEs updated.
- Help: the pictures of the buttons now show the "Modern" icons (all the small icons, the toolbar strips and every screenshot were remade from the program, in English and French, including the modern layout), and a picture of the options page shows the correspondence between the classic and the modern icons (new section "The toolbar icons" of the options help, linked from the main window page). A pass of checks on the help: same structure in English and French on every page, all links and images resolve, option names match the program.
- New guide for translators, `docs/TRANSLATING.md` and `docs/TRANSLATING_fr.md`: the whole process of a `.lng` file (create, encoding, header, rules for the texts, test with `ANTREN_MISSING=1`, credits, submission, help pages), linked from CONTRIBUTING.
- Help pages converted from HTML to Markdown (UTF-8, 76 files in 4 languages, same text): the table of contents is now `toc_<lang>.md` (a nested list of links) instead of the CHM `.hhc` file, and `main.css` is gone. This also ends the mix of encodings (iso-8859-1, windows-1250, UTF-8) of the old pages. The help window shows headings, lists, tables and screenshots as before; links to a heading (`page.md#id`) still work. The About window's history tab reads `history_<lang>.md`.
- Help images: the six screenshots of the modern layout are recompressed (palette PNG, about 385 KB down to 120 KB) and the 11 unused `*xp.png` icons are removed. The `help/` folder goes from 0.94 MB to 0.55 MB (files, not disk blocks).
- Help reviewed against the program (English and French, every page): wrong or outdated statements removed or corrected (regular expressions are PCRE2 and not TRegExpr, ID3v2 is read, "Windows tick count" and GUID details, forbidden characters always replaced in mp3 tags, "c/ddddd/dddddd" date tags do not exist, the font option changes the whole program, settings location, Linux notes such as "/" as folder delimiter), and missing items added (options of each action, Multiple string replacement buttons, Case options, offset in seconds, `z`/`zzz`/`ampm` date tags, numbering and offset of the EXIF action, Events and Language pages of the options, portable mode, Explorer menu, keyboard shortcuts table, tags table and `$if` anchors in "Formatting").
- Help: new screenshots of the real program (Files, Actions and Log pages, the five pages of the Options), in English and French. The help window opens wider (1040 x 650) for them.
- Help in Croatian and Tamil: the pages that were rewritten (Actions, Files, Adding files, Log, Options, Fonts, Formatting, Regular expressions, Main window, What the program does, About, news) show the English text until translated, so that they no longer say what is not true any more.
- Developer helpers: `ANTREN_ACTION=n`, `ANTREN_PREVIEW=1` and `ANTREN_SIZE=WxH` (see `docs/BUILDING.md`) to take reproducible screenshots.

### Fixed

- Build: changing only a help page or a language file was not copied next to the executable (the copy ran only after a link). It is now a target of its own (`antrenamer-data`), run at every build, which also removes deleted help files.
- Linux: a file name containing a colon (`a:b.txt`) or a backslash was cut at that character, as if it were a folder delimiter (Windows rules applied everywhere). On Linux only `/` delimits now. Also fixed on Linux: the files of the list that are inside a renamed folder did not follow it, `%folder-N%` (count from the root) gave nothing, and the `%count%` tags counted nothing. With "Detect absolute path" a new name that begins with `/` is now an absolute path. The unit tests use paths of the running system. Tested and working on Linux Mint 22.3 Xfce (x86-64) and on Debian 13 (Raspberry Pi 4, arm64) from the rc.12 packages.
- Help: external links (dEXIF, regular expression reference) were broken by the conversion to Markdown; links to a section of another page (`page.md#folder`...) now all land on their section (a link checker found and fixed the missing anchors).
- Help, Actions page (all four languages): the list of tags of "String insertion" (`%name%`, `%ext%`, `%folderN%`) had a broken list structure in the source.

## [3.0.0-rc.11] - 2026-10-16

### Added

- `docs/TESTED_PLATFORMS.md` / `TESTED_PLATFORMS_fr.md`: systems and versions the program has really been tested on, linked from both READMEs.
- Developer documentation (English and French, linked from both READMEs): `docs/BUILDING.md` (requirements, presets, what a build does, packaging, command line), `docs/ARCHITECTURE.md` (layout, engine, interface, one renaming end to end) and `CONTRIBUTING.md` (workflow, recipes to add an operation, option, tag, sidecar type, language, pitfalls).

### Changed

- Line endings: every text file is now LF (`.gitattributes` with `* text=auto eol=lf`; binary files are marked `binary`). Before, the repository mixed CRLF (help pages, language files, some sources) and LF. Only the line endings changed, not the content or the encodings; no effect on the program.

### Fixed

- `.deb` package refused on Debian 13 (`libqt6gui6t64 is only referenced by name`): dependencies detected on Ubuntu carried the `t64` suffix. Each `xxxt64` dependency is now written `xxxt64 | xxx`, so the package installs on both Ubuntu 24.04 and Debian 13. Checked: arm64 package installed from the `.deb` and working on Debian 13 (Raspberry Pi 4, two machines); build and dependency list on Ubuntu 24.04. Not yet confirmed on Debian 13 x86-64.
- `.deb` package: `Recommends: qt6-wayland`, so apt installs the native Wayland platform plugin by default (the program otherwise falls back to XWayland).

## [3.0.0-rc.10] - 2026-10-15

### Added

- `.github/dependabot.yml`: Dependabot checks the GitHub Actions used by the workflows once a month (pull requests when a newer version exists).
- `docs/USER_GUIDE_fr.md`: user guide (English `USER_GUIDE.md` and French `USER_GUIDE_fr.md`, linked from both READMEs and the presentation) listing every function (the 14 operations, shortcuts, files list, jobs, masks, EXIF/IPTC/XMP tags, command line, log, options) with instructions.
- `docs/USER_RECIPES.md` / `USER_RECIPES_fr.md`: recipes guide (how to reach a result: cleaning names, numbering, photos, music and videos, moving and filing, command line, common problems), linked from both READMEs, the presentation and the user guides. Samples checked against the core engine.

### Changed

- CI workflow: `actions/checkout` and `actions/upload-artifact` updated from v4 to v7 (the two first Dependabot pull requests, whose checks passed). No effect on the program.

### Fixed

- Sidecar files in the list are no longer renamed a second time by the mask when their picture is in the list too, whatever the order (they were renamed a second time: ".xmp", errors "already exists"; or before their picture).
- Sidecar files (`IMG.DNG.xmp`, `IMG.DNG.dop`) that are also in the files list now follow their picture: on Windows the list entry kept its old path (backslashes vs slashes in the comparison), so after the picture was renamed (sidecar renamed on disk) the entry failed with "Source file not found". Test with Windows paths in `test_core`.
- Date and time mask: `am/pm`, `AM/PM` and `a/p` now switch the hours to the 12-hour clock and print am / pm (a / p), as the help says and as Delphi's `FormatDateTime` does; before, the hours stayed on 24 hours and the marker was printed wrongly (`a10/p10`). `ampm` still prints AM / PM (12-hour hours now too). A marker inside quotes stays plain text. Test in `test_core`.

## [3.0.0-rc.9] - 2026-10-14

### Added

- Sidecar files follow their picture: when a file is renamed, its `.xmp` (`IMG.xmp`, `IMG.CR2.xmp`) and DxO PhotoLab `.dop` (`IMG.dop`, `IMG.CR2.dop`) sidecars (also upper-case extensions) are renamed with it, put back on revert, and reported in the log line (a sidecar that cannot be renamed, for example because the target exists, never fails the picture). A sidecar that is itself in the files list follows too. New option "Also rename the sidecar files (.xmp, .dop) of a renamed picture" (Options > Processing, on by default, `renameSidecar`); not used in copy mode. The list of extensions is `sidecarExtensions()` in `core/ren_exif.cpp`. Tested on real DNG + JPG pairs with darktable sidecars and a DxO `.dop`.
- EXIF action: the XMP sidecar of a file is read with it and wins over the embedded XMP, so the keywords, title and rating of a RAW file (that cannot embed them) are usable in the mask. `sidecarCandidates()` in `core/ren_exif.*`, tests in `test_core`.
- `docs/WHATS_NEW.md` and `WHATS_NEW_fr.md`: list of what Ant Renamer Qt adds or improves compared with Ant Renamer 2.13, with the version of each item ("a venir" for unreleased ones); to be completed with every user-visible change.
- Files list: "Changed only" button next to Preview. It hides the files whose name does not change in the preview (view filter only: hidden files stay in the list and are renamed as usual, but are never selected, so "Remove sel." and the move buttons cannot touch them). Useful to review the few files concerned in a big list (for example only the names with umlauts). Saved between sessions. English and French texts, help (Files page; also Croatian and Tamil, with the wildcards and shared-number paragraphs).
- EXIF action: IPTC and XMP data can be used in the mask, with new tags `%IptcKeywords%`, `%IptcCaption%`, `%IptcByline%`, `%IptcCity%`... (22 IPTC tags, read from the APP13 segment of JPEG files, UTF-8 or Latin-1) and `%XmpTitle%`, `%XmpKeywords%`, `%XmpRating%`, `%XmpCreateDate%`... (26 XMP tags, read from the XMP packet found in any file: JPEG, PNG, WebP, TIFF, DNG, `.xmp`). Repeated values (keywords) are joined with a comma; IPTC/XMP dates use the same layout as the EXIF ones. A file with only IPTC or XMP is no longer reported as "No EXIF info found". Covered by a test (`core`) that builds a JPEG with IPTC and XMP.
- Help: news page ("Ant Renamer Qt: news and support") lists the IPTC/XMP tags, the sidecar files, the wildcards, the shared number and the "Changed only" button, and no longer says the EXIF reader knows about forty tags; the EXIF section of the Actions page describes the sidecar files; README (EN, FR) mention IPTC and XMP.
- Help: new page "EXIF, IPTC and XMP tags" (English, French; English text in the Croatian and Tamil help) listing every usable tag with its meaning, plus tips ($if fallback between IPTC and XMP, dates, several values, limits). It replaces `exif_tags.txt`, a list of 250 dEXIF names of which the program only knew about forty.
- Enumeration and EXIF actions: "Same number for files with the same name and different extensions" option (off by default), so a JPG and its RAW (or XMP) get the same number. A file continues the previous group only if it is in the same folder, has the same base name and an extension not yet in the group (case-insensitive); otherwise it gets a new number. Saved in the batch files (`SharePairs`); older batches load with it off. English and French texts, help (Actions page, Enumeration and EXIF sections), test in `test_core`.
- String replacement: "Use wildcards" option (off by default). `*` in the search field stands for any text and `?` for any single character; every other character is searched as it is, and the replacement text stays literal (for example "IMG_*_copy" replaced by "Photo" turns `IMG_001_copy (1).jpg` into `Photo (1).jpg`). Saved in the batch files (`Wildcards`); older batches load with it off. English and French texts, help (Actions page), test in `test_core`.

### Changed

- `docs/proposals/` removed (UI modernisation draft and two HTML mockups): the modern layout and the single screen are implemented and described in the help; the files stay in the git history.
- Presentation documents (EN, FR): "Quality and testing" now says that the Explorer menu was tested in Explorer (one or several files, one or several folders with their contents) and that the Linux packages install and run; only ARM64, very large lists, network drives and an administrator install remain untested.
- Actions editor: the notes under the parameters (and the check box texts) now always wrap to the panel width instead of being cut by a horizontal scroll bar. The EXIF notes mention the IPTC/XMP tags (one short line) and their "Help" link opens the new tags page. Clicking the text of a check box still ticks it.

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

- Option "Natural sort in files list (2 before 10) instead of alphabetical" (Options > Files & Folders), enabled by default: sorting the Files list by text columns now puts `fichier 2` before `fichier 10`. Untick it to get the alphabetical order of the original program. English and French texts.

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
