# Roadmap

French version: [TODO_fr.md](TODO_fr.md). What is already done is in the [CHANGELOG](../CHANGELOG.md).

These are ideas, not promises: some may never be done. The rules:
- One theme per minor version, one version at a time. Each one is tried by users (beta, release candidate, then at least a few weeks of stable use) before the next one starts.
- The versions are sorted by importance: what users asked for most often and for the longest time comes first (sources: the "Comments" forum of the original program, forum.antp.be, 120 topics read on 2026-10-22, then the issues and discussions of this project). The order changes with the feedback.
- Nothing is written for an idea that nobody needs: an idea waits for a real request or a real problem.

## Where we are (2026-11-13)

- **3.1.0** published on 2026-11-09 (command line, update check, modern layout by default, `-q`, exportable preview, undo after a restart).
- **3.1.1-beta.1** published on 2026-11-12: numbering of files given the same name (Date and time, EXIF), one window at a time, message for an empty folder, AppImage update information, clang-tidy and `-Werror` in the CI.

## Next versions, in order

### 3.1.1 (final): no new feature

Only what the beta shows. Before it goes out:
- Not tried yet: the installer, the `.deb` and AppImage packages, an upgrade over 3.1.0, Linux Mint, the second Raspberry Pi. clang-tidy on the Pi.
- Publish the `.zsync` file next to the AppImage in the release (otherwise AppImageUpdate cannot update it).
- Flathub: publication (the MetaInfo lists 3.1.0; add 3.1.1 at release).
- Announcement on the forum.
- Screenshots: remake if the interface changed.

### 3.2: names that never collide, dates

The most asked: files refused because the new name is taken, and the dates of the files.
- **Automatic numbering in every action** (`_1`, `_2`... like Capture One, asked on the forum). Done for Date and time and EXIF. Elsewhere a name already taken is refused and logged, nothing is overwritten: with 1000 photos shot in bursts the refused ones have to be handled by hand.
- **Tag to place in the mask** for the number of duplicates (EXIF): choose the place and the form instead of `_01` at the end.
- **File dates**: change the creation and modification dates (asked many times, "on the to-do list" of the original since 2005), and the reverse of the Date and time action: set the date from a date found in the name.
- **Update notice with the news**: when a new version is announced, show under its number what is new in it (the release notes or the `WHATS_NEW` lines of that version), not only the number.

### 3.3: choosing the files

Today only the hidden and system files are filtered.
- **Filter by date and/or time**: keep the files whose modification date, creation date or EXIF shooting date falls in a period or in a time of day ("everything shot between 14:00 and 15:00"), also by size.
- **Filter by mask or regular expression**, before or after adding the files.
- **Find a file** in the list (by name, date, time).
- Command line: the same filters, and the choice of what the log keeps (successes, errors, unchanged files; today the filter only applies to the window).

### 3.4: reusing and fixing names

- **Presets for every action**: name and save the settings of any action ("Photos: date + counter"), as the grid of action 3 can already be.
- **New tags**: `%size%` and other file attributes; checksums (`%crc32%`, `%md5%`, `%sha1%`).
- **Fix names with a wrong encoding** (UTF-8 read as ISO-8859-1 after a Linux/Windows trip), as an option instead of a replacement grid.
- **Enumeration by letters** (`AA`, `AB`...).
- **"Copy instead of rename" per action** of the batch (today one global option).
- **Case change**: capital letter at a given position.

### 3.5: photo metadata, more of it

- **IPTC**: read more fields, and IPTC from TIFF files (the IPTC block is read from JPEG today).
- **EXIF**: the GPS tags (latitude, longitude), more than 36 tags, the MakerNote (lens, serial number).
- **Newer formats**: HEIC/HEIF, AVIF, JPEG XL; RAW of recent cameras (check each new body: date, sub-seconds, orientation).
- **XMP**: the `exif:` attributes of a sidecar (date, GPS) and the `crs:` (Camera Raw) settings, not mapped yet.
- **Sidecars of new photo software**: see the maintenance track.

### 3.6: audio

Tags FLAC, OGG, M4A (the first limit of the user guide; Vorbis comments first, M4A last), the duration and the bit rate of MP3 files, folders named from the tags they contain ("artist - album").

### 3.7: video and documents

Video (MP4, MKV): shooting date, duration, to name videos like photos. Properties of PDF files and Office documents (title, author, year).

## Maintenance track

Not a version: a set of checks, made on a schedule and on events, that keeps the program working. Each check ends with a line in the CHANGELOG (or "nothing to do") and, if needed, an update of `docs/TESTED_PLATFORMS.md`.

| What | When | What is checked |
|---|---|---|
| **New Qt version** (6.x minor, new LTS) | at each release of Qt, and before raising or dropping the minimum Qt | Build and tests on Windows (MSYS2), Ubuntu, the Raspberry Pi; look at the deprecation warnings; high-DPI, dark theme and the Windows 11 style; the minimum Qt of the AppImage (built on Ubuntu 22.04, Qt 6.5) and the Flathub runtime. |
| **Toolchain and CI** | every quarter | MSYS2: the CI warns about MINGW64 and the move to UCRT64 is to be tested in the sandbox; GitHub runner images (an old Ubuntu image is removed one day); clang-tidy version; the compiler warnings of a new GCC/Clang. |
| **Linux distributions** | each new Ubuntu LTS or Debian stable | `.deb` install and start, AppImage start (glibc), Flatpak; the packages the `.deb` depends on. |
| **Sidecars** | each major version of the photo software, and on a user report | See below. The rule: a sidecar must follow its photo when it is renamed, and a sidecar the program does not know is left alone, never corrupted. |
| **EXIF and metadata** | every six months, and for each new camera a user reports | New or changed tags (Exif 3.0, UTF-8 strings), new RAW formats and maker notes, sub-second fields, time zones (`OffsetTime`). One test file per format in the tests. |
| **Photo software** | on a user report | What a photographer uses beside the program: Lightroom, Capture One, DxO, darktable, RawTherapee, ON1, digiKam, the camera makers' own software. For each: its sidecars (`.xmp`, `.dop`, `.cos`, `.pp3`...), the catalog that breaks when a file is renamed (and what to tell the user about it). |
| **Translations** | at each new text | `.lng` files in step with `English.lng`, `ANTREN_MISSING=1` on every dialog, help pages in EN, FR, HR, TA. |
| **Documents** | at each release | CHANGELOG, WHATS_NEW, USER_GUIDE and the French versions, BUILDING / ARCHITECTURE / CONTRIBUTING when the structure changes, screenshots when the interface changes. |
| **Packaging and distribution** | at each release | Windows installer and portable, `.deb` amd64 and arm64, AppImage with its `.zsync`, Flathub. To evaluate: winget, Scoop, Chocolatey, an AUR package (only if someone asks and maintains it). |
| **Licences** | when a dependency or a contribution comes in | The port is GPL v2 or later. Moving to GPL v3 is allowed, but only with a real reason (a dependency or a contribution under a licence that is not compatible with v2). It touches `LICENSE`, the About box, `README`, `CONTRIBUTING`, `docs/PRESENTATION`, the `.deb` copyright, the Flatpak MetaInfo and the installer licence file. |
| **Security** | on each report, and at each Qt update | `SECURITY.md`; the CSV export (formula injection, done in 3.1.0-rc.3), paths and file names from outside. |

## Ideas to evaluate

No version yet; each waits for a real need.

- **Preview of the pictures** in the files list (today only the option to open the file on double-click). Heavy for big RAW files; thumbnails read from the embedded JPEG would be enough.
- **A second instance hands its command line to the running window** (paths, job, `-g -x`), brings it to the front and quits, instead of the information box. Needs a window busy with a run to refuse or queue the command line, and a lock tied to the settings folder. The simple way (box and quit) is in place and enough so far.
- **A batch file** (`.cmd`/`.sh`) written for a later renaming (undoing after the program is closed is done: the history is kept).
- **`-q` and the history**: a `-q` run can still overwrite the history of an open window. Accepted for now; revisit if someone loses a pass.
- **Code signing of the Windows files** (wanted by some download sites such as Softonic, and it avoids the SmartScreen warning). The SignPath Foundation declined the application on 2026-11-13: not enough public visibility yet (stars, forks, contributors, external articles and discussions). Reapply when the project is better known. The paid options (a SignPath subscription, about 500 USD a year; a certificate such as Certum's open source one, from about 50 EUR a year) are not worth it for a free project for now. The program and the installer already carry version information, which any signing needs.
- **Windows 11 modern context menu**, on demand.
- **Optional code quality** (low value): `std::span` for `exportPreview`, `qScopeGuard` for `LocalFree` in `lastSysError()`, naming rule (`readability-identifier-naming`) now that clang-tidy is in place.

## Set aside

- **Reading `.dop` data**: a DxO-only format.
- **Duplicate detection**, editing images, watching a folder, converting formats, writing the MP3 tags from the file names: outside what the program is for, as the author of the original answered.
- **A richer command line**: the current one is enough for automation.
- **Renaming from the clipboard**: too specific.
- **`File*` pointers in `FileList`**: addresses are stable (the vector holds `unique_ptr`); the only risk is a pointer kept after `remove()`.
- **`moveFile(..., QString& err)` signature**: three callers, all test the result.
- **A hash to check a file before an undo after a restart**: the size and the modification date are enough; a hash would only add the very rare case of a replacement with the same size and date, for more code and more reading of the files.
