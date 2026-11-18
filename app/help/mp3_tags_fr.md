# Tags MP3

L'opération [Infos des tags mp3](actions_fr.md#mp3) lit les tags stockés dans les fichiers MP3 et vous permet de les utiliser dans le masque, comme n'importe quel autre `%tag%` (voir [« Formatage de chaînes »](format_fr.md)). Deux sortes de tags sont lues :

- **ID3v2** (2.2, 2.3 et 2.4), au début du fichier : le format moderne, avec accents et tous les alphabets (les textes Latin-1, UTF-16 et UTF-8 sont compris).
- **ID3v1 et v1.1**, à la fin du fichier : l'ancien format, limité à 30 caractères par champ.

Quand les deux existent, la valeur v2 est utilisée et la v1 ne complète que ce que la v2 n'a pas. Un fichier sans aucun tag ID3 n'est pas renommé : le journal l'indique (ligne bleue).

## Les tags

| Tag | Signification | Frame ID3v2 | ID3v1 |
|---|---|---|---|
| `%author%` | Auteur / artiste | TPE1 (v2.2 : TP1) | Artiste |
| `%title%` | Titre du morceau | TIT2 (TT2) | Titre |
| `%album%` | Nom de l'album | TALB (TAL) | Album |
| `%year%` | Année (quatre chiffres) | TYER, TDRC (TYE) | Année |
| `%genre%` | Genre, en texte | TCON (TCO) | Numéro de genre, affiché par son nom |
| `%comm%` | Commentaire | COMM (COM) | Commentaire |
| `%track%` | Numéro de piste | TRCK (TRK) | Piste (v1.1 seulement) |

Les tags `%name%`, `%ext%`, `%folderN%` et `%%` sont aussi disponibles, ainsi que la fonction `$if`.

## Bon à savoir

- **Un tag absent donne un texte vide**, pas un nombre : `$if(%title%,...)` teste vraiment si le fichier a un titre.
- **Un fichier sans tag utilisable n'est pas renommé.** Si le masque ne donne ni lettre ni chiffre (par exemple `%author% - %title%%ext%` sur un fichier qui n'a ni l'un ni l'autre), le fichier garde son nom et le journal indique « Aucune info utilisable : le nouveau nom serait vide ».
- **Piste.** « 3/12 » donne `3`. L'option « Numéro de piste sur deux chiffres » de l'opération ajoute un zéro devant un nombre d'un seul chiffre (« 3 » devient « 03 »), pour que les fichiers soient triés dans l'ordre de l'album. Un fichier sans numéro de piste donne un texte vide.
- **Genre.** Les codes numériques v2 comme `(17)` sont remplacés par le nom du genre (« Rock »).
- **Caractères interdits.** Un tag peut contenir n'importe quel caractère, donc les valeurs sont nettoyées comme tout nom de fichier : les guillemets (") deviennent des apostrophes ('), les points d'interrogation un point, et `\ / : * < > |` ainsi que les caractères de contrôle un tiret.
- **Autres formats.** Seuls les fichiers MP3 sont lus : FLAC, OGG, M4A... ne le sont pas.

## Exemples

| Masque | Résultat |
|---|---|
| `%author% - %title%%ext%` | `Queen - Bohemian Rhapsody.mp3` |
| `%track% - %title%%ext%` (option deux chiffres) | `03 - Bohemian Rhapsody.mp3` |
| `%author% - %album% - %track% - %title%%ext%` | `Queen - A Night at the Opera - 11 - Bohemian Rhapsody.mp3` |
| `%author%$if(%title%, - %title%,)%ext%` | `Queen.mp3` quand le titre est vide, sans tiret orphelin |
| `$if(%year%,%year% - ,)%album% - %title%%ext%` | `1975 - A Night at the Opera - Bohemian Rhapsody.mp3` |
