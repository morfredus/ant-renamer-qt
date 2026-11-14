# Feuille de route

English version: [TODO.md](TODO.md). Ce qui est déjà fait est dans le [CHANGELOG](../CHANGELOG.md).

Ce sont des idées, pas des promesses : certaines ne seront peut-être jamais faites. Les règles :
- Un thème par version mineure, une version à la fois. Chacune est essayée par les utilisateurs (bêta, release candidate, puis au moins quelques semaines d'usage stable) avant que la suivante commence.
- Les versions sont classées par importance : ce que les utilisateurs ont demandé le plus souvent et depuis le plus longtemps passe en premier (sources : le forum « Comments » du programme d'origine, forum.antp.be, 120 sujets lus le 22/10/2026, puis les issues et discussions de ce projet). L'ordre change avec les retours.
- Rien n'est écrit pour une idée dont personne n'a besoin : une idée attend une vraie demande ou un vrai problème.

## Où nous en sommes (13/11/2026)

- **3.1.0** publiée le 09/11/2026 (ligne de commande, recherche de mises à jour, disposition moderne par défaut, `-q`, aperçu exportable, annulation après un redémarrage).
- **3.1.1-beta.1** publiée le 12/11/2026 : numérotation des fichiers qui reçoivent le même nom (Date et heure, EXIF), une seule fenêtre à la fois, message pour un dossier vide, informations de mise à jour de l'AppImage, clang-tidy et `-Werror` dans la CI.

## Versions à venir, dans l'ordre

### 3.1.1 (finale) : aucune nouveauté

Seulement ce que la bêta montre. Avant la sortie :
- Pas encore essayés : l'installeur, les paquets `.deb` et AppImage, une mise à jour par-dessus la 3.1.0, Linux Mint, le deuxième Raspberry Pi. clang-tidy sur le Pi.
- Publier le fichier `.zsync` à côté de l'AppImage dans la release (sinon AppImageUpdate ne peut pas la mettre à jour).
- Flathub : la première demande (flathub/flathub#11169) a été refusée le 14/11/2026, le projet étant trop récent ; l'équipe invite à la redéposer dans un mois environ (vers le 14/12/2026). Remarques sur le manifeste : utiliser `desktop-file-edit` plutôt que `sed` pour la ligne de l'icône, et installer avec des règles CMake plutôt que des commandes `install` et `cp` en vrac (voir les idées à évaluer). La politique de Flathub sur l'IA générative s'applique au manifeste et au texte de la demande : ils sont écrits à la main.
- Annonce sur le forum.
- Captures d'écran : à refaire si l'interface a changé.

### 3.2 : des noms qui ne se heurtent jamais, les dates

Le plus demandé : des fichiers refusés parce que le nouveau nom est pris, et les dates des fichiers.
- **Numérotation automatique dans toutes les actions** (`_1`, `_2`... comme Capture One, demandé sur le forum). Fait pour Date et heure et EXIF. Ailleurs, un nom déjà pris est refusé et journalisé, rien n'est écrasé : avec 1000 photos prises en rafale, les refusées sont à traiter à la main.
- **Balise à placer dans le masque** pour le numéro des doublons (EXIF) : choisir la place et la forme au lieu de `_01` à la fin.
- **Dates des fichiers** : changer les dates de création et de modification (demandé de nombreuses fois, « sur la liste des choses à faire » de l'original depuis 2005), et l'inverse de l'action Date et heure : fixer la date à partir d'une date trouvée dans le nom.
- **Annonce de mise à jour avec les nouveautés** : quand une nouvelle version est annoncée, afficher sous son numéro ce qu'elle apporte (les notes de version ou les lignes `WHATS_NEW` de cette version), pas seulement le numéro.

### 3.3 : choisir les fichiers

Aujourd'hui seuls les fichiers cachés et système sont filtrés.
- **Filtrer par date et/ou heure** : garder les fichiers dont la date de modification, de création ou de prise de vue EXIF tombe dans une période ou à une heure de la journée (« tout ce qui a été pris entre 14 h et 15 h »), et aussi par taille.
- **Filtrer par masque ou expression régulière**, avant ou après l'ajout des fichiers.
- **Retrouver un fichier** dans la liste (par nom, date, heure).
- Ligne de commande : les mêmes filtres, et le choix de ce que le journal garde (réussites, erreurs, fichiers inchangés ; aujourd'hui le filtre ne s'applique qu'à la fenêtre).

### 3.4 : réutiliser et réparer les noms

- **Préréglages pour toutes les actions** : nommer et enregistrer les réglages de n'importe quelle action (« Photos : date + compteur »), comme la grille de l'action 3 peut déjà l'être.
- **Nouvelles balises** : `%size%` et d'autres attributs de fichier ; sommes de contrôle (`%crc32%`, `%md5%`, `%sha1%`).
- **Réparer les noms mal encodés** (UTF-8 lu comme ISO-8859-1 après un aller-retour Linux/Windows), en option au lieu d'une grille de remplacement.
- **Énumération par lettres** (`AA`, `AB`...).
- **« Copier au lieu de renommer » par action** du lot (aujourd'hui une option globale).
- **Changement de casse** : majuscule à une position donnée.

### 3.5 : métadonnées des photos, davantage

- **IPTC** : lire plus de champs, et l'IPTC des fichiers TIFF (le bloc IPTC est lu dans les JPEG aujourd'hui).
- **EXIF** : les balises GPS (latitude, longitude), plus de 36 balises, la MakerNote (objectif, numéro de série).
- **Formats récents** : HEIC/HEIF, AVIF, JPEG XL ; RAW des boîtiers récents (à vérifier pour chaque nouveau boîtier : date, sous-secondes, orientation).
- **XMP** : les attributs `exif:` d'un sidecar (date, GPS) et les réglages `crs:` (Camera Raw), pas encore associés.
- **Sidecars des nouveaux logiciels photo** : voir le suivi de maintenance.

### 3.6 : audio

Tags FLAC, OGG, M4A (première limite du guide utilisateur ; commentaires Vorbis d'abord, M4A en dernier), durée et débit des fichiers MP3, dossiers nommés d'après les tags qu'ils contiennent (« artiste - album »).

### 3.7 : vidéo et documents

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
| **Traductions** | à chaque nouveau texte | Fichiers `.lng` alignés sur `English.lng`, `ANTREN_MISSING=1` sur chaque boîte de dialogue, pages d'aide en EN, FR, HR, TA. |
| **Documents** | à chaque release | CHANGELOG, WHATS_NEW, USER_GUIDE et les versions anglaises, BUILDING / ARCHITECTURE / CONTRIBUTING quand la structure change, captures d'écran quand l'interface change. |
| **Paquets et diffusion** | à chaque release | Installeur Windows et version portable, `.deb` amd64 et arm64, AppImage avec son `.zsync`, Flathub. À évaluer : winget, Scoop, Chocolatey, un paquet AUR (seulement si quelqu'un le demande et le maintient). |
| **Licence** | quand une dépendance ou une contribution arrive | Le portage est sous GPL v2 ou ultérieure. Passer à la GPL v3 est permis, mais seulement avec une vraie raison (une dépendance ou une contribution sous une licence incompatible avec la v2). Cela touche `LICENSE`, la boîte À propos, `README`, `CONTRIBUTING`, `docs/PRESENTATION`, le copyright du `.deb`, le MetaInfo Flatpak et le fichier de licence de l'installeur. |
| **Sécurité** | à chaque signalement, et à chaque mise à jour de Qt | `SECURITY_fr.md` ; l'export CSV (injection de formule, corrigée en 3.1.0-rc.3), les chemins et noms de fichiers venant de l'extérieur. |

## Idées à évaluer

Pas de version pour l'instant ; chacune attend un vrai besoin.

- **Aperçu des images** dans la liste des fichiers (aujourd'hui seulement l'option d'ouvrir le fichier au double-clic). Lourd pour les gros RAW ; des miniatures lues dans le JPEG intégré suffiraient.
- **La deuxième instance transmet sa ligne de commande à la fenêtre ouverte** (chemins, tâche, `-g -x`), la met au premier plan et quitte, au lieu de la boîte d'information. Demande qu'une fenêtre occupée par un traitement refuse ou mette la commande en file, et un verrou lié au dossier des réglages. La voie simple (boîte et sortie) est en place et suffit jusqu'ici.
- **Un fichier de commandes** (`.cmd`/`.sh`) écrit pour un renommage ultérieur (annuler après la fermeture du programme est fait : l'historique est conservé).
- **`-q` et l'historique** : un lancement `-q` peut encore écraser l'historique d'une fenêtre ouverte. Accepté pour l'instant ; à revoir si quelqu'un perd une passe.
- **Signature de code des fichiers Windows** (voulue par certains sites de téléchargement comme Softonic, et elle évite l'avertissement SmartScreen). La SignPath Foundation a refusé la candidature le 13/11/2026 : pas encore assez de visibilité publique (étoiles, forks, contributeurs, articles et discussions externes). À redemander quand le projet sera mieux connu. Les voies payantes (un abonnement SignPath, environ 500 USD par an ; un certificat comme celui de Certum pour l'open source, à partir d'environ 50 EUR par an) ne valent pas le coût pour un logiciel libre pour l'instant. Le programme et l'installeur portent déjà des informations de version, dont toute signature a besoin.
- **Règles `install()` de CMake** (programme, dossiers `Languages` et `help`, fichier `.desktop`, MetaInfo, icônes) pour qu'un seul `cmake --install` serve tous les paquets Linux. Aujourd'hui, les chemins sont écrits trois fois, dans les scripts `.deb` et AppImage et dans le manifeste Flatpak. Demandé par les relecteurs de Flathub ; utile en soi, et à faire avant de redéposer.
- **Menu contextuel moderne de Windows 11**, sur demande.
- **Qualité du code, facultatif** (peu utile) : `std::span` pour `exportPreview`, `qScopeGuard` pour `LocalFree` dans `lastSysError()`, règle de nommage (`readability-identifier-naming`) maintenant que clang-tidy est en place.

## Écartées

- **Lire les données `.dop`** : format propre à DxO.
- **Détection de doublons**, retouche d'images, surveillance d'un dossier, conversion de formats, écriture des tags MP3 depuis les noms de fichiers : hors de ce à quoi sert le programme, comme l'a répondu l'auteur de l'original.
- **Une ligne de commande plus riche** : l'actuelle suffit pour l'automatisation.
- **Renommer depuis le presse-papiers** : trop spécifique.
- **Pointeurs `File*` dans `FileList`** : les adresses sont stables (le vecteur contient des `unique_ptr`) ; le seul risque est un pointeur gardé après `remove()`.
- **Signature de `moveFile(..., QString& err)`** : trois appelants, tous testent le résultat.
- **Une empreinte pour vérifier un fichier avant une annulation après redémarrage** : la taille et la date de modification suffisent ; une empreinte n'ajouterait que le cas très rare d'un remplacement de même taille et même date, pour plus de code et plus de lecture des fichiers.
