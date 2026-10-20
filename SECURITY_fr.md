# Politique de sécurité

([English version](SECURITY.md))

## Versions prises en charge

Seule la dernière version publiée d'Ant Renamer Qt reçoit des correctifs. Si vous trouvez un problème dans une version plus ancienne, vérifiez d'abord qu'il existe encore dans la dernière.

## Signaler une vulnérabilité

Merci de **ne pas** ouvrir d'issue ni de discussion publique pour un problème de sécurité.

Utilisez plutôt le signalement privé de GitHub : sur l'[onglet Security du dépôt](https://github.com/morfredus/ant-renamer-qt/security), cliquez sur *Report a vulnerability*. Seul le mainteneur voit le signalement.

Indiquez la version, le système, ce que vous avez fait et ce qui s'est passé (un fichier renommé, déplacé ou lu alors qu'il ne devrait pas l'être, un plantage sur un nom de fichier ou un fichier `.lng` forgé, etc.).

Vous aurez une réponse dès que possible ; c'est un projet de temps libre maintenu par une seule personne, comptez quelques jours. Une fois le problème corrigé et publié, le signalement peut être rendu public avec votre nom, sauf si vous préférez rester anonyme.

## Ce qui n'est pas une vulnérabilité

Le programme renomme et déplace des fichiers volontairement, avec les droits de l'utilisateur qui le lance. Renommer un fichier que vous lui avez demandé de renommer n'est pas un problème de sécurité ; un bogue qui fait perdre des données est une [issue](https://github.com/morfredus/ant-renamer-qt/issues) normale.
