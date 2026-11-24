# Roadmap

French version: [TODO_fr.md](TODO_fr.md). What is already done is in the [CHANGELOG](../CHANGELOG.md).

These are ideas, not promises: some may never be done, and nothing here is frozen. The rules:
- **Priority order**: first the functional problems (something that gives a wrong result, loses data or does not work as announced), then the requests made on the forums and by users (the oldest and most frequent first), then everything else.
- One theme per minor version, one version at a time. Each one is tried by users (beta, release candidate) before the next one starts. While a version is developed, a numbered pre-release (`3.3.0-beta.1`, `beta.2`...) is published from time to time when a handful of changes is worth trying, not one per change. Corrections only go in a patch version (`3.x.y`) when something urgent cannot wait for the next minor one.
- The roadmap moves with the feedback. An idea planned for one version can change version, or be merged into another, when a request or another idea makes them fit together (for example, filtering by date and setting a date share the same code, so they sit in the same version).
- The sources of the requests: the "Comments" forum of the original program, forum.antp.be (120 topics read on 2026-10-22), then the issues and discussions of this project.
- Nothing is written for an idea that nobody needs: an idea waits for a real request or a real problem.

## Where we are (2026-11-25)

- **3.2.0** published on 2026-11-20: the "Use EPUB info" action, no more empty-looking names (EPUB, MP3, EXIF), the Explorer menu asking what to add, files left alone shown in orange in the preview, the update notice showing the release notes, help and credits cleaned up, links to the online help, a new Flatpak manifest built with `cmake --install`.
- **Distribution**: the application is on Flathub since 2026-11-22 (updates by pull request on `flathub/fr.morfredus.AntRenamerQt`, with `publish-antrenamer-flatpak.sh`), in AppImageHub and Portable Linux Apps; a winget pull request is waiting for a moderator.
- **3.3.0-beta.1** published on 2026-11-22 (Filter files, undo of copies, shared history), tried on Ubuntu, Fedora and Windows 11 and 10. **3.3.0-beta.2** published on 2026-11-23 as a pre-release from `dev/3.3.0` (Set dates from EXIF with the sidecar files following, `--set-dates-from-exif`, the .deb home page); the CI of the branch (build, tests, clang-tidy, `appstreamcli validate`, packages) was green on the released commit. Not tried by hand yet: the undo of the dates on real files, the function on Linux, the 3.3.0-beta.2 packages installed.
- **3.3.0** final prepared on `dev/3.3.0` (2026-11-24), waiting for the merge into `main`, the tag and the packages. It adds to beta.2: the Flatpak fix (files given one by one vanished, found on Fedora KDE), `--filesystem=home`, advice for synchronized folders. To do after the tag: `tag` and `commit` of the Flathub manifest, then `publish-antrenamer-flatpak.sh`.
- **3.3.1** prepared on `dev/3.3.1` (2026-11-25), corrective: the Flatpak without `--filesystem=home` (refused by Flathub), the folder asked with the FileChooser portal, the Flatpak messages in French. Tried on Fedora KDE (Dolphin) from the local manifest. To do after the tag: `tag` and `commit` of the Flathub manifest, then `publish-antrenamer-flatpak.sh`.
- The branch `dev/3.3.0` carries the corrections that were planned for a 3.2.1.

## Next versions, in order

### 3.3: corrections and the dates of the files

No separate 3.2.1: the corrections wait for 3.3 so that one release carries them all instead of a series of small ones. The functional problems come first, then the oldest request of the forums.

**Corrections and loose ends carried over from 3.2.1**
- **Known functional limits to look at again**:
  - Done in `dev/3.3.0`: a `-q` run and an open window no longer overwrite each other's history (each takes in the passes of the other).
  - Done in `dev/3.3.0`: the undo deletes the files copied with "Copy instead of rename" if they are unchanged (same size and date as right after the copy), and keeps them with a message otherwise.
- **Release loose ends**: check the first update of the Flathub repository with `publish-antrenamer-flatpak.sh` (a real update, not only a dry run). The release line of the version must be in the MetaInfo before the tag is made (3.2.0 was missing, so Flathub still shows 3.1.1; it is listed in `dev/3.3.0`). Done: the "EPUB is in the beta" mention is out of the online help, and the CI economy (no build on a push to `main`, the Linux release script waiting for the running CI) is tried.
- **AppImage**: the 3.3.0-beta.1 AppImage was run on Fedora and Ubuntu with the expected behaviour (2026-11-23). The metainfo of the AppImage (`--appimage-extract`, `usr/share/metainfo/fr.morfredus.AntRenamerQt.metainfo.xml`) is there and `appstreamcli validate` passes (2026-11-23, Ubuntu); `--pedantic` only notes the capital letters of the component ID, which stays as it is since it is the Flathub ID. `appstreamcli validate` is in the CI (Linux job, 2026-11-23). Still to do: check the screenshots on the AppImageHub page.
- **Screenshots**: remake them if the interface changed (the Filter files and Set dates from EXIF dialogs are new).

**The dates of the files**

- **Bring the modification date of a file back in line with its EXIF shooting date** (done in `dev/3.3.0`, right-click "Set dates from EXIF...", `--set-dates-from-exif`; asked many times, "on the to-do list" of the original since 2005). Typical case: photos that were copied and whose file system date was overwritten. Only the modification date is concerned: it is changed the same way on Windows and on Linux. The creation date is excluded: it cannot be changed on Linux with the standard tools (to check), and a plain copy resets it. Dates read from a file name would come later, only on request.
  **Deliberate choice, with safeguards** (the originals of photos must not be falsified by accident, nor easily):
  - The date comes from the file itself (EXIF). No date typed by hand and no invented date: a file without an EXIF date is left alone and the log says so.
  - The program changes only the date of the file system, never the content of the file: the EXIF shooting date and the tags are not touched.
  - A separate operation, never on by default, with a confirmation that says what is about to be done. The preview shows the old and the new date of each file.
  - The log keeps the old and the new date, and the history can undo it.
  - On the command line, the function needs its own explicit option.
  - The documentation (guide, help, README) says that the modification date is not a proof of authenticity and that, for originals that are evidence, one should work on copies (the "Copy instead of rename" option).
- **Filter by date and/or time** (done in `dev/3.3.0`, right-click "Filter files..."; the same filters on the command line come in 3.4): keep the files whose modification date, creation date or EXIF shooting date falls in a period or in a time of day ("everything shot between 14:00 and 15:00"), also by size.

### 3.4: choosing the files

Today only the hidden and system files are filtered.
- **Filter by mask or regular expression**, before or after adding the files.
- **Find a file** in the list (by name, date, time).
- Command line: the same filters, and the choice of what the log keeps (successes, errors, unchanged files; today the filter only applies to the window).

### 3.5: reusing and fixing names

- **Presets for every action**: name and save the settings of any action ("Photos: date + counter"), as the grid of action 3 can already be.
- **New tags**: `%size%` and other file attributes; checksums (`%crc32%`, `%md5%`, `%sha1%`).
- **Fix names with a wrong encoding** (UTF-8 read as ISO-8859-1 after a Linux/Windows trip), as an option instead of a replacement grid.
- **Enumeration by letters** (`AA`, `AB`...).
- **"Copy instead of rename" per action** of the batch (today one global option).
- **Case change**: capital letter at a given position.
- **Random names, more options**: today a mask and three randomisation methods only. Add the exact character sets to draw from (digits, lower/upper case, custom list, excluded look-alike characters such as `0/O`, `1/l`) and prefix/suffix rules. A fair remark received from a reader.

### 3.6: photo metadata, more of it

- **IPTC**: read more fields, and IPTC from TIFF files (the IPTC block is read from JPEG today).
- **EXIF**: the GPS tags (latitude, longitude), more than 36 tags, the MakerNote (lens, serial number).
- **Newer formats**: HEIC/HEIF, AVIF, JPEG XL; RAW of recent cameras (check each new body: date, sub-seconds, orientation).
- **XMP**: the `exif:` attributes of a sidecar (date, GPS) and the `crs:` (Camera Raw) settings, not mapped yet.
- **Sidecars of new photo software**: see the maintenance track.

### 3.7: audio

Tags FLAC, OGG, M4A (the first limit of the user guide; Vorbis comments first, M4A last), the duration and the bit rate of MP3 files, folders named from the tags they contain ("artist - album").

### 3.8: video and documents

Video (MP4, MKV): shooting date, duration, to name videos like photos. Properties of PDF files and Office documents (title, author, year).

## Maintenance track

Not a version: a set of checks, made on a schedule and on events, that keeps the program working. Each check ends with a line in the CHANGELOG (or "nothing to do") and, if needed, an update of `docs/TESTED_PLATFORMS.md`.

| What | When | What is checked |
|---|---|---|
| **New Qt version** (6.x minor, new LTS) | at each release of Qt, and before raising or dropping the minimum Qt | Build and tests on Windows (MSYS2), Ubuntu, the Raspberry Pi; look at the deprecation warnings; high-DPI, dark theme and the Windows 11 style; the minimum Qt of the AppImage (built on Ubuntu 22.04, Qt 6.5) and the Flathub runtime. |
| **Toolchain and CI** | every quarter | MSYS2: the CI and the scripts use UCRT64 (MINGW64 is deprecated), on trial in the branch `test/ucrt64`; GitHub runner images (an old Ubuntu image is removed one day); clang-tidy version; the compiler warnings of a new GCC/Clang. |
| **Linux distributions** | each new Ubuntu LTS or Debian stable | `.deb` install and start, AppImage start (glibc), Flatpak; the packages the `.deb` depends on. |
| **Sidecars** | each major version of the photo software, and on a user report | See below. The rule: a sidecar must follow its photo when it is renamed, and a sidecar the program does not know is left alone, never corrupted. |
| **EXIF and metadata** | every six months, and for each new camera a user reports | New or changed tags (Exif 3.0, UTF-8 strings), new RAW formats and maker notes, sub-second fields, time zones (`OffsetTime`). One test file per format in the tests. |
| **Photo software** | on a user report | What a photographer uses beside the program: Lightroom, Capture One, DxO, darktable, RawTherapee, ON1, digiKam, the camera makers' own software. For each: its sidecars (`.xmp`, `.dop`, `.cos`, `.pp3`...), the catalog that breaks when a file is renamed (and what to tell the user about it). |
| **Translations** | at each new text | `.lng` files in step with `English.lng`, `ANTREN_MISSING=1` on every dialog, help pages in EN, FR, HR, TA (the Croatian and Tamil pages written for Ant Renamer Qt still show English). |
| **Documents** | at each release | CHANGELOG, WHATS_NEW, USER_GUIDE and the French versions, BUILDING / ARCHITECTURE / CONTRIBUTING when the structure changes, screenshots when the interface changes, the release line in the MetaInfo. |
| **Packaging and distribution** | at each release | Windows installer and portable, `.deb` amd64 and arm64, AppImage with its `.zsync`, Flathub (pull request on `flathub/fr.morfredus.AntRenamerQt`, `master` is protected). The winget pull request, the listings (AppImageHub, Softpedia and others: see the visibility log). To evaluate: Scoop, Chocolatey, an AUR package (only if someone asks and maintains it). |
| **Licences** | when a dependency or a contribution comes in | The port is GPL v2 or later. Moving to GPL v3 is allowed, but only with a real reason (a dependency or a contribution under a licence that is not compatible with v2). It touches `LICENSE`, the About box, `README`, `CONTRIBUTING`, `docs/PRESENTATION`, the `.deb` copyright, the Flatpak MetaInfo and the installer licence file. |
| **Security** | on each report, and at each Qt update | `SECURITY.md`; the CSV export (formula injection, done in 3.1.0-rc.3), paths and file names from outside. |

## Ideas to evaluate

No version yet; each waits for a real need.

- **Preview of the pictures** in the files list (today only the option to open the file on double-click). Heavy for big RAW files; thumbnails read from the embedded JPEG would be enough.
- **A second instance hands its command line to the running window** (paths, job, `-g -x`), brings it to the front and quits, instead of the information box. Needs a window busy with a run to refuse or queue the command line, and a lock tied to the settings folder. The simple way (box and quit) is in place and enough so far.
- **A batch file** (`.cmd`/`.sh`) written for a later renaming (undoing after the program is closed is done: the history is kept).
- **Code signing of the Windows files** (wanted by some download sites such as Softonic, and it avoids the SmartScreen warning). The signing foundation declined the application on 2026-11-13: not enough public visibility yet (stars, forks, contributors, external articles and discussions). Reapply when the project is better known. The paid options (a subscription, about 500 USD a year; a certificate such as Certum's open source one, from about 50 EUR a year) are not worth it for a free project for now. The program and the installer already carry version information, which any signing needs.
- **Creation date of the files**: only if a user asks, and only where the system allows it (Windows).
- **Windows 11 modern context menu**, on demand.
- **Optional code quality** (low value): `std::span` for `exportPreview`, `qScopeGuard` for `LocalFree` in `lastSysError()`, naming rule (`readability-identifier-naming`) now that clang-tidy is in place.

## Set aside

- **A tag in the mask to place the number of duplicate names** (Date and time and EXIF actions), instead of `_01` before the extension: a deliberate choice, not useful. The suffix at the end of the name is what everyday software does; the tag would be empty without a collision and would leave its separator in the name; the Enumeration action and its `%num%` already place a counter anywhere. Only if someone really asks for it (then start with the format of the suffix: `_01`, `-1`, ` (1)`).
- **Automatic numbering of duplicate names in every action** (`_1`, `_2`...): done for Date and time and EXIF, where bursts of photos need it. In the other actions a name already taken is a mistake of the settings that the user should see, so it is refused and logged; the Enumeration action and its `%num%` tag already put a counter where the user wants it. To reconsider if a real case comes up.
- **Reading `.dop` data**: a DxO-only format.
- **Duplicate detection**, editing images, watching a folder, converting formats, writing the MP3 tags from the file names: outside what the program is for, as the author of the original answered.
- **A richer command line**: the current one is enough for automation.
- **Renaming from the clipboard**: too specific.
- **CMake `install()` rules**: done, a single `cmake --install` serves the Linux packages.
- **`File*` pointers in `FileList`**: addresses are stable (the vector holds `unique_ptr`); the only risk is a pointer kept after `remove()`.
- **`moveFile(..., QString& err)` signature**: three callers, all test the result.
- **A hash to check a file before an undo after a restart**: the size and the modification date are enough; a hash would only add the very rare case of a replacement with the same size and date, for more code and more reading of the files.
