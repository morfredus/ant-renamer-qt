# Politique de signature de code

English version: [CODE_SIGNING_POLICY.md](CODE_SIGNING_POLICY.md).

Free code signing provided by [SignPath.io](https://about.signpath.io), certificate by [SignPath Foundation](https://signpath.org).

Cette page décrit comment les fichiers Windows d'Ant Renamer Qt sont signés, qui peut approuver une signature, et ce que le programme fait de vos données. Elle s'applique aux fichiers Windows publiés une fois la signature en place. Les fichiers publiés avant ne sont pas signés ; les notes de la première version signée le disent.

## Ce qui est signé

- l'installeur Windows (`AntRenamerQt-<version>-win64-setup.exe`) ;
- le programme (`antrenamer-qt.exe`) dans l'installeur et dans le zip portable.

Seuls les fichiers construits à partir de ce dépôt sont signés. Les bibliothèques Qt et MinGW livrées avec le programme sont celles des projets d'origine et ne sont pas signées par ce projet. Les fichiers Linux (`.deb`, AppImage) ne sont pas signés avec ce certificat : ils sont accompagnés d'une signature GPG, voir le [README](../README_fr.md).

## Comment un fichier est signé

1. Les fichiers sont construits à partir du code source d'une version étiquetée par le workflow GitHub Actions du dépôt public ([`.github/workflows/build.yml`](../.github/workflows/build.yml)). Rien n'est construit sur un ordinateur personnel pour la signature.
2. Le workflow envoie le build à SignPath, qui ne le signe que s'il vient de ce workflow et de ce dépôt.
3. Un mainteneur approuve chaque demande de signature à la main, après avoir vérifié que la version, l'étiquette et le build sont ceux attendus.

Le nom de produit dans les fichiers est « Ant Renamer Qt » et la version de produit est celle de la release, la même pour tous les fichiers de cette release.

## Rôles

Le projet a un seul mainteneur, qui tient les trois rôles. L'authentification à deux facteurs est exigée sur GitHub et sur SignPath.

| Rôle | Qui | Ce que cela veut dire |
|---|---|---|
| Auteur | [morfredus](https://github.com/morfredus) (Frédéric Biron) | Écrit et commite le code. |
| Relecteur | [morfredus](https://github.com/morfredus) | Relit chaque changement venant de quelqu'un d'autre (pull request) avant de le fusionner. |
| Approbateur | [morfredus](https://github.com/morfredus) | Approuve chaque demande de signature. |

## Vie privée

Ant Renamer Qt n'envoie aucune information à d'autres systèmes, sauf si l'utilisateur le demande. La seule fonction réseau est la vérification facultative d'une nouvelle version, désactivée par défaut : quand vous l'activez, le programme interroge l'API de GitHub une fois par jour pour connaître la dernière release et n'envoie que son propre numéro de version. Aucun nom de fichier, aucun chemin et aucune donnée personnelle ne quitte votre ordinateur. L'historique des renommages et les réglages restent dans des fichiers sur votre ordinateur.

## Vérifier une signature

Sous Windows, clic droit sur le fichier, Propriétés, onglet Signatures numériques. Le signataire doit être « SignPath Foundation ».

## Questions

[Issues](https://github.com/morfredus/ant-renamer-qt/issues) sur GitHub, ou par e-mail : [ant-renamer-qt@morfredus.fr](mailto:ant-renamer-qt@morfredus.fr).
