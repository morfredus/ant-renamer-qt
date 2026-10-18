# Ant Renamer Qt : un portage d'Ant Renamer 2.13

*Présentation de ce portage C++/Qt 6 d'Ant Renamer 2.13, le programme d'Antoine Potten.*
Version anglaise : [PRESENTATION.md](PRESENTATION.md). Guide d'utilisation : [USER_GUIDE_fr.md](USER_GUIDE_fr.md), recettes : [USER_RECIPES_fr.md](USER_RECIPES_fr.md).

## À propos de ce document

Ant Renamer 2 est un programme Delphi 7, dont la dernière version est la 2.13 (décembre 2024).
L'auteur du portage (Frédéric Biron, morfredus) voulait continuer à l'utiliser sur les Windows récents et disposer d'une version qui se
compile avec les outils libres actuels. Il l'a donc porté en C++ avec Qt 6. Le portage est publié sous la même
licence que les sources d'origine (GPL v2 ou ultérieure) et réutilise plusieurs des fichiers d'origine tels quels. Ce
document liste ce qui a été fait, conservé, modifié et pourquoi, pour que chacun puisse juger le travail.

Le portage a été réalisé à partir des sources
publiées par Antoine Potten (`antrenamer2_sources.zip`).

Versions, journal des modifications et code source : [github.com/morfredus/ant-renamer-qt](https://github.com/morfredus/ant-renamer-qt).

## Nom et coexistence

En accord avec l'auteur du programme d'origine, le nom officiel est **Ant Renamer Qt**. Le portage est développé en parallèle du programme
d'origine, donc rien n'est partagé à l'exécution : exécutable propre (`antrenamer-qt`), dossier d'installation,
groupe du menu Démarrer, fichier de réglages (`antrenamer-qt.ini`), entrée du menu de l'Explorateur et noms de
paquets. Seul le format des fichiers de job (`.arb`) est commun, volontairement : les deux programmes lisent les
jobs de l'autre.

## En bref

| | Original (Delphi 7) | Portage (C++20 / Qt 6) |
|---|---|---|
| Taille | environ 14 700 lignes de Pascal | environ 2 400 lignes (moteur) + 4 400 lignes (interface) + 600 lignes de tests |
| Prérequis | Delphi 7 + 5 paquets de composants tiers | un compilateur C++20 et Qt 6, rien d'autre |
| Plateformes | Windows | Windows (MinGW, testé), Linux x86_64 et ARM64 (Raspberry Pi ; installé depuis les paquets et utilisé, testé) |
| Unicode | WideString + contrôles Tnt | `QString` (UTF-16), mêmes positions de caractères |
| Distribution | installeur Inno Setup | zip portable + installeur Inno Setup 6 (Windows), `.deb` et AppImage (Linux) |

## Ce qui a été conservé (compatibilité)

- **Les 14 actions de renommage**, avec mêmes noms, ordre, réglages, valeurs par défaut et descriptions :
  changer l'extension, remplacement de chaîne, remplacement multiple, insertion, déplacement, suppression de
  caractères, énumération, tags MP3, date et heure, noms aléatoires, changement de casse, noms depuis une liste,
  expression régulière, EXIF.
- **Le langage des masques** : `%name%`, `%ext%`, `%folderN%` (niveaux positifs et négatifs), `%count*%`, `%%`,
  guillemets et `$if(cond,alors,sinon)` avec `$$`, repris de `functions_strformat.pas` avec les mêmes règles.
- **Les fichiers de job** (`.arb`) : lus et écrits dans le même format XML, avec les mêmes noms d'éléments et
  d'attributs. Un job enregistré par la version 2 se charge dans le portage, et inversement.
- **La ligne de commande** : `-b`, `-g`, `-x`, `-a[fFr]` avec jokers, et la compatibilité avec la version 2.07.
- **La disposition de l'interface** : les trois pages (Fichiers / Opérations / Journal), les barres d'outils et
  leurs icônes, la liste des actions avec sa page de réglages, l'aperçu en direct du fichier sélectionné, le
  contenu du job repliable, le glisser-déposer avec sa fenêtre d'options, la fenêtre Options et ses onglets, le
  filtre du journal, la liste des jobs récents, la barre d'état avec progression, les mêmes raccourcis clavier.
- **Les fichiers de langue d'origine** : les 28 fichiers `.lng` (l'anglais et 27 traductions) sont utilisés tels quels (chacun avec son encodage), et la
  langue se change dans les Options. L'interface française est construite à partir du `French.lng` d'origine, plus quelques textes ajoutés par le portage.
- **L'aide d'origine** : les pages d'aide (anglais, français, croate, tamoul) et l'historique des versions sont
  livrés et ouverts depuis F1 et depuis les liens « aide » des notes des actions.
- **Les graphismes d'origine** : les jeux d'icônes de barres d'outils (Scrows, Windows XP, Windows XP Large), `renamer.ico`,
  `ant.bmp` et le set prédéfini « Remove west-european accents » de `default.xml`.
- **Le comportement** : renommage dans un fil séparé avec Stop et Annuler, renommage de dossiers avec mise à jour
  de la liste, « forcer les dossiers », détection des chemins absolus, « copier au lieu de renommer », règle de
  l'extension des dossiers, option d'heure d'été pour les dates de fichiers, sauvegarde du journal, événements au
  début et à la fin d'un renommage.

## Correspondance avec le code Delphi

| Original | Portage |
|---|---|
| `Actions.pas`, `Files.pas`, `thread.pas` | `core/ren_actions.*`, `core/ren_files.*`, `RenWorker` dans `app/mainwindow.*` |
| `functions_strformat.pas` | `core/ren_format.*` (avec un `FormatDateTime` compatible Delphi) |
| `ExtractID3.pas` | `core/ren_id3.*` |
| dEXIF / dIPTC | `core/ren_exif.*` (lecteurs minimaux maison : EXIF, IPTC, XMP) |
| JvSimpleXml | `QXmlStreamReader` / `QXmlStreamWriter` |
| TRegExpr | `QRegularExpression` (PCRE2) |
| Tnt Unicode Controls, `WideString` | `QString` |
| VirtualTreeView | `QTreeWidget` (listes plates, tri, réordonnancement par glisser) |
| Toolbar 2000 + TBX | `QToolBar` avec une feuille de style |
| `ProgramSettings.pas` (`Renamer.xml`) | fichier INI, voir « Mode portable » |
| `fMain`, `fOptions`, `fSelectDirectory`, `fDragdropOptions`, `fAbout` | `MainWindow`, `ActionPanel`, `OptionsDialog`, `AddFoldersDialog`, `DragDropDialog`, `AboutDialog` |

Le moteur (`core/`) ne dépend que de QtCore, ne contient aucun code d'interface et possède son propre programme de
test (`tests/test_core.cpp`, lancé par `ctest`).

## Différences avec l'original, volontaires

Ce sont les endroits où le portage ne fait pas exactement ce que fait la 2.13. Elles sont listées pour être explicites ; si l'une d'elles ressemble à une erreur, merci de la signaler dans les
[Issues](https://github.com/morfredus/ant-renamer-qt/issues).

1. **`$` isolé dans un masque `$if`** (`AdvancedFormat`) : l'original boucle indéfiniment sur un `$` qui n'est ni
   `$$` ni `$if(`. Le portage l'ignore et le supprime, comme les autres branches.
2. **Tags vides dans les masques MP3 / EXIF** : `ValidateFileName('')` renvoie le compteur de ticks Windows, donc un
   `%title%` vide devenait un nombre et `$if(%title%,...)` était toujours vrai, ce qui contredit l'aide. Le portage
   garde un tag vide vide, comme le décrit l'aide (`Vivaldi.mp3`, pas `Vivaldi - .mp3`).
3. **Énumération, « Restart »** : `SaveToFile` écrivait `Settings.Increment` dans l'attribut `Restart`. Le portage
   écrit la vraie valeur. Les anciens jobs qui contiennent la mauvaise valeur la chargent telle quelle.
4. **Masque MP3 par défaut** : `%author% - %title%%ext` (il manque le `%` final) devient `%author% - %title%%ext%`.
5. **Changements de casse seule** (`a.txt` vers `A.TXT`) : faits directement avec `MoveFileW`, car `QFile::rename`
   les refuse sur les volumes insensibles à la casse.
6. **Actions de métadonnées dans l'aperçu** : les actions date, MP3 et EXIF lisent le vrai fichier (celui qui est
   encore sur le disque) et non le nom produit par l'action précédente, de sorte qu'un aperçu après une autre action
   n'échoue plus avec « impossible d'obtenir la date du fichier ».
7. **Changement de casse** : sans l'option « locale », l'original utilise `UpperCase` de Delphi, qui ne traite que
   l'ASCII ; avec elle, la table Unicode complète. Le portage garde exactement ce partage, caractère par caractère
   pour que la longueur ne change jamais.
8. **Expressions régulières** : PCRE2 à la place de TRegExpr. Mêmes défauts que TRegExpr (insensible à la casse, `.`
   accepte les retours à la ligne) et même syntaxe de remplacement (`$1`, `${1}`, `$&`, `\n`, `\t`), mais des
   constructions exotiques de TRegExpr peuvent différer.
9. **Dates dans `FormatDateTime`** : réécriture du sous-ensemble documenté par l'aide (d, m, y, h, n, s, z, `ampm`,
   guillemets, « m veut dire minutes après h ou avant s »). Les noms de jours et de mois suivent la langue du système.
10. **Valeurs EXIF** : formatage simple (texte tel quel, rationnels en décimal, `ExposureTime` en `1/250`) ; dEXIF
    décode plus de balises et en formate certaines autrement. Seules 36 balises courantes sont connues.
11. **Réglages** : un fichier INI à la place de `Renamer.xml`. Les anciennes préférences ne sont pas importées.
12. **Barres d'outils** : les options « Color type » et « Office XP style » des thèmes TBX n'ont pas d'équivalent
    et ont été laissées de côté ; les états survol, enfoncé et coché sont dessinés par une feuille de style.
13. **Choix des dossiers** : pas de bouton « Réseau... » (l'arbre Qt montre ce que montre le système).
14. **Installeur** : il installe pour l'utilisateur courant par défaut et n'utilise pas `{commonappdata}` ; les
    réglages de l'utilisateur sont conservés à la désinstallation.

## Ce qui a été ajouté

- **ID3v2.2, 2.3 et 2.4** pour l'action MP3 (l'original ne lit que l'ID3v1), avec les encodages de texte usuels, la
  désynchronisation et les formes de genre `(17)` / `Rock` ; l'ID3v1 complète ce que la v2 ne donne pas.
- **Compteur `%num%` dans l'action EXIF** (départ, chiffres, incrément, retour au départ par dossier), mêmes réglages
  que l'action Énumération, enregistrés dans le fichier de job.
- **Menu contextuel de l'Explorateur** « Ajouter à Ant Renamer Qt » sur les fichiers et dossiers (Options, ou tâche de
  l'installeur). Les éléments sélectionnés ensemble arrivent dans la fenêtre déjà ouverte (instance unique via un
  socket local et un fichier verrou). C'est l'entrée du menu classique : sous Windows 11 elle se trouve dans
  « Afficher plus d'options ».
- **Mode portable** : un fichier `antrenamer-qt.ini` vide à côté du programme garde tout dans ce dossier ; sinon les
  réglages vont dans `%APPDATA%`. Une case déplace les réglages de l'un à l'autre.
- **Prêt pour Windows 10/11** : manifeste d'application avec chemins longs (plus de 260 caractères), page de code
  UTF-8, DPI par écran v2 et contrôles communs v6 ; Qt gère le thème sombre et la haute résolution.
- **Jeu d'icônes (nouveau jeu vectoriel « Modern », par défaut), police et « redimensionnement automatique des colonnes »** appliqués immédiatement, barres d'outils
  déplaçables et mémorisées, liste des langues avec version, auteurs et commentaires.
- **Système de build** : presets CMake pour Windows (MinGW), Linux x86_64, Linux ARM64 et compilation croisée
  ARM64 ; les DLL Qt et MinGW sont déployées automatiquement à côté de l'exécutable sous Windows.
- **Livrables** : `scripts/windows/package-win.ps1` produit un zip portable et, avec Inno Setup 6, un installeur
  (`installer/antrenamer-qt.iss`, anglais et français).
- **Options cachées** pour les scripts : `--install-shell` et `--remove-shell`.

## Qualité et tests

- Le moteur a un programme de test automatique : chemins, formatage des masques et `$if`, formatage des dates,
  aperçu de chaque action sur des noms d'exemple, vrais renommages sur disque (énumération, casse seule,
  annulation), aller-retour XML des jobs, ID3v2.3 avec repli sur la v1, réglages du compteur EXIF.
- À la main : des jobs avec remplacement de chaîne et EXIF + énumération ont été exécutés sur de vrais fichiers ;
  l'interface a été comparée page par page à des captures de la 2.13 ; l'installeur a été installé et retiré en
  mode silencieux ; le zip portable a été lancé avec un `PATH` minimal ; le menu « Ajouter à Ant Renamer Qt » a été
  cliqué dans l'Explorateur sur un ou plusieurs fichiers et sur un ou plusieurs dossiers (contenu compris) ; les
  paquets Linux ont été installés et le programme fonctionne.
- Pas encore couvert : très grosses listes, disques réseau, un vrai fichier `.arb` enregistré par la version
  Delphi, fichiers ID3v2.2/2.4, installation pour tous les utilisateurs
  (administrateur).

## Limites connues

- 36 balises EXIF (IFD0 et IFD Exif seulement, MakerNote non décodé) ; l'IPTC n'est lu que dans les
  JPEG ; les fichiers XMP associés sont lus et renommés avec leur image (IMG.xmp, IMG.CR2.xmp et `.dop` de DxO ; pas les
  autres types de fichiers associés).
- Seulement les tags ID3 ; pas d'autres formats de tags audio.
- Le menu court de Windows 11 demande un composant COM empaqueté et signé, non réalisé.
- L'exécutable n'est pas signé, donc SmartScreen peut avertir au premier téléchargement.
- L'interface est traduite par les fichiers `.lng` d'origine ; les textes ajoutés par le portage (À propos, mode portable, menu
  de l'Explorateur, thème) sont dans la section `<QtPort>` des fichiers de langue, traduits seulement en français pour l'instant.

## Licence et crédits

- GPL v2 ou ultérieure, comme l'original ; `LICENSE` est le `license.txt` d'origine.
- Le copyright d'origine est conservé dans la boîte À propos et dans l'installeur (Copyright 2000-2026 Antoine Potten - Frédéric Biron) ; la
  boîte À propos indique qu'il s'agit d'un portage C++/Qt et renvoie vers le site d'origine.
- Code tiers utilisé par le portage : Qt 6 (LGPL v3 / GPL), rien d'autre. Les composants Delphi de l'original ne
  sont plus nécessaires.
- Repris de l'original sans modification : `Languages/*.lng`, `help/` (images ; les pages ont été converties de HTML en Markdown et révisées pour le portage), bitmaps des barres
  d'outils, `renamer.ico`, `ant.bmp`, `default.xml`, textes des notes des actions. Les traducteurs sont cités dans
  l'onglet Crédits comme dans la boîte À propos d'origine.

## Retours et assistance

Questions, signalements de bogues et idées concernant Ant Renamer Qt : [Issues](https://github.com/morfredus/ant-renamer-qt/issues)
et [Discussions](https://github.com/morfredus/ant-renamer-qt/discussions). L'assistance du portage est séparée de celle du
programme d'origine : merci de ne pas signaler ses problèmes à l'auteur d'Ant Renamer.

Merci à Antoine Potten pour le programme et pour la publication de ses sources : sans elles ce portage n'existerait pas.

*Frédéric Biron (morfredus), octobre 2026*
