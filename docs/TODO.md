# TODO

## Ideas for after 3.0.0

French version: [TODO_fr.md](TODO_fr.md).

### P0: code quality, right after the final 3.1.0 (the code is frozen for it)

From a technical review checked against the code (2026-11-06). Nothing to touch before the 3.1.0 release:
- **CI badge** at the top of `README.md` / `README_fr.md` (the Build workflow already exists, only the badge is missing).
- **clang-tidy** (and/or `-Werror`) so that "no compiler warnings" is enforced by the CI, not only checked by hand: a `.clang-tidy` file, a CMake preset, a CI step. Today only `-Wall -Wextra -Wshadow`.
- Optional, low value: `std::span` for `exportPreview`, `qScopeGuard` for `LocalFree` in `lastSysError()`, naming rule (`readability-identifier-naming`) once clang-tidy is in place.
- Checked and not to do: `File*` in `FileList` (addresses are stable, the vector holds `unique_ptr`; the risk is only a pointer kept after `remove()`), `moveFile(..., QString& err)` (3 callers, all test the result).

### P1: undo after a restart (done, see the CHANGELOG, Unreleased)

Asked by a forum user (afraid of wrecking an archive after a typing error or a change of mind). Today the undo history lives in memory (`ren::Journal`, `core/ren_files.h`) and is lost when the program is closed. The idea: write it to disk (JSON in the data folder) at the end of each pass, reload it at startup, and keep the history window across sessions (about 50 passes). The undo stays linear, from the last pass backwards, and the existing checks stay (file still there, old name free; stop at the first failure and keep the rest of the pass). Estimated cost: 100 to 150 lines of code plus tests, translations (EN, FR, HR, TA), docs. Low technical risk.

Safeguards for files changed outside the program between two sessions:
- File renamed or moved elsewhere: already handled ("source file not found", the undo stops on that pass).
- Another file now has the name (the only real danger: the wrong file would be renamed): store the size and the modification date of each file in each move (a rename keeps both), check them before undoing, refuse with a clear message if they differ. Decided: no hash, not even a partial one. It would only add the very rare case of a replacement with the same size and date, for more code and more reading of the files.
- Folder renamed outside the program: the paths are stale, the undo stops, no guessing.

To decide: after a restart, offer to resume the last session by itself, or only through the Undo button and the history window (the simpler one, preferred).

### P2: one instance at a time (idea, not decided)

Today only the Explorer menu looks for a running window (`--shell` hands the paths to it, `app/main.cpp`); any other launch opens a second window. Two windows write the same files: the undo history (`antrenamer-qt-history.json`: the last one to save erases the passes of the other, and two windows started together get the same session number) and the settings (`.ini`: the last one closed wins). A `-q` run reads and writes the history too, with no lock.

Two ways, to decide:
- **Simple (preferred)**: when another instance is running, the new one shows an information box ("Ant Renamer Qt is already running: only one instance can be open at a time") and quits. The same lock for the installed and the portable version (nobody uses both on one computer, so no lock per settings folder). `--shell` keeps handing its paths to the running window, before the check. A few lines in `main.cpp`, two texts (EN, FR), the docs. `QLockFile` already drops the lock of a crashed process. To settle: a launch with `-g -x` and no `-q` must not wait for a click in an unattended script (no box, a message on the error output and an exit code, or let it through); `-q` stays out of the rule.
- **Complete**: the new instance hands its whole command line (paths, job, `-g -x`) to the running window, brings it to the front and quits; the lock is tied to the settings folder (hash of its path). More work: a window busy with a run must refuse or queue the command line.

Not enough on its own (a crash leaves the history written by the last run, which is fine), and `-q` could still overwrite the history of an open window: accepted for now.

### 3.1 plan (decided 2026-11-02)

Content of 3.1.0-beta.4, to be published in a few days (the work stays Unreleased on `dev/3.1.0`, `main` stays at 3.1.0-beta.3): the undo history kept between runs (done), the silent command line (`-q`) and the modern layout by default. The suffix for duplicate names in the EXIF action moves to 3.2. Then 3.1.0-rc.1 once the beta.4 has run without negative feedback (only fixes during the rc). Decided 2026-11-03: the beta.4 becomes the 3.1.0-rc.1, code frozen.

### Possible versions

These are ideas, not promises: some of them may never be done. If they are, one theme per minor version, one at a time: each one is tried by users (rc, then at least a few weeks of stable use) before the next one is started. Nothing is started before 3.0.0 is published. The versions are sorted by urgency: what users of the original program asked for most often, for the longest time, comes first (source: the "Comments" forum of the original program, forum.antp.be, 120 topics read on 2026-10-22). The order may change with the feedback.

| Version | Content |
|---|---|
| **3.1** | **Command line, trailing slash** (`-afr d:\Data\`, done, see the CHANGELOG, 3.1.0-beta.1). **Update check, pre-releases** (the options checkbox and the choice offered by the installer are done, see the CHANGELOG, 3.1.0-beta.1). **Modern layout by default**: the modern interface, single screen in bands, becomes the default of a new installation (today the classic one: `newUi` and `pageLayout` in `appsettings.h`). The installer asks nothing about it. Decided 2026-11-02: a new installation (no settings file, or an empty one) gets the modern layout, single screen in bands; anyone who already has settings keeps their layout (the classic one when they never chose); no message to invite the users of the classic layout, the documentation says it. (done, see the CHANGELOG, Unreleased) Not for 3.0.0: its code is the code of 3.0.0-rc.16. **Silent command line** (`-q`): no window, for scripts called from a context menu (asked on the forum). (done, see the CHANGELOG, Unreleased) **Exportable preview**: the current and new names (or only the list of names) written to a CSV or text file, to proofread a big batch or keep a trace. (done, see the CHANGELOG, 3.1.0-beta.1) |
| **3.2** | **Suffix for duplicate names in the EXIF action**: two photos taken the same second get the same name; a tag placeable anywhere in the mask (the Date and time action already has a suffix option). **Automatic suffix for duplicate names in every action** (`_1`, `_2`... like Capture One, asked by a user on the forum): today a name already taken is refused and logged, nothing is overwritten; with 1000 photos shot in bursts, the refused ones must be handled by hand. **File dates**: change the creation and modification dates of the files (asked many times, "on the to-do list" of the original since 2005), and the reverse of the Date and time action: set the date from a date found in the name. **Enumeration by letters** (`AA`, `AB`...). **"Copy instead of rename" per action** of the batch (today one global option). **Update notice with the news**: when a new version is announced, show under its number what is new in it (the release notes or the WHATS_NEW lines of that version), not only the number. |
| **3.3** | **Presets for every action**: name and save the settings of any action ("Photos: date + counter"), as the grid of action 3 can already be. **New tags**: `%size%` and other file attributes; checksums (`%crc32%`, `%md5%`, `%sha1%`). **Fix names with a wrong encoding** (UTF-8 read as ISO-8859-1 after a Linux/Windows trip), as an option instead of a replacement grid. |
| **3.4** | **Audio**: tags FLAC, OGG, M4A (first limit of the user guide; Vorbis comments first, M4A last), and the duration and bit rate of MP3 files. **Folders named from the MP3 tags** they contain ("artist - album"). |
| **3.5** | **Video** (MP4, MKV): the shooting date, the duration, to name videos like photos. |
| **3.6** | **Metadata, more of it**: the GPS tags (latitude, longitude), more than 36 EXIF tags, the MakerNote (lens, serial number), IPTC from TIFF files, the properties of PDF files and Office documents (title, author, year). |
| **Later** | **Windows 11 modern context menu**, to be decided on demand. |

### Smaller ideas, no version yet
- Licence: move the port from GPL v2 or later to GPL v3 (allowed, the original says "version 2 or any later version"). No technical need today, so not before the final 3.1.0; to decide with a real reason (a dependency or contribution under a licence that is not compatible with v2). It touches `LICENSE`, the About box (`app/dialogs.cpp`, `app/help/about_*.md`, the `.lng` files), `README`, `CONTRIBUTING`, `docs/PRESENTATION`, the `.deb` copyright (`scripts/linux/package-deb.sh`), the Flatpak metainfo (`GPL-3.0-or-later`) and the installer licence file.

- Read `.dop` data? Not planned: it is a DxO-only format.
- XMP properties not mapped yet: the `exif:` attributes of a sidecar (date, GPS), `crs:` (Camera Raw) settings.
- Filter the files to process with a mask or a regular expression, before or after adding them (only the hidden and system files are filtered today).
- Command line: choose what the log keeps (successes, errors, unchanged files); today the filter only applies to the window.
- Case change: capital letter at a given position.
- A batch file (`.cmd`/`.sh`) written for a later renaming (undoing after the program is closed is done: the history is kept).

### Checked in the code (2026-10-22)

- The silent command line (`-q`) did not exist (2026-10-22): the switches were `-b`, `-g`, `-x`, `-a[fFr]` and `--shell`. Added in 3.1 (Unreleased).
- A folder given with a trailing slash or backslash (`-afr d:\Data\`) added nothing, as in the original (the name used as a mask was empty); confirmed by a test on 2026-10-24 and fixed (3.1.0-beta.1).
- There is no preview of the pictures in the files list (only the option to open the file on double-click): an idea, no version.

### Not planned

Duplicate detection; a richer command line (the current one is enough for automation); writing the MP3 tags from the file names, editing images, watching a folder, converting formats (outside what the program is for, as the author of the original answered); renaming from the clipboard (too specific).
