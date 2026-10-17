# Plateformes testées

Où Ant Renamer Qt a réellement été compilé et lancé. « Testé » veut dire qu'une personne a lancé le programme ou le paquet sur ce système ; une plateforme absente de la liste n'a pas été testée, même si elle devrait fonctionner. ([English version](TESTED_PLATFORMS.md).)

Si vous essayez un autre système, merci de donner le résultat dans les [discussions](https://github.com/morfredus/ant-renamer-qt/discussions) (ça marche ou non, avec la version et le système).

| Système | Architecture | Version | Ce qui a été testé |
|---|---|---|---|
| Debian GNU/Linux 13 (trixie), Raspberry Pi 4 Model B (deux machines) | arm64 | 3.0.0-rc.11, rc.12 | rc.11 : `.deb` installé depuis le paquet (dépendances résolues par apt), entrée de menu présente, programme fonctionnel, interface en français. rc.12 : `.deb` arm64 installé depuis le paquet, programme fonctionnel, noms de fichiers Linux, pages d'aide et liens, jeux d'icônes et lisibilité |
| Linux Mint 22.3 Xfce (base Ubuntu 24.04) | x86-64 | 3.0.0-rc.11, rc.12 | rc.11 : `.deb` installé depuis le paquet, programme fonctionnel. rc.12 : `.deb` installé depuis le paquet ; noms de fichiers Linux (`:`, `/`, renommage de dossier, `%folder-N%`, `%count%`), pages d'aide et liens, jeux d'icônes et lisibilité |
| Ubuntu 24.04 (WSL2 sous Windows 11) | x86-64 | 3.0.0-rc.10, rc.11 | compilation, tests unitaires, `.deb` et AppImage construits ; programme démarré, aide affichée ; liste de dépendances du `.deb` vérifiée (`ldd` : aucune bibliothèque manquante) |
| Windows 11 Famille | x86-64 | 3.0.0-rc.10, rc.12 | rc.10 : compilation MinGW, tests unitaires, installeur et zip portable. rc.12 : testé depuis le paquet Windows ; programme, pages d'aide et liens, jeux d'icônes et lisibilité, `%count%` |

## Pas encore testé

- Debian 13 en x86-64 (le correctif de dépendances de rc.11 est le même qu'en arm64, mais le paquet amd64 n'y a pas été installé).
- Toute autre distribution avec l'AppImage, Windows 10, macOS (non pris en charge).

Mettre ce fichier à jour à chaque release : version, système, ce qui a été vérifié.
