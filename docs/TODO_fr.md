# Feuille de route

English version: [TODO.md](TODO.md). Ce qui est déjà fait est dans le [CHANGELOG](../CHANGELOG.md).

Ce sont des idées, pas des promesses : certaines ne seront peut-être jamais faites, et rien ici n'est figé. Les règles :
- **Ordre de priorité** : d'abord les problèmes fonctionnels (ce qui donne un mauvais résultat, fait perdre des données ou ne marche pas comme annoncé), ensuite les demandes faites sur les forums et par les utilisateurs (les plus anciennes et les plus fréquentes d'abord), puis tout le reste.
- Un thème par version mineure, une version à la fois. Chacune est essayée par les utilisateurs (bêta, release candidate) avant que la suivante commence. Pendant le développement d'une version, une préversion numérotée (`3.3.0-beta.1`, `beta.2`...) est publiée de temps en temps quand quelques changements valent d'être essayés, pas une par changement. Les corrections seules vont dans une version de correction (`3.x.y`) quand quelque chose d'urgent ne peut pas attendre la version mineure suivante.
- La feuille de route bouge avec les retours. Une idée prévue pour une version peut changer de version, ou être réunie avec une autre, quand une demande ou une autre idée les rapproche (par exemple, filtrer par date et fixer une date utilisent le même code, elles sont donc dans la même version).
- Les sources des demandes : le forum « Comments » du programme d'origine, forum.antp.be (120 sujets lus le 22/10/2026), puis les issues et discussions de ce projet.
- Rien n'est écrit pour une idée dont personne n'a besoin : une idée attend une vraie demande ou un vrai problème.

## Où nous en sommes (25/11/2026)

- **3.2.0** publiée le 20/11/2026 : l'opération « Infos EPUB », plus de noms qui ont l'air vides (EPUB, MP3, EXIF), le menu de l'Explorateur qui demande quoi ajouter, les fichiers laissés tels quels affichés en orange dans l'aperçu, l'annonce de mise à jour qui montre les notes de version, l'aide et les crédits nettoyés, des liens vers l'aide en ligne, un nouveau manifeste Flatpak construit avec `cmake --install`.
- **Diffusion** : l'application est sur Flathub depuis le 22/11/2026 (mises à jour par pull request sur `flathub/fr.morfredus.AntRenamerQt`, avec `publish-antrenamer-flatpak.sh`), dans AppImageHub et Portable Linux Apps ; une pull request winget attend un modérateur.
- **3.3.0-beta.1** publiée le 22/11/2026 (Filtrer les fichiers, annulation des copies, historique partagé), essayée sous Ubuntu, Fedora et Windows 11 et 10. **3.3.0-beta.2** publiée le 23/11/2026 en préversion depuis `dev/3.3.0` (Mettre les dates d'après l'EXIF avec les fichiers associés qui suivent, `--set-dates-from-exif`, page d'accueil du .deb) ; la CI de la branche (compilation, tests, clang-tidy, `appstreamcli validate`, paquets) était verte sur le commit publié. Pas encore essayé à la main : l'annulation des dates sur de vrais fichiers, la fonction sous Linux, les paquets de la 3.3.0-beta.2 installés.
- **3.3.0** finale préparée sur `dev/3.3.0` (24/11/2026), en attente de la fusion dans `main`, de l'étiquette (tag) et des paquets. Elle ajoute à la beta.2 : la correction Flatpak (des fichiers donnés un par un disparaissaient, trouvée sous Fedora KDE), `--filesystem=home`, un conseil pour les dossiers synchronisés. À faire après le tag : `tag` et `commit` du manifeste Flathub, puis `publish-antrenamer-flatpak.sh`.
- **3.3.1** préparée sur `dev/3.3.1` (25/11/2026), corrective : le Flatpak sans `--filesystem=home` (refusé par Flathub), le dossier demandé avec le portail FileChooser, les messages Flatpak en français. Essayée sous Fedora KDE (Dolphin) depuis le manifeste local. À faire après le tag : `tag` et `commit` du manifeste Flathub, puis `publish-antrenamer-flatpak.sh`.
- La branche `dev/3.3.0` porte les corrections prévues pour une 3.2.1.

## Versions à venir, dans l'ordre

### 3.3 : corrections et les dates des fichiers

Pas de 3.2.1 à part : les corrections attendent la 3.3, pour qu'une seule version les porte toutes au lieu d'une série de petites. Les problèmes fonctionnels passent d'abord, puis la plus ancienne demande des forums.

**Corrections et points laissés par la 3.2.1**
- **Limites fonctionnelles connues à revoir** :
  - Fait dans `dev/3.3.0` : un lancement `-q` et une fenêtre ouverte ne s'écrasent plus l'historique l'un de l'autre (chacun reprend les passes de l'autre).
  - Fait dans `dev/3.3.0` : l'annulation supprime les fichiers copiés avec « Copier au lieu de renommer » s'ils sont inchangés (même taille et même date qu'au moment de la copie), et les garde avec un message sinon.
- **Points de release** : vérifier la première mise à jour du dépôt Flathub avec `publish-antrenamer-flatpak.sh` (une vraie mise à jour, pas seulement un essai à blanc). La ligne de version doit être dans le MetaInfo avant de poser le tag (la 3.2.0 manquait, Flathub affiche donc encore la 3.1.1 ; elle est listée dans `dev/3.3.0`). Fait : la mention « EPUB est dans la bêta » est retirée de l'aide en ligne, et l'économie de CI (pas de build sur un push vers `main`, le script de release Linux qui attend la CI en cours) est essayée.
- **AppImage** : l'AppImage de la 3.3.0-beta.1 a été lancée sous Fedora et Ubuntu, comportement attendu (23/11/2026). Le metainfo de l'AppImage (`--appimage-extract`, `usr/share/metainfo/fr.morfredus.AntRenamerQt.metainfo.xml`) est présent et `appstreamcli validate` réussit (23/11/2026, Ubuntu) ; `--pedantic` note seulement les majuscules de l'identifiant du composant, qui reste tel quel puisque c'est l'identifiant Flathub. `appstreamcli validate` est dans la CI (job Linux, 23/11/2026). Reste à faire : vérifier les captures sur la page AppImageHub.
- **Captures d'écran** : à refaire si l'interface a changé (les boîtes Filtrer les fichiers et Mettre les dates d'après l'EXIF sont nouvelles).

**Les dates des fichiers**

- **Remettre la date de modification d'un fichier en accord avec sa date de prise de vue EXIF** (fait dans `dev/3.3.0`, clic droit « Mettre les dates d'après l'EXIF... », `--set-dates-from-exif` ; demandé de nombreuses fois, « sur la liste des choses à faire » de l'original depuis 2005). Cas typique : des photos copiées dont la date du système de fichiers a été écrasée. Seule la date de modification est concernée : elle se change de la même façon sous Windows et sous Linux. La date de création est exclue : elle ne se change pas sous Linux avec les outils standard (à vérifier), et une simple copie la remet à zéro. Les dates lues dans un nom de fichier viendraient plus tard, seulement sur demande.
  **Choix délibéré, avec des garde-fous** (les originaux des photos ne doivent pas pouvoir être falsifiés par accident ni facilement) :
  - La date vient du fichier lui-même (EXIF). Aucune date saisie à la main, et aucune date inventée : un fichier sans date EXIF est laissé tel quel et le journal le dit.
  - Le programme ne change que la date du système de fichiers, jamais le contenu du fichier : la date de prise de vue EXIF et les tags ne sont pas touchés.
  - Une opération à part, jamais activée par défaut, avec une confirmation qui dit ce qui va être fait. L'aperçu montre l'ancienne et la nouvelle date de chaque fichier.
  - Le journal garde l'ancienne et la nouvelle date, et l'historique permet d'annuler.
  - En ligne de commande, la fonction demande son option explicite.
  - La documentation (guide, aide, README) dit que la date de modification n'est pas une preuve d'authenticité et que, pour des originaux à valeur de preuve, il faut travailler sur des copies (option « Copier au lieu de renommer »).
- **Filtrer par date et/ou heure** (fait dans `dev/3.3.0`, clic droit « Filtrer les fichiers... » ; les mêmes filtres en ligne de commande viendront en 3.4) : garder les fichiers dont la date de modification, de création ou de prise de vue EXIF tombe dans une période ou à une heure de la journée (« tout ce qui a été pris entre 14 h et 15 h »), et aussi par taille.

### 3.4 : choisir les fichiers

Aujourd'hui seuls les fichiers cachés et système sont filtrés.
- **Filtrer par masque ou expression régulière**, avant ou après l'ajout des fichiers.
- **Retrouver un fichier** dans la liste (par nom, date, heure).
- Ligne de commande : les mêmes filtres, et le choix de ce que le journal garde (réussites, erreurs, fichiers inchangés ; aujourd'hui le filtre ne s'applique qu'à la fenêtre).

### 3.5 : réutiliser et réparer les noms

- **Préréglages pour toutes les actions** : nommer et enregistrer les réglages de n'importe quelle action (« Photos : date + compteur »), comme la grille de l'action 3 peut déjà l'être.
- **Nouvelles balises** : `%size%` et d'autres attributs de fichier ; sommes de contrôle (`%crc32%`, `%md5%`, `%sha1%`).
- **Réparer les noms mal encodés** (UTF-8 lu comme ISO-8859-1 après un aller-retour Linux/Windows), en option au lieu d'une grille de remplacement.
- **Énumération par lettres** (`AA`, `AB`...).
- **« Copier au lieu de renommer » par action** du lot (aujourd'hui une option globale).
- **Noms aléatoires, plus d'options** : aujourd'hui un masque et trois méthodes de tirage seulement. Ajouter les jeux de caractères précis où tirer (chiffres, minuscules/majuscules, liste personnalisée, exclusion des caractères qui se ressemblent comme `0/O`, `1/l`) et des règles de préfixe/suffixe. Remarque fondée reçue d'un lecteur.

### 3.6 : métadonnées des photos, davantage

- **IPTC** : lire plus de champs, et l'IPTC des fichiers TIFF (le bloc IPTC est lu dans les JPEG aujourd'hui).
- **EXIF** : les balises GPS (latitude, longitude), plus de 36 balises, la MakerNote (objectif, numéro de série).
- **Formats récents** : HEIC/HEIF, AVIF, JPEG XL ; RAW des boîtiers récents (à vérifier pour chaque nouveau boîtier : date, sous-secondes, orientation).
- **XMP** : les attributs `exif:` d'un sidecar (date, GPS) et les réglages `crs:` (Camera Raw), pas encore associés.
- **Sidecars des nouveaux logiciels photo** : voir le suivi de maintenance.

### 3.7 : audio

Tags FLAC, OGG, M4A (première limite du guide utilisateur ; commentaires Vorbis d'abord, M4A en dernier), durée et débit des fichiers MP3, dossiers nommés d'après les tags qu'ils contiennent (« artiste - album »).

### 3.8 : vidéo et documents

Vidéo (MP4, MKV) : date de prise de vue, durée, pour nommer les vidéos comme les photos. Propriétés des fichiers PDF et des documents Office (titre, auteur, année).

## Suivi de maintenance

Pas une version : un ensemble de vérifications, faites à date fixe et sur événement, qui gardent le programme en état de marche. Chaque vérification se termine par une ligne dans le CHANGELOG (ou « rien à faire ») et, si besoin, une mise à jour de `docs/TESTED_PLATFORMS_fr.md`.

| Quoi | Quand | Ce qui est vérifié |
|---|---|---|
| **Nouvelle version de Qt** (mineure 6.x, nouvelle LTS) | à chaque sortie de Qt, et avant de relever ou d'abandonner le Qt minimal | Compilation et tests sous Windows (MSYS2), Ubuntu, Raspberry Pi ; avertissements d'obsolescence ; haute définition, thème sombre et style de Windows 11 ; le Qt minimal de l'AppImage (construite sur Ubuntu 22.04, Qt 6.5) et le runtime Flathub. |
| **Chaîne d'outils et CI** | chaque trimestre | MSYS2 : la CI et les scripts utilisent UCRT64 (MINGW64 est obsolète), à l'essai dans la branche `test/ucrt64` ; images des runners GitHub (une ancienne image Ubuntu finit par disparaître) ; version de clang-tidy ; avertissements d'un nouveau GCC/Clang. |
| **Distributions Linux** | chaque nouvelle Ubuntu LTS ou Debian stable | Installation et démarrage du `.deb`, démarrage de l'AppImage (glibc), Flatpak ; paquets dont dépend le `.deb`. |
| **Sidecars** | à chaque version majeure d'un logiciel photo, et sur signalement | Voir plus bas. Règle : un sidecar suit sa photo quand elle est renommée, et un sidecar que le programme ne connaît pas est laissé tranquille, jamais abîmé. |
| **EXIF et métadonnées** | tous les six mois, et pour chaque nouveau boîtier signalé | Balises nouvelles ou modifiées (Exif 3.0, chaînes UTF-8), nouveaux formats RAW et MakerNotes, champs de sous-secondes, fuseaux horaires (`OffsetTime`). Un fichier d'essai par format dans les tests. |
| **Logiciels photo** | sur signalement | Ce qu'un photographe utilise à côté du programme : Lightroom, Capture One, DxO, darktable, RawTherapee, ON1, digiKam, les logiciels des fabricants d'appareils. Pour chacun : ses sidecars (`.xmp`, `.dop`, `.cos`, `.pp3`...), le catalogue qui se casse quand un fichier est renommé (et quoi dire à l'utilisateur). |
| **Traductions** | à chaque nouveau texte | Fichiers `.lng` alignés sur `English.lng`, `ANTREN_MISSING=1` sur chaque boîte de dialogue, pages d'aide en EN, FR, HR, TA (les pages croates et tamoules écrites pour Ant Renamer Qt affichent encore l'anglais). |
| **Documents** | à chaque release | CHANGELOG, WHATS_NEW, USER_GUIDE et les versions anglaises, BUILDING / ARCHITECTURE / CONTRIBUTING quand la structure change, captures d'écran quand l'interface change, la ligne de version du MetaInfo. |
| **Paquets et diffusion** | à chaque release | Installeur Windows et version portable, `.deb` amd64 et arm64, AppImage avec son `.zsync`, Flathub (pull request sur `flathub/fr.morfredus.AntRenamerQt`, `master` est protégée). La pull request winget, les fiches des catalogues (AppImageHub, Softpedia et d'autres : voir le journal de visibilité). À évaluer : Scoop, Chocolatey, un paquet AUR (seulement si quelqu'un le demande et le maintient). |
| **Licence** | quand une dépendance ou une contribution arrive | Le portage est sous GPL v2 ou ultérieure. Passer à la GPL v3 est permis, mais seulement avec une vraie raison (une dépendance ou une contribution sous une licence incompatible avec la v2). Cela touche `LICENSE`, la boîte À propos, `README`, `CONTRIBUTING`, `docs/PRESENTATION`, le copyright du `.deb`, le MetaInfo Flatpak et le fichier de licence de l'installeur. |
| **Sécurité** | à chaque signalement, et à chaque mise à jour de Qt | `SECURITY_fr.md` ; l'export CSV (injection de formule, corrigée en 3.1.0-rc.3), les chemins et noms de fichiers venant de l'extérieur. |

## Idées à évaluer

Pas de version pour l'instant ; chacune attend un vrai besoin.

- **Aperçu des images** dans la liste des fichiers (aujourd'hui seulement l'option d'ouvrir le fichier au double-clic). Lourd pour les gros RAW ; des miniatures lues dans le JPEG intégré suffiraient.
- **La deuxième instance transmet sa ligne de commande à la fenêtre ouverte** (chemins, tâche, `-g -x`), la met au premier plan et quitte, au lieu de la boîte d'information. Demande qu'une fenêtre occupée par un traitement refuse ou mette la commande en file, et un verrou lié au dossier des réglages. La voie simple (boîte et sortie) est en place et suffit jusqu'ici.
- **Un fichier de commandes** (`.cmd`/`.sh`) écrit pour un renommage ultérieur (annuler après la fermeture du programme est fait : l'historique est conservé).
- **Signature de code des fichiers Windows** (voulue par certains sites de téléchargement comme Softonic, et elle évite l'avertissement SmartScreen). La fondation de signature a refusé la candidature le 13/11/2026 : pas encore assez de visibilité publique (étoiles, forks, contributeurs, articles et discussions externes). À redemander quand le projet sera mieux connu. Les voies payantes (un abonnement, environ 500 USD par an ; un certificat comme celui de Certum pour l'open source, à partir d'environ 50 EUR par an) ne valent pas le coût pour un logiciel libre pour l'instant. Le programme et l'installeur portent déjà des informations de version, dont toute signature a besoin.
- **Date de création des fichiers** : seulement si un utilisateur la demande, et seulement là où le système le permet (Windows).
- **Menu contextuel moderne de Windows 11**, sur demande.
- **Qualité du code, facultatif** (peu utile) : `std::span` pour `exportPreview`, `qScopeGuard` pour `LocalFree` dans `lastSysError()`, règle de nommage (`readability-identifier-naming`) maintenant que clang-tidy est en place.

## Écartées

- **Une balise dans le masque pour placer le numéro des noms en double** (actions Date et heure et EXIF), au lieu de `_01` avant l'extension : choix délibéré, pas utile. Le suffixe en fin de nom est ce que fait un logiciel courant ; la balise serait vide sans collision et laisserait son séparateur dans le nom ; l'action Énumération et sa balise `%num%` placent déjà un compteur n'importe où. Seulement si quelqu'un le demande vraiment (commencer alors par le format du suffixe : `_01`, `-1`, ` (1)`).
- **Numérotation automatique des noms en double dans toutes les actions** (`_1`, `_2`...) : faite pour Date et heure et EXIF, où les rafales de photos en ont besoin. Dans les autres actions, un nom déjà pris est une erreur de réglage que l'utilisateur doit voir : il est refusé et journalisé ; l'action Énumération et sa balise `%num%` placent déjà un compteur où l'utilisateur le veut. À reconsidérer si un vrai cas se présente.
- **Lire les données `.dop`** : format propre à DxO.
- **Détection de doublons**, retouche d'images, surveillance d'un dossier, conversion de formats, écriture des tags MP3 depuis les noms de fichiers : hors de ce à quoi sert le programme, comme l'a répondu l'auteur de l'original.
- **Une ligne de commande plus riche** : l'actuelle suffit pour l'automatisation.
- **Renommer depuis le presse-papiers** : trop spécifique.
- **Règles `install()` de CMake** : faites, un seul `cmake --install` sert les paquets Linux.
- **Pointeurs `File*` dans `FileList`** : les adresses sont stables (le vecteur contient des `unique_ptr`) ; le seul risque est un pointeur gardé après `remove()`.
- **Signature de `moveFile(..., QString& err)`** : trois appelants, tous testent le résultat.
- **Une empreinte pour vérifier un fichier avant une annulation après redémarrage** : la taille et la date de modification suffisent ; une empreinte n'ajouterait que le cas très rare d'un remplacement de même taille et même date, pour plus de code et plus de lecture des fichiers.
