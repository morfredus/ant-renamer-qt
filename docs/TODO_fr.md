# TODO

## Idées pour après la 3.0.0

English version: [TODO.md](TODO.md).

- Plus de types de fichiers associés (par exemple `.pp3` de RawTherapee) : ajouter l'extension à `sidecarExtensions()` dans `core/ren_exif.cpp`.
- Lire les données `.dop` ? Pas prévu : c'est un format propre à DxO.
- Propriétés XMP pas encore reprises : les attributs `exif:` d'un fichier associé (date, GPS), les réglages `crs:` (Camera Raw).
- IPTC des fichiers TIFF (seul le JPEG est lu) ; le XMP inclus dans un TIFF est déjà trouvé par son contenu.
