# TODO

## Ideas for after 3.0.0

French version: [TODO_fr.md](TODO_fr.md).

### Possible versions

These are ideas, not promises: some of them may never be done. If they are, one theme per minor version, one at a time: each one is tried by users (rc, then at least a few weeks of stable use) before the next one is started. Nothing is started before 3.0.0 is published. The versions are sorted by urgency: what users of the original program asked for most often, for the longest time, comes first (source: the "Comments" forum of the original program, forum.antp.be, 120 topics read on 2026-10-22). The order may change with the feedback.

| Version | Content |
|---|---|
| **3.0.1** | **Command line, trailing slash**: a folder given with a trailing slash or backslash (`-afr d:\Data\`) must be added like one without. To be confirmed by a test first. The code of 3.0.0 stays untouched until then. |
| **3.1** | **Update check, pre-releases**: a checkbox "Include pre-releases (beta, rc)" in the options, off by default. Today a pre-release is offered only when the installed version is itself one. **Silent command line** (`-q`): no window, for scripts called from a context menu (asked on the forum). **Suffix for duplicate names in the EXIF action**: two photos taken the same second get the same name; a tag placeable anywhere in the mask (the Date and time action already has a suffix option). **Exportable preview**: the current and new names (or only the list of names) written to a CSV or text file, to proofread a big batch or keep a trace. |
| **3.2** | **File dates**: change the creation and modification dates of the files (asked many times, "on the to-do list" of the original since 2005), and the reverse of the Date and time action: set the date from a date found in the name. **Enumeration by letters** (`AA`, `AB`...). **"Copy instead of rename" per action** of the batch (today one global option). |
| **3.3** | **Presets for every action**: name and save the settings of any action ("Photos: date + counter"), as the grid of action 3 can already be. **New tags**: `%size%` and other file attributes; checksums (`%crc32%`, `%md5%`, `%sha1%`). **Fix names with a wrong encoding** (UTF-8 read as ISO-8859-1 after a Linux/Windows trip), as an option instead of a replacement grid. |
| **3.4** | **Audio**: tags FLAC, OGG, M4A (first limit of the user guide; Vorbis comments first, M4A last), and the duration and bit rate of MP3 files. **Folders named from the MP3 tags** they contain ("artist - album"). |
| **3.5** | **Video** (MP4, MKV): the shooting date, the duration, to name videos like photos. |
| **3.6** | **Metadata, more of it**: the GPS tags (latitude, longitude), more than 36 EXIF tags, the MakerNote (lens, serial number), IPTC from TIFF files, the properties of PDF files and Office documents (title, author, year). |
| **Later** | **Windows 11 modern context menu**, to be decided on demand. |

### Smaller ideas, no version yet

- More sidecar types (for example RawTherapee `.pp3`).
- Read `.dop` data? Not planned: it is a DxO-only format.
- XMP properties not mapped yet: the `exif:` attributes of a sidecar (date, GPS), `crs:` (Camera Raw) settings.
- Filter the files to process with a mask or a regular expression, before or after adding them (only the hidden and system files are filtered today).
- Command line: choose what the log keeps (successes, errors, unchanged files); today the filter only applies to the window.
- Case change: capital letter at a given position.
- Add a folder by typing its path.
- A batch file (`.cmd`/`.sh`) written for a later renaming, or to undo after the program is closed (the undo history is kept for the session only).

### Checked in the code (2026-10-22)

- The silent command line (`-q`) does not exist: the switches are `-b`, `-g`, `-x`, `-a[fFr]` and `--shell`.
- A folder given with a trailing slash or backslash (`-afr d:\Data\`) most probably adds nothing, as in the original (the name used as a mask is then empty); to confirm by a test, then fix in 3.0.1.
- There is no preview of the pictures in the files list (only the option to open the file on double-click): an idea, no version.

### Not planned

Duplicate detection; a richer command line (the current one is enough for automation); writing the MP3 tags from the file names, editing images, watching a folder, converting formats (outside what the program is for, as the author of the original answered); renaming from the clipboard (too specific).
