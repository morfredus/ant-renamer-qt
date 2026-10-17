# Ant Renamer Qt: function guide

*Every function of Ant Renamer Qt 3.0.0-rc.12 with instructions.*
French version: [USER_GUIDE_fr.md](USER_GUIDE_fr.md). Recipes (step-by-step use cases): [USER_RECIPES.md](USER_RECIPES.md). Full details, page by page, with screenshots: the built-in help (F1). Differences between Windows and Linux: §13.

## 1. General principle

The work is done in three steps, one page each:

1. **Files**: build the list of files (and folders) to rename.
2. **Operations**: choose one or more renaming operations (the "job").
3. **Log**: read what happened after clicking **Go**.

The preview at the bottom of the Operations page shows the current and future name of the selected file, live. The **Preview** column of the Files list (F5) shows it for the whole list.

The program only changes **names** (or copies the files under a new name); it never modifies the contents or the dates of the files. It handles Unicode names (Greek, Japanese, Cyrillic...), files and folders.

## 2. Keyboard shortcuts

| Key | Function | Key | Function |
|---|---|---|---|
| F1 | Help | F9 | Go |
| Shift+F1 | About | Esc | Stop |
| F2 | Add files | F10 | Options |
| F3 | Add folders | F11 | Always on top |
| F5 | Refresh preview | F12 | Undo |
| F6 / F7 / F8 | Remove selected / all / dead | Alt+F4 | Exit |
| Ctrl+1 / 2 / 3 | Files / Operations / Log page | Ctrl+F2 | Add to job |
| Shift+F6 / Shift+F7 | Remove selected / clear the job | Ctrl+O / Ctrl+S | Open / save a job |
| Ctrl+↑ / Ctrl+↓ | Move up / down | Ctrl+Shift+↑ / ↓ | Move to top / bottom |

## 3. Files page

### Adding files
- **Files** (F2): standard file picker (Ctrl and Shift select several files). The order the system returns is not always the order displayed: sort the list afterwards if the order matters.
- **Folders** (F3): shows the drives and folders as a tree; select one or more, then tick what to add: the files they contain, the folders themselves, the subfolders (all levels). The three choices are remembered.
- **Drag and drop** from the file manager (Explorer, Nautilus, Dolphin, Total Commander...): a window offers the same options. "Do not show this window again" remembers them (changeable in Options > Files & Folders); holding Ctrl or Shift when dropping shows the window once more.
- **Explorer menu** "Add to Ant Renamer Qt" (Windows; enable it in Options > Files & Folders). Several selected items arrive together in the window that is already open. Windows 11: *Show more options* (or Shift+F10).
- **Command line**: see §8.

### Managing the list
| Button | Effect |
|---|---|
| Remove selected (F6) | removes the selected files from the list (not from disk) |
| Remove all (F7) | empties the list |
| Remove dead (F8) | removes files that no longer exist on disk |
| Refresh preview (F5) | fills the Preview column (not automatic, for performance). It uses the job, or the action displayed if the job is empty |
| Changed only | hides the files whose name does not change in the preview (a display filter only: they are still renamed, and a hidden file is never hit by "Remove selected" or the move buttons); refresh the preview first |
| Up / Down / Top / Bottom | changes the processing order (drag and drop in the list works too) |

- **Columns**: right-click the header to show Folder, File name, Full path, Ext., Preview, Size, Creation date, Modification date. They can be resized and moved; one column at least stays visible.
- **Sorting**: click a header (again to reverse). By default the sort is natural: "file 2" before "file 10" (Options > Files & Folders gives the alphabetical order back). **Sorting really reorders the list, so it sets the processing order**, hence the numbering order of an enumeration. Right-click > *Unsorted items* removes the sort indicator.
- Right-click in the list: remove selected / all, select all, select none, invert the selection, refresh the preview.
- Double-click a file: opens it with its associated program, if "Launch file on double click" is ticked (Options > Files & Folders).
- Folders can be in the list too: they are renamed like files (in list order); the files of the list that are inside a renamed folder follow it.

### Sidecar files of a photo
When a photo is renamed, its sidecar files are renamed with it, with nothing more to do:
- **Recognised**: `.xmp` (Lightroom, Capture One: `IMG.xmp`; digiKam, darktable: `IMG.CR2.xmp`) and DxO PhotoLab `.dop` (`IMG.dop`, `IMG.CR2.dop`), upper-case extensions included.
- **Method**: in Options > Processing, tick "Also rename the sidecar files (.xmp, .dop) of a renamed picture" (on by default). Then rename the photo as usual (for example `IMG_0001.CR2` → `Paris 001.CR2`): `IMG_0001.xmp` or `IMG_0001.CR2.xmp` becomes `Paris 001.xmp` or `Paris 001.CR2.xmp`.
- **Undo** (F12): the sidecars get their old name back with the photo.
- **Log**: the photo's line mentions the renamed sidecar.
- **Special cases**: if a file with the new name already exists, the sidecar is left alone and the log says so (the photo is renamed anyway); a sidecar that is also in the list follows without being renamed twice; no effect in "Copy instead of rename" mode and for folders.
- The neighbouring `.xmp` file is also read by the EXIF / IPTC / XMP operation (§7).

## 4. Operations page: the 14 operations

Pick the operation in the list, set its parameters (the *Notes* area under the parameters recalls the available tags; the names of other operations in the notes are links, and "see Help" opens the help), then **Add to job** (Ctrl+F2). Several operations can be stacked: they apply in order, each on the name already changed by the previous ones (except the metadata operations, which read the real file). If the job is empty, **Go** and the preview use the operation currently displayed, once.

**Folders and extensions.** By default a folder name is treated like a file name without extension: operations that work on the extension do nothing on a folder, and `%ext%` is empty. Options > Processing ("The last dot of a folder name delimits the extension") changes that.

| # | Operation | Purpose | How to use it |
|---|---|---|---|
| 1 | **Change extension** | Replace or add an extension | Enter the new extension (without the dot). Empty field = removes the extension. Option: add after the existing one instead of replacing. |
| 2 | **String replacement** | Replace one text with another | "Search" / "Replace by" fields. Empty = deletes the text. Options: replace all occurrences (on by default, otherwise only the first), case sensitive (on by default), include the extension in the search, apply to the extension instead of the name, **wildcards** (see below). |
| 3 | **Multiple string replacement** | Several replacements at once (accents, etc.) | 2-column grid (searched / replaced), applied line after line. Ctrl++ adds a line, Ctrl+- deletes the current line. Options: include the extension, case sensitive. A grid can be named and saved ("Predefined sets" list + *Save*; *Delete* removes a set, *Clear Table* empties the grid). Copy/Export and Paste/Import exchange the grid with Excel or a text editor (tab between columns). A "Remove west-european accents" set is provided. |
| 4 | **String insertion** | Insert a text at a position | Text, position (0 = before the first character or after the last one), from the start or from the end. Option: count within the extension. Tags `%name%`, `%ext%`, `%folderN%`, `%%`. |
| 5 | **Move string** | Move N characters | Number of characters, "from" position and "to" position (the second counts after the characters are removed), each from the start or the end. E.g. `abcdef`, 1 character from 2 to 4 → `abdecf`. |
| 6 | **Characters deletion** | Erase N characters | Three modes: from a position (from the start or the end), after a given string, or before a given string. Nothing is deleted if the string is not found. Option: apply to the extension. |
| 7 | **Enumeration** | Number the files | Mask containing `%num%` (e.g. `File %num%%ext%`; tags `%name%`, `%ext%`, `%folderN%`). Settings: *Start at*, *Digits* (leading zeros), *Increment by* (start and increment may be negative). Options: *Go back to start value* in each new folder (list sorted by folder), *Same number for files with the same name* (IMG_1234.JPG + IMG_1234.CR2: they must follow each other, sort by name). |
| 8 | **MP3 tags** | Name from the ID3 tags (v1, v1.1, v2.2, v2.3, v2.4) | Mask with `%author%`, `%title%`, `%album%`, `%year%`, `%genre%`, `%comm%`, `%track%`, `%name%`, `%ext%`, `%folderN%`. E.g. `%author% - %title%%ext%`. v2 values win, v1 fills the gaps. Characters forbidden in a file name are always replaced in the values (`"` → `'`, `?` → `.`, `\ / : * < > \|` → `-`). Option: two-digit track number. `$if` available (§6). A file without ID3 tags is not renamed (blue line in the log). |
| 9 | **Date and time** | Name from the file date | Mask in letters: `yyyy` `yy`, `mm` `mmm` `mmmm` (month), `dd` `ddd` `dddd`, `hh` `nn` (minutes) `ss`, `zzz` (milliseconds), `am/pm` `a/p` `ampm` (12-hour clock), `f` (original name), `e` (extension). Any fixed text goes in quotes. E.g. `yyyy'-'mm'-'dd hh'-'nn'-'sse`. The names of days and months are in the language of the system. Choice of creation or modification date (the creation date is missing on many Linux file systems: the modification date is then used). **Offset** in seconds. Option to add a suffix to duplicates. The `c`, `ddddd` and `dddddd` tags of the original program do not exist. |
| 10 | **Random names** | Generate a name with a random part | Mask with `%random%` (+ `%name%`, `%ext%`, `%folderN%`). Type: random number (8 digits), "Windows tick count" (a counter of milliseconds, so sequential) or GUID (`{...}`, unique). |
| 11 | **Change case** | Upper / lower case | Four choices: upper the first letter following given characters (space, hyphen, dot... to type in; the rest goes lower), upper the first letter of the name, all upper, all lower. Keep "use locale" on to convert accents (é → É). Options: also apply to the extension, or only to the extension (by default the extension is left alone). On Linux "a.txt" and "A.txt" are two different names. |
| 12 | **Names from a list** | Apply a list of names | One line per file, in list order (typed, or loaded/saved with *Open...* / *Save...*; *Clear* empties it). Options: add the original extension; use the list as extensions. A file without a matching line, or with an empty line, is not renamed. |
| 13 | **Regular expressions** | Advanced splitting / rearranging | *Expression* (PCRE2, not case-sensitive by default; `(?-i)` makes it case-sensitive) and *New name* with `$1`, `$2`..., `$&` (all that was found), `${10}`, `\$` for a literal dollar. It applies to the whole name including the extension, and to every match in it. E.g. `(.*) - (.*)\.mp3` → `$2 - $1.mp3` swaps artist and title. An invalid expression shows `<RegExp Error>` in the preview. |
| 14 | **EXIF / IPTC / XMP** | Name photos from their metadata | Mask with the tags of §7, plus `%num%` (same settings as Enumeration) and `$if`. **Offset** in seconds shifts the date tags. A file without any metadata is not renamed ("No EXIF info found"). |

#### Wildcards in string replacement
- **Enabling**: tick "Use wildcards (* = any text, ? = any character)", off by default. It only concerns the *String replacement* operation.
- **In the "Search" field**: `*` stands for any text (even empty), `?` for exactly one character. Every other character is searched as it is (`.`, `(`, `[`... have no special meaning).
- **The "Replace by" field stays literal**: `*` and `?` are written as they are.
- **The text found is as short as possible**: `a*b` in `a1b2b` finds `a1b`, not `a1b2b`. With "all occurrences", the search then continues in the rest of the name.
- **Example**: search `IMG_*_copy`, replace by `Photo`: `IMG_001_copy (1).jpg` becomes `Photo (1).jpg`.
- The choice is saved in job files (`.arb`); an older job loads with wildcards off. The other operations have no wildcards: put a *String replacement* with wildcards before them in the same job. For more complex patterns, use the *Regular expressions* operation.
- Not to be confused with the `*` and `?` wildcards of the **command line** (§8), which select the files to add.

### Managing the job
The **Job contents** panel lists the planned operations (tick box to show/hide it).
- **Add to job** (Ctrl+F2), **Remove selected** (Shift+F6), **Clear** (Shift+F7).
- **Open** (Ctrl+O) / **Save** (Ctrl+S): `.arb` files (XML), compatible with Ant Renamer 2 in both directions.
- Reorder by drag and drop or with the 4 move buttons.
- In the classic layout an added operation cannot be edited (remove it and redo it). In the modern layout: double-click to reload it in the editor, then *Validate the modification* (or the *Modify* button).

## 5. Start, stop, undo

- **Go** (F9): renames in a separate thread; the window stays usable.
- **Stop** (Esc): interrupts the processing; the files already processed stay renamed.
- **Undo** (F12): restores the old names of the **last** run only (one level), sidecar files included.
- **Always on top** (F11): keeps the window above the others.
- **Copy instead of rename** (Options > Processing): the originals stay, the copies carry the new names.

## 6. Format masks (operations 4, 7, 8, 10, 14)

| Syntax | Meaning |
|---|---|
| `%name%` / `%ext%` | name without extension / extension with the dot (empty for a folder) |
| `%folderN%` | N-th parent folder (`%folder1%` = the closest). Negative: from the root (`%folder-1%` = the drive on Windows, the first folder on Linux). |
| `%count%`, `%countr%`, `%countfiles%`, `%countfilesr%`, `%countfolders%`, `%countfoldersr%` | **for folders in the list**: number of items inside (`r` = recursive, `files` / `folders` = only those). On a file the result is 0. |
| `%num%` | the counter (Enumeration, EXIF) |
| `%%` | a literal `%` |
| `$if(cond,then,else)` | (MP3, EXIF) if `cond` is not empty: `then`, otherwise `else`. Put values containing a comma or parenthesis in quotes (a quote itself: `""`); tags are not expanded inside quotes. `$$` = a literal `$`. |

An unknown tag (typing error) stays as it is in the name, "%" included: you notice it in the preview. Tag names are not case-sensitive.

`$if` example (MP3): `%author%$if(%title%, - %title%,)%ext%` only adds " - title" when the title exists.
A folder delimiter in the result moves the file into that folder (*Force folders* option creates it): `\` on Windows, **`/` on Linux** (it works on Windows too). `%name%/` puts each file in a folder named after it; `../%folder1% ` goes up one level.

## 7. EXIF, IPTC, XMP tags (operation 14)

- **EXIF** (JPEG and TIFF/RAW files): `DateTimeOriginal`, `DateTimeDigitized`, `DateTime`, `Make`, `Model`, `BodySerialNumber`, `LensMake`, `LensModel`, `ExposureTime`, `FNumber`, `ISOSpeedRatings`, `FocalLength`, `FocalLengthIn35mmFilm`, `ExposureBiasValue`, `ShutterSpeedValue`, `ApertureValue`, `ExposureProgram`, `ExposureMode`, `MeteringMode`, `Flash`, `WhiteBalance`, `SceneCaptureType`, `Orientation`, `ExifImageWidth`, `ExifImageLength`, `XResolution`, `YResolution`, `ResolutionUnit`, `ColorSpace`, `Software`, `Artist`, `Copyright`, `ImageDescription`, `UserComment`, `ExifVersion`, `MakerNote`.
- **IPTC** (JPEG): `IptcKeywords`, `IptcCaption`, `IptcHeadline`, `IptcObjectName`, `IptcByline`, `IptcBylineTitle`, `IptcCopyright`, `IptcCredit`, `IptcSource`, `IptcCity`, `IptcSublocation`, `IptcState`, `IptcCountry`, `IptcCountryCode`, `IptcDateCreated`, `IptcTimeCreated`, `IptcCategory`, `IptcSupplementalCategories`, `IptcSpecialInstructions`, `IptcUrgency`, `IptcWriter`, `IptcTransmissionReference`.
- **XMP** (JPEG, PNG, WebP, TIFF, DNG, `.xmp` files): `XmpTitle`, `XmpDescription`, `XmpKeywords`, `XmpHierarchicalKeywords`, `XmpCreator`, `XmpRights`, `XmpRating`, `XmpLabel`, `XmpCreateDate`, `XmpModifyDate`, `XmpDateCreated`, `XmpCreatorTool`, `XmpHeadline`, `XmpCredit`, `XmpSource`, `XmpCity`, `XmpState`, `XmpCountry`, `XmpCountryCode`, `XmpLocation`, `XmpCategory`, `XmpInstructions`, `XmpAuthorsPosition`, `XmpCaptionWriter`, `XmpTransmissionReference`, `XmpLens`.

Good to know: a missing tag gives an empty text; multiple values are joined with ", "; dates (`yyyy:MM:dd HH:mm:ss`) become `yyyy-MM-dd HH-mm-ss`; only the first 512 KB of the file are read; the neighbouring `.xmp` (IMG.xmp, IMG.CR2.xmp) overrides the embedded XMP. To take the first tag available: `$if(%XmpTitle%,%XmpTitle%,%IptcObjectName%)%ext%`.

## 8. Command line

| Parameter | Effect |
|---|---|
| `-b file.arb` | loads a job |
| `-g` | starts the processing once files and job are loaded |
| `-x` | exits when the processing is done |
| `-af` / `-aF` / `-afr` | adds the following files / folders / recursive content of folders (wildcards `*` `?` accepted) |
| `--shell path` | adds the path to the window that is already open (used by the Explorer menu) |
| `--install-shell` / `--remove-shell` | adds / removes the Explorer menu entry (Windows), then exits |

Examples: `antrenamer-qt.exe -b d:\test.arb -af c:\data\*.* -g -x` (Windows), `antrenamer-qt -b ~/test.arb -af "/data/*" -g -x` (Linux, quote the wildcards so that the shell does not expand them).
Without `-a`, 2.07 behaviour: a name ending in `.arb` is a job, a folder is added recursively.

## 9. Log page

Each processed file gives one line: **green** (renamed, old → new name; the full path when the folder changes; sidecar files mentioned), **blue** (not renamed: identical name, missing tag, empty line or end of the list, file missing), **red** (error: name already used, invalid character, file locked).
Buttons: **Filter** (Successes / Errors / Not renamed), **Clear**, **Save** (text file), **Copy** (clipboard).
The *Save log to file* option (Options > Events) saves it automatically after each run in UTF-8, two tab-separated columns (`v` success, `x` error), with an option to append to an existing file.

## 10. Options (F10)

Changes apply when you click OK; a few (language, modern layout) at the next start.

| Page | Settings |
|---|---|
| **Display** | live update, automatic column resizing, icons by extension, modern layout and its page arrangement (Tabs, Single screen in bands, Single screen in columns), forced font and size, history size of the drop-down lists (0 = none), auto-completion, **icon set** (§12) |
| **Files & Folders** | do not ask on drag and drop and the options to use, natural sort, open the file on double-click, Explorer menu entry (Windows) |
| **Processing** | copy instead of rename, generate a log, daylight saving by file date, also rename sidecar files (`.xmp`, DxO `.dop`; they get their old name back on undo), force folders (creates them), detect `x:\` and `\\` (Linux: `/`) as absolute paths, last dot of a folder name = extension |
| **Events** | at the start of a run: switch to a page, clear the log; at the end: switch to a page, clear the files list, clear the job, save the log to a file (with "append") |
| **Language** | interface language (27 `.lng` files; translator and version shown), portable mode |

Settings are stored in your profile (`%APPDATA%\AntRenamerQt` on Windows, `~/.local/share/AntRenamerQt` on Linux). Portable mode: an `antrenamer-qt.ini` file next to the program keeps all settings in that folder (the checkbox in Options > Language creates or removes it; an empty file is enough).

## 11. Modern layout (optional)

Same engine, other presentation, enabled in Options > Display (next start). Counters "Files (100)", "Operations (2)" on the page buttons; a thin toolbar per page; in single-screen modes Ctrl+1/2/3 shows or hides a pane; splitters can be dragged and are remembered; the columns layout falls back to bands below ~900 pixels wide; job operations can be edited by double-click.

## 12. Icons, help and languages

- **Icon set** (Options > Display): four sets, **Modern** (default since 3.0: line icons drawn by the program, sharp on any screen, light and dark theme), Scrows, Windows XP and Windows XP Large. The help shows the four sets side by side (Options page). The choice applies at once.
- **Help** (F1): about 20 pages with screenshots, in English, French, Croatian and Tamil (other languages see English). The pages written for Ant Renamer Qt may be only in English and French; Croatian and Tamil then show English.
- **Interface languages**: 27 language files. To add or fix one, see [TRANSLATING.md](TRANSLATING.md).

## 13. Windows and Linux

| Topic | Windows | Linux |
|---|---|---|
| Folder delimiter in a new name | `\` (and `/`) | `/` only; `\` and `:` are ordinary characters of a name |
| Absolute path option | `x:\...` or `\\server\...` | a name beginning with `/` |
| Case-only rename (`a.txt` → `A.txt`) | works (names are case-insensitive) | the two names are different files |
| Creation date | stored by the file system | often missing: the modification date is used |
| Explorer context menu | yes (Windows 11: *Show more options*) | no (use drag and drop or the command line) |
| Settings | `%APPDATA%\AntRenamerQt` | `~/.local/share/AntRenamerQt` |
| Program name | `antrenamer-qt.exe` | `antrenamer-qt` |
| Packages | installer and portable zip | `.deb` (amd64, arm64), AppImage |

## 14. Known limits

- EXIF: about forty tags only, MakerNote not decoded; IPTC read only in JPEG.
- Audio tags: ID3 only (MP3); FLAC, OGG, M4A are not read.
- Undo: one level.
- The short Windows 11 context menu is not supported (use "Show more options").
- The help in Croatian and Tamil shows English for the pages written for Ant Renamer Qt.
