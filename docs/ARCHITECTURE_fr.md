# Architecture d'Ant Renamer Qt

Ce qu'il y a où, ce que fait chaque partie, et comment un renommage se déroule du clic jusqu'au disque. Pour compiler le projet voir [BUILDING_fr.md](BUILDING_fr.md) ; pour contribuer voir [CONTRIBUTING_fr.md](../CONTRIBUTING_fr.md). ([English version](ARCHITECTURE.md))

Le programme est écrit en C++20 avec Qt 6. C'est un portage d'Ant Renamer 2.13 (Delphi), et le code garde les noms de l'original : les commentaires disent « Port of ... » quand une classe en est la traduction directe.

Le code est coupé en deux :

- **`core/`** : le moteur de renommage, sans interface. Il ne dépend que de QtCore et est compilé en bibliothèque statique, `rencore`.
- **`app/`** : l'interface (fenêtres, panneaux, aide, langues). Elle utilise le moteur et est compilée en exécutable, `antrenamer-qt`.

## Arborescence

```
core/             Moteur (bibliothèque statique rencore)
app/              Interface (exécutable antrenamer-qt)
  Languages/      Fichiers de langue de l'original (.lng, lus à l'exécution)
  help/           Aide Markdown en 4 langues, table des matières, images
tests/            test_core (moteur ; inclut id3_check.h, robust_check.h, update_check.h), test_panel (éditeur d'actions, chargeur de langues)
cmake/toolchains/ Toolchain de compilation croisée ARM64
scripts/          windows/ (compilation MinGW, zip et installeur) et linux/ (.deb, AppImage, .desktop)
installer/        Script Inno Setup de l'installeur Windows
packaging/        flatpak/ (manifeste Flathub, fichier AppStream, icône)
docs/             Guides, présentation, notes de version
.github/          Workflow d'intégration continue, Dependabot, modèles de tickets
CMakeLists.txt, CMakePresets.json, VERSION   Fichiers de compilation et version, source unique
.clang-format, .editorconfig   Style du code (voir CONTRIBUTING_fr.md)
LICENSE           Licence publique générale GNU
```

| Dans `app/` | Compilé ? |
|---|---|
| `*.cpp`, `*.h` | Oui |
| `resources.qrc` : `renamer.ico`, les trois jeux de bitmaps de la barre d'outils (le quatrième, « Modern », est dessiné dans le code), `antrenamer-qt.png` (icône de la boîte À propos), `default.xml` | Embarqués dans l'exécutable par `rcc` de Qt |
| `app.rc.in`, `app.manifest` (icône, chemins longs, UTF-8, DPI par écran, informations de version) | Windows seulement ; CMake génère `app.rc` depuis `app.rc.in` avec la version du fichier `VERSION`, compilé avec `windres` |
| `Languages/`, `help/` | Non compilés : copiés à côté de l'exécutable (et installés par les paquets) |
| `antrenamer-qt.svg`, `antrenamer-qt.png` (256 px) | Non compilés : original de l'icône et son PNG (le PNG et `renamer.ico` sont tirés du SVG) |
| `icons/hicolor/` | Non compilé : le SVG et les PNG de 16, 24, 32, 48, 64, 128 et 256 px dans l'arborescence du thème d'icônes freedesktop, installés par le paquet `.deb` dans `/usr/share/icons/hicolor` (tirés du SVG avec `rsvg-convert -w <taille> -h <taille>`) |

## Vocabulaire

| Terme | Dans le code | Sens |
|---|---|---|
| Fichier | `ren::File` | Une entrée de la liste : dossier, nom, état, aperçu, texte du journal |
| Liste de fichiers | `ren::FileList` | Les fichiers à renommer, dans l'ordre de l'écran |
| Opération (action) | `ren::Action` et ses 14 sous-classes | Une règle de renommage avec ses réglages |
| Job (lot, *batch*) | `ren::ActionList` | La suite ordonnée d'opérations, enregistrée dans un fichier XML `.arb` |
| Masque | chaîne comme `%IptcDateCreated%_%num%%ext%` | Modèle de nom dont les `%tags%` sont remplacés par leurs valeurs |
| Aperçu | `Op::Preview`, `File::preview` | Le nom calculé, sans toucher au disque |
| Fichier associé (sidecar) | `.xmp`, `.dop`, `.pp3`, `.cos`, `.acr`, `.on1`, `.thm`, `.aae` et les fichiers RAW (`rawExtensions()`) | Fichier qui suit son image ; un RAW n'emporte pas un autre RAW |

## Le moteur (`core/`)

| Fichier | Rôle |
|---|---|
| `ren_actions.h/.cpp` | Les 14 opérations, chargement et sauvegarde des jobs, boucle d'exécution (`Action::perform`) |
| `ren_files.h/.cpp` | `File` (renommer, annuler, fichiers associés), `FileList`, `Journal` (passes, annulation, enregistré en JSON par l'application), `Options` |
| `ren_format.h/.cpp` | Le langage des masques et le format de date |
| `ren_exif.h/.cpp` | Lecteur EXIF, IPTC et XMP, noms des fichiers associés |
| `ren_id3.h/.cpp` | Lecteur ID3 (MP3) |
| `ren_path.h` | Fonctions de chemins à la Delphi (en-tête seul) |
| `ren_update.h/.cpp` | Recherche de nouvelle version, partie sans réseau : comparaison des versions et lecture de la liste des releases GitHub |

**Chemins.** Les fonctions de `ren_path.h` suivent les règles du système : sous Windows `\`, `/` et `:` sont des séparateurs (`isDelim`) et `\` est celui que les fonctions ajoutent (`kSep`) ; sous Linux seul `/` en est un, `\` et `:` sont des caractères ordinaires d'un nom. Sous Windows les chemins peuvent donc mélanger les séparateurs : il faut les normaliser (`QDir::fromNativeSeparators`) avant de comparer deux chemins.

**`File`.** `rename(nouveauNom)` calcule le chemin cible, refuse un nom identique, crée les dossiers manquants si demandé, puis déplace le fichier. Sous Windows, il utilise `MoveFileW` et pas `QFile::rename`, car ce dernier refuse de changer seulement la casse (`a.txt` vers `A.txt`) sur un disque insensible à la casse et l'opération Casse en a besoin. `revert()` remet l'ancien nom. Après un renommage réussi, `moveSidecars` déplace les fichiers associés de l'ancien nom (`sidecarCandidates()` : `IMG.xmp`, `IMG.XMP`, `IMG.CR2.xmp`..., idem pour `.dop`) pour qu'ils correspondent au nouveau. Un problème sur un sidecar n'échoue jamais l'image : il est seulement ajouté au texte du journal. `FileList::add` branche deux rappels sur chaque fichier : quand un **dossier** est renommé, les fichiers dessous le suivent ; quand un sidecar qui est aussi dans la liste est déplacé par son image, son entrée suit (`followPath`).

**Le langage des masques** (`advancedFormat`). `%tag%` appelle un *getter*, une fonction fournie par l'appelant qui renvoie la valeur du tag ; un tag que le getter refuse reste en texte. `%%` donne `%`. Avec `AllowQuotes`, `"..."` protège un texte ; avec `AllowIf`, `$if(cond,alors,sinon)` prend `alors` quand `cond` se développe en texte non vide, et `$$` donne `$`. `formatDateTime` réimplémente le `FormatDateTime` de Delphi (`d`, `m`, `yy`, `h`, `n` pour les minutes, `s`, `z`, `am/pm`, `a/p`, `ampm`...) ; `m` est une minute après une heure ou avant des secondes, sinon un mois, comme en Delphi.

**Les opérations.** `Action` est la classe de base. Une sous-classe redéfinit :
- `makeNewName(cheminComplet, estDossier)` : le cœur, renvoie le nouveau nom (sans dossier). N'écrit pas sur le disque.
- `description()` : la phrase affichée dans la liste du job et dans le journal.
- `save()` / `load()` : les réglages en XML.
- `getTag()` : la valeur d'un `%tag%` (la base connaît `name`, `ext`, `folder`, `folderN`, `count...` ; les sous-classes ajoutent `num`, `random`, les tags ID3 et EXIF).
- `init()` / `finish()` : appelés avant et après le passage sur tous les fichiers.

Les réglages sont de simples champs publics que l'interface lit et écrit directement.

| Valeur | Classe | Balise XML | Principe |
|---|---|---|---|
| 0 | `ActionChangeExt` | `ChangeExt` | Remplace, supprime ou ajoute l'extension |
| 1 | `ActionStringRepl` | `StrRepl` | Remplace un texte (tout ou première occurrence, casse, extension, jokers) |
| 2 | `ActionMultstrRepl` | `MultstrRepl` | Table « de vers », une paire par ligne séparée par une tabulation |
| 3 | `ActionStringIns` | `StrIns` | Insère un texte à une position |
| 4 | `ActionMoveStr` | `MoveStr` | Déplace N caractères à une autre position |
| 5 | `ActionCharDel` | `CharDel` | Supprime des caractères à une position ou autour d'une chaîne |
| 6 | `ActionEnum` | `Enum` | Numérotation avec masque, retour au départ par dossier en option et numéro partagé JPG + RAW |
| 7 | `ActionMP3` | `MP3` | Noms à partir des tags ID3 |
| 8 | `ActionDateTime` | `DateTime` | Noms à partir de la date de création ou de modification |
| 9 | `ActionRandom` | `Random` | Noms aléatoires (nombres, compteur de ticks ou GUID) |
| 10 | `ActionCase` | `ChangeCase` | Change la casse |
| 11 | `ActionFromList` | `FromList` | Un nom par ligne d'une liste |
| 12 | `ActionRegexp` | `Regexp` | Expression régulière (insensible à la casse, `.` accepte les retours à la ligne, comme TRegExpr) |
| 13 | `ActionExif` | `Exif` | Noms à partir des tags EXIF, IPTC et XMP, avec un compteur `%num%` |

Les valeurs sont celles de l'original et sont les numéros de ligne de la liste des opérations à l'écran (`ActionType`).

`plainFolder()` est vrai pour un dossier quand l'option « les dossiers ont une extension » est décochée : le nom entier est alors traité comme un nom sans extension, c'est pourquoi la plupart des `makeNewName` ont une branche pour ce cas.

**Exécution.** `Action::perform(liste, op, progress)` boucle sur **tous les fichiers pour une opération** :
1. `init()` (pas pour une annulation).
2. Pour chaque fichier, selon `op` : `Rename` vérifie que le fichier existe encore, appelle `makeNewName` puis `File::rename` (un sidecar dont l'image est aussi dans la liste est sauté : son image le renomme) ; `Preview` ne touche pas au disque et part de l'aperçu laissé par l'opération précédente ; `Revert` appelle `File::revert()` (gardé pour les tests) ; `Undo` est traité par `FileList::undoPasses`.
3. Appelle `progress(fichier, positionDeLOperation, indexDuFichier)`.
4. `finish()`.

Une opération peut signaler un problème sans échouer (`setInfo`) : le fichier garde son nom et le journal affiche un message comme « No EXIF info found ».

`ActionList::perform` fait passer **chaque opération, l'une après l'autre, sur tous les fichiers**. En mode `Rename`, chaque opération renomme réellement les fichiers sur le disque et la suivante repart des nouveaux noms : ce n'est pas un seul renommage à la fin. `Op::Revert` appelle `File::revert()` sur chaque fichier, un seul niveau (l'ancien nom est gardé dans chaque `File`). L'interface ne l'utilise plus : chaque opération du job est une **passe** du `Journal` (`beginPass` / `record` / `endPass` ; une passe sans déplacement est abandonnée, 50 passes au plus), alimentée par le rappel `onMoved` de `File::rename`. `Op::Undo` appelle `FileList::undoPasses(n)`, qui annule les `n` dernières passes déplacement par déplacement, la plus récente d'abord (`File::undoMove`, sur l'entrée de la liste ou sur une entrée temporaire quand le fichier a quitté la liste). Elle s'arrête au premier déplacement qui échoue : la passe garde les déplacements restants. Les copies ne sont pas enregistrées.

**Fichier de job (`.arb`).** XML écrit avec `QXmlStreamWriter` :

```xml
<AntRenamer Version="3.1.1" Date="2026-11-13">
 <Batch>
  <StrRepl Search="old" Repl="new" AllOccurences="True" CaseSensitive="True" .../>
  <Enum Mask="File %num%%ext%" Start="1" Digits="3" Incr="1" Restart="False" SharePairs="False"/>
 </Batch>
</AntRenamer>
```

Les réglages sont des attributs (`True` / `False`), les textes longs (table de remplacement, liste de noms) sont des enfants en CDATA. À la lecture, chaque élément est reconnu par le nom de sa balise (tableau `kNames`), l'action est créée avec `ActionList::create` et son `load()` lit les attributs ; un attribut absent prend sa valeur par défaut, donc les anciens jobs se lisent encore, et les attributs inconnus sont ignorés.

**EXIF, IPTC, XMP** (`readExif`). Remplit une table nom vers valeur (noms insensibles à la casse) : d'abord le XMP associé au fichier (il l'emporte sur les données intégrées), puis les 512 premiers Ko du fichier lui-même. JPEG : le segment APP1 « Exif » donne le bloc TIFF, le segment APP13 « Photoshop 3.0 » donne l'IPTC. TIFF et DNG : les IFD sont lus directement. Le XMP est trouvé par son contenu (`<x:xmpmeta` ... `</x:xmpmeta>`), ce qui couvre JPEG, PNG, WebP, TIFF, DNG et les fichiers `.xmp`. Les tables `tagNames()`, `iptcNames()` et `xmpProps()` listent les tags connus, et `sidecarExtensions()` les extensions de fichiers associés (`xmp`, `dop`) : c'est le seul endroit à modifier pour en ajouter une.

**ID3.** `readId3` lit l'ID3v2.2, 2.3 et 2.4 (trames Latin-1, UTF-16 et UTF-8) et le complète avec l'ID3v1/1.1.

## L'interface (`app/`)

| Fichier | Rôle |
|---|---|
| `main.cpp` | Point d'entrée : instance unique, langue, `--install-shell`, fenêtre principale |
| `mainwindow.h`, `mainwindow.cpp`, `mainwindow_files.cpp`, `mainwindow_batch.cpp`, `mainwindow_run.cpp`, `mainwindow_internal.h` | Fenêtre principale, une seule classe découpée par sujet : construction et réglages / liste des fichiers / lot / lancement, journal et ligne de commande ; le `.h` partagé par les quatre `.cpp` contient leurs fonctions d'aide, dont `droppedPaths()` qui transforme les formats de dépôt du portail en chemins lisibles par D-Bus (Qt6::DBus, Linux seulement) |
| `actionpanel.h/.cpp` | La page « Opérations » : les 14 types et leurs réglages |
| `dialogs.h/.cpp` | Ajouter des dossiers, glisser-déposer, Options, À propos |
| `appsettings.h/.cpp` | Options du programme dans un fichier INI ; emplacement du fichier d'historique d'annulation (`historyFile()`) |
| `lngtranslator.h/.cpp` | Traduction avec les fichiers `.lng` de l'original |
| `helpwindow.h/.cpp` | Visionneuse d'aide intégrée (Markdown) |
| `icons.h/.cpp` | Icônes de la barre d'outils découpées dans les jeux de bitmaps |
| `devaids.h/.cpp` | Aides de développement pilotées par les variables d'environnement `ANTREN_*` (captures d'écran, textes non traduits) ; appelées depuis `main` |
| `shellmenu.h/.cpp` | Entrée du menu contextuel de l'Explorateur Windows (registre, Windows seulement) |
| `theme.h/.cpp` | Thème de couleurs (système, clair, sombre) |
| `updatecheck.h/.cpp` | `UpdateChecker` : demande à GitHub la liste des releases (Qt Network), facultatif |

**Démarrage** (`main.cpp`). Nomme l'application (« Ant Renamer Qt », organisation `AntRenamerQt` : c'est ce qui détermine l'emplacement du fichier INI) et donne la version (`AR_APP_VERSION`, fournie par CMake). Instance unique : un fichier verrou dans le dossier temporaire ; une seconde instance lancée avec `--shell` envoie ses chemins à la première par un socket local (Qt Network est lié pour cela et pour la recherche de mise à jour, facultative) et quitte. Une seconde instance qui ne reçoit que des chemins (aucune option : un fichier déposé sur le programme, « Ouvrir avec ») les transmet de la même façon à la première. Toute autre seconde instance (ni `--shell` ni `-q`) affiche une boîte d'information et quitte ; lancée avec `-g` ou `-x`, elle écrit le message sur la sortie d'erreur et quitte avec le code 3, puisque personne ne pourrait cliquer. Puis la langue, `--install-shell` / `--remove-shell` si demandé, et la fenêtre principale. Avec `-q` (mode silencieux) la fenêtre n'est jamais affichée, ni le verrou d'instance ni le serveur de socket ne sont pris, et le code de sortie est décidé par `MainWindow::quitSilently()` (0 fait, 1 un fichier a échoué, 2 rien n'a été fait) ; chaque boîte de dialogue modale du chemin d'exécution a une branche silencieuse, puisque personne ne pourrait y répondre.

**Traduction.** Le programme n'utilise pas les fichiers `.ts` de Qt mais les fichiers `.lng` de l'original (XML, encodages variés lus dans leur en-tête). Le code contient les textes en anglais (`tr_("Renaming process started")`). `loadLanguage` lit `English.lng` et la langue choisie, aplatit les deux en tables (clés comme `MainForm/ActionAddFiles/Caption`) et, pour chaque clé présente dans les deux, associe le texte anglais à sa traduction (les paires vides ou identiques sont ignorées ; un `Hint` de la forme « court|long » est enregistré comme deux textes). La clé de la table est le texte anglais normalisé : `%s`, `%d` (Delphi) et `%1`...`%9` (Qt) deviennent tous `%@`. Donc un texte anglais du code doit être **identique** à une entrée de `English.lng` pour être traduit ; sinon il reste en anglais. `ANTREN_MISSING=1` liste les textes sans traduction.

**Réglages.** `appSettings()` est un `QSettings` au format INI : à côté de l'exécutable si `antrenamer-qt.ini` existe (portable), sinon dans le profil utilisateur. `AppOptions` contient toutes les options de la boîte Options avec les valeurs par défaut de l'original (une exception : au premier lancement, sans fichier de réglages ou avec un fichier vide, `load()` choisit la disposition moderne en bandes et écrit aussitôt `options/newUi` et `options/pageLayout` ; qui a déjà des réglages garde la disposition classique s'il n'a jamais choisi) ; une macro `KEYS(...)` liste chaque champ une seule fois pour générer `load()` et `save()` (clés `options/nom`), et `applyToCore()` copie les options de traitement vers `ren::options()`. Autres clés : `window/*` (géométrie, colonnes, séparateurs, par disposition), `last/*` (dernières valeurs des réglages d'opérations), `history/*`, `mru/batch`, `view/changedOnly`, `log/*`.

**Fenêtre principale.** Trois pages : Fichiers, Opérations, Journal. Selon une option : onglets (classique) ou disposition moderne en un seul écran (bandes ou colonnes). `files_` (`FileList`) et `batch_` (`ActionList`) sont les vraies données ; les vues (`fileView_`, `batchView_`, des `QTreeWidget` qui acceptent le glisser-déposer) n'en sont qu'un miroir : chaque ligne porte l'`id()` de son `File` ou de son `Action` (jamais un pointeur : `FileList::byId` et `ActionList::byId` renvoient `nullptr` pour un objet retiré), et `updateItem` recopie l'état d'un fichier dans les colonnes (et masque la ligne quand « Seulement modifiés » est actif et que l'aperçu égale le nom).

**Le panneau d'opérations** (`ActionPanel`) possède **une** action en cours de configuration. La liste de gauche contient les 14 types (l'index de la ligne **est** la valeur de `ActionType`), avec une page de contrôles par type. Chaque contrôle est créé par une aide (`combo`, `spin`, `check`, `radios`) qui l'enregistre avec une clé, un lecteur et un écrivain, ce qui sert à sauvegarder et restaurer les dernières valeurs. Chaque contrôle est aussi relié une seule fois, là où il est créé, à un membre de son action par `bind(contrôle, &Action::membre)` (`bindFn` pour les rares qui demandent une conversion) ; `makeAction()` et `setFromAction()` parcourent simplement la liste du type sélectionné, la première lisant les contrôles pour construire l'`Action`, la seconde chargeant une action existante dedans (pour modifier une action du job). `test_panel` vérifie qu'elles sont réciproques pour chaque champ et chaque type : **ajouter un champ à une action sans `bind` le fait échouer**.

**Aperçu en direct.** Le panneau émet `changed()` à chaque modification ; la fenêtre prend le fichier sélectionné (ou le premier), le copie dans une `FileList` temporaire, exécute `makeAction(false)` en mode `Preview` et affiche « Nom actuel / Nouveau nom ». Le bouton Aperçu fait de même sur toute la liste et remplit la colonne Aperçu.

**Lancer un renommage** (`MainWindow::run(op)`).
1. Vérifie que rien ne tourne déjà (`worker_`), qu'il y a des fichiers et une action.
2. Journalise, numérote fichiers et actions. Avec un job vide, l'action configurée à l'écran est exécutée puis oubliée.
3. Crée un **`RenWorker`** (un `QThread`) et le démarre : le travail se fait **hors du fil de l'interface**, la fenêtre reste réactive et Arrêter fonctionne.
4. Le worker appelle `ActionList::perform` et, après chaque fichier, émet `progress(Snapshot)`. Un `Snapshot` est une **copie** (id du `File`, description, état, positions) : l'interface ne lit jamais un `File` que l'autre fil est en train de modifier. Il est transmis par une connexion en file d'attente.
5. `onProgress` met à jour la ligne (option « temps réel »), le journal, la barre de progression.
6. `onFinished` vide les signaux en attente, journalise, applique les options de fin de traitement (changer de page, vider la liste ou le job, enregistrer le journal dans un fichier avec BOM UTF-8), réactive les boutons et quitte si `-x` a été donné.

Pendant un renommage (`worker_` non nul), l'ajout, le déplacement et l'ouverture d'un job sont ignorés, et la fenêtre refuse de se fermer.

**Journal.** `addLog(texte, état)` garde la ligne en mémoire et l'affiche sauf si son état est filtré. Couleurs : bleu (non renommé), vert (OK), rouge (erreur), gris (information). Les filtres du journal reconstruisent l'affichage depuis les lignes gardées.

**Annulation et historique.** `ren::Journal` (`core/ren_files.h`) garde les passes : une `Pass` est un passage d'une opération du job (numéro, date, résumé, les `Move` et la `session`), un `Move` est un déplacement réel (`from`, `to`, les fichiers associés déplacés avec lui, et la taille et l'heure de modification du fichier en `to` juste après le déplacement ; pas pour les dossiers). `File::onMoved` enregistre les déplacements tant que `ActionList::perform` a une passe ouverte (`beginPass` / `endPass`). `FileList::undoPasses(count, progress, session)` annule les `count` dernières passes d'une session, la plus récente d'abord, chacune de son dernier déplacement au premier ; avant d'annuler un déplacement, elle refuse un fichier disparu ou dont la taille ou l'heure diffèrent (`changedSince`), et s'arrête à la première passe qu'elle ne peut pas terminer (les déplacements en échec restent dans le journal). Une session est un lancement du programme : `Journal::fromJson` donne au nouveau lancement le numéro qui suit le dernier enregistré, de sorte que les passes d'une session plus ancienne s'annulent sans toucher aux suivantes. Le journal vit en mémoire dans le cœur ; l'interface l'enregistre (`MainWindow::saveHistory`, après chaque exécution) et le relit au démarrage, en JSON (`Journal::toJson` / `fromJson`) dans `antrenamer-qt-history.json` à côté du fichier de réglages (`historyFile()` dans `appsettings.cpp` ; `setPortable` le déplace avec les réglages). 50 passes sont gardées. La fenêtre d'historique regroupe les passes par session et a un bouton *Effacer l'historique*.

**Autres fichiers.** `dialogs.*` : `AddFoldersDialog` (arbre de dossiers avec cases fichiers / dossiers / récursif ; dans un Flatpak, une liste alimentée par le sélecteur de dossier du portail, l'arbre ne voyant pas l'hôte), `DragDropDialog` (que faire d'un dépôt), `OptionsDialog`, `AboutDialog` ; `AddChoice` est la petite structure partagée « fichiers / dossiers / récursif ». `helpwindow.*` : visionneuse à deux volets (table des matières `toc_<langue>.md`, une liste imbriquée de liens, dans un `QTreeWidget`, page dans un `QTextBrowser`, boutons précédent et suivant) qui lit `help/<page>_<langue>.md` (un titre écrit `## Titre {#id}` reçoit l'ancre `id`, donc les liens `page.md#id` fonctionnent) ; la langue d'aide est `en`, `fr`, `hr` ou `ta`, anglais sinon. `icons.*` : charge un des trois jeux de bitmaps (icônes carrées côte à côte) et découpe l'icône n°`index`, ou, pour le quatrième jeu (« Modern »), renvoie un `QIconEngine` qui dessine l'icône avec `QPainter` sur une grille de 24 x 24 (toute taille, couleurs du thème, aspect grisé ; aucun fichier). `shellmenu.*` : écrit ou supprime `HKCU\Software\Classes\{*,Directory}\shell\AntRenamerQt` avec la commande `antrenamer-qt.exe --shell "%1"`.

## Un renommage de bout en bout

Job : une opération *Énumération* avec le masque `Photo %num%%ext%` ; fichiers `a.jpg` et `a.xmp` ; clic sur **Démarrer**.

1. **Démarrer** appelle `MainWindow::run(Op::Rename)` ; le worker démarre et `ActionList::perform` lance l'opération.
2. `init()` met le compteur à sa valeur de départ.
3. Pour `a.jpg` : ce n'est pas un sidecar de la liste. `makeNewName` appelle `advancedFormat("Photo %num%%ext%", ...)` ; le getter renvoie `num` = `001` et `ext` = `.jpg`, ce qui donne `Photo 001.jpg`.
4. `File::rename("Photo 001.jpg")` déplace le fichier. Le texte du journal devient « ... renamed to Photo 001.jpg ».
5. Avec l'option sidecar activée, `moveSidecars` calcule les candidats de `a.jpg` (`a.xmp`, `a.XMP`, `a.jpg.xmp`, `a.jpg.XMP`, idem pour les autres extensions) et de `Photo 001.jpg`, déplace ceux qui existent, et ajoute « (sidecar file ... renamed to ...) » au texte du journal. La ligne `a.xmp` de la liste suit (`followPath`).
6. Pour `a.xmp` dans la liste : son image `a.jpg` est dans la liste, il est donc sauté.
7. Le compteur avance, le worker émet `progress`, l'interface met à jour la ligne et le journal.
8. `onFinished` applique les options de fin de traitement.
