# Ant Renamer Qt: news and support

**Ant Renamer Qt** is a C++/Qt 6 port of Ant Renamer 2.13 by Antoine Potten, made by Frédéric Biron. It is developed in parallel with the original program: it has its own executable (`antrenamer-qt`), installation folder and settings, so both can be installed side by side. The batch files (`.arb`) are the same, so each program reads the batches of the other. The rest of this help describes Ant Renamer Qt; it started from the help of Ant Renamer 2.13 and has been brought up to date. What is new, compared with the original program, is listed below.

## What is new

- **Recent Windows and Linux**: Windows 10 and 11 (long paths, high resolution screens, dark theme) and Linux (`.deb` packages for amd64 and arm64, AppImage).
- **ID3v2 tags**: the ["Use mp3 tag info"](actions_ta.md) action reads ID3v2.2, 2.3 and 2.4 tags, in addition to ID3v1. ID3v2 values are used first.
- **Counter in the EXIF action**: the ["Use EXIF info"](actions_ta.md) action has a `%num%` tag with the same settings as the enumeration (start, digits, increment, restart in each folder), see [formatting strings](format_ta.md).
- **Windows Explorer context menu**: *Options* \> *Files & Folders* adds "Add to Ant Renamer Qt" to the right-click menu of files and folders. Everything you select is sent to the window that is already open. On Windows 11 the entry is under "Show more options" (or Shift+F10).
- **Portable mode**: the settings are stored in the file `antrenamer-qt.ini`. When this file sits next to the program, everything stays in that folder (USB key, no trace on the computer). Otherwise it is in your user profile. The checkbox in *Options* \> *Language* moves the settings from one place to the other. Old `Renamer.xml` settings are not imported.
- **More options**: icon set (Scrows, Windows XP, Windows XP Large, and the new vector set "Modern"), forced font, automatic column width, applied immediately; toolbars can be moved; the list of languages shows version, authors and comments.
- **Built-in help**: this window, with the table of contents of the original help.
- **Command line**: see [command-line parameters](params_ta.md) (`--shell`, `--install-shell`, `--remove-shell`).
- **Modern interface layout (option)**: another presentation of the main window, with the three pages on one screen if you prefer (in bands or in columns), and actions of the batch that can be edited. The classic layout stays the default. See [modern interface layout](modernui_ta.md).
- **Natural sort**: the Files list sorts "file 2" before "file 10" by default; the alphabetical order of the original program is an option (*Options* \> *Files & Folders*).
- **IPTC and XMP in the EXIF action**: keywords, title, caption, author, place, rating... can be used in the mask (`%IptcKeywords%`, `%XmpTitle%`...), see [EXIF, IPTC and XMP tags](metadata_tags_ta.md). The XMP sidecar file of a picture is read too.
- **Sidecar files follow their picture**: renaming a picture also renames its `.xmp` and DxO `.dop` files (IMG.xmp, IMG.CR2.xmp, IMG.dop...), and the revert puts them back ([Options](options_ta.md) \> *Processing*, enabled by default).
- **Wildcards** in [String replacement](actions_ta.md#stringrepl) (`*` and `?`), and an option to give **the same number to files with the same name** and different extensions (JPG + RAW) in the Enumeration and EXIF actions.
- **"Changed only" button** in the [Files list](files_ta.md): hides the files whose name does not change in the preview.

## Corrections compared with 2.13

- An empty ID3 or EXIF tag stays empty. Before, it was replaced by a number, so `$if(%title%,...)` was always true.
- A lone `$` in a mask no longer freezes the renaming.
- The "Restart" setting of the enumeration is saved correctly in batch files.
- The preview of the date, mp3 and EXIF actions works when they come after another action.
- Names with characters outside the system code page are no longer damaged by the "Use date & time" action.

## Small differences to know

- Regular expressions use the PCRE2 engine (Qt) instead of TRegExpr: very unusual constructs may behave differently.
- The EXIF reader knows 36 common EXIF tags (plus the IPTC and XMP ones), not the whole list of the original program.

## Support

Support for Ant Renamer Qt is provided on GitHub: [github.com/morfredus/ant-renamer-qt](https://github.com/morfredus/ant-renamer-qt/)

- **Issues**: to report a bug or ask for a feature. Please give the version of the program (*About* window), your system (Windows or Linux, version) and the steps to reproduce the problem.
- **Discussions**: for questions, help and ideas.

Please do not report problems of Ant Renamer Qt to the author of the original program: Ant Renamer 2.x is supported by its author at [www.antp.be/software/renamer](https://www.antp.be/software/renamer/).
