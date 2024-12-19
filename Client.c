#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define SERVER_IP "127.0.0.1"
#define PORT 12345

// Simule le chiffrement ou déchiffrement d'un fichier
void process_file(const char *action) {
    if (strcmp(action, "chiffrer") == 0) {
        printf("Encrypting file...\n");
        // Ajoutez ici votre logique de chiffrement
    } else if (strcmp(action, "dechiffrer") == 0) {
        printf("Decrypting file...\n");
        // Ajoutez ici votre logique de déchiffrement
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

    // Envoi de l'identifiant au serveur
    printf("Enter your client ID: ");
    fgets(client_id, sizeof(client_id), stdin);
    client_id[strcspn(client_id, "\n")] = '\0'; // Supprime le '\n' à la fin
    send(sock, client_id, strlen(client_id), 0);

    while (1) {
        // Lecture de l'ordre du serveur
        memset(buffer, 0, sizeof(buffer));
        read(sock, buffer, sizeof(buffer));
        printf("Received command: %s\n", buffer);

        if (strcmp(buffer, "ransomware") == 0) {
            // Lire l'action à effectuer (chiffrer ou déchiffrer)
            memset(buffer, 0, sizeof(buffer));
            read(sock, buffer, sizeof(buffer));
            printf("Action to perform: %s\n", buffer);

            // Exécuter l'action demandée
            process_file(buffer);

            // Envoyer une réponse au serveur
            send(sock, "Action completed", 17, 0);
        } else if (strcmp(buffer, "out") == 0) {
            printf("Disconnected from server.\n");
            break;
        }
    }

    close(sock);
    return 0;
}
