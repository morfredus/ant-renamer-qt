# Ant Renamer Qt : nouveautés et assistance

**Ant Renamer Qt** est un portage C++/Qt 6 d'Ant Renamer 2.13 d'Antoine Potten, réalisé par Frédéric Biron. Il est développé en parallèle du programme d'origine : il a son propre exécutable (`antrenamer-qt`), son dossier d'installation et ses réglages, donc les deux peuvent être installés côte à côte. Les fichiers de job (`.arb`) sont les mêmes : chaque programme lit les jobs de l'autre. Le reste de cette aide décrit Ant Renamer Qt ; elle est partie de l'aide d'Ant Renamer 2.13 et a été mise à jour. Ce qui est nouveau, par rapport au programme d'origine, est listé ci-dessous.

## Nouveautés

- **Windows récents et Linux** : Windows 10 et 11 (chemins longs, écrans haute résolution, thème sombre) et Linux (paquets `.deb` amd64 et arm64, AppImage).
- **Tags ID3v2** : l'opération ["Infos des tags mp3"](actions_fr.md) lit les tags ID3v2.2, 2.3 et 2.4 en plus de l'ID3v1. Les valeurs ID3v2 sont utilisées en priorité.
- **Compteur dans l'opération EXIF** : l'opération ["Infos EXIF"](actions_fr.md) a un tag `%num%` avec les mêmes réglages que l'énumération (départ, chiffres, incrément, retour au départ dans chaque dossier), voir [formatage de chaînes](format_fr.md).
- **Menu contextuel de l'Explorateur Windows** : *Options* \> *Fichiers & Dossiers* ajoute "Ajouter à Ant Renamer Qt" au menu du clic droit sur les fichiers et dossiers. Tout ce que vous sélectionnez est envoyé dans la fenêtre déjà ouverte. Sous Windows 11, l'entrée est dans "Afficher plus d'options" (ou Maj+F10).
- **Mode portable** : les réglages sont enregistrés dans le fichier `antrenamer-qt.ini`. Quand ce fichier est à côté du programme, tout reste dans ce dossier (clé USB, aucune trace sur l'ordinateur). Sinon il est dans votre profil utilisateur. La case des *Options* \> *Langue* déplace les réglages de l'un à l'autre. Les anciens réglages `Renamer.xml` ne sont pas importés.
- **Plus d'options** : jeu d'icônes (Scrows, Windows XP, Windows XP Large, et le nouveau jeu vectoriel « Modern »), police forcée, largeur automatique des colonnes, appliqués immédiatement ; les barres d'outils sont déplaçables ; la liste des langues indique version, auteurs et commentaires.
- **Aide intégrée** : cette fenêtre, avec la table des matières de l'aide d'origine.
- **Ligne de commande** : voir [paramètres en ligne de commande](params_fr.md) (`--shell`, `--install-shell`, `--remove-shell`).
- **Disposition moderne de l'interface (option)** : une autre présentation de la fenêtre principale, avec les trois pages sur un seul écran si vous préférez (en bandes ou en colonnes), et des opérations du job que l'on peut modifier. La disposition classique reste celle par défaut. Voir [disposition moderne de l'interface](modernui_fr.md).
- **Tri naturel** : la liste des fichiers trie « fichier 2 » avant « fichier 10 » par défaut ; l'ordre alphabétique du programme d'origine est une option (*Options* \> *Fichiers & Dossiers*).
- **IPTC et XMP dans l'opération EXIF** : mots-clés, titre, légende, auteur, lieu, note... utilisables dans le masque (`%IptcKeywords%`, `%XmpTitle%`...), voir [Tags EXIF, IPTC et XMP](metadata_tags_fr.md). Le fichier XMP associé à une image est lu aussi.
- **Les fichiers associés suivent leur image** : renommer une image renomme aussi ses fichiers `.xmp` et `.dop` de DxO (IMG.xmp, IMG.CR2.xmp, IMG.dop...), et l'annulation les remet ([Options](options_fr.md) \> *Traitement*, activé par défaut).
- **Jokers** dans le [remplacement de chaîne](actions_fr.md#stringrepl) (`*` et `?`), et une option pour donner **le même numéro aux fichiers de même nom** et d'extensions différentes (JPG + RAW) dans les opérations Énumération et EXIF.
- **Bouton "Seulement modifiés"** dans la [liste des fichiers](files_fr.md) : masque les fichiers dont le nom ne change pas dans l'aperçu.

## Corrections par rapport à la 2.13

- Un tag ID3 ou EXIF vide reste vide. Avant, il était remplacé par un nombre, donc `$if(%title%,...)` était toujours vrai.
- Un `$` isolé dans un masque ne bloque plus le renommage.
- Le réglage "Retour à la valeur de démarrage" de l'énumération est correctement enregistré dans les jobs.
- L'aperçu des opérations date, mp3 et EXIF fonctionne quand elles suivent une autre opération.
- Les noms contenant des caractères hors de la page de code du système ne sont plus abimés par l'opération "Date et heure".

## Petites différences à connaître

- Les expressions rationnelles utilisent le moteur PCRE2 (Qt) à la place de TRegExpr : des constructions très inhabituelles peuvent se comporter différemment.
- Le lecteur EXIF connaît 36 tags EXIF courants (plus ceux de l'IPTC et du XMP), pas toute la liste du programme d'origine.

## Assistance

L'assistance pour Ant Renamer Qt se fait sur GitHub : [github.com/morfredus/ant-renamer-qt](https://github.com/morfredus/ant-renamer-qt/)

- **Issues** : pour signaler un bug ou demander une fonction. Indiquez la version du programme (fenêtre *À propos*), votre système (Windows ou Linux, version) et les étapes pour reproduire le problème.
- **Discussions** : pour les questions, l'aide et les idées.

Merci de ne pas signaler les problèmes d'Ant Renamer Qt à l'auteur du programme d'origine : Ant Renamer 2.x est supporté par son auteur sur [www.antp.be/software/renamer/fr](https://www.antp.be/software/renamer/fr).
