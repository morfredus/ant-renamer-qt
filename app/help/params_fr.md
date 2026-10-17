# Paramètres en ligne de commande

Le programme ne supporte pas (encore) beaucoup de paramètres. Il n'est pas vraiment possible de renommer des fichiers en spécifiant tous les paramètres depuis la ligne de commande, mais ils peuvent être fournis à l'aide d'un [job sauvegardé](batch_fr.md).

Voici la liste des paramètres supportés :

- `-b FileName.arb` : Charge le fichier batch nommé FileName.arb
- `-g` : Démarre le traitement après que les fichiers aient été ajoutés et que le fichier batch ait été chargé
- `-x` : Quitte le programme après le traitement
- `--shell Chemin` : (menu contextuel de l'Explorateur) ajoute le chemin à la fenêtre déjà ouverte, ou ouvre le programme s'il n'est pas lancé
- `--install-shell` / `--remove-shell` : ajoute / retire l'entrée du menu contextuel de l'Explorateur, puis quitte
- `-a` : Change le mode d'ajout de fichiers, sur base des lettres suivant le paramètre "-a" :
  - `f` : Ajouter les fichiers dont les noms/dossiers qui suivent
  - `F` : Ajouter les dossiers dont les noms suivent, ou contenu dans les dossiers, cf. exemples
  - `r` : Ajout récursif du contenu des dossiers

Les noms de fichiers et chemins peuvent être relatifs ou absolus, et peuvent contenir des jokers (\* et ?).

**Ant Renamer Qt** : le programme s'appelle `antrenamer-qt.exe` (Windows) ou `antrenamer-qt` (Linux) ; dans les exemples ci-dessous, remplacez `Renamer.exe` par ce nom.

Voici quelques exemples :

- `Renamer.exe -b d:\test.arb -af c:\data\*.* -g -x`  

    Ceci chargera le fichier de job spécifié, et ajoutera tous les fichiers contenus dans le dossier c:\\data, démarrera le traitement puis fermera le programme
- `Renamer.exe -af c:\data`  

    Ceci ajoutera le fichier appelé c:\\data
- `Renamer.exe -afr c:\data`  

    Ceci ajoutera le fichier appelé c:\\data. S'il s'agit d'un dossier, il ajoutera les fichiers qu'il contient (recherche récursive)
- `Renamer.exe -aF c:\data`  

    Ceci ajoutera le dossier "data"
- `Renamer.exe -aF c:\data\*.*`  

    Ceci ajoutera les dossiers contenus dans le dossier "data", mais pas les fichiers
- `Renamer.exe -af c:\data\*.* c:\images\*.jpg c:\test.txt -afr c:\mp3\*.*`  

    Vous pouvez combiner plusieurs chemins/noms, utiliser des masques, et utiliser différents paramètres "-a" dans une ligne de commande

Si un nom est donné sans paramètre "-a", il sera analysé comme dans Ant Renamer 2.07 (et précédents) : si le fichier se termine par ".arb", il est chargé comme un fichier de job ; si un dossier est donné, son contenu est ajouté récursivement (fichiers uniquement, ne supporte pas les jokers, requiert un nom complet).

Sous Windows, il est possible d'ajouter Ant Renamer dans la liste "Envoyer vers" (dans le menu qui apparaît lorsque vous cliquez avec le bouton droit sur un dossier dans l'Explorateur Windows). Ceci ajoute donc la possibilité d'envoyer un dossier à Ant Renamer depuis son menu contextuel, pour charger son contenu dans la liste de fichiers. Pour faire cela, allez dans "Démarrer" -\> "Exécuter" et tapez `sendto`. Une fenêtre de dossier devrait s'ouvrir ; placez là un raccourci vers Ant Renamer.
