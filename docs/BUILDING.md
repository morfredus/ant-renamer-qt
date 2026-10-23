# Building Ant Renamer Qt

How the project is built, tested and packaged. For the structure of the code see [ARCHITECTURE.md](ARCHITECTURE.md); to contribute see [CONTRIBUTING.md](../CONTRIBUTING.md). ([Version française](BUILDING_fr.md))

## Requirements

- A C++20 compiler (g++ is what the project uses; MSVC is accepted by the CMake files).
- CMake 3.21 or newer and Ninja.
- Qt 6: modules Core, Widgets and Network (plus the Qt tools, `windeployqt`, on Windows).

Nothing else: no third-party library.

| Platform | Install |
|---|---|
| Windows | [MSYS2](https://www.msys2.org/), then in an MSYS2 MINGW64 shell: `pacman -S mingw-w64-x86_64-gcc mingw-w64-x86_64-cmake mingw-w64-x86_64-ninja mingw-w64-x86_64-qt6-base mingw-w64-x86_64-qt6-tools` |
| Debian / Ubuntu | `sudo apt install cmake ninja-build g++ qt6-base-dev libgl1-mesa-dev` |

## How the build is organised

- The **engine** (`core/`) is a static library, `rencore`, that depends on QtCore only.
- The **application** (`app/`) is the executable `antrenamer-qt` (Qt Widgets and Network). It links `rencore`.
- Two test programs: `test_core` (engine only) and `test_panel` (the actions editor, without a window).
- The version comes from the `VERSION` file at the root, the single source: CMake reads it and passes it to the program.
- `CMakePresets.json` holds the settings per platform, so a build is always `cmake --preset <name>` then `cmake --build --preset <name>`.

CMake does not compile anything itself: it generates Ninja files in a build folder, and Ninja runs the compiler.

## Presets

| Preset | For | Build folder |
|---|---|---|
| `linux` | Linux x86-64, native | `build/` |
| `linux-arm64` | Linux ARM64, native (for example a Raspberry Pi) | `build-arm64/` |
| `linux-arm64-cross` | ARM64 cross-compiled from x86-64 | `build-arm64-cross/` |
| `mingw` | Windows, MinGW-w64 (MSYS2) | `build-mingw/` |

Build folders are ignored by git and can be deleted at any time.

## Build and test

```bash
# Linux x86-64
cmake --preset linux && cmake --build --preset linux
ctest --test-dir build --output-on-failure
```

```bash
# Windows, in an MSYS2 MINGW64 shell
cmake --preset mingw && cmake --build --preset mingw
ctest --test-dir build-mingw --output-on-failure
```

On Windows, `scripts\windows\vscode-mingw.ps1 build` (also `configure`, `run`, `clean`) finds MSYS2 by itself and runs the same commands, which is what the VS Code tasks use. The `mingw` preset uses the `QT_ROOT` environment variable as the Qt location when it is set; in an MSYS2 shell Qt is found anyway.

The tests need no screen: `test_panel` sets `QT_QPA_PLATFORM=offscreen` itself.

Do not share a build folder between systems (for example one configured from WSL and then used from Windows): the CMake cache keeps the source path. `vscode-mingw.ps1` deletes a build folder whose cache points to another source path.

### ARM64 cross-compilation

`cmake --preset linux-arm64-cross` needs an ARM64 sysroot that contains Qt 6, given by the `ARM64_SYSROOT` environment variable (see `cmake/toolchains/linux-aarch64.cmake`). Optionally `ARM64_CROSS_PREFIX` (default `aarch64-linux-gnu-`) and `ARM64_QT_HOST_PATH` (a host Qt 6 of the same version, so moc and rcc run natively). Building natively on an ARM64 machine with the `linux-arm64` preset is much simpler.

## What a build does

1. **Configure**: CMake reads `CMakeLists.txt` and `app/CMakeLists.txt`, reads `VERSION`, finds Qt and writes the Ninja files. Nothing is compiled yet.
2. **Generated code**: `moc` (Qt's meta-object compiler, `CMAKE_AUTOMOC`) generates a `moc_*.cpp` for every header that declares a Qt class; `rcc` (`CMAKE_AUTORCC`) turns `app/resources.qrc` (icons, bitmaps, `default.xml`) into a C++ file, which embeds them in the executable. On Windows `app/app.rc` (icon and manifest) is compiled with `windres`.
3. **Compile** every `.cpp` into an object file.
4. **Archive** the objects of `core/` into `librencore.a`.
5. **Link**: `test_core` = its source + `rencore`; `antrenamer-qt` = the objects of `app/` + `rencore` + Qt; `test_panel` = its source + a few files of `app/` + `rencore`.
6. **After the build**: `app/Languages/` and `app/help/` are copied next to the executable at every build (target `antrenamer-data`; the help folder is emptied first, so a deleted page does not stay; `cmake --build <dir> --target antrenamer-data` refreshes them alone, even while the program is running). On Windows, `windeployqt` then copies the Qt DLLs and plugins and `scripts/windows/deploy-mingw.sh` copies the MinGW DLLs, so `build-mingw/app/antrenamer-qt.exe` starts by double-click.

```
core/*.cpp ── compile ──> librencore.a ─┬─> test_core
                                        ├─> test_panel
app/*.cpp + moc/rcc code ── compile ────┴─> antrenamer-qt(.exe)
app/Languages + app/help ── copy ───────> next to the executable
```

## Where the program looks for its files

- **Languages and help**: `Languages/` and `help/` next to the executable (Windows, build folder), otherwise `<prefix>/share/antrenamer-qt/` (Linux packages).
- **Images and `default.xml`**: inside the executable (Qt resources).
- **Settings**: `antrenamer-qt.ini` next to the executable when that file exists (portable mode, which is what the zip contains), otherwise Qt's user data folder: `%APPDATA%\AntRenamerQt\Ant Renamer Qt\` on Windows, `~/.local/share/AntRenamerQt/Ant Renamer Qt/` on Linux.

## Packaging

All packages are written to `dist/`.

| Platform | Command | Result |
|---|---|---|
| Windows | `powershell -ExecutionPolicy Bypass -File scripts\windows\package-win.ps1` | `AntRenamerQt-<version>-win64-portable.zip` and, when Inno Setup 6 is installed, `AntRenamerQt-<version>-win64-setup.exe` |
| Debian / Ubuntu | `scripts/linux/package-deb.sh` | `AntRenamerQt-<version>-linux-<arch>.deb` |
| Linux, any distribution | `scripts/linux/package-appimage.sh` | `AntRenamerQt-<version>-linux-x86_64.AppImage` |
| Linux, Flatpak (Flathub) | `flatpak-builder --user --install --force-clean build-dir packaging/flatpak/io.github.morfredus.AntRenamerQt.yaml` | the application installed for the user |

- `package-win.ps1` builds, stages the files that are needed at run time, then runs Inno Setup with `installer/antrenamer-qt.iss` for the installer. The zip contains an empty `antrenamer-qt.ini`, which switches the portable mode on.
- `package-deb.sh` expects a finished build (`--build <folder>` to choose it). The package depends on the system Qt (the dependencies are detected from the binary, or from the sysroot when cross-compiling) and installs the program in `/usr/bin` and the languages and help in `/usr/share/antrenamer-qt`.
- `package-appimage.sh` downloads `linuxdeploy` and its Qt plugin into `.cache/` the first time (an Internet connection is needed once). Both are pinned to a release and checked against a SHA-256 in the script (`LD_TAG`/`LD_SHA256`, `QT_TAG`/`QT_SHA256`): a file with another hash is refused. To update them, change the tags, download the files and recompute the hashes with `sha256sum`.
- `packaging/flatpak/` holds the Flatpak manifest (`….local.yaml` is the same built from the working copy, to try it before a tag exists; it needs `flatpak-builder` and the KDE runtime and SDK from Flathub), the AppStream file (`.metainfo.xml`: description, screenshots, releases) and the icon is `app/antrenamer-qt.svg`. Flathub builds from the Git tag named in the manifest (`tag` and `commit`): before submitting a version, set both and the release date in the `.metainfo.xml`. Under Flatpak the program has no network access, so the new-version check is hidden (`runsInFlatpak()` in `app/updatecheck.h`).
- `.github/workflows/build.yml` builds on Linux and Windows for every push and pull request, runs the tests, and keeps the Windows packages as workflow artifacts.

## Command line options of the program

| Option | Effect |
|---|---|
| `-b file.arb` (or a `.arb` file alone) | Load a batch |
| `-g` | Start renaming |
| `-x` | Exit when finished |
| `-a[fFr]` | What the following paths add: `f` files, `F` folders, `r` recursive; wildcards allowed |
| `--shell path...` | Used by the Explorer context menu: add the paths to the window that is already open |
| `--install-shell` / `--remove-shell` | Add or remove the Explorer context menu entry, then exit |

Developer helpers (environment variables): `ANTREN_MISSING=1` logs the strings that have no translation in every dialog; `ANTREN_SHOT=file.png` saves a screenshot of the window and exits (with `ANTREN_TAB`, `ANTREN_DELAY`, `ANTREN_DIALOG`, `ANTREN_DTAB` and `ANTREN_HELPTOPIC` to choose the page or dialog, `ANTREN_SIZE=WxH` to set the window size, `ANTREN_THEME_TOGGLE=n` to press the light / dark button n times, `ANTREN_ACTION=n` to pick action `n` (0 to 13) on the Actions page and `ANTREN_PREVIEW=1` to refresh the preview first). With a `antrenamer-qt.ini` file next to the program (portable mode) containing `language=French`, this is how the screenshots of the help are made without touching your own settings.
