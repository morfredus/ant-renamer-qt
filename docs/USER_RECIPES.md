# Ant Renamer Qt: recipes

*How to reach a specific result, from the simplest to the most complex.*
French version: [USER_RECIPES_fr.md](USER_RECIPES_fr.md). Reference of every function: [USER_GUIDE.md](USER_GUIDE.md).

Each recipe gives the goal, the actions to stack in the batch (in order), the settings and a before/after. The sample names were checked with the program's engine, except those reading metadata (EXIF, ID3, XMP), which follow the help.

## 0. Before you start

1. **Add the files** (F2 / F3 / drag and drop). The **list order is the processing order**: sort by name before an enumeration.
2. **Pick the action**, set it, then **Add to batch** (Ctrl+F2). Repeat for each action: they apply one after the other, each on the result of the previous one.
3. **Check the preview**: the "Preview of the selected file" frame shows one file; **F5** fills the Preview column for the whole list. "Changed only" hides unchanged files so you only review the others.
4. **Go** (F9), then read the **Log**. **Undo** (F12) reverts the last run, and only that one.
5. **Caution on a big batch**: try a few copies first, or tick "Copy instead of rename" (Options > Processing): the originals stay untouched, the copies carry the new names. Remember to untick it afterwards.
6. **Save the batch** (Ctrl+S) as soon as it works: it reloads (Ctrl+O) and runs from the command line (§6.4).

Two rules to remember:
- An action sees the **name already changed** by the previous ones, except the metadata actions (Date, MP3, EXIF), which always read the **real file** on disk.
- A **folder separator** in a new name moves the file into that folder (§5): write `/` (it works on Windows and on Linux; `\` also works on Windows only). On Linux `\` and `:` are ordinary characters of a file name.

---

## 1. Clean up and fix names

### 1.1 Replace spaces with underscores (or the opposite)
*String replacement*: Search ` ` (one space), Replace by `_`. Keep "Replace all occurrences".
`mon fichier 1.txt` → `mon_fichier_1.txt`. For the opposite, swap the two fields.

### 1.2 Remove a piece of variable length (wildcards)
*String replacement*, "Use wildcards" ticked. Search `_*_final`, Replace by (empty).
`Film_ABC123_final.mov` → `Film.mov`. `*` stands for any text, `?` for one character. The text found is as short as possible. Wildcards exist only in this action: to clean up before another action, put this replacement before it in the batch.

### 1.3 Remove accents
*Multiple string replacement*: pick the **Remove west-european accents** set in the drop-down list. `Éte été.txt` → `Ete ete.txt`. For other characters (`&` → `and`, `œ` → `oe`), add lines to the grid (Ctrl++), name the set and **Save**. Copy / Paste lets you edit the grid in Excel.

### 1.4 Lower-case the extension, or normalise `.jpeg` to `.jpg`
- *Change case*: "Lower", tick **Apply to extension instead of name**: `PHOTO.JPG` → `PHOTO.jpg`.
- *Change extension*: enter `jpg` (no dot): `a.jpeg` → `a.jpg`. To add instead of replace, tick "Do not replace extension": `a.tar` → `a.tar.bak`.

### 1.5 Capital letter on every word
*Change case*, first choice "Upper first letter following any of these characters", with the list of separators (default space, hyphen, dot, plus, parenthesis). `mon beau fichier-2.txt` → `Mon Beau Fichier-2.txt`. To lower everything first (`PHOTO Grand` → `Photo Grand`): a first *Change case* "Lower", then this one. Case-only changes (`a.txt` → `A.txt`) also work on Windows.

### 1.6 Remove the first N characters
*Character deletion*: number of characters = 5, "From position" 0, from the start. `2024_IMG_1.jpg` → `IMG_1.jpg`. Other modes: delete N characters **after** or **before** a given string (`IMG_001_x.jpg`, 3 characters after `IMG_` → `IMG__x.jpg`). The extension is not counted, unless "Apply to extension" is ticked.

### 1.7 Remove a numeric prefix ("001 - ")
*Regular expressions*: Expression `^\d+ - `, New name empty. `001 - Titre.mp3` → `Titre.mp3`. **The expression applies to the full name, extension included**: add `\.mp3` if you want to put it back in the new name.

### 1.8 Collapse multiple spaces
*Regular expressions*: Expression ` {2,}`, New name ` ` (one space). `a   b    c.txt` → `a b c.txt`.

### 1.9 Replace dots with spaces, except the extension's
*Regular expressions*: Expression `\.(?=.*\.)`, New name ` `. `my.great.file.name.txt` → `my great file name.txt` (a dot followed by another dot is replaced, the last one is kept).

### 1.10 Add a suffix before the extension, or a prefix
*String insertion*:
- suffix: text `_web`, position 0, **From end**: `IMG_1.jpg` → `IMG_1_web.jpg` (the extension is not counted);
- prefix: text `2024_`, position 0, from the start.

### 1.11 Prefix with the parent folder name
*String insertion*: text `%folder1%_`, position 0. `C:\photos\Paris\IMG_1.jpg` → `Paris_IMG_1.jpg`. `%folder2%` is the folder above, `%folder-1%` the first one from the root.

### 1.12 Move a part of the name
*String move*: for example 1 character from position 2 to position 4: `abcdef.txt` → `abdecf.txt` (the arrival position counts after the character is removed). If the part is not always at the same position, use a regular expression (§1.13).

### 1.13 Swap two parts of the name (Artist - Title)
*Regular expressions*: Expression `(.*) - (.*)\.mp3`, New name `$2 - $1.mp3`. `Blur - Song 2.mp3` → `Song 2 - Blur.mp3`.

### 1.14 Put the year first
*Regular expressions*: Expression `(.*) \((\d{4})\)\.mkv`, New name `$2 - $1.mkv`. `Film (1999).mkv` → `1999 - Film.mkv`. The parentheses of the expression capture (`$1`, `$2`...); for a literal parenthesis, precede it with `\`. The expression is case insensitive by default. `$&` stands for everything that was found.

### 1.15 Keep only the number of a camera name
*Regular expressions*: Expression `^IMG_(\d+).*(\.jpg)$`, New name `$1$2`. `IMG_0042_copy.jpg` → `0042.jpg`.

---

## 2. Numbering

### 2.1 Number 001, 002, 003...
*Enumeration*: Mask `File %num%%ext%`, Start at 1, Digits 3, Increment by 1. `File 001.jpg`, `File 002.jpg`... Keep the original name: `%name%_%num%%ext%`. Numbers follow the list order: **sort the list first** (click the header, or drag and drop).

### 2.2 Restart at 1 in each folder
Add the files of several folders, **sort by folder** (Folder header, so folder contents are not mixed), tick **Go back to start value when changing folder**.

### 2.3 Count down
*Enumeration*: Start at 100, Increment by -1: 100, 99, 98... (the fields accept negative values).

### 2.4 A JPEG and its RAW with the same number
Sort by name so `IMG_1234.JPG` and `IMG_1234.CR2` follow each other, then tick **Same number for files with the same name and different extensions**. Both become `Photo 001.JPG` and `Photo 001.CR2`. An extension already present in the group gets a new number. If the matching `.xmp` files are in the list they follow too; otherwise the "Also rename the sidecar files" option (§3.6) takes care of them.

---

## 3. Photos

### 3.1 Name by shooting date (EXIF)
*EXIF info*: Mask `%DateTimeOriginal%%ext%`. `IMG_4521.jpg` → `2024-05-17 14-30-05.jpg` (forbidden `:` become `-`). Another form with tags: `%Make% %Model% - %DateTimeOriginal%%ext%`. A tag missing from the file gives an empty text.

### 3.2 Avoid date duplicates (bursts)
*EXIF info*: Mask `%DateTimeOriginal%_%num%%ext%`; the `%num%` counter has the same settings as Enumeration (digits, restart per folder). For JPEG and RAW to share the number, tick the matching option (§2.4).

### 3.3 Fix the clock of a badly set camera
*EXIF info* or *Date and time*, **Offset** setting in seconds; every date tag of the form `yyyy:MM:dd HH:mm:ss` is shifted. Example: camera left on winter time, offset `3600` (+1 hour); negative to go back. `-86400` removes a day.

### 3.4 Name by file date (no EXIF)
*Date and time*: mask `yyyy'-'mm'-'dd'_'fe` → `2026-10-15_IMG_1.jpg` (`f` = original name, `e` = extension). Any fixed text goes in quotes or apostrophes. Date and time: `yyyymmdd'_'hhnnss'_'fe` → `20261015_223515_IMG_1.jpg`. For a 12-hour clock add `am/pm` (or `a/p`) to the mask: `hh'-'nn am/pm`, 22:35 → `10-35 pm`. Pick **Creation** or **Last modification** (many Linux file systems do not store the creation date: the modification date is used then). Tick **Add suffix if more than one file with the same date/time** to avoid identical names. `nn` is always minutes; `mm` is minutes when it follows `hh` or precedes `ss`, otherwise the month.

### 3.5 Name by title or keywords (IPTC / XMP)
*EXIF info*, mask with `$if` to take the first existing tag and avoid a stray separator:
- `$if(%XmpTitle%,%XmpTitle%,%IptcObjectName%)%ext%`
- `%DateTimeOriginal%$if(%XmpKeywords%, - %XmpKeywords%,)%ext%`

Multiple values are joined with ", ". For a RAW, keywords and title entered in the photo software are in the neighbouring `.xmp`, read automatically.

### 3.6 Rename photos with their sidecar files
Nothing to do if "Also rename the sidecar files (.xmp, .dop) of a renamed picture" (Options > Processing) is ticked, which is the default. `IMG_0001.CR2` and `IMG_0001.CR2.xmp` leave together, undo (F12) puts both back. The log says so. See also §2.4.

### 3.7 File photos into year / month folders
*Date and time* (or EXIF if the shooting date matters): mask `yyyy'/'mm'/'fe` → `2026/10/name.jpg`. The `/` makes a subfolder, created next to the file. Tick **Force folders** (Options > Processing) so they are created automatically; otherwise the folder must exist.

---

## 4. Music and videos

### 4.1 "Artist - Title" from ID3 tags
*MP3 tags info*: mask `%author% - %title%%ext%`. The characters that a file name cannot hold are always replaced in the tags (`"` → `'`, `?` → `.`, `\ / : * < > |` → `-`). Handles ID3v1 and ID3v2.2 / 2.3 / 2.4. A file without tags: "not renamed" in the log.

### 4.2 Track number, album, incomplete tags
- `%track% - %title%%ext%` with the **Two-digit track number** box → `01 - Titre.mp3`.
- `%album%/%track% - %title%%ext%` files into one folder per album (tick Force folders).
- Empty tags: `%author%$if(%title%, - %title%,)%ext%` → `Blur - Song 2.mp3` but `Vivaldi.mp3` if the title is empty (no stray "-"). Values containing a comma or parenthesis go in quotes: `%author%$if(%album%," ("%album%")",)%ext%`.

### 4.3 TV series episodes
*Regular expressions*: Expression `Code\.Quantum\.S([0-9]{2})E([0-9]{2})\.(.*)\.FRENCH.XViD\.avi`, New name `Code Quantum - $1$2 - $3.avi`. `Code.Quantum.S01E02.Titre.FRENCH.XViD.avi` → `Code Quantum - 0102 - Titre.avi`. Adapt the fixed parts to your files; the `([0-9]{2})` groups extract season, episode, title.

---

## 5. Folders: move, file, flatten

Principle: a `/` in the new name denotes a folder (on Windows `\` works too). With **Force folders**, it is created if it does not exist. The examples use `/`, valid everywhere.

### 5.1 One folder per file name
Files `cars.doc`, `cars.xls`, `cars.jpg`, `houses.doc`... *String insertion*: text `%name%/`, position 0, from the start. Tick Force folders. Result: `cars/cars.doc`, `cars/cars.xls`... `houses/houses.doc`...

### 5.2 Bring files up from a subfolder, keeping its name
Files `images\movies\poster1.jpg` and `images\games\poster1.jpg`: *String insertion*, text `../%folder1% `, position 0. Result in `images`: `movies poster1.jpg`, `games poster1.jpg`. Empty folders are then deleted by hand.

### 5.3 Move somewhere else (full path)
Options > Processing: tick **Detect ":\" or "\\" as absolute paths** (on Linux: a name that begins with `/`). *String insertion*: text `D:\Sorted\` on Windows or `/home/me/Sorted/` on Linux, position 0. `a.txt` → `D:\Sorted\a.txt` (or `/home/me/Sorted/a.txt`). Combine with Force folders to create the tree. To keep the original, tick "Copy instead of rename".

### 5.4 Rename folders
Folders (F3): tick "add the folders" (and untick the files if needed). Folders are treated as files without an extension. If your folder names contain a dot that is not an extension (`Photos 2024.05`), leave "The last dot of a folder name delimits the extension" unticked (default). `%countfiles%` and `%countfolders%` (recursive with `r`) give the number of items in the folder: `Holidays (%countfiles%)`.

---

## 6. Advanced

### 6.1 Apply a list of names (from Excel or a text)
*Names from a list*: paste one name per line, in the exact order of the files list (sort it first). Tick "Add the original extension" if the list has no extensions. More files than lines, or an empty line: the file is not renamed. "Use the items as extensions" changes extensions only.

### 6.2 Anonymise with random names
*Random names*: mask `%random%%ext%`, GUID method (unique guaranteed), 8-digit number or "Windows tick count" (a counter of milliseconds, so sequential names).

### 6.3 Chain several actions
A typical full clean-up, in this order: 1) *Multiple string replacement* (accents); 2) *String replacement* (wildcards, remove `_copy`); 3) *String replacement* (spaces → `_`); 4) *Change case* (lower); 5) *Enumeration* last, since the number must apply to the final name. Check with F5, then save the batch (Ctrl+S).
To fix a step already added: remove it and redo it (classic layout) or double-click it in the modern layout.

### 6.4 Run a saved batch from the command line
```
antrenamer-qt -b d:\batches\cleanup.arb -af c:\import\*.* -g -x
```
loads the batch, adds the files of the folder, renames and exits. `-afr folder` adds recursively, `-aF` adds folders. On Linux: `antrenamer-qt -b ~/batches/cleanup.arb -af "/import/*" -g -x` (quote the wildcards). The same call can be run by the Windows Task Scheduler, by cron or from a script. The command line's `*` and `?` select files; they have nothing to do with the wildcards of string replacement.

### 6.5 Send files in from the file manager
Enable "Add to Ant Renamer Qt" (Options > Files & Folders). Right-click a selection of files and folders (Windows 11: *Show more options*): everything arrives in the window that is already open.

### 6.6 Undo a rename
**Undo** (F12) restores the names of the last run, sidecars included. One level only: do not chain a second run before checking. When in doubt, work in "Copy instead of rename" mode and keep the log saved (Options > Events > Store the log).

---

## 7. Common problems

| Symptom | Cause and solution |
|---|---|
| Empty preview in the list | F5 was not used: the preview is not automatic. |
| Numbers do not follow the wanted order | The list is not sorted as expected: sort or reorder before starting. |
| "Name already used" (red) | Two files give the same new name: add a counter (`%num%`) or the Date and time suffix option. |
| A file is not renamed (blue) | New name identical to the old one, missing tag (MP3, EXIF) or missing file: read the log message. |
| A folder separator (`/`) makes the rename fail | The target folder does not exist: tick Force folders. |
| Accents are wrong in capitals | Keep the locale option of the Change case action ticked. |
| The regular expression finds nothing | It applies to the full name, extension included; also check the `\` before `.` `(` `)` `[`. |
