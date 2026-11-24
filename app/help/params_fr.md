# Paramètres en ligne de commande

Le programme ne supporte pas (encore) beaucoup de paramètres. Il n'est pas vraiment possible de renommer des fichiers en spécifiant tous les paramètres depuis la ligne de commande, mais ils peuvent être fournis à l'aide d'un [job sauvegardé](batch_fr.md).

Voici la liste des paramètres supportés :

- `-b FileName.arb` : Charge le fichier batch nommé FileName.arb
- `-g` : Démarre le traitement après que les fichiers aient été ajoutés et que le fichier batch ait été chargé
- `-x` : Quitte le programme après le traitement
- `-q` : Mode silencieux, pour les scripts : ni fenêtre ni boîte de dialogue, le traitement démarre et le programme quitte à la fin (comme `-g -x`, sans la fenêtre). Le code de sortie dit comment ça s'est passé : 0 tout est fait, 1 au moins un fichier n'a pas été renommé à cause d'une erreur, 2 rien n'a été fait (aucun fichier, aucun batch, fichier batch illisible, ou fichiers cachés dans la liste). Le fichier journal des options est toujours écrit
- `--set-dates-from-exif` : Au lieu de renommer, donne aux fichiers la date de prise de vue écrite dans leurs données EXIF (ou leur fichier XMP associé) comme date de modification, voir [Mettre les dates d'après l'EXIF](files_fr.md#setdates). Avec `-g` ou `-q` elle s'exécute tout de suite (le paramètre vaut consentement : aucune question n'est posée ; un fichier sans date EXIF est laissé tel quel) ; seule, la fenêtre montre le tableau des dates et demande d'abord. Les fichiers associés suivent leur photo et l'historique peut annuler.
- `--shell Chemin` : (menu contextuel de l'Explorateur) ajoute le chemin à la fenêtre déjà ouverte, ou ouvre le programme s'il n'est pas lancé
- Une seule fenêtre à la fois : un deuxième lancement affiche un message et quitte. Les chemins donnés seuls (un fichier déposé sur le programme ou son raccourci, « Ouvrir avec ») sont ajoutés à la fenêtre déjà ouverte. Avec `-g` ou `-x`, le message part sur la sortie d'erreur et le code de sortie est 3.
- `--install-shell` / `--remove-shell` : ajoute / retire l'entrée du menu contextuel de l'Explorateur, puis quitte
- `-a` : Change le mode d'ajout de fichiers, sur base des lettres suivant le paramètre "-a" :
  - `f` : Ajouter les fichiers dont les noms/dossiers qui suivent
  - `F` : Ajouter les dossiers dont les noms suivent, ou contenu dans les dossiers, cf. exemples
  - `r` : Ajout récursif du contenu des dossiers

Les noms de fichiers et chemins peuvent être relatifs ou absolus, et peuvent contenir des jokers (\* et ?).

**Ant Renamer Qt** : le programme s'appelle `antrenamer-qt.exe` (Windows) ou `antrenamer-qt` (Linux) ; les exemples ci-dessous utilisent le nom Windows ; sous Linux, tapez `antrenamer-qt`.

Voici quelques exemples :

- `antrenamer-qt.exe -b d:\test.arb -af c:\data\*.* -g -x`  

    Ceci chargera le fichier de job spécifié, et ajoutera tous les fichiers contenus dans le dossier c:\\data, démarrera le traitement puis fermera le programme
- `antrenamer-qt.exe -af c:\data`  

    Ceci ajoutera le fichier appelé c:\\data
- `antrenamer-qt.exe -afr c:\data`  

    Ceci ajoutera le fichier appelé c:\\data. S'il s'agit d'un dossier, il ajoutera les fichiers qu'il contient (recherche récursive)
- `antrenamer-qt.exe -aF c:\data`  

    Ceci ajoutera le dossier "data"
- `antrenamer-qt.exe -aF c:\data\*.*`  

    Ceci ajoutera les dossiers contenus dans le dossier "data", mais pas les fichiers
- `antrenamer-qt.exe -af c:\data\*.* c:\images\*.jpg c:\test.txt -afr c:\mp3\*.*`  

    Vous pouvez combiner plusieurs chemins/noms, utiliser des masques, et utiliser différents paramètres "-a" dans une ligne de commande

Si un nom est donné sans paramètre "-a", il est analysé simplement : si le fichier se termine par ".arb", il est chargé comme un fichier de job ; si un dossier est donné, son contenu est ajouté récursivement (fichiers uniquement, ne supporte pas les jokers, requiert un nom complet).

Sous Windows, il est possible d'ajouter Ant Renamer Qt dans la liste "Envoyer vers" (dans le menu qui apparaît lorsque vous cliquez avec le bouton droit sur un dossier dans l'Explorateur Windows). Ceci ajoute donc la possibilité d'envoyer un dossier à Ant Renamer Qt depuis son menu contextuel, pour charger son contenu dans la liste de fichiers. Pour faire cela, appuyez sur Windows+R, tapez `shell:sendto` puis Entrée. Une fenêtre de dossier devrait s'ouvrir ; placez-y un raccourci vers `antrenamer-qt.exe`.
