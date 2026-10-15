# Ant Renamer Qt : guide des fonctions

*Liste de toutes les fonctions de Ant Renamer Qt 3.0.0-rc.9 avec leur mode d'emploi.*
Version anglaise : [USER_GUIDE.md](USER_GUIDE.md). Détail complet : l'aide intégrée (touche F1).

## 1. Principe général

Le travail se fait en trois étapes, une page chacune :

1. **Fichiers** : on constitue la liste des fichiers (et dossiers) à renommer.
2. **Opérations** : on choisit une ou plusieurs opérations de renommage (le « job »).
3. **Journal** : on lit ce qui s'est passé après avoir cliqué sur **Démarrer**.

L'aperçu en bas de la page Opérations montre en direct le nom actuel et le futur nom du fichier sélectionné.

## 2. Raccourcis clavier

| Touche | Fonction | Touche | Fonction |
|---|---|---|---|
| F1 | Aide | F9 | Démarrer |
| Maj+F1 | À propos | Échap | Arrêter |
| F2 | Ajouter des fichiers | F10 | Options |
| F3 | Ajouter des dossiers | F11 | Toujours au premier plan |
| F5 | Actualiser l'aperçu | F12 | Annuler |
| F6 / F7 / F8 | Retirer la sélection / tout / les manquants | Alt+F4 | Quitter |
| Ctrl+1 / 2 / 3 | Page Fichiers / Opérations / Journal | Ctrl+F2 | Ajouter au job |
| Maj+F6 / Maj+F7 | Retirer la sélection / vider le job | Ctrl+O / Ctrl+S | Ouvrir / enregistrer un job |
| Ctrl+↑ / Ctrl+↓ | Monter / descendre | Ctrl+Maj+↑ / ↓ | Tout en haut / tout en bas |

## 3. Page Fichiers

### Ajouter des fichiers
- **Fichiers** (F2) : fenêtre de sélection classique.
- **Dossiers** (F3) : choisir un ou plusieurs dossiers, puis cocher ce qu'on ajoute : les fichiers qu'ils contiennent, les dossiers eux-mêmes, les sous-dossiers (tous niveaux).
- **Glisser-déposer** depuis l'Explorateur : une fenêtre propose les mêmes options. « Ne pas demander » la mémorise ; maintenir Ctrl ou Maj au dépôt la fait réapparaître une fois.
- **Menu de l'Explorateur** « Ajouter à Ant Renamer Qt » (à activer dans Options > Fichiers & Dossiers). Plusieurs éléments sélectionnés arrivent ensemble dans la fenêtre déjà ouverte. Windows 11 : *Afficher plus d'options*.
- **Ligne de commande** : voir §8.

### Gérer la liste
| Bouton | Effet |
|---|---|
| Retirer la sélection (F6) | enlève les fichiers sélectionnés de la liste (pas du disque) |
| Retirer tout (F7) | vide la liste |
| Retirer les manquants (F8) | enlève les fichiers qui n'existent plus sur le disque |
| Actualiser l'aperçu (F5) | remplit la colonne Aperçu (non automatique, pour la performance) |
| Seulement modifiés | masque les fichiers dont le nom ne change pas dans l'aperçu (simple filtre d'affichage : ils sont quand même renommés) ; actualiser l'aperçu d'abord |
| Haut / Bas / Début / Fin | change l'ordre de traitement (le glisser-déposer dans la liste marche aussi) |

- **Colonnes** : clic droit sur l'en-tête pour afficher Dossier, Nom, Chemin complet, Ext., Aperçu, Taille, Date de création, Date de modification. Elles se redimensionnent et se déplacent.
- **Tri** : clic sur un en-tête (tri naturel par défaut : « fichier 2 » avant « fichier 10 »). Le tri choisit l'ordre de traitement, donc l'ordre de l'énumération. Clic droit > *Non trié* pour le désactiver.
- Clic droit dans la liste : sélectionner tout, etc.

### Fichiers associés (sidecars) d'une photo
Quand une photo est renommée, ses fichiers associés sont renommés avec elle, sans rien faire de plus :
- **Reconnus** : `.xmp` (Lightroom, Capture One : `IMG.xmp` ; digiKam, darktable : `IMG.CR2.xmp`) et `.dop` de DxO PhotoLab (`IMG.dop`, `IMG.CR2.dop`), extension en majuscules comprise.
- **Méthode** : dans Options > Traitement, la case « Renommer aussi les fichiers associés (.xmp, .dop) d'une image renommée » (cochée par défaut). On renomme ensuite la photo normalement (par exemple `IMG_0001.CR2` → `Paris 001.CR2`) : `IMG_0001.xmp` ou `IMG_0001.CR2.xmp` devient `Paris 001.xmp` ou `Paris 001.CR2.xmp`.
- **Annulation** (F12) : les fichiers associés reprennent leur ancien nom avec la photo.
- **Journal** : la ligne de la photo indique le fichier associé renommé.
- **Cas particuliers** : si un fichier portant le nouveau nom existe déjà, le fichier associé n'est pas touché et le journal le signale (la photo, elle, est renommée) ; un fichier associé présent aussi dans la liste suit sans être renommé deux fois ; sans effet en mode « Copier plutôt que renommer » et pour les dossiers.
- Le fichier `.xmp` voisin est aussi lu par l'opération EXIF / IPTC / XMP (§7).

## 4. Page Opérations : les 14 opérations

Choisir l'opération dans la liste, régler ses paramètres (la zone *Notes* sous les paramètres rappelle les tags disponibles), puis **Ajouter au job** (Ctrl+F2). On peut empiler plusieurs opérations : elles s'appliquent dans l'ordre.

| # | Opération | À quoi ça sert | Comment l'utiliser |
|---|---|---|---|
| 1 | **Changer l'extension** | Remplacer ou ajouter une extension | Saisir la nouvelle extension (sans le point). Champ vide = supprime l'extension. Option : ajouter après l'existante au lieu de remplacer. |
| 2 | **Remplacement de chaîne** | Remplacer un texte par un autre | Champs « Rechercher » / « Remplacer par ». Vide = supprime le texte. Options : sensible à la casse, toutes les occurrences (sinon seulement la première), appliquer aussi à l'extension ou seulement à l'extension, **jokers** (nouveauté 3.0.0-rc.9, voir ci-dessous). |
| 3 | **Remplacements multiples** | Plusieurs remplacements d'un coup (accents, etc.) | Grille à 2 colonnes (cherché / remplacé). Ctrl++ ajoute une ligne, Ctrl+- supprime la ligne courante. Une grille se nomme et se sauvegarde (liste déroulante + *Sauver*). Copier / Coller échangent la grille avec Excel ou un éditeur (tabulation entre colonnes). Un set « Remove west-european accents » est fourni. |
| 4 | **Insertion de chaîne** | Insérer un texte à une position | Texte, position (0 = avant le premier caractère ou après le dernier), à partir du début ou de la fin. Option : compter dans l'extension. Tags `%name%`, `%ext%`, `%folderN%`. |
| 5 | **Déplacement de chaîne** | Déplacer N caractères | Nombre de caractères, position « de » et position « vers » (la seconde se compte après retrait des caractères). Ex. `abcdef`, 1 caractère de 2 vers 4 → `abdecf`. |
| 6 | **Suppression de caractères** | Effacer N caractères | Depuis une position, ou après une chaîne donnée. |
| 7 | **Énumération** | Numéroter les fichiers | Masque contenant `%num%` (ex. `Fichier %num%%ext%`). Réglages : *Démarrer à*, *Nombre de chiffres* (zéros devant), *Incrémenter de*. Options : *Retour à la valeur de démarrage* à chaque nouveau dossier (liste triée par dossier), *Même numéro pour les fichiers de même nom* (IMG_1234.JPG + IMG_1234.CR2). |
| 8 | **Tags MP3** | Nommer d'après les tags ID3 (v1, v2.2, v2.3, v2.4) | Masque avec `%author%`, `%title%`, `%album%`, `%year%`, `%genre%`, `%comm%`, `%track%`, `%name%`, `%ext%`. Ex. `%author% - %title%%ext%`. Option : remplacer les caractères interdits. `$if` disponible (§6). |
| 9 | **Date et heure** | Nommer d'après la date du fichier | Masque en lettres : `yyyy` `yy` `mm` `mmm` `mmmm` `dd` `ddd` `dddd` `hh` `nn` (minutes) `ss` `am/pm`, `f` (nom d'origine), `e` (extension). Tout texte fixe va entre guillemets. Ex. `yyyy'-'mm'-'dd hh'-'nn'-'sse`. Choix date de création ou de modification ; option de suffixe pour les doublons. |
| 10 | **Noms aléatoires** | Générer un nom avec une part aléatoire | Masque avec `%random%` (+ `%name%`, `%ext%`). Type : nombre aléatoire (8 chiffres), compteur Windows, ou GUID. |
| 11 | **Changement de casse** | Majuscules / minuscules | Quatre choix : majuscule à la première lettre après certains caractères (espace, tiret, point... à saisir), majuscule à la première lettre du nom, tout en majuscules, tout en minuscules. Garder « options régionales » activé pour traiter correctement les accents. |
| 12 | **Noms depuis une liste** | Appliquer une liste de noms | Une ligne par fichier, dans l'ordre de la liste. Options : ajouter l'extension d'origine ; utiliser la liste comme extensions. Fichier sans ligne correspondante ou ligne vide = non renommé. |
| 13 | **Expressions rationnelles** | Découpage / réorganisation avancée | *Expression* (PCRE2, insensible à la casse) et *Nouveau nom* avec `$1`, `$2`... Ex. `(.*) - (.*)\.mp3` → `$2 - $1.mp3` inverse artiste et titre. |
| 14 | **EXIF / IPTC / XMP** | Nommer les photos d'après leurs métadonnées | Masque avec les tags de la liste du §7, plus `%num%` (même réglages que l'énumération), `$if`. Réglage « Décalage » pour décaler les dates. |

#### Jokers dans le remplacement de chaîne (nouveauté 3.0.0-rc.9)
- **Activation** : case « Utiliser les jokers (* = n'importe quel texte, ? = n'importe quel caractère) », décochée par défaut. Elle ne concerne que l'opération *Remplacement de chaîne*.
- **Dans le champ « Rechercher »** : `*` remplace n'importe quel texte (même vide), `?` exactement un caractère. Tous les autres caractères sont cherchés tels quels (`.`, `(`, `[`... n'ont aucun sens spécial).
- **Le champ « Remplacer par » reste littéral** : `*` et `?` y sont écrits tels quels.
- **Le texte trouvé est le plus court possible** : `a*b` dans `a1b2b` trouve `a1b`, pas `a1b2b`. Avec « toutes les occurrences », la recherche continue ensuite dans le reste du nom.
- **Exemple** : chercher `IMG_*_copie`, remplacer par `Photo` : `IMG_001_copie (1).jpg` devient `Photo (1).jpg`.
- Le choix est enregistré dans les fichiers de job (`.arb`) ; un ancien job se charge avec les jokers désactivés. Les autres opérations n'ont pas de jokers : pour en profiter, placer avant elles un *Remplacement de chaîne* avec jokers (par exemple pour retirer un morceau de longueur variable), dans le même job. Pour des motifs plus complexes, utiliser l'opération *Expressions rationnelles*.
- À ne pas confondre avec les jokers `*` et `?` de la **ligne de commande** (§8), qui servent à choisir les fichiers à ajouter.

### Gérer le job
Le panneau **Contenu du job** liste les opérations planifiées (case à cocher pour l'afficher/masquer).
- **Ajouter au job** (Ctrl+F2), **Retirer la sélection** (Maj+F6), **Vider** (Maj+F7).
- **Ouvrir** (Ctrl+O) / **Enregistrer** (Ctrl+S) : fichiers `.arb` (XML), compatibles avec Ant Renamer 2.
- Réordonner par glisser-déposer ou avec les 4 boutons de déplacement.
- En disposition classique une opération ajoutée n'est pas modifiable (la retirer et la refaire). En disposition moderne : double-clic pour la recharger dans l'éditeur, puis *Valider la modification* (ou le bouton *Modifier*).

## 5. Lancer, arrêter, annuler

- **Démarrer** (F9) : renomme dans un fil séparé ; la fenêtre reste utilisable.
- **Arrêter** (Échap) : interrompt le traitement.
- **Annuler** (F12) : restaure les anciens noms de la **dernière** opération seulement (un seul niveau).
- **Toujours au premier plan** (F11) : garde la fenêtre au-dessus des autres.

## 6. Masques de formatage (opérations 4, 7, 8, 10, 14)

| Syntaxe | Signification |
|---|---|
| `%name%` / `%ext%` | nom sans extension / extension avec le point |
| `%folderN%` | N-ième dossier parent (`%folder1%` = le plus proche). Négatif : depuis la racine (`%folder-1%` = lecteur). |
| `%count%`, `%countr%`, `%countfiles%`, `%countfilesr%`, `%countfolders%`, `%countfoldersr%` | pour les dossiers : nombre d'éléments contenus (`r` = récursif) |
| `%%` | un `%` littéral |
| `$if(cond,alors,sinon)` | si `cond` n'est pas vide : `alors`, sinon `sinon`. Mettre entre guillemets les valeurs contenant virgule ou parenthèse. `$$` = un `$` littéral. |

Exemples `$if` (MP3) : `%author%$if(%title%, - %title%,)%ext%` n'ajoute « - titre » que si le titre existe.
Un `\` dans le résultat désigne un dossier : `%name%\` range chaque fichier dans un dossier à son nom (option *Forcer les dossiers*). `../%folder1% ` remonte d'un niveau.

## 7. Tags EXIF, IPTC, XMP (opération 14)

- **EXIF** (JPEG et fichiers TIFF/RAW) : `DateTimeOriginal`, `DateTimeDigitized`, `DateTime`, `Make`, `Model`, `BodySerialNumber`, `LensMake`, `LensModel`, `ExposureTime`, `FNumber`, `ISOSpeedRatings`, `FocalLength`, `FocalLengthIn35mmFilm`, `ExposureBiasValue`, `ShutterSpeedValue`, `ApertureValue`, `ExposureProgram`, `ExposureMode`, `MeteringMode`, `Flash`, `WhiteBalance`, `SceneCaptureType`, `Orientation`, `ExifImageWidth`, `ExifImageLength`, `XResolution`, `YResolution`, `ResolutionUnit`, `ColorSpace`, `Software`, `Artist`, `Copyright`, `ImageDescription`, `UserComment`, `ExifVersion`, `MakerNote`.
- **IPTC** (JPEG) : `IptcKeywords`, `IptcCaption`, `IptcHeadline`, `IptcObjectName`, `IptcByline`, `IptcBylineTitle`, `IptcCopyright`, `IptcCredit`, `IptcSource`, `IptcCity`, `IptcSublocation`, `IptcState`, `IptcCountry`, `IptcCountryCode`, `IptcDateCreated`, `IptcTimeCreated`, `IptcCategory`, `IptcSupplementalCategories`, `IptcSpecialInstructions`, `IptcUrgency`, `IptcWriter`, `IptcTransmissionReference`.
- **XMP** (JPEG, PNG, WebP, TIFF, DNG, fichiers `.xmp`) : `XmpTitle`, `XmpDescription`, `XmpKeywords`, `XmpHierarchicalKeywords`, `XmpCreator`, `XmpRights`, `XmpRating`, `XmpLabel`, `XmpCreateDate`, `XmpModifyDate`, `XmpDateCreated`, `XmpCreatorTool`, `XmpHeadline`, `XmpCredit`, `XmpSource`, `XmpCity`, `XmpState`, `XmpCountry`, `XmpCountryCode`, `XmpLocation`, `XmpCategory`, `XmpInstructions`, `XmpAuthorsPosition`, `XmpCaptionWriter`, `XmpTransmissionReference`, `XmpLens`.

Bon à savoir : un tag absent donne un texte vide ; les valeurs multiples sont jointes par « , » ; les dates (`aaaa:MM:jj HH:mm:ss`) deviennent `aaaa-MM-jj HH-mm-ss` ; les 512 premiers Ko du fichier sont lus ; le `.xmp` voisin (IMG.xmp, IMG.CR2.xmp) l'emporte sur le XMP intégré. Pour prendre le premier tag disponible : `$if(%XmpTitle%,%XmpTitle%,%IptcObjectName%)%ext%`.

## 8. Ligne de commande

| Paramètre | Effet |
|---|---|
| `-b fichier.arb` | charge un job |
| `-g` | démarre le traitement une fois fichiers et job chargés |
| `-x` | quitte à la fin du traitement |
| `-af` / `-aF` / `-afr` | ajoute des fichiers / des dossiers / le contenu récursif des dossiers qui suivent (jokers `*` `?` acceptés) |
| `--shell chemin` | ajoute le chemin à la fenêtre déjà ouverte (utilisé par le menu de l'Explorateur) |
| `--install-shell` / `--remove-shell` | ajoute / retire l'entrée du menu de l'Explorateur, puis quitte |

Exemple : `antrenamer-qt -b d:\test.arb -af c:\data\*.* -g -x`
Sans `-a`, comportement de la 2.07 : un nom en `.arb` est un job, un dossier est ajouté récursivement.

## 9. Page Journal

Chaque fichier traité donne une ligne : **verte** (renommé, ancien → nouveau nom), **bleue** (non renommé : nom identique, pas de tag, fichier absent), **rouge** (erreur : nom déjà pris, caractère invalide, fichier verrouillé).
Boutons : **Filtrer** (Succès / Erreurs / Non renommés), **Vider**, **Enregistrer** (fichier texte), **Copier** (presse-papiers).
L'option *Stocker le journal* (Options > Événements) l'enregistre automatiquement après chaque renommage en UTF-8, deux colonnes séparées par une tabulation (`v` réussi, `x` erreur), avec option d'ajout à la fin d'un fichier existant.

## 10. Options (F10)

| Page | Réglages |
|---|---|
| **Affichage** | mise à jour en temps réel, redimensionnement automatique des colonnes, icônes selon l'extension, police pour les caractères spéciaux, taille de l'historique des listes déroulantes (0 = aucun), complétion automatique, jeu d'icônes (Scrows, Windows XP, Windows XP Large), **disposition moderne** et sa disposition des pages (Onglets, Écran unique en bandes, Écran unique en colonnes), pris en compte au prochain démarrage |
| **Fichiers & Dossiers** | ne pas demander lors d'un glisser-déposer et options à utiliser, tri naturel, ouvrir le fichier au double-clic, entrée du menu de l'Explorateur |
| **Traitement** | copier au lieu de renommer, générer un journal, heure d'été selon la date du fichier, renommer aussi les fichiers associés (`.xmp`, `.dop` de DxO) ; ils reprennent leur ancien nom à l'annulation, forcer les dossiers (les crée), détecter `x:\` et `\\` comme chemins absolus, dernier point d'un nom de dossier = extension |
| **Événements** | actions au début et à la fin du renommage, stocker le journal |
| **Langue** | langue de l'interface (27 fichiers `.lng`), mode portable, déplacement des réglages entre `%APPDATA%` et le dossier du programme |

Mode portable : un fichier `antrenamer-qt.ini` vide à côté du programme garde tous les réglages dans ce dossier.

## 11. Disposition moderne (optionnelle)

Même moteur, autre présentation, activée dans Options > Affichage (au prochain démarrage). Compteurs « Fichiers (100) », « Opérations (2) » sur les boutons de pages ; barre fine par page ; en écran unique, Ctrl+1/2/3 affiche ou masque un volet ; les séparateurs se déplacent et sont mémorisés ; la disposition en colonnes passe en bandes sous ~900 pixels de large ; opérations du job modifiables par double-clic.

## 12. Limites connues

- EXIF : une quarantaine de tags seulement, MakerNote non décodé ; IPTC lu uniquement dans les JPEG.
- Tags audio : ID3 uniquement.
- Annulation : un seul niveau.
- Le menu court de Windows 11 n'est pas pris en charge (passer par « Afficher plus d'options »).
