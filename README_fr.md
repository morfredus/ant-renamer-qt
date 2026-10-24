# Ant Renamer Qt

![version](https://img.shields.io/badge/version-3.0.0-blue)

Portage C++/Qt 6 d'[Ant Renamer](https://www.antp.be/software/renamer/) 2.13 (Delphi 7 à l'origine,
GPL v2 ou ultérieure). Renommage de fichiers par lots : remplacer, insérer, déplacer, supprimer, énumérer, changer
la casse, expressions régulières, noms depuis une liste, date du fichier, tags MP3 (ID3v1/v2), EXIF / IPTC / XMP.

**Ant Renamer Qt** est le nom officiel de ce portage, convenu avec l'auteur du programme d'origine. Il est
développé en parallèle de l'Ant Renamer d'origine et a son propre exécutable, son propre dossier d'installation,
ses propres réglages et ses propres noms de paquets : les deux peuvent être installés côte à côte sans confusion.

## Téléchargement

Installeur et zip portable Windows, `.deb` Linux (x86_64, arm64) et AppImage : [Releases](https://github.com/morfredus/ant-renamer-qt/releases).

**Vérifier les fichiers Linux.** Les fichiers Linux des versions postérieures à la 3.0.0 sont accompagnés d'une signature GPG détachée (`.asc`), y compris `SHA256SUMS-linux.txt`. Clé publique : [`docs/antrenamer-qt-public.asc`](docs/antrenamer-qt-public.asc), empreinte `D82C 3ED0 BE99 1611 C2C9 06FA 910B 46ED 3C84 B4A7` (Frederic Biron, Ant Renamer Qt releases). Comparez l'empreinte avant de faire confiance à la clé.

```
gpg --import antrenamer-qt-public.asc
gpg --verify AntRenamerQt-X.Y.Z-linux-amd64.deb.asc AntRenamerQt-X.Y.Z-linux-amd64.deb
```

Les fichiers Windows ne sont pas encore signés (les sommes de contrôle sont dans `SHA256SUMS.txt`).

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

Prérequis Windows : MSYS2 avec `mingw-w64-x86_64-{gcc,cmake,ninja,qt6-base,qt6-tools}`.

## Organisation

| Dossier | Contenu |
|---|---|
| `core/` | moteur de renommage, QtCore seulement |
| `app/` | interface Qt Widgets, `Languages/` et `help/` de l'original |
| `tests/` | vérifications du moteur et de l'éditeur d'opérations (`ctest`) |
| `cmake/toolchains/` | toolchain de cross-compilation ARM64 |
| `scripts/windows/` | déploiement des DLL MinGW, aide VS Code, zip et installeur |
| `scripts/linux/` | `.deb`, AppImage, fichier `.desktop` |
| `installer/` | script Inno Setup (installeur Windows) |
| `docs/` | guides, présentation, notes de version |
| `.github/` | workflow CI, Dependabot, modèles de tickets |

## À propos de ce portage

Ce qui a été conservé, modifié et ajouté par rapport à Ant Renamer 2.13 :
[docs/PRESENTATION_fr.md](docs/PRESENTATION_fr.md) ([en](docs/PRESENTATION.md)).

## Guide d'utilisation

Toutes les fonctions avec leur mode d'emploi (opérations, raccourcis, masques, tags, ligne de commande, options, jeux d'icônes, différences entre Windows et Linux) :
[docs/USER_GUIDE_fr.md](docs/USER_GUIDE_fr.md) ([en](docs/USER_GUIDE.md)).
Recettes pas à pas pour les cas courants et complexes (nettoyer des noms, numéroter, photos, musique, déplacer des fichiers, ligne de commande) :
[docs/USER_RECIPES_fr.md](docs/USER_RECIPES_fr.md) ([en](docs/USER_RECIPES.md)).

## Développement

Compilation, tests et paquets : [docs/BUILDING_fr.md](docs/BUILDING_fr.md) ([en](docs/BUILDING.md)).
Systèmes sur lesquels le programme a réellement été testé : [docs/TESTED_PLATFORMS_fr.md](docs/TESTED_PLATFORMS_fr.md) ([en](docs/TESTED_PLATFORMS.md)).
Organisation du code et déroulé d'un renommage : [docs/ARCHITECTURE_fr.md](docs/ARCHITECTURE_fr.md) ([en](docs/ARCHITECTURE.md)).
Contribuer : [CONTRIBUTING_fr.md](CONTRIBUTING_fr.md) ([en](CONTRIBUTING.md)). Code de conduite : [CODE_OF_CONDUCT_fr.md](CODE_OF_CONDUCT_fr.md) ([en](CODE_OF_CONDUCT.md)). Signaler un problème de sécurité : [SECURITY_fr.md](SECURITY_fr.md) ([en](SECURITY.md)).
Traduire l'interface (les fichiers `.lng`) : [docs/TRANSLATING_fr.md](docs/TRANSLATING_fr.md) ([en](docs/TRANSLATING.md)).

## Réglages

Mode portable : créer un fichier `antrenamer-qt.ini` vide à côté du programme (ou cocher la case dans
Options > Langue). Sinon les réglages sont dans `%APPDATA%\AntRenamerQt\Ant Renamer Qt\antrenamer-qt.ini` (Windows) ou `~/.local/share/AntRenamerQt/Ant Renamer Qt/antrenamer-qt.ini` (Linux).

Menu contextuel de l'Explorateur (Windows) : Options > Fichiers & Dossiers ajoute « Ajouter à Ant Renamer Qt » aux fichiers et
dossiers (Windows 11 : « Afficher plus d'options »). Tout ce qui est sélectionné arrive dans la fenêtre déjà ouverte.

Ligne de commande : `-b fichier.arb` charge un job, `-g` démarre, `-x` quitte à la fin, `-a[fFr]`
choisit ce qu'ajoutent les chemins suivants (f fichiers, F dossiers, r récursif ; jokers acceptés).

## Confidentialité

Le programme n'utilise pas le réseau, sauf pour la recherche de nouvelle version, **désactivée par défaut** (Options > Affichage) : si vous l'activez, il interroge `api.github.com` une fois par jour pour obtenir la liste des versions de ce projet, et rien d'autre n'est envoyé. Si GitHub est inaccessible, il ne se passe rien et le programme fonctionne comme d'habitude.

## Assistance

L'assistance pour Ant Renamer Qt se fait sur GitHub : [Issues](https://github.com/morfredus/ant-renamer-qt/issues) (bugs, demandes de fonctions) et [Discussions](https://github.com/morfredus/ant-renamer-qt/discussions) (questions, idées). Merci de ne pas signaler les problèmes d'Ant Renamer Qt à l'auteur du programme d'origine.

## Crédits

- Programme d'origine : Antoine Potten, [Ant Renamer](https://www.antp.be/software/renamer/) (Delphi 7, GPL v2 ou ultérieure).
- Portage C++/Qt 6 et portage Linux : Frédéric Biron ([morfredus](https://github.com/morfredus)).

## Licence

GPL v2 ou ultérieure, comme le programme d'origine.
