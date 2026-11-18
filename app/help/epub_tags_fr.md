# Tags EPUB

L'opération [Infos EPUB](actions_fr.md#epub) lit les métadonnées stockées dans les fichiers EPUB (livres numériques) et vous permet de les utiliser dans le masque, comme n'importe quel autre `%tag%` (voir [« Formatage de chaînes »](format_fr.md)). Un EPUB est une archive ZIP : le programme l'ouvre, trouve le fichier OPF indiqué dans `META-INF/container.xml`, et lit ses champs Dublin Core (`dc:title`, `dc:creator`...) et les informations de série. Rien n'est extrait sur votre disque et le fichier n'est jamais modifié.

Un fichier qui n'est pas un EPUB valide (pas un ZIP, pas de `container.xml`, pas de fichier OPF) n'est pas renommé : le journal l'indique (ligne bleue).

## Les tags

| Tag | Signification | D'où ça vient |
|---|---|---|
| `%author%` | Auteur(s) | `dc:creator`, sans rôle ou avec le rôle `aut`. Plusieurs auteurs sont reliés par « & » |
| `%title%` | Titre | Le premier `dc:title` |
| `%year%` | Année de publication | Les quatre premiers chiffres du premier `dc:date` |
| `%publisher%` | Éditeur | `dc:publisher` |
| `%lang%` | Code de langue (`fr`, `en`...) | `dc:language` |
| `%isbn%` | ISBN | Le `dc:identifier` marqué ISBN (`opf:scheme="ISBN"` ou `urn:isbn:...`) |
| `%series%` | Nom de la série | Calibre (`calibre:series`) ou EPUB 3 (`belongs-to-collection`) |
| `%seriesnum%` | Numéro dans la série | Calibre (`calibre:series_index`) ou EPUB 3 (`group-position`) |

Les tags `%name%`, `%ext%`, `%folderN%` et `%%` sont aussi disponibles, ainsi que la fonction `$if`.

## Bon à savoir

- **La qualité des noms est celle des métadonnées.** Les livres trouvés sur Internet sont inégaux : le titre contient parfois déjà la série ou l'édition (« (French Edition) »), la série et son numéro manquent souvent, l'auteur peut être faux. Vérifiez l'aperçu avant de renommer.
- **Un tag absent donne un texte vide.** Utilisez `$if` pour éviter un séparateur orphelin quand il est vide.
- **Un fichier sans tag utilisable n'est pas renommé.** Si le masque ne donne ni lettre ni chiffre (par exemple `%author% - %title%%ext%` sur un fichier qui n'a ni l'un ni l'autre), le fichier garde son nom et le journal indique « Aucune info utilisable : le nouveau nom serait vide ».
- **Numéro de série.** Calibre écrit « 1.0 » : il est affiché `1`. L'option « Numéro de série sur deux chiffres » de l'opération ajoute un zéro devant un nombre d'un seul chiffre (« 1 » devient « 01 »), pour que les tomes soient triés dans le bon ordre.
- **Auteurs.** Les créateurs ayant un autre rôle (illustrateur, traducteur, éditeur scientifique...) sont ignorés.
- **Année.** Le premier `dc:date` est utilisé. Certains livres donnent la date de conversion au lieu de celle de la première publication.
- **Espaces.** Les retours à la ligne et les espaces répétés dans une valeur sont remplacés par une seule espace.
- **Caractères interdits.** Les valeurs sont nettoyées comme tout nom de fichier : `/ \ : * ? " < > |` sont remplacés.

## Exemples

| Masque | Résultat |
|---|---|
| `%author% - %title%%ext%` | `Antoine de Saint-Exupéry - Le Petit Prince.epub` |
| `%author% - %title% (%year%)%ext%` | `Antoine de Saint-Exupéry - Le Petit Prince (1943).epub` |
| `%author% - %title%$if(%seriesnum%, - %seriesnum%,)%ext%` | pas de tiret orphelin quand le livre n'a pas de numéro de série |
| `$if(%series%,%series% %seriesnum% - ,)%author% - %title%%ext%` (option deux chiffres) | `Contes 03 - Antoine de Saint-Exupéry - Le Petit Prince.epub`, rien d'ajouté sans série |
