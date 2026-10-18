# Ce que le programme fait...

Ce programme est fait pour renommer des fichiers et des dossiers. Il se contentera donc de toucher à leurs noms, il ne modifiera pas le contenu des fichiers.

Il a été fait pour renommer, mais dans les [Options](options_fr.md#processing) vous pouvez demander qu'il copie les fichiers au lieu de les renommer.

Il peut [stocker une liste prédéfinie d'opérations](batch_fr.md) pour les réutiliser plus tard, et la lancer depuis la [ligne de commande](params_fr.md).

Il supporte l'unicode, ce qui signifie que vous pouvez renommer des fichiers qui ont des caractères grecs, japonais, chinois, arabes ou n'importe quelle autre langue. Comme la plupart des programmes ont des problèmes avec ce genre de noms, il peut être utile de les renommer vers des caractères latins (ASCII de base) par exemple.

Il lit les informations stockées à l'intérieur de certains fichiers pour construire les nouveaux noms :

- les tags ID3 des fichiers MP3 (ID3v1, ID3v1.1 et ID3v2.2, 2.3 et 2.4) ;
- les données EXIF, IPTC et XMP des images (JPEG, et fichiers basés sur TIFF comme la plupart des fichiers RAW ; XMP aussi depuis les PNG, WebP... et depuis les fichiers `.xmp`) ;
- la date et l'heure des fichiers.

Quand une image est renommée, ses fichiers associés (`.xmp`, `.dop` de DxO) peuvent être renommés avec elle, voir les [options](options_fr.md#processing).

Ant Renamer Qt fonctionne sous Windows et sous Linux.

# ... et ce qu'il ne fait pas

Comme je l'ai dit, il ne modifiera pas le contenu des fichiers.

Il ne va pas non plus modifier la date et l'heure des fichiers.

Il ne modifie pas les tags ID3 contenus dans les MP3 ni les infos EXIF des JPEG ; il va seulement lire ces infos pour construire un nouveau nom de fichier.

Il ne lit aucune info de fichiers autres que les MP3 et les images décrits plus haut.

Il ne peut pas être entièrement contrôlé depuis la ligne de commande, mais il supporte quelques [paramètres](params_fr.md).

Notez que le contenu de cette liste n'est pas définitif. À part pour ce qui est de la modification du contenu des fichiers, toutes les choses qu'il ne fait pas peuvent apparaître dans les versions futures.

## Limitations connues

- Le bouton Annuler n'a qu'un niveau : il ne restaure que le dernier renommage.
- EXIF : 36 tags sont lus, et la MakerNote (données privées des fabricants d'appareils) n'est pas décodée. L'IPTC n'est lu que dans les fichiers JPEG. Voir [Tags EXIF, IPTC et XMP](metadata_tags_fr.md).
- Les tags audio sont les tags ID3 des fichiers MP3 ; les autres formats (FLAC, OGG, M4A...) ne sont pas lus.
- La date de création d'un fichier n'est pas conservée par tous les systèmes de fichiers (elle manque souvent sous Linux) : la date de modification est alors utilisée.
- L'aide existe en anglais, français, croate et tamoul ; l'interface elle-même existe en 28 langues (l'anglais et 27 traductions). Les pages écrites pour Ant Renamer Qt (disposition moderne, tags, fichiers associés...) peuvent n'exister qu'en anglais et en français, les autres langues affichent alors le texte anglais.
- Windows 11 : l'entrée ajoutée au menu contextuel de l'Explorateur est dans le menu « Afficher plus d'options » (ou Maj+F10), le menu court n'est pas pris en charge.
