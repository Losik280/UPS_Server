#include <stdio.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

#include "def_n_struct.h"
#include "player_manager.h"
#include "network_interface.h"

/**
 * Global structure holding the server's IP and port information.
 */
server_address server_info;


/**
 * @brief Validates an IPv4 address string.
 *
 * Uses inet_pton to check if the address is a valid IPv4 format.
 *
 * @param ip_address The string containing the IP address to be checked.
 * @return 1 if the IP is valid, otherwise 0.
 */
int is_valid_ip(const char *ip_address) {
    struct sockaddr_in testAddr;
    return inet_pton(AF_INET, ip_address, &(testAddr.sin_addr)) != 0;
}

/**
 * @brief Configures the server IP address and port based on user-supplied arguments or defaults.
 *
 * If no address is provided, it binds to INADDR_ANY. If no port is specified, it uses a default PORT.
 *
 * @param argc The number of arguments passed in.
 * @param argv The array of string arguments.
 */
void configure_server_settings(int argc, char *argv[]) {
    // Default to empty IP (which means any available interface) and the standard port
    strcpy(server_info.ip_address, "");
    server_info.port = PORT;

    // Attempt to read arguments
    if (argc > 1) {
        if (is_valid_ip(argv[1])) {
            strncpy(server_info.ip_address, argv[1], sizeof(server_info.ip_address) - 1);
            server_info.ip_address[sizeof(server_info.ip_address) - 1] = '\0';
        } else {
            fprintf(stderr, "Provided IP not valid: %s\n", argv[1]);
            exit(EXIT_FAILURE);
        }
    }
    if (argc > 2) {
        server_info.port = atoi(argv[2]);
        if (server_info.port <= 0 || server_info.port > 65535) {
            fprintf(stderr, "Port out of range: %s (valid range is 1-65535)\n", argv[2]);
            exit(EXIT_FAILURE);
        }
    }
}


/**
 * @brief Creates a socket, binds it, and listens for incoming client connections in an infinite loop.
 *
 * Whenever a valid connection is accepted, this function attempts to parse the login message.
 * If valid, a new thread is created for that client.
 *
 * @return A void pointer (unused).
 */
void *start_server_socket() {
    int sockSrv = socket(AF_INET, SOCK_STREAM, 0);
    if (sockSrv == -1) {
        perror("Server socket creation failed");
        exit(EXIT_FAILURE);
    }

    setsockopt(sockSrv, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int));

    struct sockaddr_in srvAddr;
    memset(&srvAddr, 0, sizeof(srvAddr));
    srvAddr.sin_family = AF_INET;
    srvAddr.sin_port = htons(server_info.port);

    // If no IP address was provided, bind to all interfaces
    if (strlen(server_info.ip_address) == 0) {
        srvAddr.sin_addr.s_addr = INADDR_ANY;
        printf("[INFO] Listening on all available interfaces.\n");
    } else {
        srvAddr.sin_addr.s_addr = inet_addr(server_info.ip_address);
        printf("[INFO] Server configured for IP: %s\n", server_info.ip_address);
    }

    // Attempt to bind
    if (bind(sockSrv, (struct sockaddr *)&srvAddr, sizeof(srvAddr)) == -1) {
        perror("Binding server socket failed");
        close(sockSrv);
        exit(EXIT_FAILURE);
    }
    printf("[INFO] Bound to port %d successfully.\n", server_info.port);

    // Switch to listening mode
    if (listen(sockSrv, MAX_CLIENTS) == -1) {
        perror("Failed to set socket to listen");
        close(sockSrv);
        return NULL;
    }

    printf("[INFO] Server is now running, waiting for clients...\n");

    // Accept client connections indefinitely
    struct sockaddr_in clAddr;
    socklen_t lenClient = sizeof(clAddr);

    while (1) {
        int sockCl = accept(sockSrv, (struct sockaddr *)&clAddr, &lenClient);
        if (sockCl == -1) {
            perror("Unable to accept client connection");
            continue;
        }

        printf("[CONNECTION] A client connected.\n");
        char loginMsg[LOGIN_MESSAGE_SIZE] = {0};
        recv(sockCl, loginMsg, sizeof(loginMsg), 0);

        // Check protocol message
        char *token = strtok(loginMsg, MESS_DELIMITER);
        if (token && strcmp(token, "LOGIN") == 0) {
            printf("[PROTOCOL] LOGIN request received.\n");

            token = strtok(NULL, MESS_END_CHAR);
            if (!token) {
                perror("Invalid login message - closing client socket");
                close(sockCl);
                continue;
            }

            char tempUser[PLAYER_NAME_SIZE];
            strncpy(tempUser, token, PLAYER_NAME_SIZE - 1);
            tempUser[PLAYER_NAME_SIZE - 1] = '\0';

            pthread_t thClient;
            // Attempt client registration
            if (!register_client(sockCl, tempUser, &thClient)) {
                perror("Could not register the client - closing socket");
                close(sockCl);
                continue;
            }

            // Retrieve the newly created client reference
            client *connectedClient = locate_client_by_socket(sockCl);

            // Start the client thread
            if (pthread_create(&thClient, NULL, client_thread_main, connectedClient) != 0) {
                perror("Failed to launch client thread");
                close(sockCl);
                detach_client_by_socket(sockCl);
                continue;
            }

            display_all_clients();
        } else {
            // If message doesn't match expected protocol
            perror("Unrecognized message - closing client socket");
            close(sockCl);
        }
    }
}

/**
 * @brief Entry point of the server program.
 *
 * Sets up the server configuration, starts the main server thread, and also
 * begins a separate monitoring thread that pings connected clients.
 *
 * @param argc Number of command-line arguments.
 * @param argv Array of command-line arguments.
 * @return Returns 0 upon normal termination of the program.
 */
int main(int argc, char *argv[]) {
    configure_server_settings(argc, argv);

    pthread_t thServer, thPing;
    if (pthread_create(&thServer, NULL, start_server_socket, NULL) != 0) {
        perror("Unable to launch server thread");
        exit(EXIT_FAILURE);
    }

    if (pthread_create(&thPing, NULL, monitor_client_pings, NULL) != 0) {
        perror("Could not initiate ping thread");
        exit(EXIT_FAILURE);
    }

    // Wait for threads to finish
    pthread_join(thServer, NULL);
    pthread_join(thPing, NULL);

    // Additional cleanup if necessary
    pthread_cancel(thServer);
    return 0;
}



