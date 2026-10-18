# Tags EXIF, IPTC et XMP

L'action [Utiliser les informations EXIF](actions_fr.md#exif) lit les informations stockées dans les fichiers image et permet de les utiliser dans le masque, comme n'importe quel `%tag%` (voir [« Formatage »](format_fr.md)). Trois types de métadonnées sont lus :

- **EXIF** : ce qu'écrit l'appareil photo (date, appareil, exposition...). Lu dans les fichiers JPEG et ceux basés sur TIFF (dont la plupart des RAW : DNG, NEF, CR2, ARW).
- **IPTC** : ce qui est saisi dans un logiciel de gestion de photos (mots-clés, légende, auteur, lieu...). Les tags commencent par `Iptc`. Lu dans les fichiers JPEG.
- **XMP** : l'équivalent moderne de l'IPTC, écrit par Lightroom, Capture One, Photoshop, digiKam... Les tags commencent par `Xmp`. Lu dans tout fichier contenant un bloc XMP (JPEG, PNG, WebP, TIFF, DNG...) et dans les fichiers `.xmp`.

Les noms de tags ne tiennent pas compte des majuscules. Un tag absent du fichier donne un texte vide ; l'action n'indique « Pas d'info EXIF trouvée » que si le fichier ne contient aucun des trois types de métadonnées.

## À savoir

- **Même information en IPTC et en XMP.** Lightroom et la plupart des gestionnaires de photos écrivent les deux ; d'autres n'écrivent que du XMP. Utilisez `$if` pour prendre le premier qui existe, par ex. `$if(%XmpTitle%,%XmpTitle%,%IptcObjectName%)%ext%`.
- **Plusieurs valeurs** (mots-clés, auteurs) : elles sont jointes par une virgule et une espace. Utilisez `$if` pour éviter un séparateur en trop quand un tag est vide, par ex. `%DateTimeOriginal%$if(%XmpKeywords%, - %XmpKeywords%,)%ext%`.
- **Dates.** Les dates EXIF, IPTC et XMP sont toutes données sous la forme `aaaa:MM:jj HH:mm:ss`. Les caractères interdits dans un nom de fichier (comme « : ») sont remplacés par « - » : `2024:05:17 14:30:05` donne `2024-05-17 14-30-05`. Le réglage « Décalage » de l'action décale tous les tags date ayant cette forme.
- **Codes.** Des tags comme `Flash`, `Orientation` ou `MeteringMode` donnent le nombre stocké dans le fichier (voir la spécification EXIF pour la signification de chaque valeur).
- **Caractères interdits.** Les valeurs sont nettoyées comme tout nom de fichier : `/ \ : * ? " < > |` sont remplacés.
- **Fichiers XMP associés.** Le fichier `.xmp` à côté d'une image (`IMG.xmp` ou `IMG.CR2.xmp`) est lu avec l'image et l'emporte sur le XMP intégré : les mots-clés, le titre et la note donnés à un fichier RAW dans votre logiciel photo sont utilisables. Le fichier associé (et un fichier `.dop` de DxO) est aussi renommé avec l'image (Options, Traitement).
- **Limites.** Seuls les 512 premiers Ko de chaque fichier sont examinés. L'IPTC n'est lu que dans les JPEG.

## Tags EXIF {#exif}

| Tag | Signification |
|---|---|
| `%DateTimeOriginal%` | Date et heure de la prise de vue |
| `%DateTimeDigitized%` | Date et heure de numérisation |
| `%DateTime%` | Date et heure de la dernière modification de l'image, écrites par le logiciel qui l'a retouchée (pas la date du fichier sur le disque) |
| `%Make%` | Fabricant de l'appareil |
| `%Model%` | Modèle de l'appareil |
| `%BodySerialNumber%` | Numéro de série de l'appareil |
| `%LensMake%` | Fabricant de l'objectif |
| `%LensModel%` | Modèle de l'objectif |
| `%ExposureTime%` | Temps de pose, sous la forme 1/250 pour les vitesses rapides |
| `%FNumber%` | Ouverture (nombre f), par ex. 2.8 |
| `%ISOSpeedRatings%` | Sensibilité ISO |
| `%FocalLength%` | Focale en mm |
| `%FocalLengthIn35mmFilm%` | Focale en mm, équivalent 24x36 |
| `%ExposureBiasValue%` | Correction d'exposition (IL) |
| `%ShutterSpeedValue%` | Vitesse d'obturation, valeur APEX |
| `%ApertureValue%` | Ouverture, valeur APEX |
| `%ExposureProgram%` | Programme d'exposition (code) |
| `%ExposureMode%` | Mode d'exposition (code) |
| `%MeteringMode%` | Mode de mesure (code) |
| `%Flash%` | État du flash (code ; 0 = n'a pas déclenché) |
| `%WhiteBalance%` | Balance des blancs (0 = auto, 1 = manuelle) |
| `%SceneCaptureType%` | Type de scène (code) |
| `%Orientation%` | Orientation de l'image (1 à 8) |
| `%ExifImageWidth%` | Largeur en pixels |
| `%ExifImageLength%` | Hauteur en pixels |
| `%XResolution%` | Résolution horizontale |
| `%YResolution%` | Résolution verticale |
| `%ResolutionUnit%` | Unité de résolution (2 = pouce, 3 = cm) |
| `%ColorSpace%` | Espace colorimétrique (1 = sRGB) |
| `%Software%` | Logiciel ayant écrit le fichier |
| `%Artist%` | Photographe (champ EXIF) |
| `%Copyright%` | Copyright (champ EXIF) |
| `%ImageDescription%` | Description (champ EXIF) |
| `%UserComment%` | Commentaire utilisateur (souvent vide ou illisible) |
| `%ExifVersion%` | Version d'EXIF |
| `%MakerNote%` | Notes du fabricant (rarement exploitable) |

## Tags IPTC {#iptc}

| Tag | Signification |
|---|---|
| `%IptcKeywords%` | Mots-clés (plusieurs valeurs jointes par une virgule) |
| `%IptcCaption%` | Légende / description |
| `%IptcHeadline%` | Titre court (headline) |
| `%IptcObjectName%` | Titre (nom de l'objet) |
| `%IptcByline%` | Auteur / photographe |
| `%IptcBylineTitle%` | Fonction de l'auteur |
| `%IptcCopyright%` | Mention de copyright |
| `%IptcCredit%` | Crédit |
| `%IptcSource%` | Source |
| `%IptcCity%` | Ville |
| `%IptcSublocation%` | Lieu précis dans la ville |
| `%IptcState%` | Région / état |
| `%IptcCountry%` | Nom du pays |
| `%IptcCountryCode%` | Code du pays (par ex. FRA) |
| `%IptcDateCreated%` | Date de création, sous la forme 2024:05:17 |
| `%IptcTimeCreated%` | Heure de création, sous la forme 14:30:05 |
| `%IptcCategory%` | Catégorie |
| `%IptcSupplementalCategories%` | Catégories supplémentaires |
| `%IptcSpecialInstructions%` | Instructions spéciales |
| `%IptcUrgency%` | Urgence (1 à 8) |
| `%IptcWriter%` | Rédacteur de la légende |
| `%IptcTransmissionReference%` | Identifiant de travail (référence de transmission) |

## Tags XMP {#xmp}

| Tag | Signification |
|---|---|
| `%XmpTitle%` | Titre (dc:title) |
| `%XmpDescription%` | Description (dc:description) |
| `%XmpKeywords%` | Mots-clés (dc:subject, joints par une virgule) |
| `%XmpHierarchicalKeywords%` | Hiérarchie des mots-clés de Lightroom, par ex. Lieux\|France\|Paris |
| `%XmpCreator%` | Auteur / photographe (dc:creator) |
| `%XmpRights%` | Copyright (dc:rights) |
| `%XmpRating%` | Note de 0 à 5 étoiles (-1 = rejetée) |
| `%XmpLabel%` | Label de couleur (par ex. Rouge) |
| `%XmpCreateDate%` | Date et heure de création, sous la forme 2024:05:17 14:30:05 |
| `%XmpModifyDate%` | Date et heure de dernière modification, même forme |
| `%XmpDateCreated%` | Date de création du contenu (photoshop:DateCreated) |
| `%XmpCreatorTool%` | Logiciel ayant créé le fichier |
| `%XmpHeadline%` | Titre court (headline) |
| `%XmpCredit%` | Crédit |
| `%XmpSource%` | Source |
| `%XmpCity%` | Ville |
| `%XmpState%` | Région / état |
| `%XmpCountry%` | Nom du pays |
| `%XmpCountryCode%` | Code du pays |
| `%XmpLocation%` | Lieu précis (sublocation) |
| `%XmpCategory%` | Catégorie |
| `%XmpInstructions%` | Instructions spéciales |
| `%XmpAuthorsPosition%` | Fonction de l'auteur |
| `%XmpCaptionWriter%` | Rédacteur de la légende |
| `%XmpTransmissionReference%` | Identifiant de travail |
| `%XmpLens%` | Description de l'objectif (aux:Lens) |
