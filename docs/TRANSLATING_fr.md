# Traduire Ant Renamer Qt : le processus du fichier `.lng`

*Comment ajouter une langue au programme, ou corriger une traduction existante.*
English version: [TRANSLATING.md](TRANSLATING.md). Vue d'ensemble du code : [ARCHITECTURE_fr.md](ARCHITECTURE_fr.md), règles de contribution : [CONTRIBUTING_fr.md](../CONTRIBUTING_fr.md).

## 1. Le principe, en bref

Ant Renamer Qt n'utilise pas les fichiers `.ts` de Qt. Il lit les fichiers de langue du programme d'origine : `app/Languages/*.lng` (fichiers texte XML, un par langue). Les textes du programme sont écrits en anglais dans le code source ; à l'exécution, chaque texte anglais est cherché dans la traduction.

- `English.lng` est la **référence** : il contient tous les textes anglais, chacun sous une *clé* (le chemin de l'élément XML, par exemple `MainForm/ActionAddFiles/Caption`).
- Un fichier de traduction a **la même structure** : mêmes éléments, mêmes clés, textes dans une autre langue.
- Le programme associe le texte anglais et sa traduction clé par clé. Si une clé manque, ou si son texte est vide ou identique à l'anglais, le texte anglais est affiché : rien ne casse, le texte n'est simplement pas traduit.
- Un même texte anglais à plusieurs endroits (le titre et l'info-bulle d'un bouton) reçoit la même traduction, la première trouvée dans l'ordre alphabétique des clés (le titre, en pratique).

## 2. Créer une nouvelle langue

1. **Copiez `app/Languages/English.lng`** vers `app/Languages/<Nom de la langue>.lng`. Le nom du fichier est le nom de la langue en anglais, tel qu'il sera affiché dans le programme et comparé à la langue du système : `Italian.lng`, `Portuguese (Brazil).lng`, `Spanish (Spain).lng`. Quand une langue a plusieurs variantes, le nom est « Langue (variante) ».
2. **Choisissez l'encodage** et déclarez-le sur la première ligne, `<?xml version="1.0" encoding="..."?>`. Le programme lit l'encodage écrit là : `UTF-8` (recommandé pour un nouveau fichier), mais aussi `windows-1250`, `windows-1251`, `windows-1252`, `windows-1253`, `shift_jis`, `gb2312`, `big5`... Sans déclaration il suppose `windows-1252`. Enregistrez le fichier dans l'encodage que vous avez déclaré (un éditeur de texte qui l'affiche, comme VS Code ou Notepad++, évite les erreurs).
3. **Remplissez l'en-tête**, le commentaire `<!-- ... -->` en haut. Le programme montre ces champs dans *Options > Langue* :

   | Champ | Contenu |
   |---|---|
   | `Name` | Nom de la langue en anglais |
   | `LocalName` | Nom de la langue dans elle-même (`Français`, `Deutsch`...), affiché dans la liste |
   | `Version` / `VersionFormat` | Version du fichier, par ex. `3.0` (`VersionFormat` reste comme dans le modèle) |
   | `Comments` | Texte libre ; `|` commence une nouvelle ligne |
   | `Authors` | Nom(s) du ou des traducteurs ; `|` sépare plusieurs valeurs |
   | `Icon` | Drapeau BMP 16 x 16 facultatif en base64 : laissez celui du modèle ou supprimez la ligne |

4. **Traduisez les textes** : dans chaque élément, remplacez la valeur de `Caption`, `Hint`, `Text`... par la traduction (règles à la section 4). Ne changez ni les noms d'éléments, ni les attributs `Name=` / `Index=`, ni la structure.
5. **Vérifiez que le fichier est du XML valide** (un seul guillemet mal placé rend tout le fichier inutilisable, et le programme retombe alors silencieusement sur l'anglais) : `xmllint --noout "Italian.lng"` sous Linux, ou ouvrez le fichier dans un navigateur ou VS Code.
6. **Essayez-le** (section 5), puis ajoutez-vous aux crédits (section 6) et envoyez le fichier (section 7).

## 3. Corriger ou compléter une traduction existante

Modifiez le texte de la clé dans le fichier existant `app/Languages/<Langue>.lng`, en gardant son encodage, puis testez (section 5). Les nouveaux textes d'une version du programme (nouveaux boutons, options) sont ajoutés à `English.lng` et au français par le projet ; pour les autres langues ils restent en anglais tant qu'un traducteur n'a pas ajouté la clé. Pour les trouver : lancez le programme avec `ANTREN_MISSING=1` (section 5), ou comparez les clés de `English.lng` avec celles de votre fichier avec un outil de comparaison (`diff`, WinMerge, *Comparer* de VS Code) : chaque ligne de `English.lng` sans équivalent dans votre fichier est un texte à traduire.

## 4. Règles pour les textes

- **Paramètres de substitution** : gardez `%s`, `%d`, `%1`, `%2`... exactement tels quels, vous pouvez seulement les déplacer. `%s`/`%d` (programme d'origine) et `%1`...`%9` (Qt) sont tous deux compris ; l'ordre de plusieurs `%s` est celui du texte anglais.
- **Info-bulles** : un `Hint` a la forme `Texte court|Texte long` (titre de l'info-bulle, puis l'explication, aussi affichée dans la barre d'état). Gardez le `|`, même quand la première partie est vide (`Hint="|Clear the string table"`).
- **Notes sous les opérations** (éléments `lbl...Notes`) : le texte contient de petits codes HTML (`&lt;BR&gt;` pour un saut de ligne, `&lt;LINK&gt;...&lt;/LINK&gt;` pour un lien). Gardez-les, sous la forme utilisée dans le fichier. **Important** : le texte entre les codes `LINK` qui nomme une autre opération (par exemple « Remplacement de chaîne ») doit être **exactement le nom traduit de cette opération dans la liste des opérations**, sinon un clic sur le lien ne fait rien. Le lien « Aide » (`<LINK>Help</LINK>`) ouvre la page d'aide.
- **Caractères XML** : écrivez `&amp;` pour `&`, `&lt;` pour `<`, `&gt;` pour `>`, `&quot;` pour `"` dans un attribut. Dans le modèle, `&#38;&#38;` dans un titre est la façon d'afficher un seul `&`.
- **Tags** comme `%name%`, `%ext%`, `%folderN%`, `%num%`, `%author%` ou les lettres du masque de date (`yyyy`, `mm`...) : **ne se traduisent pas**, ce sont eux que le programme reconnaît.
- **Longueur** : les boutons des barres d'outils ont peu de place, préférez des titres courts.
- **Tous les textes anglais ne sont pas dans le fichier** : quelques textes ne sont encore que dans le code du programme (la fenêtre *À propos*, par exemple). Ils restent en anglais, ou le projet les traduit dans le code (français seulement pour l'instant).

## 5. Tester

1. Mettez le fichier `.lng` dans le dossier `Languages` **à côté du programme** (`build-.../app/Languages` pour une compilation, rafraîchi à chaque compilation depuis `app/Languages` ; `<préfixe>/share/antrenamer-qt/Languages` pour un paquet Linux installé).
2. Lancez le programme, *Options > Langue*, choisissez la langue, cliquez sur OK et **redémarrez** (la langue est lue au démarrage). Pour ne pas toucher à vos propres réglages, un fichier `antrenamer-qt.ini` contenant `language=Italian` (le nom du fichier sans `.lng`) à côté du programme fait la même chose en mode portable.
3. **Lister les textes sans traduction** : lancez le programme avec la variable d'environnement `ANTREN_MISSING=1` et lisez ce qu'il écrit sur la sortie d'erreur, des lignes `MISSING: <texte anglais>` (cela ouvre aussi toutes les boîtes de dialogue du programme pour que tous leurs textes soient cherchés). Sous Windows le programme n'a pas de console, redirigez la sortie : `set ANTREN_MISSING=1` puis `antrenamer-qt.exe 2> missing.txt`. Sous Linux : `ANTREN_MISSING=1 antrenamer-qt 2>&1 | grep MISSING`.
4. Regardez chaque page (Fichiers, Opérations avec chacune des 14 opérations, Journal), la fenêtre *Options* avec ses cinq pages, les boîtes d'ajout de dossiers et de drag & drop, et les messages. Vérifiez que rien n'est coupé. Le programme sait faire lui-même des captures d'écran, voir les aides de développement dans [BUILDING_fr.md](BUILDING_fr.md).
5. Vérifiez aussi que l'**aide** s'affiche correctement (voir la section 8).

## 6. Citer le traducteur

- Le nom du traducteur va dans l'en-tête du fichier (`Authors`), affiché dans *Options > Langue*.
- La liste « Traductions » de la fenêtre *À propos* (onglet Crédits) est écrite dans `app/dialogs.cpp` (fonction de l'`AboutDialog`), ainsi que la liste en bas de la page d'aide *À propos de l'auteur, crédits & liens* (`app/help/about_<code>.md`, toutes les langues) : ajoutez votre ligne, dans l'ordre alphabétique de la langue.

## 7. Envoyer la traduction

Sur GitHub, [github.com/morfredus/ant-renamer-qt](https://github.com/morfredus/ant-renamer-qt) : ouvrez une *pull request* avec le fichier `.lng` nouveau ou modifié (et les lignes de crédits), ou, si vous n'en avez pas l'habitude, une *issue* avec le fichier en pièce jointe. Indiquez avec quelle version du programme vous avez testé. Une ligne dans `CHANGELOG.md` (section *Unreleased*) et dans `docs/WHATS_NEW.md` / `WHATS_NEW_fr.md` mentionne la nouvelle langue.

Les traducteurs de l'Ant Renamer d'origine peuvent envoyer leur fichier tel quel : le format est le même, seuls les textes ajoutés par Ant Renamer Qt manqueront (ils restent en anglais).

## 8. L'aide

L'aide est séparée du fichier `.lng` : des pages Markdown dans `app/help/`, **anglais, français, croate et tamoul** pour l'instant (`<page>_en.md`, `_fr.md`, `_hr.md`, `_ta.md`, avec la table des matières `toc_<code>.md`). Pour les autres langues de l'interface, l'aide s'affiche en anglais.

Pour ajouter une langue à l'aide : copiez les pages anglaises et `toc_en.md` vers `<page>_<code>.md` et `toc_<code>.md`, traduisez-les (gardez les noms de fichiers des liens et les `{#id}` après les titres : ce sont les cibles des liens entre pages), puis ajoutez la langue à `helpLanguageCode()` dans `app/lngtranslator.cpp` (nom de langue du fichier `.lng` -> code). L'aide est longue (une vingtaine de pages) : une traduction de l'interface seule est déjà très utile, l'aide peut venir plus tard.

## 9. Liste de contrôle

- [ ] `app/Languages/<Langue>.lng` créé à partir de `English.lng`, encodage déclaré et respecté
- [ ] en-tête rempli (`Name`, `LocalName`, `Version`, `Authors`)
- [ ] XML valide (`xmllint --noout`)
- [ ] paramètres (`%s`, `%1`...), `|` des info-bulles et textes des `<LINK>` vérifiés
- [ ] testé avec `ANTREN_MISSING=1`, aucune ligne `MISSING` inattendue, chaque page regardée
- [ ] traducteur cité (en-tête du fichier, fenêtre À propos, crédits de l'aide)
- [ ] pull request (ou issue) envoyée
