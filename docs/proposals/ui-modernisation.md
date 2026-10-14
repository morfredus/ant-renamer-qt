# Proposition : moderniser l'interface d'Ant Renamer Qt

Branche : `dev/ui-modernisation` (rien n'est fusionné dans `main`).
Statut : **première implémentation** disponible sur cette branche, en option (Options > Affichage > « Disposition moderne de l interface », prise en compte au prochain démarrage). L ancienne interface reste la valeur par défaut.
Maquette interactive de la page Actions : [ui-actions-mockup.html](ui-actions-mockup.html) (à ouvrir dans un navigateur).

## 1. Constat (l'interface actuelle, lue dans `app/mainwindow.cpp`)

- **Six barres d'outils sur deux rangées** : Pages, Program, Files, Actions, Log, Move Items. Les outils de la page courante sont mélangés à ceux du programme, et les icônes font 16 px.
- **Les pages (Files / Actions / Log) se changent avec des boutons de barre d'outils**, pas avec de vrais onglets : on ne voit pas bien où l'on est.
- **Page Actions** : un panneau de saisie en haut, un petit cadre « Preview of selected file » limité à 80 px, et la liste des actions planifiées cachée derrière une case à cocher (`Batch contents`). Le résultat d'une action n'est donc visible qu'en partie, et la liste du plan l'est rarement.
- **Style** : feuille de style bleu codé en dur (`rgba(0,120,215,…)`), boutons à plat à l'ancienne, aucune marge cohérente. Le tout rappelle le Delphi d'origine.
- **Aucun état vide** : au démarrage, la liste des fichiers est un grand rectangle blanc sans indication.

## 2. Principes

1. **Rien n'est retiré ni renommé** : mêmes actions, mêmes raccourcis (F9 Go, F10 Options, F11 Au premier plan, F12 Annuler, Échap Stop, F1 Aide), mêmes libellés. Les 28 fichiers de langue restent valables.
2. **Le flux principal se lit de gauche à droite** : ajouter des fichiers, choisir des actions, vérifier l'aperçu, lancer.
3. **L'aperçu est toujours visible** : c'est ce qui rassure avant de renommer.
4. **Les outils se trouvent là où on s'en sert** : ceux des fichiers dans la page Fichiers, ceux du plan dans la page Actions.
5. **Mode « classique » conservé** pendant la transition (option de l'écran Options), pour ne pas dérouter les utilisateurs actuels.

## 3. Proposition

### 3.1 Navigation : de vrais onglets

Remplacer la barre « Pages » par une barre d'onglets en haut de la fenêtre :

```
┌─────────────────────────────────────────────────────────────────────────┐
│  Fichiers (42)   Actions (3)   Journal          [ ▶ Lancer ]  ⟲  ⚙  ?   │
└─────────────────────────────────────────────────────────────────────────┘
```

- Les compteurs entre parenthèses donnent l'état d'un coup d'œil (42 fichiers, 3 actions planifiées).
- **Lancer** (F9) devient le seul bouton coloré : l'action principale est évidente. Stop apparaît à sa place pendant le traitement.
- Annuler, Options et Aide restent à droite en icônes. « Au premier plan » et « Quitter » passent dans un menu `⋯` (ils restent accessibles par F11 et Alt+F4).

### 3.2 Une seule barre d'outils, plus des barres de page

On passe de six barres à **une barre principale** (ci-dessus) et **une barre fine propre à chaque page**, placée en haut de la page :

- **Fichiers** : `Ajouter des fichiers` · `Ajouter des dossiers` · `Retirer la sélection` · `Tout retirer` · `Retirer les fichiers introuvables` · `Aperçu` · et à droite, les quatre boutons de déplacement (haut, bas, début, fin) et un **champ de filtre** par nom.
- **Actions** : `Ajouter` · `Retirer` · `Vider` · `Ouvrir` (avec les récents) · `Enregistrer`.
- **Journal** : filtre (OK / Erreur / Non traité) · `Vider` · `Enregistrer` · `Copier`.

Les boutons principaux gardent leur texte à côté de l'icône ; les autres ont l'icône seule avec une infobulle (comme aujourd'hui).

### 3.3 Page Fichiers

```
┌ Ajouter fichiers │ Ajouter dossiers │ ✕ │ ✕✕ │ …        ⌕ Filtrer…  ↑ ↓ ⤒ ⤓ ┐
├─────────────────────────────────────────────────────────────────────────────┤
│ Nom actuel                     │  Nouveau nom (aperçu)         │ Dossier    │
│ fichier 1.txt                  │  ► photo_001.txt              │ C:\test    │
│ fichier 2.txt                  │  ► photo_002.txt              │ C:\test    │
│ …                              │                               │            │
└─────────────────────────────────────────────────────────────────────────────┘
```

- **État vide** : « Glissez des fichiers ou des dossiers ici, ou cliquez sur Ajouter » avec un grand bouton, à la place du rectangle blanc.
- **Colonne « Nouveau nom » affichée par défaut** (c'est aujourd'hui la colonne `Preview`, cachée). C'est la modification qui apporte le plus.
- Lignes alternées, hauteur de ligne un peu plus grande, texte atténué pour les fichiers qui ne changeront pas, texte en évidence pour ceux qui changent, en rouge si un conflit de nom est détecté.
- Le tri naturel/alphabétique et le menu des colonnes restent inchangés.

### 3.4 Page Actions : le plan et l'éditeur côte à côte

Aujourd'hui l'éditeur est en haut et le plan est caché en bas. On les met côte à côte, avec l'aperçu toujours en bas :

```
┌ Plan (3 actions) ────────────┬ Éditeur : Remplacer ──────────────────────────┐
│ 1  Remplacer « IMG » → photo │  Rechercher : [ IMG          ]                │
│ 2  Énumérer  (001…)          │  Remplacer par : [ photo      ]               │
│ 3  Minuscules                │  ☐ Respecter la casse  ☐ Expression régulière │
│ ┌ + Ajouter une action ▾ ┐   │                                               │
├──────────────────────────────┴───────────────────────────────────────────────┤
│ Aperçu : IMG_0042.JPG  →  photo_001.jpg                      (fichier 1 sur 42)│
└───────────────────────────────────────────────────────────────────────────────┘
```

- La liste des **types d'actions** actuelle (liste + pile de pages) devient le menu `+ Ajouter une action`, regroupé (Texte, Numérotation, Date, Casse, Étiquettes…). Chaque action planifiée s'édite dans le panneau de droite.
- L'aperçu est une **bande fixe** qui montre le fichier sélectionné, et reste lisible sur les trois pages.
- Les contrôles (boutons radio, cases) restent ceux de `ActionPanel` : on réorganise leur contenant, pas leur logique, ce qui limite le risque.

### 3.5 Journal

Pas de changement de fonctionnalité. Icône d'état par ligne (✔ ✖ ○) et colonne de l'ancien/nouveau nom, au lieu d'une simple liste de texte.

### 3.6 Style et accessibilité

- Style **Fusion** de Qt, avec la palette du système (clair/sombre) ; la couleur d'accent vient de `QPalette::Highlight`, plus de bleu codé en dur.
- Icônes **SVG** (nettes à toutes les échelles, écrans haute densité), au lieu des jeux de bitmaps 16 px. Les trois jeux actuels (Scrows, XP, XP Large) restent proposés en mode classique.
- Grille d'espacement de 8 px, polices système, cibles de clic d'au moins 28 px.
- Navigation au clavier et ordre de tabulation vérifiés sur chaque page.

## 4. Ce qui ne change pas

Le moteur de renommage (`core/`), les actions et leurs options, les fichiers de langue, l'aide (les captures d'écran de l'aide seront à refaire à la fin), les raccourcis et le format des fichiers de plan enregistrés.

## 5. Plan de mise en œuvre (par étapes, chacune livrable)

| Étape | Contenu | Risque |
|---|---|---|
| 0 | Style Fusion, palette système, espacements ; suppression du bleu en dur | faible |
| 1 | Barre d'onglets et barre principale ; barres propres à chaque page | moyen (réorganise `createToolbars`) |
| 2 | Page Fichiers : état vide, colonne « Nouveau nom » par défaut, filtre, lignes en évidence | faible |
| 3 | Page Actions : plan et éditeur côte à côte, aperçu fixe | **plus élevé** (touche `ActionPanel`) |
| 4 | Icônes SVG, mode classique en option | moyen (travail de ressources) |

Chaque étape serait un commit séparé sur cette branche, avec une capture avant/après dans la description. Rien n'entre dans `main` sans ton accord, et pas avant la 3.0.0 si on veut garder une rc stable : cette refonte est plutôt un chantier pour **3.1**.

## 6. Questions pour toi

1. **Mode classique** : le garder indéfiniment, ou seulement le temps d'une version ?
2. **Colonne « Nouveau nom » par défaut** : d'accord pour l'afficher d'emblée ?
3. **Page Actions** : êtes-vous à l'aise avec le plan visible en permanence à gauche, ou préférez-vous le garder repliable comme aujourd'hui ?
4. **Calendrier** : 3.1, ou plus tôt sous forme d'une option « Nouvelle interface » dans une rc ?
