# Ant Renamer Qt : nouveautés depuis la version d'origine

*Ce que Ant Renamer Qt ajoute ou améliore par rapport à Ant Renamer 2.13 (Antoine Potten, Delphi 7).*
Version anglaise : [WHATS_NEW.md](WHATS_NEW.md). Liste tenue à jour au fil du développement. Pour le détail technique : [CHANGELOG](../CHANGELOG.md). Pour le
portage lui-même (ce qui est conservé, les différences voulues) : [PRESENTATION_fr.md](PRESENTATION_fr.md).

Version courante : **3.0.0-rc.16**. Ce qui est marqué *(à venir)* est dans le code mais pas encore publié.

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
| **Historique des renommages** : chaque opération du job lancé par Démarrer est une *passe* ; **Annuler** (F12) remonte passe après passe (l'original n'avait qu'un niveau), et le bouton **Historique** (Maj+F12) liste les passes de la session et revient à une passe choisie, en l'annulant avec toutes les suivantes. Toujours linéaire, les 50 dernières passes sont gardées pour la session. Une option fait ouvrir l'historique par le bouton Annuler. | 3.0.0-rc.16 |
| **Recherche de nouvelle version** (désactivée par défaut) : *Options > Affichage > Rechercher une nouvelle version au démarrage*, ou le bouton *Rechercher une mise à jour* de la fenêtre À propos. Une fois par jour, le programme demande à GitHub la liste des versions du projet et propose d'ouvrir la page de téléchargement ; rien d'autre n'est envoyé. | 3.0.0-rc.16 |
| **Fichiers cachés et système** (`desktop.ini`, `.git`...) d'un dossier ajouté : ignorés par défaut (*Options > Fichiers & Dossiers* pour les inclure) ; un bouton **Fichiers cachés** les affiche ou les masque dans la liste, et tant qu'ils sont masqués aucune opération ne les touche. Un avertissement s'affiche avant un renommage s'ils sont affichés et que la liste en contient. | 3.0.0-rc.16 |
| **Bouton « Seulement modifiés »** : masque les fichiers dont le nom ne change pas dans l'aperçu (idéal pour contrôler les 2 000 noms concernés dans une liste de 50 000). Simple filtre d'affichage : les fichiers masqués restent dans la liste, sont renommés normalement et ne peuvent pas être touchés par « Retirer la sélection » ou les boutons de déplacement. | 3.0.0-rc.9 |
| **Tri naturel** dans la liste des fichiers (`fichier 2` avant `fichier 10`), activé par défaut ; décochable pour retrouver l'ordre alphabétique d'origine. | 3.0.0-rc.7 |

### Interface

| Nouveauté | Depuis |
|---|---|
| **Recherche de nouvelle version** (désactivée par défaut) : une fois par jour, ou à la demande depuis la fenêtre À propos, le programme demande à GitHub s'il existe une version plus récente et propose d'ouvrir sa page. Rien d'autre n'est envoyé. | 3.0.0-rc.16 |
| **Thème clair / sombre** : un bouton de la barre d'outils change aussitôt, et *Options > Affichage > Thème* peut suivre le système, forcer le clair ou forcer le sombre (le journal reste lisible dans les deux). | 3.0.0-rc.14 |
| **Jeu d'icônes « Modern »** (par défaut) : 30 icônes au trait dessinées par le programme, nettes sur tout écran et toute mise à l'échelle, thèmes clair et sombre, grisées quand elles sont désactivées. Options > Affichage > Set d'icônes le propose avec les trois sets classiques (Scrows, Windows XP, Windows XP Large) ; l'aide compare les quatre. | 3.0.0-rc.12 |
| **Aide réécrite et vérifiée contre le programme** : pages Markdown (UTF-8, une table des matières par langue), chaque option et opération décrite, tableaux des tags et des raccourcis clavier, nouvelles captures du vrai programme en anglais et en français, liens entre pages et sections vérifiés. Le croate et le tamoul affichent le texte anglais pour les pages réécrites. | 3.0.0-rc.12 |
| **Disposition moderne**, en option (Options > Affichage) : barre principale avec onglets indiquant le nombre de fichiers et d'actions, bouton Go bien visible, barre propre à chaque page, colonne du nouveau nom visible par défaut, thème clair ou sombre du système. La disposition classique reste celle par défaut. | 3.0.0-rc.8 |
| **Écran unique** : Fichiers, Actions et Journal visibles en même temps, en bandes ou en colonnes, séparateurs mobiles, tailles mémorisées. Idée d'Antoine Potten. | 3.0.0-rc.8 |
| **Modifier une action du job en place** : double-clic sur l'action pour recharger ses réglages dans l'éditeur, puis « Modifier » (ou « Valider / Annuler la modification »). | 3.0.0-rc.8 |
| **Mode portable** : un fichier `antrenamer-qt.ini` à côté du programme garde tous les réglages dans ce dossier. | 3.0.0-rc.1 |
| **Prêt pour Windows 10/11** : chemins de plus de 260 caractères, UTF-8, écrans haute résolution, thème sombre. | 3.0.0-rc.1 |
| **Menu contextuel de l'Explorateur** « Ajouter à Ant Renamer Qt » sur les fichiers et dossiers, envoyé à la fenêtre déjà ouverte. | 3.0.0-rc.1 |

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
| **Robustesse** : une expression régulière invalide ne renomme rien, des tags MP3 abîmés (année vide, en-tête corrompu) ne coupent plus les noms et ne font plus planter, les boutons sont désactivés pendant un renommage, `*.*` accepte aussi les fichiers sans extension, un fichier de langue en erreur est signalé au lieu d'être ignoré en silence (l'allemand est maintenant entièrement traduit). | 3.0.0-rc.13 |
| **Noms de fichiers sous Linux** : un nom contenant deux-points ou antislash (`a:b.txt`) était coupé à ce caractère. Sous Linux seul `/` délimite un chemin maintenant ; les fichiers d'un dossier renommé le suivent, `%folder-N%` et les tags `%count%` fonctionnent, et « Détecter un chemin absolu » reconnaît un nom qui commence par `/`. | 3.0.0-rc.12 |
| **Guide du traducteur** (`docs/TRANSLATING_fr.md`) : tout le processus d'un fichier `.lng`, de la copie de `English.lng` à la pull request. | 3.0.0-rc.12 |
| **Guide d'utilisation et recettes mis à jour** : différences Windows / Linux, options de chaque opération, nouveau jeu d'icônes. | 3.0.0-rc.12 |
| Éditeur d'actions : les longs textes des cases à cocher passent à la ligne quand le panneau est étroit (au lieu d'être coupés avec une barre de défilement). | 3.0.0-rc.9 |
| Traductions : un texte présent deux fois dans les fichiers de langue avec deux traductions différentes recevait l'une ou l'autre au hasard ; le choix est maintenant fixe. | 3.0.0-rc.8 |
| Liste des fichiers : le tri par clic sur un en-tête ne marchait plus après le premier clic. | 3.0.0-rc.7 |
| Expressions régulières en PCRE2, aperçu qui lit le vrai fichier pour les actions date / MP3 / EXIF, changements de casse seule faits correctement sur les volumes insensibles à la casse (voir « Différences voulues » dans la présentation). | 3.0.0-rc.1 |
| Aide : chapitre « Disposition moderne » avec captures d'écran ; pages Fichiers et Actions complétées (anglais, français ; croate et tamoul pour les ajouts récents). | 3.0.0-rc.8 et suivantes |

## Comment tenir ce fichier à jour

Chaque changement visible pour l'utilisateur ajoute une ligne ici, dans « Ajouts » ou « Améliorations », avec
*(à venir)* dans la colonne « Depuis ». À la publication d'une version, remplacer *(à venir)* par son numéro.
Tenir aussi [WHATS_NEW.md](WHATS_NEW.md) (anglais) en phase.
