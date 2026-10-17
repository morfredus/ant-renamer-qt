# Contribuer à Ant Renamer Qt

Merci de votre intérêt. ([English version](CONTRIBUTING.md))

## Avant de commencer

- **Un bug, une idée, une question ?** Ouvrez une [issue](https://github.com/morfredus/ant-renamer-qt/issues) (bugs, demandes de fonctionnalités) ou lancez une [discussion](https://github.com/morfredus/ant-renamer-qt/discussions) (questions, idées). Merci de ne pas signaler les problèmes d'Ant Renamer Qt à l'auteur du programme d'origine.
- **Un changement important ?** Parlez-en d'abord dans une issue, pour que personne ne travaille sur quelque chose qui ne sera pas intégré.
- Les issues et les pull requests peuvent être écrites en anglais ou en français.

## Récupérer le code et le compiler

Tout ce qui concerne la compilation, les tests et les paquets est dans [docs/BUILDING_fr.md](docs/BUILDING_fr.md). En bref :

```bash
cmake --preset linux && cmake --build --preset linux      # ou --preset mingw sous Windows (MSYS2)
ctest --test-dir build --output-on-failure
```

Pour comprendre où sont les choses et comment un renommage se déroule, lisez [docs/ARCHITECTURE_fr.md](docs/ARCHITECTURE_fr.md). Le moteur (`core/`) n'a pas d'interface et est couvert par `tests/test_core.cpp` ; l'interface est dans `app/`.

## Faire un changement

1. Faites un fork du dépôt et créez une branche depuis `main`.
2. Gardez le changement ciblé : un sujet par pull request.
3. Suivez le style du code voisin (noms, densité des commentaires, idiomes). Il n'y a pas de formateur à lancer.
4. Ajoutez ou mettez à jour un test quand le moteur change (`tests/test_core.cpp`) : un nouveau cas qui échoue sans votre correctif est la meilleure preuve qu'il fonctionne.
5. Lancez les tests (`ctest`) avant d'ouvrir la pull request ; le workflow GitHub compile sous Linux et Windows et les lance aussi.
6. Ajoutez une ligne sous `## [Unreleased]` dans [CHANGELOG.md](CHANGELOG.md) (Added, Changed ou Fixed). Les numéros de version sont donnés par le mainteneur au moment d'une release : ne modifiez pas `VERSION`.
7. Si le changement est visible par les utilisateurs, ajoutez une ligne à `docs/WHATS_NEW.md` et `docs/WHATS_NEW_fr.md` (laissez la colonne « Depuis » à *(upcoming)* / *(à venir)*) et mettez à jour les pages d'aide et les guides qui décrivent le comportement. Si vous n'écrivez pas en français, dites-le dans la pull request : le mainteneur s'occupera des textes français.

## Comment faire pour...

**Ajouter une opération (un 15e type).**
1. `core/ren_actions.h` : ajouter la valeur à la **fin** de `enum class ActionType` (les numéros sont ceux de l'original et sont les numéros de ligne de la liste à l'écran) et déclarer `class ActionXxx : public Action` avec ses champs, `description`, `makeNewName`, `save`, `load` (et `getTag` / `init` si besoin).
2. `core/ren_actions.cpp` : les implémenter, ajouter la balise XML au tableau `kNames` et le type à `ActionList::create`.
3. `app/actionpanel.cpp` : ajouter la page dans `buildPages` (l'ordre des pages est l'index du type), puis la branche dans `makeAction` et dans `setFromAction`.
4. Ajouter les textes anglais dans le code et leurs traductions dans les fichiers `.lng` (`app/Languages/`).
5. Ajouter un cas à `tests/test_core.cpp`. `tests/test_panel.cpp` détecte un `setFromAction` oublié.
6. Documentation : pages d'aide (`app/help/actions_*.md`), `docs/USER_GUIDE*.md`, `docs/WHATS_NEW*.md`, `CHANGELOG.md`.

**Ajouter un réglage à une opération existante.** Un champ dans la classe (avec sa valeur par défaut), `save` / `load` (un attribut XML ; prendre la valeur par défaut s'il manque pour que les anciens jobs se lisent encore), l'usage dans `makeNewName`, le contrôle dans `buildPages`, `makeAction` **et** `setFromAction`, le texte dans les fichiers `.lng`, un cas dans `test_core.cpp`.

**Ajouter une option du programme.** Un champ dans `AppOptions` (`app/appsettings.h`), son nom dans la macro `KEYS` (`app/appsettings.cpp`), le contrôle dans `OptionsDialog` (`app/dialogs.cpp`) ; si le moteur en a besoin, un champ dans `ren::Options` et la recopie dans `applyToCore()`.

**Ajouter un tag EXIF, IPTC ou XMP.** Une ligne dans la table correspondante (`tagNames`, `iptcNames`, `xmpProps` dans `core/ren_exif.cpp`), la page d'aide des tags, et un test avec un fichier synthétique dans `test_core.cpp`.

**Ajouter un type de fichier associé.** Ajouter l'extension à `sidecarExtensions()` dans `core/ren_exif.cpp` : la recherche des candidats, le déplacement, l'annulation et le saut des sidecar présents dans la liste en découlent.

**Ajouter un fichier source.** Ajouter le `.cpp` à `add_library(rencore ...)` (moteur) ou à `set(SRC ...)` dans `app/CMakeLists.txt` : les listes de sources sont écrites à la main, rien n'est détecté automatiquement. Un fichier oublié se voit par une erreur « undefined reference » à l'édition de liens. Si `test_panel` en a aussi besoin, l'ajouter à sa liste.

**Ajouter une langue ou corriger une traduction.** Suivre [docs/TRANSLATING_fr.md](docs/TRANSLATING_fr.md). Modifier ou ajouter un fichier `.lng` dans `app/Languages/`. Pour l'aide, ajouter les pages `_<code>.md` et le `toc_<code>.md`, et déclarer le code dans `helpLanguageCode()` (`app/lngtranslator.cpp`).

## À savoir

- **Séparateurs de chemin.** `\` et `/` cohabitent dans le moteur. Normalisez les chemins avant d'en comparer deux.
- **Fils d'exécution.** Le moteur tourne dans un fil de travail (`RenWorker`). L'interface ne doit lire que les copies `Snapshot` qu'elle reçoit, jamais un `File` pendant un renommage.
- **Un job renomme réellement à chaque opération**, pas une seule fois à la fin. L'annulation ne remonte que d'un niveau (l'ancien nom de chaque fichier).
- **Traductions.** Le texte anglais du code doit être identique à une entrée de `English.lng` pour être traduit, sinon il reste en anglais. `ANTREN_MISSING=1` liste les textes sans traduction.
- **Fins de ligne.** Tous les fichiers texte sont en LF, dans le dépôt et dans le dossier de travail (`.gitattributes`). Si votre éditeur ou un script écrit du CRLF, Git le reconvertit, mais corrigez l'outil si possible. Les encodages ne sont pas touchés : les pages d'aide et les fichiers `.lng` gardent les leurs, ne les réenregistrez pas dans un autre encodage.

## Licence

Ant Renamer Qt est publié sous licence GPL v2, comme le programme d'origine. En contribuant, vous acceptez que votre contribution soit distribuée sous la même licence.
