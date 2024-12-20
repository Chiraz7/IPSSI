# IPSSI 
# Application Client-Serveur 

## Description
Mise en œuvre un système de communication entre un client et un serveur ( Slave
/ Master ) en utilisant le langage C. 

## Serveur :
Accepte les connexions des clients.
Reçoit un identifiant unique de chaque client.
Vérifie et envoie des ordres prédéfinis aux clients.

## Client :
Établit une connexion avec le serveur.
Transmet son identifiant.
Attend les ordres à exécuter.
## Ordres Gérés :
- **Ransomware** : Chiffrement des fichiers avec une clé générée par le serveur et renvoi du nombre de fichiers chiffrés.
-**Exfiltration** : Lecture et transmission d'un fichier cible au serveur.
-**Fork**  : Si le serveur envoie l'ordre exfiltration
Le client lit le contenu d'un fichier sur l'ordinateur de la cible & transmet le text au
serveur.
-**Out**: Le Si le serveur envoie l'ordre fork. Le client lance un code.

## Fichiers inclus
- **client.c** : Implémentation du client.
- **serveur.c** : Implémentation du serveur.
- **README.txt** : Ce fichier, expliquant le projet.
  
## Prérequis
- Un compilateur gcc .
- Système d'exploitation compatible avec les sockets.

## Compilation et Exécution
### 1. Compilation
Utilisez les commandes suivantes pour compiler les fichiers :
gcc -o serveur serveur.c
gcc -o client client.c
./Serveur
./Client

