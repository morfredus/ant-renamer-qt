# Ant Renamer Qt

![version](https://img.shields.io/badge/version-3.0.0--rc.9-blue)

Portage C++/Qt 6 d'[Ant Renamer](https://www.antp.be/software/renamer/) 2.13 (Delphi 7 à l'origine,
GPL v2). Renommage de fichiers par lots : remplacer, insérer, déplacer, supprimer, énumérer, changer
la casse, expressions régulières, noms depuis une liste, date du fichier, tags MP3 (ID3v1/v2), EXIF / IPTC / XMP.

**Ant Renamer Qt** est le nom officiel de ce portage, convenu avec l'auteur du programme d'origine. Il est
développé en parallèle de l'Ant Renamer d'origine et a son propre exécutable, son propre dossier d'installation,
ses propres réglages et ses propres noms de paquets : les deux peuvent être installés côte à côte sans confusion.

Le projet est **autonome** : Qt 6 et un compilateur C++20 suffisent.

## Compilation

```bash
cmake --preset mingw   && cmake --build --preset mingw     # Windows (MSYS2 / MinGW-w64)
cmake --preset linux   && cmake --build --preset linux     # Linux x86_64
cmake --preset linux-arm64 && cmake --build --preset linux-arm64   # machine ARM64 (ex. Raspberry Pi)
ctest --preset linux                                       # tests du cœur
```

Résultat : `build-mingw/app/antrenamer-qt.exe` (les DLL Qt et MinGW sont déployées à côté) ou
`build/app/antrenamer-qt`. `linux-arm64-cross` compile en croisé depuis x86_64 : définir `ARM64_SYSROOT`
vers un sysroot ARM64 contenant Qt 6 (voir `cmake/toolchains/linux-aarch64.cmake`).

Livrables (Windows) : `powershell -ExecutionPolicy Bypass -File scripts\windows\package-win.ps1` produit
`dist/AntRenamerQt-<version>-win64-portable.zip` et, si Inno Setup 6 est installé, l'installeur `-setup.exe`.

Paquets Linux (sous Linux ou dans WSL) : `scripts/linux/package-deb.sh` produit `dist/AntRenamerQt-<version>-linux-<arch>.deb` et
`scripts/linux/package-appimage.sh` l'AppImage x86_64, après `cmake --preset linux && cmake --build --preset linux`.
Sur aarch64 utilisez le preset `linux-arm64` ; depuis x86_64, `linux-arm64-cross` construit le paquet arm64 avec un sysroot
arm64 (`ARM64_SYSROOT`), puis `package-deb.sh --build build-arm64-cross`.

Prérequis Windows : MSYS2 avec `mingw-w64-x86_64-{gcc,cmake,ninja,qt6-base}`.

## Organisation

| Dossier | Contenu |
|---|---|
| `core/` | moteur de renommage, QtCore seulement |
| `app/` | interface Qt Widgets, `Languages/` et `help/` de l'original |
| `tests/` | vérifications du cœur (`ctest`) |
| `cmake/toolchains/` | toolchain de cross-compilation ARM64 |
| `scripts/windows/` | déploiement des DLL MinGW, aide VS Code |

## À propos de ce portage

Ce qui a été conservé, modifié et ajouté par rapport à Ant Renamer 2.13 :
[docs/PRESENTATION_fr.md](docs/PRESENTATION_fr.md) ([en](docs/PRESENTATION.md)).

## Réglages

Mode portable : créer un fichier `antrenamer-qt.ini` vide à côté du programme (ou cocher la case dans
Options > Langue). Sinon les réglages sont dans `%APPDATA%\AntRenamerQt\Ant Renamer Qt\antrenamer-qt.ini`.

Menu contextuel de l'Explorateur : Options > Fichiers && Dossiers ajoute « Ajouter à Ant Renamer Qt » aux fichiers et
dossiers (Windows 11 : « Afficher plus d'options »). Tout ce qui est sélectionné arrive dans la fenêtre déjà ouverte.

Ligne de commande : `-b fichier.arb` charge un job, `-g` démarre, `-x` quitte à la fin, `-a[fFr]`
choisit ce qu'ajoutent les chemins suivants (f fichiers, F dossiers, r récursif ; jokers acceptés).

## Assistance

L'assistance pour Ant Renamer Qt se fait sur GitHub : [Issues](https://github.com/morfredus/ant-renamer-qt/issues) (bugs, demandes de fonctions) et [Discussions](https://github.com/morfredus/ant-renamer-qt/discussions) (questions, idées). Merci de ne pas signaler les problèmes d'Ant Renamer Qt à l'auteur du programme d'origine.

## Crédits

- Programme d'origine : Antoine Potten, [Ant Renamer](https://www.antp.be/software/renamer/) (Delphi 7, GPL v2).
- Portage C++/Qt 6 et portage Linux : Frédéric Biron ([morfredus](https://github.com/morfredus)).

## Licence

GPL v2 ou ultérieure, comme le programme d'origine.
