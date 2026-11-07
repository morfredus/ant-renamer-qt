# TODO

## Idées pour après la 3.0.0

English version: [TODO.md](TODO.md).

### P0 : qualité du code, juste après la 3.1.0 finale (le code est figé pour elle)

Issu d'une revue technique vérifiée dans le code (06/11/2026). Rien à toucher avant la sortie de la 3.1.0 :
- **Badge CI** en tête de `README.md` / `README_fr.md` (le workflow Build existe déjà, seul le badge manque).
- **clang-tidy** (et/ou `-Werror`) pour que « aucun avertissement du compilateur » soit imposé par la CI et pas seulement vérifié à la main : un fichier `.clang-tidy`, un preset CMake, une étape de CI. Aujourd'hui seulement `-Wall -Wextra -Wshadow`.
- Facultatif, peu utile : `std::span` pour `exportPreview`, `qScopeGuard` pour `LocalFree` dans `lastSysError()`, règle de nommage (`readability-identifier-naming`) une fois clang-tidy en place.
- Vérifié et à ne pas faire : `File*` dans `FileList` (les adresses sont stables, le vecteur contient des `unique_ptr` ; le seul risque est un pointeur gardé après `remove()`), `moveFile(..., QString& err)` (3 appelants, tous testent le résultat).

### P1 : annuler après un redémarrage (fait, voir le CHANGELOG, Unreleased)

Demandé par un utilisateur du forum (qui craint d'abîmer ses archives après une faute de frappe ou un changement d'avis). Aujourd'hui l'historique d'annulation est gardé en mémoire (`ren::Journal`, `core/ren_files.h`) et il est perdu à la fermeture du programme. L'idée : l'écrire sur disque (JSON dans le dossier de données) à la fin de chaque passe, le recharger au démarrage, et garder la fenêtre d'historique d'une session à l'autre (une cinquantaine de passes). L'annulation reste linéaire, de la dernière passe vers la première, et les contrôles actuels restent (fichier toujours là, ancien nom libre ; arrêt au premier échec, le reste de la passe est conservé). Coût estimé : 100 à 150 lignes de code, plus les tests, les traductions (EN, FR, HR, TA) et la documentation. Risque technique faible.

Garde-fous pour les fichiers modifiés hors du programme entre deux sessions :
- Fichier renommé ou déplacé ailleurs : déjà géré (« fichier source introuvable », l'annulation s'arrête sur cette passe).
- Un autre fichier porte maintenant le nom (le seul vrai danger : on renommerait le mauvais fichier) : mémoriser la taille et la date de modification de chaque fichier dans chaque déplacement (un renommage garde les deux), les vérifier avant d'annuler, refuser avec un message clair si elles diffèrent. Décidé : pas de hash, pas même partiel. Il n'ajouterait que le cas très rare d'un remplacement de même taille et de même date, pour plus de code et plus de lecture des fichiers.
- Dossier renommé hors du programme : les chemins sont périmés, l'annulation s'arrête, sans deviner.

À trancher : après un redémarrage, proposer de reprendre seule la dernière session, ou seulement par le bouton Annuler et la fenêtre d'historique (le plus simple, à privilégier).

### P2 : une seule instance à la fois (idée, pas décidée)

Aujourd'hui seul le menu de l'Explorateur cherche une fenêtre ouverte (`--shell` lui passe les chemins, `app/main.cpp`) ; tout autre lancement ouvre une deuxième fenêtre. Deux fenêtres écrivent les mêmes fichiers : l'historique d'annulation (`antrenamer-qt-history.json` : la dernière qui enregistre efface les passes de l'autre, et deux fenêtres lancées ensemble ont le même numéro de session) et les réglages (`.ini` : la dernière fermée l'emporte). Un lancement `-q` lit et écrit aussi l'historique, sans verrou.

Deux façons, à trancher :
- **Simple (à privilégier)** : quand une autre instance tourne, la nouvelle affiche une boîte d'information (« Ant Renamer Qt est déjà lancé : une seule instance peut être ouverte à la fois ») et se ferme. Le même verrou pour la version installée et la version portable (personne n'utilise les deux sur un même poste, donc pas de verrou par dossier de réglages). `--shell` continue de passer ses chemins à la fenêtre ouverte, avant la vérification. Quelques lignes dans `main.cpp`, deux textes (EN, FR), la doc. `QLockFile` abandonne déjà le verrou d'un processus planté. À régler : un lancement `-g -x` sans `-q` ne doit pas attendre un clic dans un script sans surveillance (pas de boîte, un message sur la sortie d'erreur et un code de sortie, ou le laisser passer) ; `-q` reste hors de la règle.
- **Complète** : la nouvelle instance passe toute sa ligne de commande (chemins, job, `-g -x`) à la fenêtre ouverte, la met au premier plan et se ferme ; le verrou est lié au dossier des réglages (hash de son chemin). Plus de travail : une fenêtre occupée par une exécution doit refuser ou mettre la ligne de commande en attente.

Ce n'est pas suffisant seul (un plantage laisse l'historique écrit par le dernier lancement, ce qui convient), et `-q` pourrait encore écraser l'historique d'une fenêtre ouverte : accepté pour l'instant.

### Plan de la 3.1 (décidé le 02/11/2026)

Contenu de la 3.1.0-beta.4, à publier dans quelques jours (le travail reste en Unreleased sur `dev/3.1.0`, `main` reste à la 3.1.0-beta.3) : l'historique d'annulation conservé entre deux lancements (fait), la ligne de commande silencieuse (`-q`) et l'interface moderne par défaut. Le suffixe pour les noms en double de l'action EXIF passe en 3.2. Ensuite la 3.1.0-rc.1, une fois la beta.4 passée sans retour négatif (seulement des corrections pendant la rc). Décidé le 03/11/2026 : la beta.4 devient la 3.1.0-rc.1, code figé.

### Versions possibles

Ce sont des idées, pas des promesses : certaines ne seront peut-être jamais réalisées. Si elles le sont, un thème par version mineure, une à la fois : chacune est éprouvée par les utilisateurs (rc, puis au moins quelques semaines de version stable) avant de commencer la suivante. Rien n'est commencé avant la publication de la 3.0.0. Les versions sont triées par urgence : ce que les utilisateurs du programme d'origine ont demandé le plus souvent, depuis le plus longtemps, passe en premier (source : le forum « Comments » du programme d'origine, forum.antp.be, 120 sujets lus le 22/10/2026). L'ordre peut changer selon les retours.

| Version | Contenu |
|---|---|
| **3.1** | **Ligne de commande, barre finale** (`-afr d:\Data\`, fait, voir le CHANGELOG, 3.1.0-beta.1). **Recherche de mises à jour, préversions** (la case des options et le choix proposé par l'installeur sont faits, voir le CHANGELOG, 3.1.0-beta.1). **Interface moderne par défaut** : l'interface moderne, écran unique en bandes, devient celle d'une nouvelle installation (aujourd'hui la classique : `newUi` et `pageLayout` dans `appsettings.h`). L'installateur ne pose aucune question à ce sujet. Décidé le 02/11/2026 : une nouvelle installation (pas de fichier de réglages, ou un fichier vide) reçoit la disposition moderne, écran unique en bandes ; qui a déjà des réglages garde sa disposition (la classique s'il n'a jamais choisi) ; pas de message pour inviter les utilisateurs de la classique, la documentation le dit. (fait, voir le CHANGELOG, Unreleased) Pas pour la 3.0.0 : son code est celui de la 3.0.0-rc.16. **Ligne de commande silencieuse** (`-q`) : pas de fenêtre, pour les scripts appelés depuis un menu contextuel (demandé sur le forum). (fait, voir le CHANGELOG, Unreleased) **Aperçu exportable** : les noms actuels et nouveaux (ou seulement la liste des noms) écrits dans un fichier CSV ou texte, pour relire un gros lot ou garder une trace. (fait, voir le CHANGELOG, 3.1.0-beta.1) |
| **3.2** | **Suffixe pour les noms en double dans l'action EXIF** : deux photos prises la même seconde ont le même nom ; une balise à placer où l'on veut dans le masque (l'action Date et heure a déjà une option de suffixe). **Suffixe automatique pour les noms en double dans toutes les actions** (`_1`, `_2`... comme Capture One, demandé par un utilisateur sur le forum) : aujourd'hui un nom déjà pris est refusé et signalé au journal, rien n'est écrasé ; avec 1000 photos prises en rafale, les fichiers refusés sont à traiter à la main. **Dates des fichiers** : modifier les dates de création et de modification des fichiers (demandé de nombreuses fois, « sur la liste » de l'original depuis 2005), et l'inverse de l'action Date et heure : fixer la date d'après une date trouvée dans le nom. **Énumération par lettres** (`AA`, `AB`...). **« Copier au lieu de renommer » par action** du lot (aujourd'hui une option globale). **Annonce de mise à jour avec les nouveautés** : quand une nouvelle version est signalée, afficher sous son numéro ce qu'elle apporte (les notes de release ou les lignes du WHATS_NEW de cette version), et pas seulement le numéro. |
| **3.3** | **Préréglages pour toutes les actions** : nommer et enregistrer les réglages de n'importe quelle action (« Photos : date + compteur »), comme on peut déjà le faire pour la grille de l'action 3. **Nouvelles balises** : `%size%` et d'autres attributs de fichier ; sommes de contrôle (`%crc32%`, `%md5%`, `%sha1%`). **Corriger les noms mal encodés** (UTF-8 lu comme ISO-8859-1 après un passage Linux/Windows), sous forme d'option plutôt que d'une grille de remplacements. |
| **3.4** | **Audio** : tags FLAC, OGG, M4A (première limite du guide utilisateur ; commentaires Vorbis d'abord, M4A en dernier), et durée et débit des fichiers MP3. **Dossiers nommés d'après les tags MP3** qu'ils contiennent (« artiste - album »). |
| **3.5** | **Vidéo** (MP4, MKV) : la date de prise de vue, la durée, pour nommer les vidéos comme les photos. |
| **3.6** | **Métadonnées, plus complètes** : les tags GPS (latitude, longitude), plus de 36 tags EXIF, le MakerNote (objectif, numéro de série), IPTC des fichiers TIFF, les propriétés des fichiers PDF et documents Office (titre, auteur, année). |
| **Plus tard** | **Menu contextuel moderne de Windows 11**, à décider à la demande. |

### Petites idées, sans version

- Ajouter un dossier vide (ou sans fichier, avec « Ajouter les fichiers des dossiers sélectionnés » cochée) n'affiche rien et ne dit rien : un court message dans la barre d'état ou le journal (« aucun fichier trouvé dans ... ») montrerait à l'utilisateur que l'ajout a bien eu lieu. Rencontré par l'auteur le 2026-11-07 en testant la rc.3.- Licence : passer le portage de GPL v2 ou ultérieure à GPL v3 (permis, l'original dit « version 2 ou toute version ultérieure »). Aucun besoin technique aujourd'hui, donc pas avant la 3.1.0 finale ; à décider avec une vraie raison (une dépendance ou une contribution sous une licence incompatible avec la v2). Touche `LICENSE`, la boîte À propos (`app/dialogs.cpp`, `app/help/about_*.md`, les fichiers `.lng`), `README`, `CONTRIBUTING`, `docs/PRESENTATION`, le copyright du `.deb` (`scripts/linux/package-deb.sh`), le metainfo Flatpak (`GPL-3.0-or-later`) et le fichier de licence de l'installeur.

- Lire les données `.dop` ? Pas prévu : c'est un format propre à DxO.
- Propriétés XMP pas encore reprises : les attributs `exif:` d'un fichier associé (date, GPS), les réglages `crs:` (Camera Raw).
- Filtrer les fichiers à traiter par un masque ou une expression régulière, avant ou après l'ajout (seuls les fichiers cachés et système sont filtrés aujourd'hui).
- Ligne de commande : choisir ce que le journal garde (réussites, erreurs, fichiers inchangés) ; aujourd'hui le filtre ne vaut que pour la fenêtre.
- Changement de casse : majuscule à une position donnée.
- Un fichier de commandes (`.cmd`/`.sh`) écrit pour un renommage différé (annuler une fois le programme fermé est fait : l'historique est conservé).

### Vérifié dans le code (22/10/2026)

- La ligne de commande silencieuse (`-q`) n'existait pas (22/10/2026) : les options étaient `-b`, `-g`, `-x`, `-a[fFr]` et `--shell`. Ajoutée en 3.1 (Unreleased).
- Un dossier donné avec une barre oblique finale (`-afr d:\Data\`) n'ajoutait rien, comme dans l'original (le nom utilisé comme masque était vide) ; confirmé par un test le 24/10/2026 et corrigé (3.1.0-beta.1).
- Il n'y a pas d'aperçu des images dans la liste des fichiers (seulement l'option d'ouvrir le fichier par double-clic) : une idée, sans version.

### Pas prévu

La détection de doublons ; une ligne de commande plus riche (l'actuelle suffit à l'automatisation) ; écrire les tags MP3 d'après les noms de fichiers, retoucher les images, surveiller un dossier, convertir des formats (hors de l'objet du programme, comme l'auteur de l'original l'a répondu) ; renommer depuis le presse-papiers (trop spécifique).
