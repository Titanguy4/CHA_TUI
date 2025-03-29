# CHA_TUI

Un projet de chat en ligne avec une interface en ligne de commande.

## 📦 Compilation

Pour compiler le projet, utilisez la commande suivante :

```bash
make
```

## 🚀 Exécution

### Serveur

```bash
./serveur <NB_CLIENTS> <PORT> <PORT_FILE> <NB_SALONS>
```

### Client

```bash
./client <ServeurIP> <PORT> <FILEPORT>
```

## 💬 Commandes

Voici la liste des commandes disponibles dans le chat :

- `/commands` : Affiche toutes les commandes disponibles.
- `/members` : Liste tous les membres connectés au chat.
- `/whisper <username> <message>` : Envoie un message privé à un utilisateur spécifique.
- `/kick <username>` : Expulse un utilisateur spécifique.
- `/bye` : Quitte le chat.

## 📖 Licence

Ce projet est sous licence MIT. Vous êtes libre de l'utiliser, de le modifier et de le distribuer.
