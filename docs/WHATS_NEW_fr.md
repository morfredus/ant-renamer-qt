# Ant Renamer Qt : nouveautés depuis la version d'origine

*Ce que Ant Renamer Qt ajoute ou améliore par rapport à Ant Renamer 2.13 (Antoine Potten, Delphi 7).*
Version anglaise : [WHATS_NEW.md](WHATS_NEW.md). Liste tenue à jour au fil du développement. Pour le détail technique : [CHANGELOG](../CHANGELOG.md). Pour le
portage lui-même (ce qui est conservé, les différences voulues) : [PRESENTATION_fr.md](PRESENTATION_fr.md).

Version courante : **3.0.0-rc.9**. Ce qui est marqué *(à venir)* est dans le code mais pas encore publié.

## Ajouts

### Pour renommer

| Nouveauté | Depuis |
|---|---|
| **Jokers dans le remplacement de chaîne** : case « Utiliser les jokers ». `*` = n'importe quel texte, `?` = un caractère (`IMG_*_copie` remplacé par `Photo` : `IMG_001_copie (1).jpg` devient `Photo (1).jpg`). | 3.0.0-rc.9 |
| **Données IPTC et XMP dans l'action EXIF** : mots-clés, titre, légende, auteur, lieu, note, label de couleur... utilisables dans le masque comme les tags EXIF (`%IptcKeywords%`, `%XmpTitle%`, `%XmpRating%`...), lues dans les JPEG (IPTC) et dans tout fichier contenant du XMP (JPEG, PNG, WebP, TIFF, DNG, `.xmp`). L'aide liste tous les tags disponibles. | 3.0.0-rc.9 |
| **Les fichiers associés suivent leur image** : renommer `IMG.CR2` renomme aussi son `.xmp` (`IMG.xmp` ou `IMG.CR2.xmp` : Lightroom, Capture One, digiKam, darktable) et son `.dop` de DxO PhotoLab, et l'annulation les remet. Activé par défaut, Options > Traitement. Le fichier XMP associé est aussi lu par l'action EXIF (mots-clés, note d'un fichier RAW). | 3.0.0-rc.9 |
| **Même numéro pour les paires JPG + RAW** (et XMP) : case optionnelle dans Énumération et EXIF, pour que `IMG_1234.JPG` et `IMG_1234.CR2` reçoivent le même numéro. | 3.0.0-rc.9 |
| **Compteur `%num%` dans l'action EXIF** (départ, chiffres, incrément, retour au départ par dossier), mêmes réglages que l'action Énumération. | 3.0.0-rc.1 |
| **Tags ID3v2.2, 2.3 et 2.4** pour l'action MP3 (l'original ne lisait que l'ID3v1). | 3.0.0-rc.1 |

### Pour travailler dans la liste

| Nouveauté | Depuis |
|---|---|
| **Bouton « Seulement modifiés »** : masque les fichiers dont le nom ne change pas dans l'aperçu (idéal pour contrôler les 2 000 noms concernés dans une liste de 50 000). Simple filtre d'affichage : les fichiers masqués restent dans la liste, sont renommés normalement et ne peuvent pas être touchés par « Retirer la sélection » ou les boutons de déplacement. | 3.0.0-rc.9 |
| **Tri naturel** dans la liste des fichiers (`fichier 2` avant `fichier 10`), activé par défaut ; décochable pour retrouver l'ordre alphabétique d'origine. | 3.0.0-rc.7 |

### Interface

| Nouveauté | Depuis |
|---|---|
| **Disposition moderne**, en option (Options > Affichage) : barre principale avec onglets indiquant le nombre de fichiers et d'actions, bouton Go bien visible, barre propre à chaque page, colonne du nouveau nom visible par défaut, thème clair ou sombre du système. La disposition classique reste celle par défaut. | 3.0.0-rc.8 |
| **Écran unique** : Fichiers, Actions et Journal visibles en même temps, en bandes ou en colonnes, séparateurs mobiles, tailles mémorisées. Idée d'Antoine Potten. | 3.0.0-rc.8 |
| **Modifier une action du job en place** : double-clic sur l'action pour recharger ses réglages dans l'éditeur, puis « Modifier » (ou « Valider / Annuler la modification »). | 3.0.0-rc.8 |
| **Mode portable** : un fichier `antrenamer-qt.ini` à côté du programme garde tous les réglages dans ce dossier. | 3.0.0-rc.1 |
| **Prêt pour Windows 10/11** : chemins de plus de 260 caractères, UTF-8, écrans haute résolution, thème sombre. | 3.0.0-rc.1 |
| **Menu contextuel de l'Explorateur** « Ajouter à Ant Renamer » sur les fichiers et dossiers, envoyé à la fenêtre déjà ouverte. | 3.0.0-rc.1 |

### Plateformes et distribution

| Nouveauté | Depuis |
|---|---|
| **Linux** : paquets `.deb` (amd64, arm64) et AppImage x86_64. | 3.0.0-rc.2 |
| **Aide intégrée** dans une fenêtre du programme (plus de navigateur externe), avec la table des matières de l'aide d'origine. | 3.0.0-rc.3 |
| **Windows** : installeur et zip portable. | 3.0.0-rc.1 |
| Nom officiel **Ant Renamer Qt**, vit en parallèle de l'original (exécutable, réglages, menu Explorateur distincts) ; les jobs `.arb` restent lisibles par les deux programmes. | 3.0.0-rc.2 |

## Améliorations

| Amélioration | Depuis |
|---|---|
| Éditeur d'actions : les longs textes des cases à cocher passent à la ligne quand le panneau est étroit (au lieu d'être coupés avec une barre de défilement). | 3.0.0-rc.9 |
| Traductions : un texte présent deux fois dans les fichiers de langue avec deux traductions différentes recevait l'une ou l'autre au hasard ; le choix est maintenant fixe. | 3.0.0-rc.8 |
| Liste des fichiers : le tri par clic sur un en-tête ne marchait plus après le premier clic. | 3.0.0-rc.7 |
| Expressions régulières en PCRE2, aperçu qui lit le vrai fichier pour les actions date / MP3 / EXIF, changements de casse seule faits correctement sur les volumes insensibles à la casse (voir « Différences voulues » dans la présentation). | 3.0.0-rc.1 |
| Aide : chapitre « Disposition moderne » avec captures d'écran ; pages Fichiers et Actions complétées (anglais, français ; croate et tamoul pour les ajouts récents). | 3.0.0-rc.8 et suivantes |

## Comment tenir ce fichier à jour

Chaque changement visible pour l'utilisateur ajoute une ligne ici, dans « Ajouts » ou « Améliorations », avec
*(à venir)* dans la colonne « Depuis ». À la publication d'une version, remplacer *(à venir)* par son numéro.
Tenir aussi [WHATS_NEW.md](WHATS_NEW.md) (anglais) en phase.
