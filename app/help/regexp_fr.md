# Expressions rationnelles

Note : bien que la traduction officielle de "regular expressions" soit "expressions rationnelles", on trouve souvent le terme "expressions régulières".

Ant Renamer Qt utilise les expressions régulières de Qt (QRegularExpression, c'est-à-dire la bibliothèque PCRE2). Vous pouvez trouver un [guide de référence syntaxique sur le site de Qt](https://doc.qt.io/qt-6/qregularexpression.html), ou le [résumé de la syntaxe PCRE2](https://www.pcre.org/current/doc/html/pcre2syntax.html). Ces expressions sont assez complexes mais également assez "standard", je ne vais donc pas faire ici un guide détaillé sur leur fonctionnement. (L'Ant Renamer d'origine utilisait la bibliothèque TRegExpr, très proche pour un usage courant : les expressions écrites pour elle continuent de fonctionner.)

Pour l'opération "Expressions rationnelles", vous devez fournir deux informations :

- **Expression**, contenant une expression rationnelle valide ;
- **Nouveau nom**, contenant le nouveau nom du fichier, avec des références de type $*n* (*n* étant le numéro d'un groupe, entre parenthèses, de l'expression) pour indiquer où doivent être insérées les chaînes trouvées.

## Comment ça marche

- L'expression est cherchée dans le nom de fichier entier, extension comprise (pas dans le chemin du dossier).
- La recherche **ne tient pas compte de la casse**. Pour qu'elle en tienne compte, commencez l'expression par `(?-i)`.
- Chaque partie du nom qui correspond à l'expression est remplacée par le nouveau nom ; ce qui ne correspond pas est conservé tel quel. Ainsi `[0-9]+` avec un nouveau nom `X` transforme "a12b345.txt" en "aXbX.txt". Utilisez `^` pour indiquer le début du nom et `$` pour la fin du nom afin de ne retenir que le nom entier.
- Dans le nouveau nom, `$1`, `$2`... `$9` insèrent le texte trouvé par le groupe correspondant, `$&` (ou `$0`) insère tout ce qui a été trouvé, et `${10}` permet d'atteindre les groupes au-delà du 9 (`$10` signifie le groupe 1 suivi d'un "0"). Un groupe qui n'a rien trouvé donne un texte vide.
- Beaucoup de caractères non-alphanumériques ont une signification spéciale dans les expressions rationnelles. Pour les utiliser, le caractère devant les précéder est le *backslash* ("\\"). Ceci s'applique aussi au "$" qui a une signification spéciale dans le champ "nouveau nom" : écrivez `\$` pour obtenir un "$". Si vous devez utiliser le backslash comme caractère (par ex. comme délimiteur dans un chemin pour envoyer le fichier dans un autre dossier) vous devez le doubler ("\\\\"), voir l'[option « Forcer les dossiers »](options_fr.md#processing).
- Si l'expression n'est pas valide, l'aperçu affiche le nom "<RegExp Error>" suivi du message de la bibliothèque, et les fichiers ne sont pas renommés correctement : corrigez l'expression avant de cliquer sur "Démarrer".

## Exemples

Échanger le nom d'artiste et le titre des noms de fichiers mp3 :

- Expression = `(.*) - (.*)\.mp3`
- Nouveau nom = `$2 - $1.mp3`

Extraire le numéro d'épisode et le titre de fichiers vidéo d'une série, avec numéros d'épisodes sous la forme S*nn*E*mm*, suivis du titre :

- Expression = `Code\.Quantum\.S([0-9]{2})E([0-9]{2})\.(.*)\.FRENCH.XViD\.avi`
- Nouveau nom = `Code Quantum - $1$2 - $3.avi`

Supprimer le numéro de piste qui commence un nom ("01 - Chanson.mp3" devient "Chanson.mp3") :

- Expression = `^[0-9]+ - (.*)`
- Nouveau nom = `$1`

Mettre une date écrite jour-mois-année devant le nom, dans l'ordre année-mois-jour ("Voyage 25-12-2024.jpg" devient "2024-12-25 Voyage .jpg") :

- Expression = `(.*)([0-9]{2})-([0-9]{2})-([0-9]{4})`
- Nouveau nom = `$4-$3-$2 $1`

Si un simple remplacement suffit, l'opération [Remplacement de chaîne](actions_fr.md#stringrepl) (avec son option jokers) est plus facile à utiliser.
