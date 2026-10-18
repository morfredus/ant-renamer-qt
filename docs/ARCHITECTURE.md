# Architecture of Ant Renamer Qt

What is where, what each part does, and how a renaming runs from the click to the disk. To build the project see [BUILDING.md](BUILDING.md); to contribute see [CONTRIBUTING.md](../CONTRIBUTING.md). ([Version française](ARCHITECTURE_fr.md))

The program is written in C++20 with Qt 6. It is a port of Ant Renamer 2.13 (Delphi), and the code keeps the names of the original: the comments say "Port of ..." where a class is a direct translation.

The code is cut in two:

- **`core/`**: the renaming engine, with no user interface. It depends on QtCore only and is built as a static library, `rencore`.
- **`app/`**: the interface (windows, panels, help, languages). It uses the engine and is built as the executable `antrenamer-qt`.

## Folder layout

```
core/             Engine (static library rencore)
app/              Interface (executable antrenamer-qt)
  Languages/      Language files of the original (.lng, read at run time)
  help/           Markdown help in 4 languages, table of contents, images
tests/            test_core (engine; includes id3_check.h, robust_check.h), test_panel (actions editor, language loader)
cmake/toolchains/ ARM64 cross-compilation toolchain
scripts/          windows/ (MinGW build helper, zip and installer) and linux/ (.deb, AppImage, .desktop)
installer/        Inno Setup script of the Windows installer
docs/             Guides, presentation, release notes
.github/          CI workflow, Dependabot, issue templates
CMakeLists.txt, CMakePresets.json, VERSION   Build files and the version, the single source
LICENSE           GNU General Public License
```

| In `app/` | Compiled? |
|---|---|
| `*.cpp`, `*.h` | Yes |
| `resources.qrc`: `renamer.ico`, the three toolbar bitmap sets (the fourth, "Modern", is drawn in code), `ant.bmp`, `default.xml` | Embedded in the executable by Qt's `rcc` |
| `app.rc`, `app.manifest` (icon, long paths, UTF-8, per-monitor DPI) | Windows only, compiled with `windres` |
| `Languages/`, `help/` | Not compiled: copied next to the executable (and installed by the packages) |
| `antrenamer-qt.png` | Not compiled: Linux icon, used by the packaging scripts |

## Vocabulary

| Term | In the code | Meaning |
|---|---|---|
| File | `ren::File` | One entry of the list: folder, name, state, preview, log text |
| File list | `ren::FileList` | The files to rename, in screen order |
| Operation (action) | `ren::Action` and its 14 subclasses | One renaming rule with its settings |
| Job (batch) | `ren::ActionList` | The ordered operations, saved as an `.arb` XML file |
| Mask | a string like `%IptcDateCreated%_%num%%ext%` | Name template whose `%tags%` are replaced by their values |
| Preview | `Op::Preview`, `File::preview` | The computed name, without touching the disk |
| Sidecar | `.xmp`, `.dop` | A file that follows its picture |

## The engine (`core/`)

| File | Role |
|---|---|
| `ren_actions.h/.cpp` | The 14 operations, loading and saving of jobs, the run loop (`Action::perform`) |
| `ren_files.h/.cpp` | `File` (rename, revert, sidecars), `FileList`, `Journal` (passes of the session, undo), `Options` |
| `ren_format.h/.cpp` | The mask language and the date format |
| `ren_exif.h/.cpp` | EXIF, IPTC and XMP reader, sidecar file names |
| `ren_id3.h/.cpp` | ID3 reader (MP3) |
| `ren_path.h` | Delphi-style path helpers (header only) |

**Paths.** The helpers of `ren_path.h` follow the rules of the system: on Windows `\`, `/` and `:` are separators (`isDelim`) and `\` is the one the helpers add (`kSep`); on Linux only `/` is, and `\` or `:` are ordinary characters of a name. On Windows paths may therefore mix separators: normalise them (`QDir::fromNativeSeparators`) before comparing two paths.

**`File`.** `rename(newName)` computes the target path, refuses an identical name, creates missing folders when asked, then moves the file. On Windows it uses `MoveFileW` and not `QFile::rename`, because the latter refuses case-only renames (`a.txt` to `A.txt`) on case-insensitive disks and the Case operation needs them. `revert()` puts the old name back. After a successful rename, `moveSidecars` moves the sidecar files of the old name (`sidecarCandidates()`: `IMG.xmp`, `IMG.XMP`, `IMG.CR2.xmp`..., same with `.dop`) to match the new name. A problem with a sidecar never fails the picture: it is only added to the log text. `FileList::add` plugs two callbacks on each file: when a **folder** is renamed the files below follow it, and when a sidecar that is also in the list is moved by its picture, its list entry follows (`followPath`).

**The mask language** (`advancedFormat`). `%tag%` calls a *getter*, a function supplied by the caller that returns the value of the tag; a tag the getter refuses stays as text. `%%` gives `%`. With `AllowQuotes`, `"..."` protects a text; with `AllowIf`, `$if(cond,then,else)` takes `then` when `cond` expands to a non-empty text, and `$$` gives `$`. `formatDateTime` reimplements Delphi's `FormatDateTime` (`d`, `m`, `yy`, `h`, `n` for minutes, `s`, `z`, `am/pm`, `a/p`, `ampm`...); `m` is a minute after an hour or before seconds, otherwise a month, as in Delphi.

**The operations.** `Action` is the base class. A subclass overrides:
- `makeNewName(fullPath, isFolder)`: the core, returns the new name (without folder). It does not write to the disk.
- `description()`: the sentence shown in the job list and the log.
- `save()` / `load()`: XML settings.
- `getTag()`: the value of a `%tag%` (the base class knows `name`, `ext`, `folder`, `folderN`, `count...`; subclasses add `num`, `random`, ID3 and EXIF tags).
- `init()` / `finish()`: called before and after the pass over all the files.

Settings are plain public fields that the interface reads and writes directly.

| Value | Class | XML tag | Principle |
|---|---|---|---|
| 0 | `ActionChangeExt` | `ChangeExt` | Replace, remove or append the extension |
| 1 | `ActionStringRepl` | `StrRepl` | Replace a text (all or first, case, extension, wildcards) |
| 2 | `ActionMultstrRepl` | `MultstrRepl` | "from to" table, one pair per line separated by a tab |
| 3 | `ActionStringIns` | `StrIns` | Insert a text at a position |
| 4 | `ActionMoveStr` | `MoveStr` | Move N characters to another position |
| 5 | `ActionCharDel` | `CharDel` | Delete characters at a position or around a string |
| 6 | `ActionEnum` | `Enum` | Numbering with a mask, optional restart per folder and shared number for JPG + RAW |
| 7 | `ActionMP3` | `MP3` | Names from ID3 tags |
| 8 | `ActionDateTime` | `DateTime` | Names from the creation or modification date |
| 9 | `ActionRandom` | `Random` | Random names (numbers, tick counter or GUID) |
| 10 | `ActionCase` | `ChangeCase` | Change case |
| 11 | `ActionFromList` | `FromList` | One name per line of a list |
| 12 | `ActionRegexp` | `Regexp` | Regular expression (case-insensitive, `.` matches line breaks, as TRegExpr) |
| 13 | `ActionExif` | `Exif` | Names from EXIF, IPTC and XMP tags, with a `%num%` counter |

The values are those of the original and are the row numbers of the operations list on screen (`ActionType`).

`plainFolder()` is true for a folder when the "folders have an extension" option is off: the whole name is then handled as a name without extension, which is why most `makeNewName` have a branch for it.

**Running.** `Action::perform(list, op, progress)` loops over **all the files for one operation**:
1. `init()` (not for a revert).
2. For each file, according to `op`: `Rename` checks that the file still exists, calls `makeNewName`, then `File::rename` (a sidecar whose picture is also in the list is skipped: its picture renames it); `Preview` does not touch the disk and builds on the preview left by the previous operation; `Revert` calls `File::revert()` (kept for the tests); `Undo` is handled by `FileList::undoPasses`.
3. Calls `progress(file, operationPosition, fileIndex)`.
4. `finish()`.

An operation can report a problem without failing (`setInfo`): the file keeps its name and the log shows a message such as "No EXIF info found".

`ActionList::perform` runs **each operation in turn over all the files**. In `Rename` mode every operation really renames the files on the disk and the next one starts from the new names; it is not one rename at the end. `Op::Revert` calls `File::revert()` on every file, one level only (the previous name is kept in each `File`). The interface does not use it any more: each action of the batch is a **pass** of the `Journal` (`beginPass` / `record` / `endPass`; a pass without any move is dropped, 50 passes at most), fed by the `onMoved` callback of `File::rename`. `Op::Undo` calls `FileList::undoPasses(n)`, which undoes the last `n` passes move by move, newest first (`File::undoMove`, on the list entry or on a temporary one when the file left the list). It stops at the first move that fails: the pass keeps the moves that remain. Copies are not recorded.

**Job file (`.arb`).** XML written with `QXmlStreamWriter`:

```xml
<AntRenamer Version="3.0.0-rc.15" Date="2026-10-18">
 <Batch>
  <StrRepl Search="old" Repl="new" AllOccurences="True" CaseSensitive="True" .../>
  <Enum Mask="File %num%%ext%" Start="1" Digits="3" Incr="1" Restart="False" SharePairs="False"/>
 </Batch>
</AntRenamer>
```

Settings are attributes (`True` / `False`), long texts (replacement table, list of names) are CDATA children. When loading, each element is matched by its tag name (table `kNames`), the action is created with `ActionList::create` and its `load()` reads the attributes; a missing attribute takes its default, so older jobs still load, and unknown attributes are ignored.

**EXIF, IPTC, XMP** (`readExif`). Fills a name-to-value table (names are case-insensitive): first the XMP sidecar of the file (it wins over embedded data), then the first 512 KB of the file itself. JPEG: the "Exif" APP1 segment gives the TIFF block, the "Photoshop 3.0" APP13 segment gives the IPTC. TIFF and DNG: the IFDs are read directly. XMP is found by content (`<x:xmpmeta` ... `</x:xmpmeta>`), which covers JPEG, PNG, WebP, TIFF, DNG and `.xmp` files. The tables `tagNames()`, `iptcNames()` and `xmpProps()` list the known tags, and `sidecarExtensions()` lists the sidecar extensions (`xmp`, `dop`): it is the only place to change to add one.

**ID3.** `readId3` reads ID3v2.2, 2.3 and 2.4 (Latin-1, UTF-16 and UTF-8 frames) and completes it with ID3v1/1.1.

## The interface (`app/`)

| File | Role |
|---|---|
| `main.cpp` | Entry point: single instance, language, `--install-shell`, main window |
| `mainwindow.h/.cpp` | Main window: files list, job, preview, start, log |
| `actionpanel.h/.cpp` | The "Operations" page: the 14 types and their settings |
| `dialogs.h/.cpp` | Add folders, drag and drop, Options, About |
| `appsettings.h/.cpp` | Program options in an INI file |
| `lngtranslator.h/.cpp` | Translation with the original `.lng` files |
| `helpwindow.h/.cpp` | Built-in help viewer (Markdown) |
| `icons.h/.cpp` | Toolbar icons cut from the bitmap sets |
| `shellmenu.h/.cpp` | Windows Explorer context menu entry (registry, Windows only) |

**Startup** (`main.cpp`). Names the application ("Ant Renamer Qt", organisation `AntRenamerQt`: this decides where the INI file is) and sets the version (`AR_APP_VERSION`, from CMake). Single instance: a lock file in the temporary folder; a second instance started with `--shell` sends its paths to the first one through a local socket (this is why Qt Network is linked) and exits. Then the language, `--install-shell` / `--remove-shell` if asked, and the main window.

**Translation.** The program does not use Qt `.ts` files but the original `.lng` files (XML, various encodings read from their header). The code contains the English texts (`tr_("Renaming process started")`). `loadLanguage` reads `English.lng` and the chosen language, flattens both into tables (keys such as `MainForm/ActionAddFiles/Caption`) and, for every key present in both, maps the English text to the translation (empty or identical pairs are ignored; a `Hint` of the form "short|long" is registered as two texts). The key of the table is the normalised English text: `%s`, `%d` (Delphi) and `%1`...`%9` (Qt) all become `%@`. So an English text of the code must be **identical** to an entry of `English.lng` to be translated; otherwise it stays in English. `ANTREN_MISSING=1` lists the texts without translation.

**Settings.** `appSettings()` is a `QSettings` in INI format: next to the executable if `antrenamer-qt.ini` exists (portable), otherwise in the user profile. `AppOptions` holds every option of the Options dialog with the defaults of the original; a macro `KEYS(...)` lists each field once to generate `load()` and `save()` (keys `options/name`), and `applyToCore()` copies the processing options to `ren::options()`. Other keys: `window/*` (geometry, columns, splitters, per layout), `last/*` (last values of the operation settings), `history/*`, `mru/batch`, `view/changedOnly`, `log/*`.

**Main window.** Three pages: Files, Actions, Log. Depending on an option: tabs (classic) or the modern single-screen layout (bands or columns). `files_` (`FileList`) and `batch_` (`ActionList`) are the real data; the views (`fileView_`, `batchView_`, `QTreeWidget`s that accept drag and drop) only mirror them: each row carries a pointer to its `File` or `Action`, and `updateItem` copies a file's state into the columns (and hides the row when "Changed only" is on and the preview equals the name).

**The Operations panel** (`ActionPanel`) owns **one** action being configured. The list on the left holds the 14 types (the row index **is** the `ActionType` value), with one page of controls per type. Every control is created by a helper (`combo`, `spin`, `check`, `radios`) that registers it with a key, a getter and a setter, which is how the last values are saved and restored. Two functions are the reverse of each other: `makeAction()` reads the controls and builds the `Action`; `setFromAction()` loads an existing action into the controls (to edit an action of the job). `test_panel` checks that they round-trip for every field and every type: **adding a field to an action without updating `setFromAction` makes it fail**.

**Live preview.** The panel emits `changed()` at every modification; the window takes the selected file (or the first), copies it into a temporary `FileList`, runs `makeAction(false)` in `Preview` mode and shows "Current name / New name". The Preview button does the same on the whole list and fills the Preview column.

**Starting a renaming** (`MainWindow::run(op)`).
1. Checks that nothing is already running (`worker_`), that there are files and an action.
2. Logs, numbers files and actions. With an empty job, the action configured on screen is run and then forgotten.
3. Creates a **`RenWorker`** (a `QThread`) and starts it: the work runs **off the interface thread**, so the window stays responsive and Stop works.
4. The worker calls `ActionList::perform` and, after each file, emits `progress(Snapshot)`. A `Snapshot` is a **copy** (pointer, description, state, positions): the interface never reads a `File` that the other thread is modifying. It is delivered with a queued connection.
5. `onProgress` updates the row (option "real time"), the log, the progress bar.
6. `onFinished` drains the pending signals, logs, applies the end-of-process options (switch page, clear the list or the job, save the log to a file with a UTF-8 BOM), re-enables the buttons and exits if `-x` was given.

While a renaming runs (`worker_` set), adding, moving and opening a job are ignored, and the window refuses to close.

**Log.** `addLog(text, state)` stores the line in memory and shows it unless its state is filtered. Colours: blue (not renamed), green (OK), red (error), grey (information). The log filters rebuild the display from the stored lines.

**Other files.** `dialogs.*`: `AddFoldersDialog` (a folder tree with files / folders / recursive boxes), `DragDropDialog` (what to do with a drop), `OptionsDialog`, `AboutDialog`; `AddChoice` is the small shared "files / folders / recursive" structure. `helpwindow.*`: two-pane viewer (table of contents `toc_<lang>.md`, a nested list of links, in a `QTreeWidget`, page in a `QTextBrowser`, back and forward buttons) that reads `help/<page>_<lang>.md` (a heading written `## Title {#id}` gets the anchor `id`, so `page.md#id` links work); the help language is `en`, `fr`, `hr` or `ta`, English otherwise. `icons.*`: loads one of the three bitmap sets (square icons side by side) and cuts icon number `index`, or, for the fourth set ("Modern"), returns a `QIconEngine` that draws the icon with `QPainter` on a 24 x 24 grid (any size, theme colours, disabled look; no file). `shellmenu.*`: writes or removes `HKCU\Software\Classes\{*,Directory}\shell\AntRenamerQt` with the command `antrenamer-qt.exe --shell "%1"`.

## One renaming, end to end

Job: an *Enumeration* operation with the mask `Photo %num%%ext%`; files `a.jpg` and `a.xmp`; click **Go**.

1. **Go** calls `MainWindow::run(Op::Rename)`; the worker starts and `ActionList::perform` runs the operation.
2. `init()` sets the counter to the start value.
3. For `a.jpg`: it is not a listed sidecar. `makeNewName` calls `advancedFormat("Photo %num%%ext%", ...)`; the getter returns `num` = `001` and `ext` = `.jpg`, giving `Photo 001.jpg`.
4. `File::rename("Photo 001.jpg")` moves the file. The log text becomes "... renamed to Photo 001.jpg".
5. With the sidecar option on, `moveSidecars` computes the candidates of `a.jpg` (`a.xmp`, `a.XMP`, `a.jpg.xmp`, `a.jpg.XMP`, same for `.dop`) and of `Photo 001.jpg`, moves those that exist, and appends "(sidecar file ... renamed to ...)" to the log text. The `a.xmp` row of the list follows (`followPath`).
6. For `a.xmp` in the list: its picture `a.jpg` is in the list, so it is skipped.
7. The counter advances, the worker emits `progress`, the interface updates the row and the log.
8. `onFinished` applies the end-of-process options.
