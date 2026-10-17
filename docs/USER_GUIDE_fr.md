# Ant Renamer Qt : guide des fonctions

*Toutes les fonctions d'Ant Renamer Qt 3.0.0-rc.12, avec le mode d'emploi.*
English version : [USER_GUIDE.md](USER_GUIDE.md). Recettes (cas d'usage pas à pas) : [USER_RECIPES_fr.md](USER_RECIPES_fr.md). Détails complets, page par page, avec captures d'écran : l'aide intégrée (F1). Différences entre Windows et Linux : §13.

## 1. Principe général

Le travail se fait en trois étapes, une page chacune :

1. **Fichiers** : construire la liste des fichiers (et dossiers) à renommer.
2. **Opérations** : choisir une ou plusieurs opérations de renommage (le « job »).
3. **Journal** : lire ce qui s'est passé après avoir cliqué sur **Démarrer**.

L'aperçu en bas de la page Opérations montre en direct le nom actuel et le futur nom du fichier sélectionné. La colonne **Aperçu** de la liste des Fichiers (F5) le montre pour toute la liste.

Le programme ne modifie que les **noms** (ou copie les fichiers sous un nouveau nom) ; il ne touche jamais au contenu ni aux dates des fichiers. Il gère les noms Unicode (grec, japonais, cyrillique...), les fichiers et les dossiers.

## 2. Raccourcis clavier

| Touche | Fonction | Touche | Fonction |
|---|---|---|---|
| F1 | Aide | F9 | Démarrer |
| Maj+F1 | À propos | Échap | Arrêter |
| F2 | Ajouter des fichiers | F10 | Options |
| F3 | Ajouter des dossiers | F11 | Avant-plan |
| F5 | Actualiser l'aperçu | F12 | Annuler |
| F6 / F7 / F8 | Retirer sélectionnés / tous / manquants | Alt+F4 | Quitter |
| Ctrl+1 / 2 / 3 | Page Fichiers / Opérations / Journal | Ctrl+F2 | Ajouter au job |
| Maj+F6 / Maj+F7 | Retirer la sélection / vider le job | Ctrl+O / Ctrl+S | Ouvrir / sauver un job |
| Ctrl+↑ / Ctrl+↓ | Monter / descendre | Ctrl+Maj+↑ / ↓ | Déplacer au début / à la fin |

## 3. Page Fichiers

### Ajouter des fichiers
- **Fichiers** (F2) : sélecteur de fichiers standard (Ctrl et Maj sélectionnent plusieurs fichiers). L'ordre que renvoie le système n'est pas toujours l'ordre affiché : triez la liste ensuite si l'ordre compte.
- **Dossiers** (F3) : affiche les lecteurs et dossiers sous forme d'arbre ; en choisir un ou plusieurs, puis cocher ce qu'il faut ajouter : les fichiers qu'ils contiennent, les dossiers eux-mêmes, les sous-dossiers (tous niveaux). Les trois choix sont mémorisés.
- **Glisser-déposer** depuis le gestionnaire de fichiers (Explorateur, Nautilus, Dolphin, Total Commander...) : une fenêtre propose les mêmes options. « Ne plus afficher cette fenêtre » les mémorise (modifiable dans Options > Fichiers & Dossiers) ; maintenir Ctrl ou Maj en déposant réaffiche la fenêtre une fois.
- **Menu de l'Explorateur** « Ajouter à Ant Renamer Qt » (Windows ; à activer dans Options > Fichiers & Dossiers). Plusieurs éléments sélectionnés arrivent ensemble dans la fenêtre déjà ouverte. Windows 11 : *Afficher plus d'options* (ou Maj+F10).
- **Ligne de commande** : voir §8.

### Gérer la liste
| Bouton | Effet |
|---|---|
| Retirer sélec. (F6) | retire de la liste les fichiers sélectionnés (pas du disque) |
| Retirer tout (F7) | vide la liste |
| Retirer manquants (F8) | retire les fichiers qui n'existent plus sur le disque |
| Aperçu (F5) | remplit la colonne Aperçu (pas automatique, pour la performance). Il utilise le job, ou l'opération affichée si le job est vide |
| Seulement modifiés | masque les fichiers dont le nom ne change pas dans l'aperçu (simple filtre d'affichage : ils sont quand même renommés, et un fichier masqué n'est jamais touché par « Retirer sélec. » ni par les boutons de déplacement) ; actualisez l'aperçu d'abord |
| Haut / Bas / Début / Fin | change l'ordre de traitement (le glisser-déposer dans la liste fonctionne aussi) |

- **Colonnes** : clic droit sur l'en-tête pour afficher Dossier, Nom de fichier, Chemin complet, Ext., Aperçu, Taille, Date de création, Date de modification. Elles se redimensionnent et se déplacent ; une colonne au moins reste visible.
- **Tri** : clic sur un en-tête (à nouveau pour inverser). Par défaut le tri est naturel : « fichier 2 » avant « fichier 10 » (Options > Fichiers & Dossiers redonne l'ordre alphabétique). **Trier réordonne vraiment la liste, donc fixe l'ordre de traitement**, et donc l'ordre de numérotation d'une énumération. Clic droit > *Non trié* retire l'indicateur de tri.
- Clic droit dans la liste : retirer la sélection / tout, tout sélectionner, ne rien sélectionner, inverser la sélection, actualiser l'aperçu.
- Double-clic sur un fichier : l'ouvre avec son programme associé, si « Exécuter le fichier lors d'un double clic » est coché (Options > Fichiers & Dossiers).
- Des dossiers peuvent aussi être dans la liste : ils sont renommés comme les fichiers (dans l'ordre de la liste) ; les fichiers de la liste qui se trouvent dans un dossier renommé le suivent.

### Fichiers annexes d'une photo
Quand une photo est renommée, ses fichiers annexes sont renommés avec elle, sans rien faire de plus :
- **Reconnus** : `.xmp` (Lightroom, Capture One : `IMG.xmp` ; digiKam, darktable : `IMG.CR2.xmp`) et `.dop` de DxO PhotoLab (`IMG.dop`, `IMG.CR2.dop`), extensions en majuscules comprises.
- **Méthode** : dans Options > Traitement, cocher « Renommer aussi les fichiers associés (.xmp, .dop) d'une image renommée » (activé par défaut). Puis renommer la photo comme d'habitude (par exemple `IMG_0001.CR2` → `Paris 001.CR2`) : `IMG_0001.xmp` ou `IMG_0001.CR2.xmp` devient `Paris 001.xmp` ou `Paris 001.CR2.xmp`.
- **Annuler** (F12) : les fichiers annexes retrouvent leur ancien nom avec la photo.
- **Journal** : la ligne de la photo mentionne le fichier annexe renommé.
- **Cas particuliers** : si un fichier portant le nouveau nom existe déjà, le fichier annexe n'est pas touché et le journal le dit (la photo est renommée quand même) ; un fichier annexe qui est aussi dans la liste suit sans être renommé deux fois ; sans effet en mode « Copier plutôt que renommer » et pour les dossiers.
- Le fichier `.xmp` voisin est aussi lu par l'opération EXIF / IPTC / XMP (§7).

## 4. Page Opérations : les 14 opérations

Choisir l'opération dans la liste, régler ses paramètres (la zone *Notes* sous les paramètres rappelle les tags disponibles ; les noms d'autres opérations dans les notes sont des liens, et « voir Aide » ouvre l'aide), puis **Ajouter au job** (Ctrl+F2). Plusieurs opérations peuvent être empilées : elles s'appliquent dans l'ordre, chacune sur le nom déjà modifié par les précédentes (sauf les opérations de métadonnées, qui lisent le vrai fichier). Si le job est vide, **Démarrer** et l'aperçu utilisent l'opération actuellement affichée, une fois.

**Dossiers et extensions.** Par défaut un nom de dossier est traité comme un nom de fichier sans extension : les opérations qui travaillent sur l'extension ne font rien sur un dossier, et `%ext%` est vide. Options > Traitement (« Le dernier "." d'un nom de dossier délimite l'extension ») change cela.

| # | Opération | Usage | Mode d'emploi |
|---|---|---|---|
| 1 | **Changer l'extension** | Remplacer ou ajouter une extension | Saisir la nouvelle extension (sans le point). Champ vide = supprime l'extension. Option : l'ajouter après l'existante au lieu de la remplacer. |
| 2 | **Remplacement de chaîne** | Remplacer un texte par un autre | Champs « Rechercher » / « Remplacer par ». Vide = supprime le texte. Options : remplacer toutes les occurrences (cochée par défaut, sinon seulement la première), sensible à la casse (cochée par défaut), inclure l'extension dans la recherche, appliquer à l'extension plutôt qu'au nom, **jokers** (voir plus bas). |
| 3 | **Remplacement de chaînes multiples** | Plusieurs remplacements d'un coup (accents, etc.) | Grille à 2 colonnes (cherché / remplacé), appliquée ligne après ligne. Ctrl++ ajoute une ligne, Ctrl+- supprime la ligne courante. Options : inclure l'extension, sensible à la casse. Une grille peut être nommée et sauvée (liste « Sets prédéfinis » + *Sauver* ; *Supprimer* enlève un set, *Effacer* vide la grille). Copier/Exporter et Coller/Importer échangent la grille avec Excel ou un éditeur de texte (tabulation entre les colonnes). Un set « Remove west-european accents » est fourni. |
| 4 | **Insertion de chaîne** | Insérer un texte à une position | Texte, position (0 = avant le premier caractère ou après le dernier), depuis le début ou depuis la fin. Option : compter dans l'extension. Tags `%name%`, `%ext%`, `%folderN%`, `%%`. |
| 5 | **Déplacement d'une chaîne** | Déplacer N caractères | Nombre de caractères, position « de » et position « vers » (la seconde compte après le retrait des caractères), chacune depuis le début ou la fin. Ex. `abcdef`, 1 caractère de 2 vers 4 → `abdecf`. |
| 6 | **Suppression de caractères** | Effacer N caractères | Trois modes : à partir d'une position (depuis le début ou la fin), après une chaîne donnée, ou avant une chaîne donnée. Rien n'est supprimé si la chaîne n'est pas trouvée. Option : appliquer à l'extension. |
| 7 | **Énumération** | Numéroter les fichiers | Masque contenant `%num%` (ex. `Fichier %num%%ext%` ; tags `%name%`, `%ext%`, `%folderN%`). Réglages : *Démarrer à*, *Nombre de chiffres* (zéros devant), *Incrémenter de* (départ et incrément peuvent être négatifs). Options : *Retour à la valeur de démarrage* à chaque nouveau dossier (liste triée par dossier), *Même numéro pour les fichiers de même nom* (IMG_1234.JPG + IMG_1234.CR2 : ils doivent se suivre, triez par nom). |
| 8 | **Infos des tags mp3** | Nommer d'après les tags ID3 (v1, v1.1, v2.2, v2.3, v2.4) | Masque avec `%author%`, `%title%`, `%album%`, `%year%`, `%genre%`, `%comm%`, `%track%`, `%name%`, `%ext%`, `%folderN%`. Ex. `%author% - %title%%ext%`. Les valeurs v2 l'emportent, la v1 comble les trous. Les caractères interdits dans un nom de fichier sont toujours remplacés dans les valeurs (`"` → `'`, `?` → `.`, `\ / : * < > \|` → `-`). Option : numéro de piste sur deux chiffres. `$if` disponible (§6). Un fichier sans tag ID3 n'est pas renommé (ligne bleue dans le journal). |
| 9 | **Date et heure** | Nommer d'après la date du fichier | Masque en lettres : `yyyy` `yy`, `mm` `mmm` `mmmm` (mois), `dd` `ddd` `dddd`, `hh` `nn` (minutes) `ss`, `zzz` (millisecondes), `am/pm` `a/p` `ampm` (horloge de 12 h), `f` (nom d'origine), `e` (extension). Tout texte fixe va entre guillemets. Ex. `yyyy'-'mm'-'dd hh'-'nn'-'sse`. Les noms des jours et des mois sont dans la langue du système. Choix de la date de création ou de modification (la date de création manque sur beaucoup de systèmes de fichiers Linux : la date de modification est alors utilisée). **Décalage** en secondes. Option pour ajouter un suffixe aux doublons. Les tags `c`, `ddddd` et `dddddd` du programme d'origine n'existent pas. |
| 10 | **Noms aléatoires** | Générer un nom avec une partie aléatoire | Masque avec `%random%` (+ `%name%`, `%ext%`, `%folderN%`). Type : nombre aléatoire (8 chiffres), « Compteur Windows » (un compteur de millisecondes, donc séquentiel) ou GUID (`{...}`, unique). |
| 11 | **Changement de casse** | Majuscules / minuscules | Quatre choix : majuscule à la première lettre suivant des caractères donnés (espace, tiret, point... à saisir ; le reste passe en minuscules), majuscule à la première lettre du nom, tout en majuscules, tout en minuscules. Garder « utiliser les options régionales » pour convertir les accents (é → É). Options : appliquer aussi à l'extension, ou seulement à l'extension (par défaut l'extension est laissée telle quelle). Sous Linux « a.txt » et « A.txt » sont deux noms différents. |
| 12 | **Noms à partir d'une liste** | Appliquer une liste de noms | Une ligne par fichier, dans l'ordre de la liste (saisie, ou chargée/sauvée avec *Ouvrir...* / *Sauver...* ; *Vider* l'efface). Options : ajouter l'extension d'origine ; utiliser la liste comme extensions. Un fichier sans ligne correspondante, ou avec une ligne vide, n'est pas renommé. |
| 13 | **Expressions rationnelles** | Découpage / réarrangement avancé | *Expression* (PCRE2, insensible à la casse par défaut ; `(?-i)` la rend sensible) et *Nouveau nom* avec `$1`, `$2`..., `$&` (tout ce qui a été trouvé), `${10}`, `\$` pour un dollar littéral. Elle s'applique au nom entier, extension comprise, et à chaque correspondance. Ex. `(.*) - (.*)\.mp3` → `$2 - $1.mp3` échange artiste et titre. Une expression invalide affiche `<RegExp Error>` dans l'aperçu. |
| 14 | **EXIF / IPTC / XMP** | Nommer les photos d'après leurs métadonnées | Masque avec les tags du §7, plus `%num%` (mêmes réglages que l'Énumération) et `$if`. Le **Décalage** en secondes décale les tags de date. Un fichier sans aucune métadonnée n'est pas renommé (« No EXIF info found »). |

#### Jokers dans le remplacement de chaîne
- **Activation** : cocher « Utiliser les jokers (* = n'importe quel texte, ? = n'importe quel caractère) », décoché par défaut. Cela ne concerne que l'opération *Remplacement de chaîne*.
- **Dans le champ « Rechercher »** : `*` représente n'importe quel texte (même vide), `?` exactement un caractère. Tous les autres caractères sont cherchés tels quels (`.`, `(`, `[`... n'ont aucun sens spécial).
- **Le champ « Remplacer par » reste littéral** : `*` et `?` sont écrits tels quels.
- **Le texte trouvé est le plus court possible** : `a*b` dans `a1b2b` trouve `a1b`, pas `a1b2b`. Avec « toutes les occurrences », la recherche continue ensuite dans le reste du nom.
- **Exemple** : chercher `IMG_*_copie`, remplacer par `Photo` : `IMG_001_copie (1).jpg` devient `Photo (1).jpg`.
- Le choix est enregistré dans les fichiers de job (`.arb`) ; un ancien job se charge avec les jokers désactivés. Les autres opérations n'ont pas de jokers : placez avant elles un *Remplacement de chaîne* avec jokers dans le même job. Pour des motifs plus complexes, utilisez l'opération *Expressions rationnelles*.
- À ne pas confondre avec les jokers `*` et `?` de la **ligne de commande** (§8), qui sélectionnent les fichiers à ajouter.

### Gérer le job
Le panneau **Contenu du job** liste les opérations planifiées (case à cocher pour l'afficher/masquer).
- **Ajouter au job** (Ctrl+F2), **Retirer la sélection** (Maj+F6), **Vider** (Maj+F7).
- **Ouvrir** (Ctrl+O) / **Sauver** (Ctrl+S) : fichiers `.arb` (XML), compatibles avec Ant Renamer 2 dans les deux sens.
- Réordonner par glisser-déposer ou avec les 4 boutons de déplacement.
- Dans la disposition classique, une opération ajoutée ne peut pas être modifiée (la retirer et la refaire). Dans la disposition moderne : double-clic pour la recharger dans l'éditeur, puis *Valider la modification* (ou le bouton *Modifier*).

## 5. Démarrer, arrêter, annuler

- **Démarrer** (F9) : renomme dans un fil séparé ; la fenêtre reste utilisable.
- **Arrêter** (Échap) : interrompt le traitement ; les fichiers déjà traités restent renommés.
- **Annuler** (F12) : restaure les anciens noms de la **dernière** exécution seulement (un niveau), fichiers annexes compris.
- **Avant-plan** (F11) : garde la fenêtre au-dessus des autres.
- **Copier plutôt que renommer** (Options > Traitement) : les originaux restent, les copies portent les nouveaux noms.

## 6. Masques de formatage (opérations 4, 7, 8, 10, 14)

| Syntaxe | Signification |
|---|---|
| `%name%` / `%ext%` | nom sans extension / extension avec le point (vide pour un dossier) |
| `%folderN%` | N-ième dossier parent (`%folder1%` = le plus proche). Négatif : depuis la racine (`%folder-1%` = le lecteur sous Windows, le premier dossier sous Linux). |
| `%count%`, `%countr%`, `%countfiles%`, `%countfilesr%`, `%countfolders%`, `%countfoldersr%` | **pour les dossiers de la liste** : nombre d'éléments contenus (`r` = récursif, `files` / `folders` = seulement ceux-là). Sur un fichier le résultat est 0. |
| `%num%` | le compteur (Énumération, EXIF) |
| `%%` | un `%` littéral |
| `$if(cond,alors,sinon)` | (MP3, EXIF) si `cond` n'est pas vide : `alors`, sinon `sinon`. Mettre entre guillemets les valeurs contenant une virgule ou une parenthèse (un guillemet lui-même : `""`) ; les tags ne sont pas développés entre guillemets. `$$` = un `$` littéral. |

Un tag inconnu (faute de frappe) reste tel quel dans le nom, « % » compris : on le remarque dans l'aperçu. Les noms de tags ne tiennent pas compte de la casse.

Exemple `$if` (MP3) : `%author%$if(%title%, - %title%,)%ext%` n'ajoute « - titre » que si le titre existe.
Un délimiteur de dossier dans le résultat déplace le fichier dans ce dossier (l'option *Forcer les dossiers* le crée) : `\` sous Windows, **`/` sous Linux** (il fonctionne aussi sous Windows). `%name%/` met chaque fichier dans un dossier portant son nom ; `../%folder1% ` remonte d'un niveau.

## 7. Tags EXIF, IPTC, XMP (opération 14)

- **EXIF** (fichiers JPEG et TIFF/RAW) : `DateTimeOriginal`, `DateTimeDigitized`, `DateTime`, `Make`, `Model`, `BodySerialNumber`, `LensMake`, `LensModel`, `ExposureTime`, `FNumber`, `ISOSpeedRatings`, `FocalLength`, `FocalLengthIn35mmFilm`, `ExposureBiasValue`, `ShutterSpeedValue`, `ApertureValue`, `ExposureProgram`, `ExposureMode`, `MeteringMode`, `Flash`, `WhiteBalance`, `SceneCaptureType`, `Orientation`, `ExifImageWidth`, `ExifImageLength`, `XResolution`, `YResolution`, `ResolutionUnit`, `ColorSpace`, `Software`, `Artist`, `Copyright`, `ImageDescription`, `UserComment`, `ExifVersion`, `MakerNote`.
- **IPTC** (JPEG) : `IptcKeywords`, `IptcCaption`, `IptcHeadline`, `IptcObjectName`, `IptcByline`, `IptcBylineTitle`, `IptcCopyright`, `IptcCredit`, `IptcSource`, `IptcCity`, `IptcSublocation`, `IptcState`, `IptcCountry`, `IptcCountryCode`, `IptcDateCreated`, `IptcTimeCreated`, `IptcCategory`, `IptcSupplementalCategories`, `IptcSpecialInstructions`, `IptcUrgency`, `IptcWriter`, `IptcTransmissionReference`.
- **XMP** (JPEG, PNG, WebP, TIFF, DNG, fichiers `.xmp`) : `XmpTitle`, `XmpDescription`, `XmpKeywords`, `XmpHierarchicalKeywords`, `XmpCreator`, `XmpRights`, `XmpRating`, `XmpLabel`, `XmpCreateDate`, `XmpModifyDate`, `XmpDateCreated`, `XmpCreatorTool`, `XmpHeadline`, `XmpCredit`, `XmpSource`, `XmpCity`, `XmpState`, `XmpCountry`, `XmpCountryCode`, `XmpLocation`, `XmpCategory`, `XmpInstructions`, `XmpAuthorsPosition`, `XmpCaptionWriter`, `XmpTransmissionReference`, `XmpLens`.

Bon à savoir : un tag absent donne un texte vide ; les valeurs multiples sont jointes par « , » ; les dates (`yyyy:MM:dd HH:mm:ss`) deviennent `yyyy-MM-dd HH-mm-ss` ; seuls les 512 premiers Ko du fichier sont lus ; le `.xmp` voisin (IMG.xmp, IMG.CR2.xmp) l'emporte sur le XMP intégré. Pour prendre le premier tag disponible : `$if(%XmpTitle%,%XmpTitle%,%IptcObjectName%)%ext%`.

## 8. Ligne de commande

| Paramètre | Effet |
|---|---|
| `-b fichier.arb` | charge un job |
| `-g` | démarre le traitement une fois fichiers et job chargés |
| `-x` | quitte à la fin du traitement |
| `-af` / `-aF` / `-afr` | ajoute les fichiers / dossiers / contenu récursif des dossiers qui suivent (jokers `*` `?` acceptés) |
| `--shell chemin` | ajoute le chemin à la fenêtre déjà ouverte (utilisé par le menu de l'Explorateur) |
| `--install-shell` / `--remove-shell` | ajoute / retire l'entrée du menu de l'Explorateur (Windows), puis quitte |

Exemples : `antrenamer-qt.exe -b d:\test.arb -af c:\data\*.* -g -x` (Windows), `antrenamer-qt -b ~/test.arb -af "/data/*" -g -x` (Linux, mettre les jokers entre guillemets pour que le shell ne les développe pas).
Sans `-a`, comportement de la 2.07 : un nom finissant par `.arb` est un job, un dossier est ajouté récursivement.

## 9. Page Journal

Chaque fichier traité donne une ligne : **verte** (renommé, ancien → nouveau nom ; le chemin complet quand le dossier change ; fichiers annexes mentionnés), **bleue** (non renommé : nom identique, tag absent, ligne vide ou fin de liste, fichier manquant), **rouge** (erreur : nom déjà pris, caractère invalide, fichier verrouillé).
Boutons : **Filtrer** (Succès / Erreurs / Non renommés), **Vider**, **Sauver** (fichier texte), **Copier** (presse-papiers).
L'option *Stocker le journal* (Options > Événements) l'enregistre automatiquement après chaque exécution en UTF-8, deux colonnes séparées par une tabulation (`v` succès, `x` erreur), avec une option pour ajouter à un fichier existant.

## 10. Options (F10)

Les changements s'appliquent en cliquant sur OK ; quelques-uns (langue, disposition moderne) au prochain démarrage.

| Page | Réglages |
|---|---|
| **Affichage** | mise à jour en temps réel, redimensionnement automatique des colonnes, icônes selon l'extension, disposition moderne et son arrangement des pages (Onglets, Écran unique en bandes, Écran unique en colonnes), police et taille forcées, taille de l'historique des listes déroulantes (0 = aucun), complétion automatique, **set d'icônes** (§12) |
| **Fichiers & Dossiers** | ne pas demander lors d'un glisser-déposer et options à utiliser, tri naturel, ouvrir le fichier au double-clic, entrée du menu de l'Explorateur (Windows) |
| **Traitement** | copier plutôt que renommer, générer un journal, heure d'été selon la date du fichier, renommer aussi les fichiers associés (`.xmp`, `.dop` de DxO ; ils retrouvent leur ancien nom à l'annulation), forcer les dossiers (les crée), détecter `x:\` et `\\` (Linux : `/`) comme chemins absolus, dernier point d'un nom de dossier = extension |
| **Événements** | au début d'une exécution : basculer vers une page, vider le journal ; à la fin : basculer vers une page, vider la liste de fichiers, vider le job, stocker le journal dans un fichier (avec « ajouter ») |
| **Langue** | langue de l'interface (27 fichiers `.lng` ; traducteur et version affichés), mode portable |

Les réglages sont stockés dans votre profil (`%APPDATA%\AntRenamerQt` sous Windows, `~/.local/share/AntRenamerQt` sous Linux). Mode portable : un fichier `antrenamer-qt.ini` à côté du programme garde tous les réglages dans ce dossier (la case d'Options > Langue le crée ou le supprime ; un fichier vide suffit).

## 11. Disposition moderne (facultative)

Même moteur, autre présentation, activée dans Options > Affichage (au prochain démarrage). Compteurs « Fichiers (100) », « Opérations (2) » sur les boutons de pages ; une fine barre d'outils par page ; en écran unique, Ctrl+1/2/3 affiche ou masque un volet ; les séparateurs se déplacent et sont mémorisés ; la disposition en colonnes retombe en bandes sous ~900 pixels de large ; les opérations du job se modifient par double-clic.

## 12. Icônes, aide et langues

- **Set d'icônes** (Options > Affichage) : quatre sets, **Modern** (par défaut depuis la 3.0 : icônes au trait dessinées par le programme, nettes sur tout écran, thèmes clair et sombre), Scrows, Windows XP et Windows XP Large. L'aide montre les quatre sets côte à côte (page des Options). Le choix s'applique tout de suite.
- **Aide** (F1) : une vingtaine de pages avec captures d'écran, en anglais, français, croate et tamoul (les autres langues voient l'anglais). Les pages écrites pour Ant Renamer Qt peuvent n'exister qu'en anglais et en français ; le croate et le tamoul affichent alors l'anglais.
- **Langues de l'interface** : 27 fichiers de langue. Pour en ajouter ou en corriger une, voir [TRANSLATING_fr.md](TRANSLATING_fr.md).

## 13. Windows et Linux

| Sujet | Windows | Linux |
|---|---|---|
| Délimiteur de dossier dans un nouveau nom | `\` (et `/`) | `/` seulement ; `\` et `:` sont des caractères ordinaires d'un nom |
| Option de chemin absolu | `x:\...` ou `\\serveur\...` | un nom commençant par `/` |
| Renommage de la seule casse (`a.txt` → `A.txt`) | fonctionne (les noms ne tiennent pas compte de la casse) | les deux noms sont des fichiers différents |
| Date de création | conservée par le système de fichiers | souvent absente : la date de modification est utilisée |
| Menu contextuel de l'Explorateur | oui (Windows 11 : *Afficher plus d'options*) | non (utiliser le glisser-déposer ou la ligne de commande) |
| Réglages | `%APPDATA%\AntRenamerQt` | `~/.local/share/AntRenamerQt` |
| Nom du programme | `antrenamer-qt.exe` | `antrenamer-qt` |
| Paquets | installeur et zip portable | `.deb` (amd64, arm64), AppImage |

## 14. Limites connues

- EXIF : une quarantaine de tags seulement, MakerNote non décodée ; IPTC lu seulement dans les JPEG.
- Tags audio : ID3 seulement (MP3) ; FLAC, OGG, M4A ne sont pas lus.
- Annuler : un seul niveau.
- Le menu contextuel court de Windows 11 n'est pas pris en charge (utiliser « Afficher plus d'options »).
- L'aide en croate et en tamoul affiche l'anglais pour les pages écrites pour Ant Renamer Qt.
