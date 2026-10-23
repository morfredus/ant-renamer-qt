# Compiler Ant Renamer Qt

Comment le projet est compilé, testé et empaqueté. Pour la structure du code voir [ARCHITECTURE_fr.md](ARCHITECTURE_fr.md) ; pour contribuer voir [CONTRIBUTING_fr.md](../CONTRIBUTING_fr.md). ([English version](BUILDING.md))

## Prérequis

- Un compilateur C++20 (g++ est celui qu'utilise le projet ; MSVC est prévu dans les fichiers CMake).
- CMake 3.21 ou plus récent, et Ninja.
- Qt 6 : modules Core, Widgets et Network (plus les outils Qt, `windeployqt`, sous Windows).

Rien d'autre : aucune bibliothèque tierce.

| Plateforme | Installation |
|---|---|
| Windows | [MSYS2](https://www.msys2.org/), puis dans un shell MSYS2 MINGW64 : `pacman -S mingw-w64-x86_64-gcc mingw-w64-x86_64-cmake mingw-w64-x86_64-ninja mingw-w64-x86_64-qt6-base mingw-w64-x86_64-qt6-tools` |
| Debian / Ubuntu | `sudo apt install cmake ninja-build g++ qt6-base-dev libgl1-mesa-dev` |

## Organisation de la compilation

- Le **moteur** (`core/`) est une bibliothèque statique, `rencore`, qui ne dépend que de QtCore.
- L'**application** (`app/`) est l'exécutable `antrenamer-qt` (Qt Widgets et Network). Il utilise `rencore`.
- Deux programmes de test : `test_core` (moteur seul) et `test_panel` (l'éditeur d'actions, sans fenêtre).
- La version vient du fichier `VERSION` à la racine, source unique : CMake la lit et la passe au programme.
- `CMakePresets.json` contient les réglages par plateforme : une compilation, c'est toujours `cmake --preset <nom>` puis `cmake --build --preset <nom>`.

CMake ne compile rien lui-même : il génère des fichiers Ninja dans un dossier de build, et Ninja lance le compilateur.

## Presets

| Preset | Pour | Dossier de build |
|---|---|---|
| `linux` | Linux x86-64, natif | `build/` |
| `linux-arm64` | Linux ARM64, natif (par exemple un Raspberry Pi) | `build-arm64/` |
| `linux-arm64-cross` | ARM64 compilé depuis un x86-64 | `build-arm64-cross/` |
| `mingw` | Windows, MinGW-w64 (MSYS2) | `build-mingw/` |

Les dossiers de build sont ignorés par git et peuvent être supprimés à tout moment.

## Compiler et tester

```bash
# Linux x86-64
cmake --preset linux && cmake --build --preset linux
ctest --test-dir build --output-on-failure
```

```bash
# Windows, dans un shell MSYS2 MINGW64
cmake --preset mingw && cmake --build --preset mingw
ctest --test-dir build-mingw --output-on-failure
```

Sous Windows, `scripts\windows\vscode-mingw.ps1 build` (ou `configure`, `run`, `clean`) retrouve MSYS2 tout seul et lance les mêmes commandes ; c'est ce qu'utilisent les tâches VS Code. Le preset `mingw` prend la variable d'environnement `QT_ROOT` comme emplacement de Qt quand elle est définie ; dans un shell MSYS2, Qt est trouvé de toute façon.

Les tests n'ont pas besoin d'écran : `test_panel` règle lui-même `QT_QPA_PLATFORM=offscreen`.

Ne partagez pas un dossier de build entre deux systèmes (par exemple configuré depuis WSL puis utilisé sous Windows) : le cache CMake garde le chemin des sources. `vscode-mingw.ps1` supprime un dossier de build dont le cache pointe vers un autre chemin.

### Compilation croisée ARM64

`cmake --preset linux-arm64-cross` demande un sysroot ARM64 contenant Qt 6, indiqué par la variable d'environnement `ARM64_SYSROOT` (voir `cmake/toolchains/linux-aarch64.cmake`). Facultatives : `ARM64_CROSS_PREFIX` (par défaut `aarch64-linux-gnu-`) et `ARM64_QT_HOST_PATH` (un Qt 6 hôte de même version, pour que moc et rcc s'exécutent nativement). Compiler directement sur une machine ARM64 avec le preset `linux-arm64` est bien plus simple.

## Ce que fait une compilation

1. **Configuration** : CMake lit `CMakeLists.txt` et `app/CMakeLists.txt`, lit `VERSION`, trouve Qt et écrit les fichiers Ninja. Rien n'est encore compilé.
2. **Code généré** : `moc` (le compilateur de méta-objets de Qt, `CMAKE_AUTOMOC`) génère un `moc_*.cpp` pour chaque en-tête qui déclare une classe Qt ; `rcc` (`CMAKE_AUTORCC`) transforme `app/resources.qrc` (icônes, bitmaps, `default.xml`) en fichier C++, ce qui les embarque dans l'exécutable. Sous Windows, `app/app.rc` (icône et manifeste) est compilé avec `windres`.
3. **Compilation** de chaque `.cpp` en fichier objet.
4. **Archivage** des objets de `core/` dans `librencore.a`.
5. **Édition de liens** : `test_core` = sa source + `rencore` ; `antrenamer-qt` = les objets de `app/` + `rencore` + Qt ; `test_panel` = sa source + quelques fichiers de `app/` + `rencore`.
6. **Après la compilation** : `app/Languages/` et `app/help/` sont copiés à côté de l'exécutable à chaque compilation (cible `antrenamer-data` ; le dossier d'aide est vidé d'abord, une page supprimée ne reste donc pas ; `cmake --build <dossier> --target antrenamer-data` les rafraîchit seuls, même programme lancé). Sous Windows, `windeployqt` copie ensuite les DLL et les plugins Qt, et `scripts/windows/deploy-mingw.sh` les DLL MinGW, de sorte que `build-mingw/app/antrenamer-qt.exe` démarre par double-clic.

```
core/*.cpp ── compilation ──> librencore.a ─┬─> test_core
                                            ├─> test_panel
app/*.cpp + code moc/rcc ── compilation ────┴─> antrenamer-qt(.exe)
app/Languages + app/help ── copie ──────────> à côté de l'exécutable
```

## Où le programme cherche ses fichiers

- **Langues et aide** : `Languages/` et `help/` à côté de l'exécutable (Windows, dossier de build), sinon `<préfixe>/share/antrenamer-qt/` (paquets Linux).
- **Images et `default.xml`** : à l'intérieur de l'exécutable (ressources Qt).
- **Réglages** : `antrenamer-qt.ini` à côté de l'exécutable si ce fichier existe (mode portable, ce que contient le zip), sinon dans le dossier de données utilisateur de Qt : `%APPDATA%\AntRenamerQt\Ant Renamer Qt\` sous Windows, `~/.local/share/AntRenamerQt/Ant Renamer Qt/` sous Linux.

## Paquets

Tous les paquets sont écrits dans `dist/`.

| Plateforme | Commande | Résultat |
|---|---|---|
| Windows | `powershell -ExecutionPolicy Bypass -File scripts\windows\package-win.ps1` | `AntRenamerQt-<version>-win64-portable.zip` et, si Inno Setup 6 est installé, `AntRenamerQt-<version>-win64-setup.exe` |
| Debian / Ubuntu | `scripts/linux/package-deb.sh` | `AntRenamerQt-<version>-linux-<arch>.deb` |
| Linux, toute distribution | `scripts/linux/package-appimage.sh` | `AntRenamerQt-<version>-linux-x86_64.AppImage` |
| Linux, Flatpak (Flathub) | `flatpak-builder --user --install --force-clean build-dir packaging/flatpak/io.github.morfredus.AntRenamerQt.yaml` | l'application installée pour l'utilisateur |

- `package-win.ps1` compile, prépare les fichiers utiles à l'exécution, puis lance Inno Setup avec `installer/antrenamer-qt.iss` pour l'installeur. Le zip contient un `antrenamer-qt.ini` vide, qui active le mode portable.
- `package-deb.sh` attend une compilation terminée (`--build <dossier>` pour la choisir). Le paquet dépend du Qt du système (les dépendances sont détectées depuis le binaire, ou depuis le sysroot en compilation croisée) et installe le programme dans `/usr/bin`, les langues et l'aide dans `/usr/share/antrenamer-qt`.
- `package-appimage.sh` télécharge `linuxdeploy` et son greffon Qt dans `.cache/` la première fois (une connexion Internet est nécessaire une fois). Les deux sont épinglés à une version et vérifiés par un SHA-256 dans le script (`LD_TAG`/`LD_SHA256`, `QT_TAG`/`QT_SHA256`) : un fichier dont l'empreinte diffère est refusé. Pour les mettre à jour, changer les tags, télécharger les fichiers et recalculer les empreintes avec `sha256sum`.
- `packaging/flatpak/` contient le manifeste Flatpak (`….local.yaml` est le même compilé depuis la copie de travail, pour l'essayer avant qu'un tag existe ; il demande `flatpak-builder` et le runtime et le SDK KDE de Flathub), le fichier AppStream (`.metainfo.xml` : description, captures, versions) et l'icône est `app/antrenamer-qt.svg`. Flathub compile depuis le tag Git indiqué dans le manifeste (`tag` et `commit`) : avant de soumettre une version, renseigner les deux ainsi que la date de la version dans le `.metainfo.xml`. Sous Flatpak le programme n'a pas d'accès réseau, la recherche de nouvelle version y est donc masquée (`runsInFlatpak()` dans `app/updatecheck.h`).
- `.github/workflows/build.yml` compile sous Linux et Windows à chaque push et pull request, lance les tests, et garde les paquets Windows comme artefacts du workflow.

## Options de ligne de commande du programme

| Option | Effet |
|---|---|
| `-b fichier.arb` (ou un `.arb` seul) | Charge un job |
| `-g` | Lance le renommage |
| `-x` | Quitte à la fin |
| `-a[fFr]` | Ce que les chemins suivants ajoutent : `f` fichiers, `F` dossiers, `r` récursif ; jokers acceptés |
| `--shell chemin...` | Utilisé par le menu de l'Explorateur : ajoute les chemins à la fenêtre déjà ouverte |
| `--install-shell` / `--remove-shell` | Ajoute ou retire l'entrée du menu contextuel de l'Explorateur, puis quitte |

Aides de développement (variables d'environnement) : `ANTREN_MISSING=1` journalise les textes sans traduction dans toutes les boîtes de dialogue ; `ANTREN_SHOT=fichier.png` enregistre une capture de la fenêtre puis quitte (avec `ANTREN_TAB`, `ANTREN_DELAY`, `ANTREN_DIALOG`, `ANTREN_DTAB` et `ANTREN_HELPTOPIC` pour choisir la page ou la boîte, `ANTREN_SIZE=LxH` pour fixer la taille de la fenêtre, `ANTREN_THEME_TOGGLE=n` pour appuyer n fois sur le bouton clair / sombre, `ANTREN_ACTION=n` pour choisir l'opération `n` (0 à 13) sur la page Opérations et `ANTREN_PREVIEW=1` pour actualiser l'aperçu d'abord). Avec un fichier `antrenamer-qt.ini` à côté du programme (mode portable) contenant `language=French`, c'est ainsi que sont faites les captures d'écran de l'aide sans toucher à vos propres réglages.
