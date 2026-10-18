# Formatage de chaînes

Pour certaines opérations, vous pouvez utiliser des tags spéciaux qui sont délimités par des caractères "%". Ils sont disponibles dans les opérations qui utilisent un masque : [Insertion de chaîne](actions_fr.md#stringinsert), [Énumération](actions_fr.md#enum), [Infos des tags mp3](actions_fr.md#mp3), [Noms aléatoires](actions_fr.md#random) et [Infos EXIF](actions_fr.md#exif). (L'opération [Date et heure](actions_fr.md#datetime) a son propre système de lettres.)

Étant donné que le "%" est utilisé comme caractère spécial, pour obtenir un simple "%" dans le nom de fichier vous devez entrer "%%".

Dans chaque opération qui supporte des tags spéciaux, les tags disponibles sont listés dans la zone "Notes" en dessous des paramètres de cette opération.

Par exemple, l'opération d'[Infos des tags mp3](actions_fr.md#mp3) supporte des tags relatifs à l'ID3, comme %author% et %title% en plus de %name% et %ext%. Donc un masque typique pour les fichiers mp3 serait quelque chose comme `%author% - %title%%ext%` (%ext% inclut le point, donc nous ne devons pas en mettre un dans le masque utilisé ici).

## Les tags

| Tag | Signification | Disponible dans |
|---|---|---|
| `%name%` | Nom original du fichier, sans extension | toutes |
| `%ext%` | Extension originale, point compris (vide pour un dossier) | toutes |
| `%folderN%` | Nom du N-ième dossier parent, voir [plus bas](#folder) | toutes |
| `%count%` et dérivés | Nombre d'éléments d'un dossier, voir [plus bas](#count) | toutes (dossiers renommés) |
| `%%` | Un caractère "%" | toutes |
| `%num%` | Le compteur | Énumération, EXIF |
| `%author%`, `%title%`, `%album%`, `%year%`, `%genre%`, `%comm%`, `%track%` | Tags ID3 | mp3 |
| `%random%` | La partie aléatoire | Noms aléatoires |
| `%DateTimeOriginal%`, `%Make%`, `%IptcKeywords%`, `%XmpTitle%`... | Données EXIF, IPTC et XMP, voir [la liste](metadata_tags_fr.md) | EXIF |
| `$if(...)` | Texte conditionnel, voir [plus bas](#if) | mp3, EXIF |

Les tags doivent être écrits exactement comme dans le tableau (en minuscules : `%name%`, pas `%Name%`) ; seuls les tags EXIF, IPTC et XMP sont reconnus quelle que soit leur casse. Un tag inconnu (par exemple à cause d'une faute de frappe) est laissé tel quel dans le nom, caractères "%" compris : c'est ainsi que vous le remarquez dans l'aperçu.

## Le tag folder {#folder}

Un tag %folderN% est souvent disponible. Ce tag vous permet d'utiliser le nom des dossiers parents dans le nom de fichier lui-même. C'est un peu compliqué à expliquer, je vais donc vous donner quelques exemples.

Si vous avez un fichier dans "c:\\data\\images\\movies\\poster.jpg" et que vous utilisez l'opération [Insertion de chaîne](actions_fr.md#stringinsert) :

- Si masque = `%folder1% - `, le fichier sera renommé en "movies - poster.jpg"
- Si masque = `%folder2% - `, le fichier sera renommé en "images - poster.jpg"
- etc.

Vous pouvez aussi utiliser un chemin relatif, par exemple si vous avez plusieurs dossiers, contenant chacun beaucoup de fichiers, et que tous ces fichiers ne sont pas spécialement uniques entre deux dossiers différents. Vous pouvez vouloir insérer le nom de dossier et ensuite déplacer tous les fichiers vers le dossier parent commun. Voici un exemple ; vous avez les fichiers suivants :

- c:\\data\\images\\movies\\poster1.jpg
- c:\\data\\images\\games\\poster1.jpg
- c:\\data\\images\\games\\poster2.jpg

Si vous les renommez avec l'opération Insertion de chaîne et une chaîne comme `../%folder1% ` insérée à la position 0 à partir du début, vous obtiendrez ceci dans le dossier "images" :

- movies poster1.jpg
- games poster1.jpg
- games poster2.jpg

Vous pouvez maintenant supprimer les dossiers vides "movies" & "games".

Si vous entrez un nombre négatif à la place, le compte sera fait à partir de la racine plutôt qu'à partir du dossier courant. Donc en utilisant toujours "c:\\data\\images\\movies\\" comme dossier :

- `%folder-1%` = "c"
- `%folder-2%` = "data"
- `%folder-3%` = "images"
- etc.

En utilisant le tag %name%, vous pouvez aussi vouloir placer les fichiers dans un dossier qui prend le nom du fichier. Pour cela, l'[option « Forcer les dossiers »](options_fr.md#processing) doit être activée. Si vous avez les fichiers suivants :

- voitures.doc
- voitures.xls
- voitures.jpg
- maisons.doc
- maisons.xls
- maisons.jpg

Toujours avec l'opération Insertion de chaîne, en utilisant une chaîne comme `%name%\` insérée à la position 0 à partir du début, vous obtiendrez les trois fichiers "voitures" dans un dossier "voitures", et les trois fichiers "maisons" dans un dossier "maisons".

(Sous Linux, écrivez "/" au lieu de "\\" comme délimiteur de dossier, par exemple `%name%/` : l'antislash est un caractère ordinaire dans un nom de fichier Linux. Le "/" fonctionne aussi sous Windows.)

## Les tags count {#count}

Lors du renommage de dossiers, `%count%` et quelques tags dérivés sont disponibles :

- `%count%` = nombre de fichiers et sous-dossiers contenus dans le dossier renommé
- `%countr%` = pareil mais compte aussi ce qui est dans les sous-dossiers (récursif)
- `%countfiles%` = ne compte que les fichiers
- `%countfilesr%` = pareil mais inclut les fichiers des sous-dossiers (récursif)
- `%countfolders%` = ne compte que les sous-dossiers
- `%countfoldersr%` = pareil mais inclut les sous-dossiers des sous-dossiers (récursif)

## La fonction $if {#if}

Pour les opérations [Infos des tags mp3](actions_fr.md#mp3) et [Infos EXIF](actions_fr.md#exif), il y a aussi un tag spécial : $if. Il permet de faire du formatage conditionnel. Par exemple, quand un champ est vide vous pouvez ne pas vouloir afficher le " - " qui le suit. Ou mettre une valeur spéciale.

La syntaxe est la suivante : $if(condition,valeur-si-vrai,valeur-si-faux). Si "condition" n'est pas une chaîne vide, valeur-si-vrai sera utilisée. Dans l'autre cas, valeur-si-faux sera utilisée. Comme vous pouvez avoir besoin d'utiliser des virgules et des parenthèses dans la fonction $if, vous pouvez mettre les valeurs entre guillemets ("...") pour éviter qu'elles ne soient analysées. Mais faites attention : ils empêchent aussi l'analyse du %tag%. Pour obtenir un guillemet lui-même, tapez-le deux fois (""). Un "$" qui n'est pas suivi de "if(" est conservé comme caractère ; "$$" donne un "$".

Voici quelques exemples qui l'expliqueront mieux. Dans la liste de fichiers il y a deux fichiers mp3 qui ont un mauvais nom (1.mp3 et 2.mp3) mais qui ont les informations ID3 valides suivantes :

- 1.mp3 : Auteur = Blur, Titre = Song 2, Album est vide, Piste est vide
- 2.mp3 : Auteur = Vivaldi, Titre est vide, Album = Le Quattro Stagioni, Piste = 1

Voici le résultat avec divers masques :

`%author% - %title%%ext%`  

Prend simplement l'auteur et le titre, séparés par un tiret. Ici nous n'utilisons pas la fonction if, et le résultat n'est pas très joli quand il y a des champs vides.

- Blur - Song 2.mp3
- Vivaldi - .mp3

`%author%$if(%title%, - %title%,)%ext%`  

Prend l'auteur et, si le titre n'est pas vide, ajoute un tiret et le titre.

- Blur - Song 2.mp3
- Vivaldi.mp3

`%author%$if(%album%," ("%album%")",)$if(%title%,", "%title%,)%ext%`  

Prend l'auteur, s'il existe ajoute le nom de l'album entre parenthèses, puis s'il existe ajoute le titre séparé par une virgule. Vous voyez ici que les parenthèses et la virgule doivent être mises entre guillemets pour ne pas être prises comme "fin du if" ou séparateur de valeurs.

- Blur, Song 2.mp3
- Vivaldi (Le Quattro Stagioni).mp3

`%author%$if(%album%," ("%album%")",)$if(%title%,", "%title%,$if(%track%, - %track%,))%ext%`  

Pareil que le précédent, mais à l'intérieur du if du titre on ajoute un autre if pour ajouter le numéro de piste seulement dans le cas où le titre est vide.

- Blur, Song 2.mp3
- Vivaldi (Le Quattro Stagioni) - 1.mp3
