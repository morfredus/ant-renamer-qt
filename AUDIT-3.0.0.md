# Audit avant 3.0.0 - rapport consolidé

*Base : rc.12 (HEAD `d1ea868` de `main`). Six audits indépendants en lecture seule (moteur, interface, build/paquets/CI, aide, documentation, traductions), recoupés et en partie vérifiés par moi. Aucun fichier du projet n'a été modifié par l'audit. Ce fichier n'est pas suivi par git (exclu localement).*

Légende : **V** = vérifié par moi dans le code ou par un test ; **A** = rapporté par un agent avec preuve (code lu), non reproduit ; **?** = doute à trancher.

---

## 1. Ce qu'il faut corriger avant la 3.0.0

### 1.1 Bloquants et graves (robustesse, perte de données)

| ID | Constat | Où | Statut |
|---|---|---|---|
| R1 | **Fichier mp3 sans année ID3v1 : le fichier perd son extension.** `year` est lue sans retirer les NUL (les autres champs passent par `v1field`) ; `validateFileName` ne filtre pas les caractères de contrôle ; le système coupe le nom au premier NUL. Masque `%title% (%year%)%ext%`. Idem possible avec des valeurs IPTC. | `core/ren_id3.cpp:160`, `core/ren_actions.cpp:44-51`, `core/ren_exif.cpp:137` | **V** (code) + A (reproduit par l'agent sous Windows) |
| R2 | **En-tête étendu ID3v2.3 hostile : décalage négatif, lecture avant le tampon, plantage.** `pos = int(be32(...) + 4)` non borné puis `body[pos]`. Un mp3 corrompu suffit à le provoquer (action MP3 sélectionnée). | `core/ren_id3.cpp:248-251` | **V** (code) + A (plantage reproduit) |
| R3 | **EXIF/IPTC : dépassements d'entiers signés dans l'arithmétique des offsets** (comportement indéfini, plantage selon compilateur ; arm64/MSVC non testés). | `core/ren_exif.cpp:124-203` | A |
| R4 | **Expression régulière invalide : le texte d'erreur devient le nom du fichier.** Sous Linux `<`, `>` et espaces sont légaux : les fichiers sont réellement renommés en `<RegExp Error> ...`. | `core/ren_actions.cpp:763`, `app/mainwindow.cpp:1281` | **V** (lecture) + signalé par 2 agents |
| R5 | **Ajout de fichiers (F2/F3/`addFiles`) sans garde `worker_` pendant un renommage** : `push_back` sur le vecteur lu par le fil de travail (UB). Aussi `saveBatch()` → `stable_sort` des actions pendant l'exécution, et lecture/écriture concurrente des `File` (aperçu, options, bascule « Seulement modifiés »). | `app/mainwindow.cpp:299-321, 945-962, 1200-1209, 867-911` | **V** (absence de garde) + A (effets) |
| R6 | **Bouton « Supprimer » du remplacement multiple avec nom vide : efface tous les jeux enregistrés** (et le jeu `Remove west-european accents` ne revient pas). | `app/actionpanel.cpp:446-449` | **V** |
| R7 | **`German.lng` n'est pas du XML valide** (`&#34"` sans `;`, ligne 199) : le chargeur n'affiche aucune erreur et garde la moitié des clés → ~57 % de l'interface allemande en anglais. Seul fichier invalide sur 28. | `app/Languages/German.lng:199` | **V** |
| R8 | **Paquet Windows : des fichiers de test partent dans le zip portable et l'installeur** (`test_panel_autogen/`, avec des chemins de la machine de build) : le filtre n'exclut que `CMakeFiles` et `antrenamer-qt_autogen`. | `scripts/windows/package-win.ps1:33-34` | **V** |

### 1.2 Comportements faux ou surprenants (à corriger ou à documenter)

| ID | Constat | Où | Statut |
|---|---|---|---|
| B1 | **`-af dossier\*.*` : le joker Qt `*.*` n'accepte que les noms contenant un point** → fichiers sans extension et dossiers sans point ignorés. Les exemples de l'aide et des recettes utilisent `*.*`. | `app/mainwindow.cpp:1456` ; aide `params_*`, `USER_RECIPES*` | **V** (test Qt : `*.*` → `with.txt` seulement ; `*` → tout) |
| B2 | **Aperçu et renommage donnent des numéros différents quand un fichier annexe (.xmp) est dans la liste** (Énumération, `%num%` EXIF, liste de noms). | `core/ren_actions.cpp:237` | A |
| B3 | **Décalage de date EXIF appliqué en heure locale** : faux autour des changements d'heure (collisions de noms en octobre). | `core/ren_actions.cpp:801-803` | A |
| B4 | **« Appliquer à l'extension » ajoute un point final** quand le résultat est vide (`Makefile` → `Makefile.` sous Linux). 3 opérations. | `core/ren_actions.cpp:334, 407, 487` | A |
| B5 | **Masque Date et heure : `f`/`e` sont substitués même entre apostrophes** (`yyyy'_final'` produit des guillemets dans le nom) ; `aa` perd un caractère. | `core/ren_actions.cpp:603-614`, `core/ren_format.cpp:328` | A (le help en parle aussi) |
| B6 | **Regex avec expression vide : insère le remplacement entre chaque caractère** (l'expression par défaut est vide). | `core/ren_actions.cpp:762-773` | A |
| B7 | **Chargement d'un fichier XML qui n'est pas un job : réussit en silence et vide le job** ; `save()` tronque le fichier avant d'écrire. | `core/ren_actions.cpp:899-953` | A |
| B8 | **Valeurs d'un job non bornées** (`Digits="2000000000"` → blocage/mémoire ; `Pos=INT_MAX` → débordement). | `load()` des actions | A |
| B9 | **`%datetime%` n'est pas la date de prise de vue** (c'est la date de dernière modification EXIF) ; le masque par défaut de l'action EXIF et la note du programme le présentent comme tel. | `app/actionpanel.cpp:390`, notes | A (aide `metadata_tags` le dit juste) |
| B10 | **Tags de masque sensibles à la casse sauf EXIF/IPTC/XMP** : `%Name%`, `%EXT%` restent littéraux ; l'aide dit « non sensibles à la casse ». | `core/ren_actions.cpp:167-171, 518, 554` | **V** |
| B11 | **Glisser-déposer : l'aide promet « Ctrl/Maj pour réafficher la fenêtre »** ; ce n'est pas implémenté. | `app/mainwindow.cpp` (`filesDropped`), aide `addfiles_*` | **V** (aucun test de modificateur) |
| B12 | **Option « historique » : l'aide dit « 0 = aucun »** ; le champ va de 1 à 200 (défaut 20). | `app/dialogs.cpp:191` | **V** |
| B13 | **Journal auto-enregistré : duplique les exécutions précédentes** (si « vider le journal » est décoché) et échoue sans message. | `app/mainwindow.cpp:1369-1390` | A |
| B14 | **Migration des icônes : écrase aussi un vrai choix « Windows XP / XP Large »** fait avant la rc.12 (le code ne remet à zéro que 0 par intention). | `app/appsettings.cpp:63-66` | A (code) |
| B15 | **Fichiers de langue : le notes/liens `<LINK>` de 9 langues ne correspondent pas au nom de l'action** (clic sans effet) : italien, polonais, portugais (Brésil), hongrois, japonais, slovaque, tamoul (×2), espagnols (Argentine/Colombie). Grec et polonais : balises/tags traduits (le programme n'accepte que l'anglais) ; grec : marqueur `%d` manquant. | `app/Languages/*.lng` | A |
| B16 | **La légende Date et heure n'est traduisible dans aucune langue** (nœud CDATA ignoré par le chargeur) ; elle liste encore `c`, `ddddd`, `dddddd` qui n'existent pas. | `app/actionpanel.cpp:335`, `.lng` | A (+ aide MAJ-8) |
| B17 | **Descriptions du journal jamais traduites** : « Generate **namees** with MP3 tags » (faute côté code) / « names with EXIF » (faute côté `.lng`) ; « min offset » alors que le décalage est en secondes ; « Could not get file date » absent des `.lng`. | `core/ren_actions.cpp:548, 589, 598, 786-787`, `English.lng` | A |
| B18 | **Le chargeur de langues masque ses erreurs** : XML invalide ignoré, BOM UTF-8 → fichier ignoré, UTF-8 invalide sans repli. | `app/lngtranslator.cpp:38-84` | A |

### 1.3 Aide : contradictions avec le code (8 majeures)

À corriger dans les pages EN **et** FR (le croate/tamoul suivent par recopie) :
1. `addfiles` : Ctrl/Maj au dépôt (B11).
2. `actions` : `%datetime%` = date de prise de vue (B9).
3. `format` : « les noms de tags ne tiennent pas compte de la casse » (B10).
4. `actions` (Changement de casse) : « le nom ne change pas vraiment sous Windows » - faux, le programme renomme bien la casse sous Windows.
5. `log`/`actions` : couleurs - **bleu** = nom identique ; **gris** = information manquante (pas d'ID3/EXIF, ligne vide, fin de liste), toujours affichée quel que soit le filtre ; **rouge** = erreur y compris « fichier source introuvable ».
6. `actions_fr` : « No EXIF info found (non traduit) » est faux en français (« Pas d'info EXIF trouvée »).
7. `actions` Date et heure : « entre guillemets simples ou doubles » faux pour `f`/`e` (B5).
8. `options` : « 0 = aucun » (B12).

Mineurs notables : bouton « Add to batch » (EN) ; format du journal auto-enregistré (`v`/`x`/espace, BOM, CRLF) ; chemin complet des réglages ; nombre de tags EXIF (**36**, pas « une quarantaine »/« trente-cinq ») ; `UserComment` et `MakerNote` toujours vides alors que listés ; `params` (chemins relatifs acceptés, fichier simple ignoré sans `-a`, « sendto » Windows 10/11, `--install-shell` Windows seulement) ; terminologie FR (« associés » vs « annexes », « action » vs « opération », « Avant-plan », « le bouton `...` ») ; `toc_fr` (« Actions » → « Opérations ») ; crédits (« CVG » → « GVG », noms de langues non traduits dans `about_fr`, licence en anglais dans la page FR) ; fautes d'orthographe EN/FR (liste détaillée dans le rapport d'agent, ~25 fautes) ; croate/tamoul : liens vers le mauvais code de langue (`batch_ta`, `params_ta`), `??.arb` (mojibake) dans `batch_hr`, `toc_ta` « Log » traduit « version », `index_ta` parle encore d'Ant Renamer 2.13 ; `<RegExp Error>` à mettre en bloc de code (balise HTML pour MD4C) ; captures : bandeau « Pages » rogné, barre Opérations tronquée, option « Mode portable » cochée sur la capture FR.

### 1.4 Documentation projet

| ID | Constat | Statut |
|---|---|---|
| D1 | `ARCHITECTURE`/`CONTRIBUTING` : « `\`, `/` et `:` séparateurs quel que soit le système » faux depuis rc.12 (Windows seulement) ; « `readId3` lit ID3v2.3 » faux (2.2/2.3/2.4) ; exemple `.arb` en `3.0.0-rc.10` ; arbre incomplet (`id3_check.h`, `LICENSE`...) | **V** |
| D2 | `PRESENTATION` : « ARM64 non testé » (testé), « construit en 2002 », nombre de lignes, « même texte » pour l'aide, paquets Linux absents de la section distribution, première personne | A |
| D3 | Libellé du menu : « Ajouter à Ant Renamer » → **« Ajouter à Ant Renamer Qt »** (4 endroits ; `README` a raison) | A |
| D4 | Option « tri naturel » placée dans « Affichage » (CHANGELOG rc.7, notes rc.7, historique) alors qu'elle est dans « Fichiers & Dossiers » | A |
| D5 | Guide EN : vocabulaire « Operations / job » alors que l'interface anglaise dit « Actions / batch » | A |
| D6 | Compte des tags EXIF (36), langues (27 langues / 28 fichiers), « une vingtaine de pages » (18), `ANTREN_SIZE` absent de `BUILDING` | A |
| D7 | `README` : prérequis Windows (`qt6-tools` manquant), arbre incomplet, `&&` doublé, pas de section Téléchargement | A |
| D8 | CHANGELOG : ordre des sections rc.11, `xmpSidecarCandidates()` inexistant (rc.9), puces en double (rc.9), « 72 pages » (76 fichiers) | A |
| D9 | `TODO.md` : un point ouvert retiré à tort (textes d'emballage : IPTC/XMP absents de `.desktop` et du `.deb`) ; fichier EN seul, sans `_fr` ; titre « pas pour 3.0.0 » | A |
| D10 | Gabarits d'issues : noms non ASCII, français seul, rapport de bug non adapté (version, paquet, système) | A |
| D11 | `build.yml` cite les scripts privés (`Codage/scripts`) | **V** |

---

## 2. Décisions à prendre (par ordre d'importance)

1. **Tags du dépôt de production** : vérifier où pointent `v3.0.0-rc.11` et `v3.0.0-rc.12` dans `C:\Users\frede\Codage\ant-renamer-qt` (`git ls-remote --tags origin`). Dans la sandbox j'ai supprimé les tags incohérents (rc.11 → `c0f64c3`/`c9f6eac`, rc.12 → `40a4a8d`).
2. **Licence** : « GPL v2 » (fichier `LICENSE`, `README` dit « ou ultérieure ») ou « GPL v2 ou ultérieure » (boîte À propos, `.deb`) ? Aucun fichier source ne porte d'en-tête. À trancher puis uniformiser (`README`, `CONTRIBUTING`, `.deb`, `LICENSE`, en-têtes).
3. **Regex invalide (R4)** : bloquer « Démarrer » (contrôle dans l'interface) ou faire que le moteur signale une erreur au lieu de renvoyer un nom ?
4. **État « en cours »** : désactiver toutes les actions qui modifient la liste, le job ou les options pendant un renommage (recommandé), ou seulement les protéger par un garde ?
5. **`%datetime%`** : alias de `%DateTimeOriginal%` (code), ou corriger seulement l'aide et le masque par défaut ?
6. **Casse des tags** : minuscules obligatoires (documenter) ou normaliser dans `advancedFormat` ?
7. **Date et heure : apostrophes** : suivre `'` comme guillemet pour `f`/`e` (code) ou documenter « guillemets doubles seulement » ?
8. **Couleurs/filtre du journal** : le gris « information » toujours visible et « fichier source introuvable » en rouge sont-ils voulus ? (sinon c'est le code à changer, pas l'aide.)
9. **Joker `*.*`** : traiter `*.*` comme `*` dans le code, ou changer les exemples en `*` ? (Je recommande le code : c'est la sémantique attendue.)
10. **Ctrl/Maj au dépôt** : supprimer la phrase d'aide ou implémenter ?
11. **Fichiers `.lng`** : déplacer les 40 textes du tableau « français seul » dans `English.lng`/`French.lng` (les autres langues pourront les traduire) ? Réparer les liens et tags des 9 langues, le fichier allemand ; renommer ou aliaser `Belarussian` ; **Tamil Tace16** (garder, renommer, retirer ?) ; choix par territoire des locales espagnoles/chinoises ; compléter les crédits (13 noms manquants).
12. **Icônes** : la migration doit-elle ne remettre à zéro que `iconSet == 0` ? Une release avant la rc.12 permettait-elle de choisir XP ?
13. **Vocabulaire** : EN « Actions/batch » ou « Operations/job » ; FR « opération » vs « action », « associés » vs « annexes », « » vs "…".
14. **Fichiers simples en argument** (`antrenamer-qt a b` sans `-a`, `.desktop` avec `%F`) : les ajouter ?
15. **Windows 10** : supporté (annoncé) ou non testé (liste « non testé ») ?
16. **Paquets Linux/CI** : construire `.deb`/AppImage dans la CI ; épingler et vérifier `linuxdeploy` (AppImage téléchargé non vérifié) ; `install()` CMake ; licence dans l'AppImage ; Wayland ; plancher glibc/Qt ; entrée Explorateur cochée par défaut dans l'installeur ; fond de CI Windows instable (échec de clé MSYS2 constaté) et `ctest` absent côté Windows.
17. **`TODO.md`, `docs`** : garder (titre, `_fr`) ou migrer vers des issues ; première personne dans `PRESENTATION` ; ton final des notes de release 3.0.0 (retirer « release candidate »).
18. **Adresse e-mail personnelle** dans l'historique des commits (réécriture d'historique non recommandée).
19. **Doutes non vérifiés** (aucun agent n'a pu les tester) : comportement Wayland (always-on-top), verrou d'instance Linux, `qtbase_*.qm`, taille des polices à fort facteur d'échelle, `QStringDecoder` sans ICU (arm64), `ExposureTime` (fractions), décodage EXIF des types 7/10, formats RAW (ORF, RW2...).

---

## 3. À faire pour passer en 3.0.0 (liste mécanique)

`VERSION` → `3.0.0` ; badge README (EN/FR) → `version-3.0.0-blue` ; `docs/USER_GUIDE*` (ligne de version) ; `docs/WHATS_NEW*` (version courante ; une éventuelle section « À venir » prend le numéro de version) ; section `## [3.0.0]` dans le CHANGELOG ; entrée « 3.0.0 » dans les quatre `history_*.md` ; notes de release sans « release candidate » ni « avis bienvenus avant la 3.0.0 » ; `TESTED_PLATFORMS*` (ligne 3.0.0 par machine) ; `TODO.md` ; commentaires de version (`CMakeLists.txt:9`, `.iss:2`) ; années de copyright si nouvelle année.

---

## 4. Propositions de lots de travail (dans une branche `dev/...`, jamais sur `main`)

| Lot | Contenu | Risque |
|---|---|---|
| **L1 - Robustesse du moteur** | R1, R2, R3 (qint64 + bornes), R4, B3 à B8 ; tests T1 à T10 du rapport moteur | moyen (comportement des actions) |
| **L2 - Interface** | R5 (désactivation pendant l'exécution), R6, B13, B14, bornes de `load()`, `*.*` (B1), garde du dépôt | faible à moyen |
| **L3 - Langues** | R7 (allemand), B15 à B18, déplacement des 40 textes, crédits, aliasing des locales | faible (données) |
| **L4 - Aide** | corrections EN/FR du §1.3, recopie croate/tamoul | faible |
| **L5 - Documentation** | §1.4 | faible |
| **L6 - Build/CI/paquets** | R8, `install()`, CI Linux, épinglage AppImage, gabarits d'issues, `.gitignore`, `build.yml` | faible |
| **L7 - Tests** | couverture manquante des actions, règles de chemin Linux, `QTemporaryDir` (collisions ctest en parallèle) | faible |

Ordre conseillé : L1 et L2 d'abord (risques de plantage et de perte de données), puis L3, L4, L5, L6, L7, enfin le bump 3.0.0.
