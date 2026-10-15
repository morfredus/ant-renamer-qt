# Ant Renamer Qt : recettes

*Comment arriver à un résultat précis, du plus simple au plus complexe.*
Version anglaise : [USER_RECIPES.md](USER_RECIPES.md). Référence de toutes les fonctions : [USER_GUIDE_fr.md](USER_GUIDE_fr.md).

Chaque recette donne le but, les opérations à empiler dans le job (dans l'ordre), les réglages et un avant/après. Les exemples de noms ont été vérifiés avec le moteur du programme, sauf ceux qui lisent des métadonnées (EXIF, ID3, XMP), qui suivent l'aide.

## 0. Avant de commencer

1. **Ajoutez les fichiers** (F2 / F3 / glisser-déposer). Le **tri de la liste décide de l'ordre de traitement** : triez par nom avant une énumération.
2. **Choisissez l'opération**, réglez-la, puis **Ajouter au job** (Ctrl+F2). Répétez pour chaque opération : elles s'appliquent l'une après l'autre, chacune sur le résultat de la précédente.
3. **Vérifiez l'aperçu** : le cadre « Aperçu du fichier sélectionné » montre un fichier ; **F5** remplit la colonne Aperçu pour toute la liste. « Seulement modifiés » masque les fichiers inchangés pour ne contrôler que les autres.
4. **Démarrer** (F9), puis lisez le **Journal**. **Annuler** (F12) défait le dernier traitement, et lui seul.
5. **Précaution sur un lot important** : essayez d'abord sur quelques copies, ou cochez « Copier plutôt que renommer » (Options > Traitement) : les originaux restent intacts, les copies portent les nouveaux noms. Pensez à décocher ensuite.
6. **Enregistrez le job** (Ctrl+S) dès qu'il marche : il se recharge (Ctrl+O) et se lance en ligne de commande (§8).

Deux règles à retenir :
- Une opération voit le **nom déjà modifié** par les précédentes, sauf les opérations de métadonnées (Date, MP3, EXIF) qui lisent toujours le **vrai fichier** sur le disque.
- Un `\` dans un nouveau nom est un **séparateur de dossier** : c'est ainsi qu'on déplace des fichiers (§5).

---

## 1. Nettoyer et corriger des noms

### 1.1 Remplacer les espaces par des underscores (ou l'inverse)
*Remplacement de chaîne* : Rechercher ` ` (une espace), Remplacer par `_`. Garder « Remplacer toutes les occurrences ».
`mon fichier 1.txt` → `mon_fichier_1.txt`. Pour l'inverse, échangez les deux champs.

### 1.2 Retirer un morceau de longueur variable (jokers)
*Remplacement de chaîne*, case « Utiliser les jokers » cochée. Rechercher `_*_final`, Remplacer par (vide).
`Film_ABC123_final.mov` → `Film.mov`. `*` couvre n'importe quel texte, `?` un caractère. Le texte trouvé est le plus court possible. Les jokers n'existent que dans cette opération : pour nettoyer avant une autre opération, placez ce remplacement avant dans le job.

### 1.3 Supprimer les accents
*Remplacements multiples* : choisir le set **Remove west-european accents** dans la liste déroulante. `Éte été.txt` → `Ete ete.txt`. Pour d'autres caractères (`&` → `et`, `œ` → `oe`), ajoutez des lignes à la grille (Ctrl++), nommez le set et **Sauver**. Le Copier / Coller permet d'éditer la grille dans Excel.

### 1.4 Mettre l'extension en minuscules, ou normaliser `.jpeg` en `.jpg`
- *Changement de casse* : « Minuscules », cocher **Appliquer à l'extension plutôt qu'au nom** : `PHOTO.JPG` → `PHOTO.jpg`.
- *Changer l'extension* : saisir `jpg` (sans point) : `a.jpeg` → `a.jpg`. Pour ajouter au lieu de remplacer, cocher « Ne pas remplacer l'extension » : `a.tar` → `a.tar.bak`.

### 1.5 Majuscule à chaque mot
*Changement de casse*, premier choix « Mettre en majuscule la première lettre suivant ces caractères », avec la liste des séparateurs (par défaut espace, tiret, point, plus, parenthèse). `mon beau fichier-2.txt` → `Mon Beau Fichier-2.txt`. Pour tout passer en minuscules d'abord (`PHOTO Grand` → `Photo Grand`) : un premier *Changement de casse* « Minuscules », puis celui-ci. Les changements de casse seule (`a.txt` → `A.txt`) fonctionnent aussi sous Windows.

### 1.6 Retirer les N premiers caractères
*Suppression de caractères* : nombre de caractères = 5, « À partir de la position » 0, depuis le début. `2024_IMG_1.jpg` → `IMG_1.jpg`. Autres modes : supprimer N caractères **après** ou **avant** une chaîne donnée (`IMG_001_x.jpg`, 3 caractères après `IMG_` → `IMG__x.jpg`). L'extension n'est pas comptée, sauf avec « Appliquer à l'extension ».

### 1.7 Retirer un préfixe numérique (« 001 - »)
*Expressions rationnelles* : Expression `^\d+ - `, Nouveau nom vide. `001 - Titre.mp3` → `Titre.mp3`. **L'expression porte sur le nom complet, extension comprise** : ajoutez `\.mp3` si vous voulez la reprendre dans le nouveau nom.

### 1.8 Réduire les espaces multiples
*Expressions rationnelles* : Expression ` {2,}`, Nouveau nom ` ` (une espace). `a   b    c.txt` → `a b c.txt`.

### 1.9 Remplacer les points par des espaces, sauf celui de l'extension
*Expressions rationnelles* : Expression `\.(?=.*\.)`, Nouveau nom ` `. `my.great.file.name.txt` → `my great file name.txt` (le point suivi d'un autre point est remplacé, le dernier est gardé).

### 1.10 Ajouter un suffixe avant l'extension, ou un préfixe
*Insertion de chaîne* :
- suffixe : texte `_web`, position 0, **À partir de la fin** : `IMG_1.jpg` → `IMG_1_web.jpg` (l'extension n'est pas comptée) ;
- préfixe : texte `2024_`, position 0, à partir du début.

### 1.11 Préfixer par le nom du dossier parent
*Insertion de chaîne* : texte `%folder1%_`, position 0. Pour `C:\photos\Paris\IMG_1.jpg` → `Paris_IMG_1.jpg`. `%folder2%` est le dossier au-dessus, `%folder-1%` le premier depuis la racine.

### 1.12 Déplacer une partie du nom
*Déplacement de chaîne* : par exemple 1 caractère de la position 2 à la position 4 : `abcdef.txt` → `abdecf.txt` (la position d'arrivée se compte après retrait du caractère). Si la partie n'est pas toujours à la même position, utiliser une expression rationnelle (§1.13).

### 1.13 Inverser deux parties du nom (Artiste - Titre)
*Expressions rationnelles* : Expression `(.*) - (.*)\.mp3`, Nouveau nom `$2 - $1.mp3`. `Blur - Song 2.mp3` → `Song 2 - Blur.mp3`.

### 1.14 Mettre l'année en tête
*Expressions rationnelles* : Expression `(.*) \((\d{4})\)\.mkv`, Nouveau nom `$2 - $1.mkv`. `Film (1999).mkv` → `1999 - Film.mkv`. Les parenthèses de l'expression capturent (`$1`, `$2`...) ; pour une parenthèse littérale, la faire précéder de `\`. L'expression est insensible à la casse par défaut. `$&` désigne tout ce qui a été trouvé.

### 1.15 Garder seulement le numéro d'un nom d'appareil
*Expressions rationnelles* : Expression `^IMG_(\d+).*(\.jpg)$`, Nouveau nom `$1$2`. `IMG_0042_copy.jpg` → `0042.jpg`.

---

## 2. Numéroter

### 2.1 Numéroter 001, 002, 003...
*Énumération* : Masque `Fichier %num%%ext%`, Démarrer à 1, Nombre de chiffres 3, Incrémenter de 1. `Fichier 001.jpg`, `Fichier 002.jpg`... Garder le nom d'origine : `%name%_%num%%ext%`. Les numéros suivent l'ordre de la liste : **triez la liste d'abord** (clic sur l'en-tête, ou glisser-déposer).

### 2.2 Recommencer à 1 dans chaque dossier
Ajoutez les fichiers de plusieurs dossiers, **triez par dossier** (en-tête Dossier, de façon que les contenus ne soient pas mélangés), cochez **Retour à la valeur de démarrage lors du changement de dossier**.

### 2.3 Compter à rebours
*Énumération* : Démarrer à 100, Incrémenter de -1 : 100, 99, 98... (les champs acceptent les valeurs négatives).

### 2.4 Un JPEG et son RAW avec le même numéro
Triez par nom pour que `IMG_1234.JPG` et `IMG_1234.CR2` se suivent, puis cochez **Même numéro pour les fichiers de même nom et d'extensions différentes**. Les deux deviennent `Photo 001.JPG` et `Photo 001.CR2`. Une extension déjà présente dans le groupe reçoit un nouveau numéro. Si les fichiers `.xmp` associés sont dans la liste ils suivent aussi ; sinon l'option « Renommer aussi les fichiers associés » (§3.6) s'en charge.

---

## 3. Photos

### 3.1 Nommer par date de prise de vue (EXIF)
*Infos EXIF* : Masque `%DateTimeOriginal%%ext%`. `IMG_4521.jpg` → `2024-05-17 14-30-05.jpg` (les `:` interdits deviennent `-`). Autre forme avec des tags : `%Make% %Model% - %DateTimeOriginal%%ext%`. Un tag absent du fichier donne un texte vide.

### 3.2 Éviter les doublons de date (rafales)
*Infos EXIF* : Masque `%DateTimeOriginal%_%num%%ext%` ; le compteur `%num%` a les mêmes réglages que l'énumération (chiffres, retour à la valeur de départ par dossier). Pour que JPEG et RAW partagent le numéro, cocher l'option correspondante (§2.4).

### 3.3 Corriger l'heure d'un appareil mal réglé
*Infos EXIF* ou *Date et heure*, réglage **Décalage** en secondes, tous les tags date de la forme `aaaa:MM:jj HH:mm:ss` sont décalés. Exemple : appareil resté à l'heure d'hiver, décalage `3600` (+1 heure) ; négatif pour reculer. `-86400` retire un jour.

### 3.4 Nommer d'après la date du fichier (pas d'EXIF)
*Date et heure* : masque `yyyy'-'mm'-'dd'_'fe` → `2026-10-15_IMG_1.jpg` (`f` = nom d'origine, `e` = extension). Tout texte fixe va entre guillemets ou apostrophes. Date et heure : `yyyymmdd'_'hhnnss'_'fe` → `20261015_223515_IMG_1.jpg`. Pour l'heure sur 12 h, ajouter `am/pm` (ou `a/p`) au masque : `hh'-'nn am/pm`, 22:35 → `10-35 pm`. Choisir **Création** ou **Dernière modification**. Cocher **Ajouter un suffixe si plusieurs fichiers ont la même date** pour éviter les noms identiques. `nn` est toujours les minutes ; `mm` est les minutes s'il suit `hh` ou précède `ss`, sinon le mois.

### 3.5 Nommer d'après le titre ou les mots-clés (IPTC / XMP)
*Infos EXIF*, masque avec `$if` pour prendre le premier tag existant et éviter un séparateur en trop :
- `$if(%XmpTitle%,%XmpTitle%,%IptcObjectName%)%ext%`
- `%DateTimeOriginal%$if(%XmpKeywords%, - %XmpKeywords%,)%ext%`

Les valeurs multiples sont jointes par « , ». Pour un RAW, les mots-clés et le titre donnés dans le logiciel photo sont dans le `.xmp` voisin, lu automatiquement.

### 3.6 Renommer les photos avec leurs fichiers associés
Rien à faire si la case « Renommer aussi les fichiers associés (.xmp, .dop) d'une image renommée » (Options > Traitement) est cochée, ce qui est le cas par défaut. `IMG_0001.CR2` et `IMG_0001.CR2.xmp` partent ensemble, l'annulation (F12) remet les deux. Le journal le signale. Voir aussi §2.4.

### 3.7 Ranger les photos dans des dossiers année / mois
*Date et heure* (ou EXIF si la date de prise de vue compte) : masque `yyyy'\'mm'\'fe` → `2026\10\nom.jpg`. Le `\` fait un sous-dossier, créé à côté du fichier. Cochez **Forcer les dossiers** (Options > Traitement) pour qu'ils soient créés automatiquement ; sans cela le dossier doit exister.

---

## 4. Musique et vidéos

### 4.1 « Artiste - Titre » depuis les tags ID3
*Infos des tags MP3* : masque `%author% - %title%%ext%`. Option « Remplacer les caractères interdits » (`"` → `''`, `?` → `.`, les autres → `-`). Gère ID3v1 et ID3v2.2 / 2.3 / 2.4. Fichier sans tag : « non renommé » dans le journal.

### 4.2 Numéro de piste, album, tags incomplets
- `%track% - %title%%ext%` avec la case **Numéro de piste sur deux chiffres** → `01 - Titre.mp3`.
- `%album%\%track% - %title%%ext%` range dans un dossier par album (cocher Forcer les dossiers).
- Tags vides : `%author%$if(%title%, - %title%,)%ext%` → `Blur - Song 2.mp3` mais `Vivaldi.mp3` si le titre est vide (pas de « - » en trop). Les valeurs contenant virgule ou parenthèse se mettent entre guillemets : `%author%$if(%album%," ("%album%")",)%ext%`.

### 4.3 Épisodes de séries
*Expressions rationnelles* : Expression `Code\.Quantum\.S([0-9]{2})E([0-9]{2})\.(.*)\.FRENCH.XViD\.avi`, Nouveau nom `Code Quantum - $1$2 - $3.avi`. `Code.Quantum.S01E02.Titre.FRENCH.XViD.avi` → `Code Quantum - 0102 - Titre.avi`. Adaptez les parties fixes à vos fichiers ; les groupes `([0-9]{2})` extraient saison, épisode, titre.

---

## 5. Dossiers : déplacer, ranger, aplatir

Principe : un `\` dans le nouveau nom désigne un dossier. Avec **Forcer les dossiers**, il est créé s'il n'existe pas.

### 5.1 Un dossier par nom de fichier
Fichiers `cars.doc`, `cars.xls`, `cars.jpg`, `houses.doc`... *Insertion de chaîne* : texte `%name%\`, position 0, début. Cochez Forcer les dossiers. Résultat : `cars\cars.doc`, `cars\cars.xls`... `houses\houses.doc`...

### 5.2 Remonter des fichiers d'un sous-dossier en gardant son nom
Fichiers `images\movies\poster1.jpg` et `images\games\poster1.jpg` : *Insertion de chaîne*, texte `../%folder1% `, position 0. Résultat dans `images` : `movies poster1.jpg`, `games poster1.jpg`. Les dossiers vides se suppriment ensuite à la main.

### 5.3 Déplacer vers un autre endroit (chemin complet)
Options > Traitement : cocher **Détecter les « :\ » ou « \\ » comme chemins absolus**. *Insertion de chaîne* : texte `D:\Tri\`, position 0. `a.txt` → `D:\Tri\a.txt`. Combinez avec Forcer les dossiers pour créer l'arborescence. Pour garder l'original, cochez « Copier plutôt que renommer ».

### 5.4 Renommer des dossiers
Dossiers (F3) : cochez « ajouter les dossiers » (et décochez les fichiers si besoin). Les dossiers sont traités comme des fichiers sans extension. Si vos noms de dossier contiennent un point qui n'est pas une extension (`Photos 2024.05`), laissez « Le dernier point d'un nom de dossier délimite l'extension » décoché (défaut). `%countfiles%` et `%countfolders%` (récursifs avec `r`) donnent le nombre d'éléments du dossier : `Vacances (%countfiles%)`.

---

## 6. Avancé

### 6.1 Appliquer une liste de noms (depuis Excel ou un texte)
*Noms à partir d'une liste* : collez un nom par ligne, dans l'ordre exact de la liste de fichiers (triez-la d'abord). Cochez « Ajouter l'extension originale » si la liste n'a pas d'extensions. Plus de fichiers que de lignes, ou ligne vide : le fichier n'est pas renommé. Option « Utiliser les éléments comme extensions » pour ne changer que les extensions.

### 6.2 Anonymiser avec des noms aléatoires
*Noms aléatoires* : masque `%random%%ext%`, méthode GUID (unique garanti), nombre à 8 chiffres ou compteur Windows.

### 6.3 Enchaîner plusieurs opérations
Un nettoyage complet type, dans cet ordre : 1) *Remplacements multiples* (accents) ; 2) *Remplacement de chaîne* (jokers, retirer `_copie`) ; 3) *Remplacement de chaîne* (espaces → `_`) ; 4) *Changement de casse* (minuscules) ; 5) *Énumération* en dernier, puisque le numéro doit s'appliquer au nom définitif. Vérifiez avec F5, puis enregistrez le job (Ctrl+S).
Pour corriger une étape déjà ajoutée : retirez-la et refaites-la (disposition classique) ou double-cliquez dessus dans la disposition moderne.

### 6.4 Lancer un job enregistré en ligne de commande
```
antrenamer-qt -b d:\jobs\nettoyage.arb -af c:\import\*.* -g -x
```
charge le job, ajoute les fichiers du dossier, renomme et quitte. `-afr dossier` ajoute récursivement, `-aF` ajoute des dossiers. Le même appel peut être lancé par le Planificateur de tâches Windows ou depuis un script. Les `*` et `?` de la ligne de commande choisissent les fichiers ; ils n'ont rien à voir avec les jokers du remplacement de chaîne.

### 6.5 Faire entrer des fichiers depuis l'Explorateur
Activer « Ajouter à Ant Renamer Qt » (Options > Fichiers & Dossiers). Clic droit sur une sélection de fichiers et de dossiers (Windows 11 : *Afficher plus d'options*) : tout arrive dans la fenêtre déjà ouverte.

### 6.6 Défaire un renommage
**Annuler** (F12) restaure les noms de la dernière exécution, y compris les fichiers associés. Un seul niveau : n'enchaînez pas un second traitement avant d'avoir vérifié. En cas de doute, travaillez en mode « Copier plutôt que renommer » et gardez le journal enregistré (Options > Événements > Stocker le journal).

---

## 7. Problèmes courants

| Symptôme | Cause et solution |
|---|---|
| Aperçu vide dans la liste | F5 n'a pas été utilisé : l'aperçu n'est pas automatique. |
| Les numéros ne suivent pas l'ordre voulu | La liste n'est pas triée comme prévu : trier ou réordonner avant de démarrer. |
| « Nom déjà utilisé » (rouge) | Deux fichiers donnent le même nouveau nom : ajouter un compteur (`%num%`) ou le suffixe de l'opération Date et heure. |
| Un fichier n'est pas renommé (bleu) | Nouveau nom identique à l'ancien, tag manquant (MP3, EXIF) ou fichier absent : lire le message du journal. |
| Un `\` fait échouer le renommage | Le dossier cible n'existe pas : cochez Forcer les dossiers. |
| Les accents sont mal gérés en majuscules | Garder cochée l'option de langue de l'opération Changement de casse. |
| L'expression rationnelle ne trouve rien | Elle s'applique au nom complet, extension comprise ; vérifier aussi les `\` devant `.` `(` `)` `[`. |
