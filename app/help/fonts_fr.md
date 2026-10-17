# Police utilisée pour les caractères spéciaux

Dans la [fenêtre des options](options_fr.md#display), l'option "Forcer la police suivante..." remplace la police de tout le programme (menus, listes, champs, journal) par la police et la taille que vous choisissez. Décochée, le programme utilise la police de votre système.

Vous n'en avez besoin que si certains caractères ne s'affichent pas correctement (par exemple vous voyez des carrés à la place des caractères d'un nom de fichier écrit dans une langue asiatique, ou des symboles). Comme Ant Renamer travaille en Unicode, n'importe quel nom de fichier peut être traité ; il ne s'agit ici que de son affichage.

Une police qui possède tous les caractères dont vous avez besoin fait la différence. Le programme en recommande une parmi celles-ci, si elle est installée sur votre système :

- Noto Sans, qui peut être obtenue via [Google Fonts](https://fonts.google.com/noto),
- DejaVu Sans, qui peut être obtenue via [DejaVu fonts](https://github.com/dejavu-fonts/dejavu-fonts),
- Arial Unicode MS, qui accompagnait d'anciennes versions de Microsoft Office mais n'était pas incluse dans l'installation par défaut (elle s'appelle "Police universelle" dans les options d'installation).

Si vous n'utilisez que des caractères latins basiques, vous pouvez sélectionner n'importe quelle police. La police "Arial" fournie avec Windows devrait fonctionner pour toutes les langues européennes (Europe de l'ouest & centrale), cyrilliques, le grec, l'hébreu et l'arabe.

Pour quelques langues asiatiques, voici une liste de polices qui peuvent être utilisées si elles sont installées sur votre système :

| Japonais : | MS PGothic, MS PMincho, MS Gothic, MS Mincho |
|---|---|
| Chinois traditionnel : | MingLiU |
| Chinois simplifié : | SimSun, NSimSun |
| Coréen : | GulimChe |
| Thaï : | Tahoma |

Sous Linux, des polices comme "Noto Sans CJK" couvrent ces langues.

Le changement de police est appliqué quand vous fermez la fenêtre des options avec "OK".
