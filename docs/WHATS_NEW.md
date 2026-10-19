# Ant Renamer Qt: what is new since the original version

*What Ant Renamer Qt adds or improves compared with Ant Renamer 2.13 (Antoine Potten, Delphi 7).*
French version: [WHATS_NEW_fr.md](WHATS_NEW_fr.md). This list is kept up to date as development goes on. For the
technical detail: [CHANGELOG](../CHANGELOG.md). For the port itself (what is kept, the deliberate differences):
[PRESENTATION.md](PRESENTATION.md).

Current version: **3.0.0-rc.16**. Items marked *(upcoming)* are in the code but not published yet.

## Additions

### Renaming

| What is new | Since |
|---|---|
| **Wildcards in string replacement**: "Use wildcards" option. `*` = any text, `?` = any single character (`IMG_*_copy` replaced by `Photo`: `IMG_001_copy (1).jpg` becomes `Photo (1).jpg`). | 3.0.0-rc.9 |
| **IPTC and XMP data in the EXIF action**: keywords, title, caption, author, place, rating, color label... usable in the mask like the EXIF tags (`%IptcKeywords%`, `%XmpTitle%`, `%XmpRating%`...), read from JPEG (IPTC) and from any file embedding XMP (JPEG, PNG, WebP, TIFF, DNG, `.xmp`). The help lists every available tag. | 3.0.0-rc.9 |
| **Sidecar files follow their picture**: renaming `IMG.CR2` also renames its `.xmp` (`IMG.xmp` or `IMG.CR2.xmp`: Lightroom, Capture One, digiKam, darktable) and its DxO PhotoLab `.dop`, and the revert puts them back. On by default, Options > Processing. The XMP sidecar is also read for the EXIF action (keywords, rating of a RAW file). | 3.0.0-rc.9 |
| **Same number for JPG + RAW pairs** (and XMP): optional box in Enumeration and EXIF, so `IMG_1234.JPG` and `IMG_1234.CR2` get the same number. | 3.0.0-rc.9 |
| **`%num%` counter in the EXIF action** (start, digits, increment, restart per folder), same settings as the Enumeration action. | 3.0.0-rc.1 |
| **ID3v2.2, 2.3 and 2.4 tags** for the MP3 action (the original only read ID3v1). | 3.0.0-rc.1 |

### Working in the list

| What is new | Since |
|---|---|
| **History of the renamings**: every action of the batch run by Go is a *pass*; **Undo** (F12) goes back pass after pass (the original had one level), and the **History** button (Shift+F12) lists the passes of the session and goes back to a chosen one, undoing it and all the passes after it. Always linear, the last 50 passes are kept for the session. An option makes the Undo button open the history instead. | 3.0.0-rc.16 |
| **New-version check** (off by default): *Options > Display > Look for a new version at start*, or the *Check for updates* button of the About window. Once a day it asks GitHub for the list of releases of the project and offers to open the download page; nothing else is sent. | 3.0.0-rc.16 |
| **Hidden and system files** (`desktop.ini`, `.git`...) of an added folder are skipped by default (*Options > Files & Folders* to include them); a **Hidden files** button shows or hides those in the list, and while they are hidden no operation touches them. A warning is shown before a renaming if they are shown and the list holds some. | 3.0.0-rc.16 |
| **"Changed only" button**: hides the files whose name does not change in the preview (ideal to check the 2,000 names concerned in a list of 50,000). It is only a view filter: hidden files stay in the list, are renamed as usual, and cannot be hit by "Remove sel." or the move buttons. | 3.0.0-rc.9 |
| **Natural sort** in the files list (`file 2` before `file 10`), on by default; untick it to get the original alphabetical order. | 3.0.0-rc.7 |

### Interface

| What is new | Since |
|---|---|
| **New-version check** (off by default): once a day, or on demand from the About window, the program asks GitHub whether a newer release exists and offers to open its page. Nothing else is sent. | 3.0.0-rc.16 |
| **Light / dark theme**: a toolbar button switches at once, and *Options > Display > Theme* can follow the system, force light or force dark (the log is readable in both). | 3.0.0-rc.14 |
| **"Modern" icon set** (default): 30 line icons drawn by the program, sharp on any screen and screen scaling, light and dark theme, greyed when disabled. Options > Display > Icon set offers it with the three classic sets (Scrows, Windows XP, Windows XP Large); the help compares the four. | 3.0.0-rc.12 |
| **Help rewritten and checked against the program**: Markdown pages (UTF-8, one table of contents per language), every option and action described, tables of tags and keyboard shortcuts, new screenshots of the real program in English and French, links between pages and sections verified. Croatian and Tamil show the English text for the rewritten pages. | 3.0.0-rc.12 |
| **Modern layout**, as an option (Options > Display): one main bar with tabs showing the number of files and actions, a prominent Go button, a bar per page, the new-name column visible by default, the system light or dark theme. The classic layout stays the default. | 3.0.0-rc.8 |
| **Single screen**: Files, Actions and Log all visible, in bands or in columns, movable splitters, remembered sizes. Idea: Antoine Potten. | 3.0.0-rc.8 |
| **Edit an action of the batch in place**: double-click the action to load its settings in the editor, then "Modify" (or "Validate / Cancel the modification"). | 3.0.0-rc.8 |
| **Portable mode**: an `antrenamer-qt.ini` file next to the program keeps all settings in that folder. | 3.0.0-rc.1 |
| **Ready for Windows 10/11**: paths over 260 characters, UTF-8, high-resolution screens, dark theme. | 3.0.0-rc.1 |
| **Explorer context menu** "Add to Ant Renamer Qt" on files and folders, sent to the window that is already open. | 3.0.0-rc.1 |

### Platforms and distribution

| What is new | Since |
|---|---|
| **Linux**: `.deb` packages (amd64, arm64) and x86_64 AppImage. | 3.0.0-rc.2 |
| **Built-in help** in a program window (no external browser), with the table of contents of the original help. | 3.0.0-rc.3 |
| **Windows**: installer and portable zip. | 3.0.0-rc.1 |
| Official name **Ant Renamer Qt**, living next to the original (separate executable, settings and Explorer entry); `.arb` batches stay readable by both programs. | 3.0.0-rc.2 |

## Improvements

| Improvement | Since |
|---|---|
| **Robustness**: invalid regular expressions rename nothing, damaged MP3 tags (blank year, corrupt header) no longer cut names or crash, buttons are disabled during a renaming, `*.*` also matches files without extension, a language file with an error is reported instead of silently ignored (German is now fully translated). | 3.0.0-rc.13 |
| **Linux file names**: a name containing a colon or a backslash (`a:b.txt`) was cut at that character. On Linux only `/` delimits a path now; files inside a renamed folder follow it, `%folder-N%` and the `%count%` tags work, and "Detect absolute path" recognises a name that begins with `/`. | 3.0.0-rc.12 |
| **Guide for translators** (`docs/TRANSLATING.md`): the whole process of a `.lng` file, from the copy of `English.lng` to the pull request. | 3.0.0-rc.12 |
| **User guide and recipes brought up to date**: Windows / Linux differences, the options of every action, the new icon set. | 3.0.0-rc.12 |
| Actions editor: the long texts of the check boxes wrap to the next line when the panel is narrow (instead of being cut off with a scroll bar). | 3.0.0-rc.9 |
| Translations: a text appearing twice in the language files with two different translations got one or the other at random; the choice is now fixed. | 3.0.0-rc.8 |
| Files list: sorting by clicking a column header stopped working after the first click. | 3.0.0-rc.7 |
| Regular expressions with PCRE2, preview that reads the real file for the date / MP3 / EXIF actions, case-only renames done properly on case-insensitive volumes (see "Deliberate differences" in the presentation). | 3.0.0-rc.1 |
| Help: "Modern interface layout" chapter with screenshots; Files and Actions pages completed (English, French; Croatian and Tamil for the recent additions). | 3.0.0-rc.8 and later |

## Keeping this file up to date

Every change visible to the user adds a line here, under "Additions" or "Improvements", with *(upcoming)* in the
"Since" column. When a version is published, replace *(upcoming)* with its number. Keep
[WHATS_NEW_fr.md](WHATS_NEW_fr.md) in step.
