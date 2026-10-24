# TODO

## Idées pour après la 3.0.0

English version: [TODO.md](TODO.md).

### Versions possibles

Ce sont des idées, pas des promesses : certaines ne seront peut-être jamais réalisées. Si elles le sont, un thème par version mineure, une à la fois : chacune est éprouvée par les utilisateurs (rc, puis au moins quelques semaines de version stable) avant de commencer la suivante. Rien n'est commencé avant la publication de la 3.0.0. Les versions sont triées par urgence : ce que les utilisateurs du programme d'origine ont demandé le plus souvent, depuis le plus longtemps, passe en premier (source : le forum « Comments » du programme d'origine, forum.antp.be, 120 sujets lus le 22/10/2026). L'ordre peut changer selon les retours.

| Version | Contenu |
|---|---|
| **3.1** | **Ligne de commande, barre finale** (`-afr d:\Data\`, fait, voir le CHANGELOG, Unreleased). **Recherche de mises à jour, préversions** (la case des options et le choix proposé par l'installeur sont faits, voir le CHANGELOG, Unreleased). **Interface moderne par défaut** : l'interface moderne, écran unique en bandes, devient celle d'une nouvelle installation (aujourd'hui la classique : `newUi` et `pageLayout` dans `appsettings.h`). L'installateur ne pose aucune question à ce sujet. Au premier lancement de la nouvelle version, un message unique propose aux utilisateurs qui ont encore la disposition classique de passer à la moderne (à juger le moment venu : seulement si ce n'est pas intrusif, jamais répété après un refus) ; les réglages déjà enregistrés sont sinon conservés. Pas pour la 3.0.0 : son code est celui de la 3.0.0-rc.16. **Ligne de commande silencieuse** (`-q`) : pas de fenêtre, pour les scripts appelés depuis un menu contextuel (demandé sur le forum). **Suffixe pour les noms en double dans l'action EXIF** : deux photos prises la même seconde ont le même nom ; une balise à placer où l'on veut dans le masque (l'action Date et heure a déjà une option de suffixe). **Aperçu exportable** : les noms actuels et nouveaux (ou seulement la liste des noms) écrits dans un fichier CSV ou texte, pour relire un gros lot ou garder une trace. (fait, voir le CHANGELOG, Unreleased) |
| **3.2** | **Dates des fichiers** : modifier les dates de création et de modification des fichiers (demandé de nombreuses fois, « sur la liste » de l'original depuis 2005), et l'inverse de l'action Date et heure : fixer la date d'après une date trouvée dans le nom. **Énumération par lettres** (`AA`, `AB`...). **« Copier au lieu de renommer » par action** du lot (aujourd'hui une option globale). |
| **3.3** | **Préréglages pour toutes les actions** : nommer et enregistrer les réglages de n'importe quelle action (« Photos : date + compteur »), comme on peut déjà le faire pour la grille de l'action 3. **Nouvelles balises** : `%size%` et d'autres attributs de fichier ; sommes de contrôle (`%crc32%`, `%md5%`, `%sha1%`). **Corriger les noms mal encodés** (UTF-8 lu comme ISO-8859-1 après un passage Linux/Windows), sous forme d'option plutôt que d'une grille de remplacements. |
| **3.4** | **Audio** : tags FLAC, OGG, M4A (première limite du guide utilisateur ; commentaires Vorbis d'abord, M4A en dernier), et durée et débit des fichiers MP3. **Dossiers nommés d'après les tags MP3** qu'ils contiennent (« artiste - album »). |
| **3.5** | **Vidéo** (MP4, MKV) : la date de prise de vue, la durée, pour nommer les vidéos comme les photos. |
| **3.6** | **Métadonnées, plus complètes** : les tags GPS (latitude, longitude), plus de 36 tags EXIF, le MakerNote (objectif, numéro de série), IPTC des fichiers TIFF, les propriétés des fichiers PDF et documents Office (titre, auteur, année). |
| **Plus tard** | **Menu contextuel moderne de Windows 11**, à décider à la demande. |

### Petites idées, sans version

- Plus de types de fichiers associés (par exemple `.pp3` de RawTherapee).
- Lire les données `.dop` ? Pas prévu : c'est un format propre à DxO.
- Propriétés XMP pas encore reprises : les attributs `exif:` d'un fichier associé (date, GPS), les réglages `crs:` (Camera Raw).
- Filtrer les fichiers à traiter par un masque ou une expression régulière, avant ou après l'ajout (seuls les fichiers cachés et système sont filtrés aujourd'hui).
- Ligne de commande : choisir ce que le journal garde (réussites, erreurs, fichiers inchangés) ; aujourd'hui le filtre ne vaut que pour la fenêtre.
- Changement de casse : majuscule à une position donnée.
- Un fichier de commandes (`.cmd`/`.sh`) écrit pour un renommage différé, ou pour annuler une fois le programme fermé (l'historique d'annulation n'est gardé que pour la session).

### Vérifié dans le code (22/10/2026)

- La ligne de commande silencieuse (`-q`) n'existe pas : les options sont `-b`, `-g`, `-x`, `-a[fFr]` et `--shell`.
- Un dossier donné avec une barre oblique finale (`-afr d:\Data\`) n'ajoutait rien, comme dans l'original (le nom utilisé comme masque était vide) ; confirmé par un test le 24/10/2026 et corrigé (Unreleased).
- Il n'y a pas d'aperçu des images dans la liste des fichiers (seulement l'option d'ouvrir le fichier par double-clic) : une idée, sans version.

### Pas prévu

La détection de doublons ; une ligne de commande plus riche (l'actuelle suffit à l'automatisation) ; écrire les tags MP3 d'après les noms de fichiers, retoucher les images, surveiller un dossier, convertir des formats (hors de l'objet du programme, comme l'auteur de l'original l'a répondu) ; renommer depuis le presse-papiers (trop spécifique).
