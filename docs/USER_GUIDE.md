# Ant Renamer Qt: function guide

*Every function of Ant Renamer Qt 3.0.0-rc.9 with instructions.*
French version: [USER_GUIDE_fr.md](USER_GUIDE_fr.md). Full details: the built-in help (F1).

## 1. General principle

The work is done in three steps, one page each:

1. **Files**: build the list of files (and folders) to rename.
2. **Operations**: choose one or more renaming operations (the "job").
3. **Log**: read what happened after clicking **Go**.

The preview at the bottom of the Operations page shows the current and future name of the selected file, live.

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
- **Files** (F2): standard file picker.
- **Folders** (F3): pick one or more folders, then tick what to add: the files they contain, the folders themselves, the subfolders (all levels).
- **Drag and drop** from the file manager: a window offers the same options. "Do not ask" remembers them; holding Ctrl or Shift when dropping shows the window once more.
- **Explorer menu** "Add to Ant Renamer Qt" (enable it in Options > Files & Folders). Several selected items arrive together in the window that is already open. Windows 11: *Show more options*.
- **Command line**: see §8.

### Managing the list
| Button | Effect |
|---|---|
| Remove selected (F6) | removes the selected files from the list (not from disk) |
| Remove all (F7) | empties the list |
| Remove dead (F8) | removes files that no longer exist on disk |
| Refresh preview (F5) | fills the Preview column (not automatic, for performance) |
| Changed only | hides the files whose name does not change in the preview (a display filter only: they are still renamed); refresh the preview first |
| Up / Down / Top / Bottom | changes the processing order (drag and drop in the list works too) |

- **Columns**: right-click the header to show Folder, File name, Full path, Ext., Preview, Size, Creation date, Modification date. They can be resized and moved.
- **Sorting**: click a header (natural sort by default: "file 2" before "file 10"). Sorting sets the processing order, so the numbering order of an enumeration. Right-click > *Unsorted* to turn it off.
- Right-click in the list: select all, etc.

### Sidecar files of a photo
When a photo is renamed, its sidecar files are renamed with it, with nothing more to do:
- **Recognised**: `.xmp` (Lightroom, Capture One: `IMG.xmp`; digiKam, darktable: `IMG.CR2.xmp`) and DxO PhotoLab `.dop` (`IMG.dop`, `IMG.CR2.dop`), upper-case extensions included.
- **Method**: in Options > Processing, tick "Also rename the sidecar files (.xmp, .dop) of a renamed picture" (on by default). Then rename the photo as usual (for example `IMG_0001.CR2` → `Paris 001.CR2`): `IMG_0001.xmp` or `IMG_0001.CR2.xmp` becomes `Paris 001.xmp` or `Paris 001.CR2.xmp`.
- **Undo** (F12): the sidecars get their old name back with the photo.
- **Log**: the photo's line mentions the renamed sidecar.
- **Special cases**: if a file with the new name already exists, the sidecar is left alone and the log says so (the photo is renamed anyway); a sidecar that is also in the list follows without being renamed twice; no effect in "Copy instead of rename" mode and for folders.
- The neighbouring `.xmp` file is also read by the EXIF / IPTC / XMP operation (§7).

## 4. Operations page: the 14 operations

Pick the operation in the list, set its parameters (the *Notes* area under the parameters recalls the available tags), then **Add to job** (Ctrl+F2). Several operations can be stacked: they apply in order.

| # | Operation | Purpose | How to use it |
|---|---|---|---|
| 1 | **Change extension** | Replace or add an extension | Enter the new extension (without the dot). Empty field = removes the extension. Option: add after the existing one instead of replacing. |
| 2 | **String replacement** | Replace one text with another | "Search" / "Replace by" fields. Empty = deletes the text. Options: case sensitive, all occurrences (otherwise only the first), also apply to the extension or only to the extension, **wildcards** (new in 3.0.0-rc.9, see below). |
| 3 | **Multiple string replacement** | Several replacements at once (accents, etc.) | 2-column grid (searched / replaced). Ctrl++ adds a line, Ctrl+- deletes the current line. A grid can be named and saved (drop-down list + *Save*). Copy / Paste exchange the grid with Excel or a text editor (tab between columns). A "Remove west-european accents" set is provided. |
| 4 | **String insertion** | Insert a text at a position | Text, position (0 = before the first character or after the last one), from the start or from the end. Option: count within the extension. Tags `%name%`, `%ext%`, `%folderN%`. |
| 5 | **String move** | Move N characters | Number of characters, "from" position and "to" position (the second counts after the characters are removed). E.g. `abcdef`, 1 character from 2 to 4 → `abdecf`. |
| 6 | **Character deletion** | Erase N characters | From a position, or after a given string. |
| 7 | **Enumeration** | Number the files | Mask containing `%num%` (e.g. `File %num%%ext%`). Settings: *Start at*, *Digits* (leading zeros), *Increment by*. Options: *Restart at the start value* in each new folder (list sorted by folder), *Same number for files with the same name* (IMG_1234.JPG + IMG_1234.CR2). |
| 8 | **MP3 tags** | Name from the ID3 tags (v1, v2.2, v2.3, v2.4) | Mask with `%author%`, `%title%`, `%album%`, `%year%`, `%genre%`, `%comm%`, `%track%`, `%name%`, `%ext%`. E.g. `%author% - %title%%ext%`. Option: replace forbidden characters. `$if` available (§6). |
| 9 | **Date and time** | Name from the file date | Mask in letters: `yyyy` `yy` `mm` `mmm` `mmmm` `dd` `ddd` `dddd` `hh` `nn` (minutes) `ss` `am/pm`, `f` (original name), `e` (extension). Any fixed text goes in quotes. E.g. `yyyy'-'mm'-'dd hh'-'nn'-'sse`. Choice of creation or modification date; option to add a suffix to duplicates. |
| 10 | **Random names** | Generate a name with a random part | Mask with `%random%` (+ `%name%`, `%ext%`). Type: random number (8 digits), Windows tick count, or GUID. |
| 11 | **Change case** | Upper / lower case | Four choices: upper the first letter following given characters (space, hyphen, dot... to type in), upper the first letter of the name, all upper, all lower. Keep "use locale" on to handle accents correctly. |
| 12 | **Names from a list** | Apply a list of names | One line per file, in list order. Options: add the original extension; use the list as extensions. A file without a matching line, or with an empty line, is not renamed. |
| 13 | **Regular expressions** | Advanced splitting / rearranging | *Expression* (PCRE2, case insensitive) and *New name* with `$1`, `$2`... E.g. `(.*) - (.*)\.mp3` → `$2 - $1.mp3` swaps artist and title. |
| 14 | **EXIF / IPTC / XMP** | Name photos from their metadata | Mask with the tags of §7, plus `%num%` (same settings as Enumeration), `$if`. "Offset" setting to shift dates. |

#### Wildcards in string replacement (new in 3.0.0-rc.9)
- **Enabling**: tick "Use wildcards (* = any text, ? = any character)", off by default. It only concerns the *String replacement* operation.
- **In the "Search" field**: `*` stands for any text (even empty), `?` for exactly one character. Every other character is searched as it is (`.`, `(`, `[`... have no special meaning).
- **The "Replace by" field stays literal**: `*` and `?` are written as they are.
- **The text found is as short as possible**: `a*b` in `a1b2b` finds `a1b`, not `a1b2b`. With "all occurrences", the search then continues in the rest of the name.
- **Example**: search `IMG_*_copy`, replace by `Photo`: `IMG_001_copy (1).jpg` becomes `Photo (1).jpg`.
- The choice is saved in job files (`.arb`); an older job loads with wildcards off. The other operations have no wildcards: to benefit from them, put a *String replacement* with wildcards before them (for example to remove a piece of variable length), in the same job. For more complex patterns, use the *Regular expressions* operation.
- Not to be confused with the `*` and `?` wildcards of the **command line** (§8), which select the files to add.

### Managing the job
The **Job contents** panel lists the planned operations (tick box to show/hide it).
- **Add to job** (Ctrl+F2), **Remove selected** (Shift+F6), **Clear** (Shift+F7).
- **Open** (Ctrl+O) / **Save** (Ctrl+S): `.arb` files (XML), compatible with Ant Renamer 2.
- Reorder by drag and drop or with the 4 move buttons.
- In the classic layout an added operation cannot be edited (remove it and redo it). In the modern layout: double-click to reload it in the editor, then *Apply the change* (or the *Modify* button).

## 5. Start, stop, undo

- **Go** (F9): renames in a separate thread; the window stays usable.
- **Stop** (Esc): interrupts the processing.
- **Undo** (F12): restores the old names of the **last** run only (one level).
- **Always on top** (F11): keeps the window above the others.

## 6. Format masks (operations 4, 7, 8, 10, 14)

| Syntax | Meaning |
|---|---|
| `%name%` / `%ext%` | name without extension / extension with the dot |
| `%folderN%` | N-th parent folder (`%folder1%` = the closest). Negative: from the root (`%folder-1%` = drive). |
| `%count%`, `%countr%`, `%countfiles%`, `%countfilesr%`, `%countfolders%`, `%countfoldersr%` | for folders: number of items inside (`r` = recursive) |
| `%%` | a literal `%` |
| `$if(cond,then,else)` | if `cond` is not empty: `then`, otherwise `else`. Put values containing a comma or parenthesis in quotes. `$$` = a literal `$`. |

`$if` example (MP3): `%author%$if(%title%, - %title%,)%ext%` only adds " - title" when the title exists.
A `\` in the result means a folder: `%name%\` puts each file in a folder named after it (*Force folders* option). `../%folder1% ` goes up one level.

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
| `--install-shell` / `--remove-shell` | adds / removes the Explorer menu entry, then exits |

Example: `antrenamer-qt -b d:\test.arb -af c:\data\*.* -g -x`
Without `-a`, 2.07 behaviour: a name ending in `.arb` is a job, a folder is added recursively.

## 9. Log page

Each processed file gives one line: **green** (renamed, old → new name), **blue** (not renamed: identical name, no tag, file missing), **red** (error: name already used, invalid character, file locked).
Buttons: **Filter** (Successes / Errors / Not renamed), **Clear**, **Save** (text file), **Copy** (clipboard).
The *Store the log* option (Options > Events) saves it automatically after each run in UTF-8, two tab-separated columns (`v` success, `x` error), with an option to append to an existing file.

## 10. Options (F10)

| Page | Settings |
|---|---|
| **Display** | live update, automatic column resizing, icons by extension, font for special characters, history size of the drop-down lists (0 = none), auto-completion, icon set (Scrows, Windows XP, Windows XP Large), **modern layout** and its page arrangement (Tabs, Single screen in rows, Single screen in columns), applied at the next start |
| **Files & Folders** | do not ask on drag and drop and the options to use, natural sort, open the file on double-click, Explorer menu entry |
| **Processing** | copy instead of rename, generate a log, daylight saving by file date, also rename sidecar files (`.xmp`, DxO `.dop`; they get their old name back on undo), force folders (creates them), detect `x:\` and `\\` as absolute paths, last dot of a folder name = extension |
| **Events** | actions at the start and the end of a run, store the log |
| **Language** | interface language (27 `.lng` files), portable mode, moving the settings between `%APPDATA%` and the program folder |

Portable mode: an empty `antrenamer-qt.ini` file next to the program keeps all settings in that folder.

## 11. Modern layout (optional)

Same engine, other presentation, enabled in Options > Display (next start). Counters "Files (100)", "Operations (2)" on the page buttons; a thin toolbar per page; in single-screen modes Ctrl+1/2/3 shows or hides a pane; splitters can be dragged and are remembered; the columns layout falls back to rows below ~900 pixels wide; job operations can be edited by double-click.

## 12. Known limits

- EXIF: about forty tags only, MakerNote not decoded; IPTC read only in JPEG.
- Audio tags: ID3 only.
- Undo: one level.
- The short Windows 11 context menu is not supported (use "Show more options").
