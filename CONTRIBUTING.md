# Contributing to Ant Renamer Qt

Thank you for your interest. ([Version française](CONTRIBUTING_fr.md))

## Before you start

- **Bug, idea or question?** Open an [issue](https://github.com/morfredus/ant-renamer-qt/issues) (bugs, feature requests) or start a [discussion](https://github.com/morfredus/ant-renamer-qt/discussions) (questions, ideas). Please do not report Ant Renamer Qt problems to the author of the original program.
- **Bigger change?** Talk about it first in an issue, so nobody works on something that will not be merged.
- Issues and pull requests can be written in English or in French.

## Get the code and build it

Everything about building, testing and packaging is in [docs/BUILDING.md](docs/BUILDING.md). In short:

```bash
cmake --preset linux && cmake --build --preset linux      # or --preset mingw on Windows (MSYS2)
ctest --test-dir build --output-on-failure
```

To understand where things are and how a renaming runs, read [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md). The engine (`core/`) has no user interface and is covered by `tests/test_core.cpp`; the interface is in `app/`.

## Making a change

1. Fork the repository and create a branch from `main`.
2. Keep the change focused: one subject per pull request.
3. Follow the style of the surrounding code (naming, comment density, idioms). There is no formatter to run.
4. Add or update a test when the engine changes (`tests/test_core.cpp`): a new case that fails without your fix is the best proof that it works.
5. Run the tests (`ctest`) before you open the pull request; the GitHub workflow builds on Linux and Windows and runs them too.
6. Add a line under `## [Unreleased]` in [CHANGELOG.md](CHANGELOG.md) (Added, Changed or Fixed). Version numbers are given by the maintainer when a release is made: do not change `VERSION`.
7. If the change is visible to users, add a line to `docs/WHATS_NEW.md` and `docs/WHATS_NEW_fr.md` (leave the "Since" column as *(upcoming)* / *(à venir)*) and update the help pages and the guides when they describe the behaviour. If you do not write French, say so in the pull request and the maintainer will take care of the French texts.

## How to...

**Add an operation (a 15th type).**
1. `core/ren_actions.h`: add the value at the **end** of `enum class ActionType` (the numbers are those of the original and are the row numbers of the list on screen) and declare `class ActionXxx : public Action` with its fields, `description`, `makeNewName`, `save`, `load` (and `getTag` / `init` if needed).
2. `core/ren_actions.cpp`: implement them, add the XML tag to the `kNames` table and the type to `ActionList::create`.
3. `app/actionpanel.cpp`: add the page in `buildPages` (the order of the pages is the type index), then the branch in `makeAction` and in `setFromAction`.
4. Add the English texts in the code and their translations in the `.lng` files (`app/Languages/`).
5. Add a case to `tests/test_core.cpp`. `tests/test_panel.cpp` catches a forgotten `setFromAction`.
6. Documentation: help pages (`app/help/actions_*.md`), `docs/USER_GUIDE*.md`, `docs/WHATS_NEW*.md`, `CHANGELOG.md`.

**Add a setting to an existing operation.** A field in the class (with its default), `save` / `load` (an XML attribute; use the default when it is missing so older jobs still load), the use in `makeNewName`, the control in `buildPages`, `makeAction` **and** `setFromAction`, the text in the `.lng` files, a case in `test_core.cpp`.

**Add a program option.** A field in `AppOptions` (`app/appsettings.h`), its name in the `KEYS` macro (`app/appsettings.cpp`), the control in `OptionsDialog` (`app/dialogs.cpp`); if the engine needs it, a field in `ren::Options` and the copy in `applyToCore()`.

**Add an EXIF, IPTC or XMP tag.** A line in the matching table (`tagNames`, `iptcNames`, `xmpProps` in `core/ren_exif.cpp`), the help page of the tags, and a test with a synthetic file in `test_core.cpp`.

**Add a sidecar type.** Add the extension to `sidecarExtensions()` in `core/ren_exif.cpp`: finding the candidates, moving, reverting and skipping the sidecars that are in the list all follow from it.

**Add a source file.** Add the `.cpp` to `add_library(rencore ...)` (engine) or to `set(SRC ...)` in `app/CMakeLists.txt`: the lists of sources are written by hand, nothing is detected automatically. A forgotten file shows up as an "undefined reference" at link time. If `test_panel` also needs it, add it to its list too.

**Add a language or fix a translation.** Follow [docs/TRANSLATING.md](docs/TRANSLATING.md). Edit or add a `.lng` file in `app/Languages/`. For the help, add the `_<code>.md` pages and the `toc_<code>.md`, and declare the code in `helpLanguageCode()` (`app/lngtranslator.cpp`).

## Things to know

- **Path separators.** `\` and `/` live together in the engine. Normalise paths before comparing two of them.
- **Threads.** The engine runs in a worker thread (`RenWorker`). The interface must only read the `Snapshot` copies it receives, never a `File` while a renaming runs.
- **A job renames for real at each operation**, not once at the end. The revert goes back one level only (the previous name of each file).
- **Translations.** The English text in the code must be identical to an entry of `English.lng` to be translated, otherwise it stays in English. `ANTREN_MISSING=1` lists the texts without translation.
- **Line endings.** Every text file is LF, in the repository and in the working tree (`.gitattributes`). If your editor or a script writes CRLF, Git converts it back, but fix the tool when you can. Encodings are not touched: the help pages and the `.lng` files keep theirs, so do not re-save them with another encoding.

## License

Ant Renamer Qt is released under the GPL v2, like the original program. By contributing you agree that your contribution is distributed under the same license.
