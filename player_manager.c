#include "player_manager.h"
#include <stdio.h>
#include <malloc.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include "def_n_struct.h"
#include "network_interface.h"

/**
 * Mutex used to safely synchronize access to the global clients array.
 */
pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;

/**
 * Global array storing all current client pointers (NULL if slot is free).
 */
client *clients[MAX_CLIENTS] = {0};

/**
 * Prints a list of clients, showing their IDs, assigned game ID, and socket descriptor.
 */
void display_all_clients() {
    pthread_mutex_lock(&clients_mutex);
    printf("Connected clients:\n");
    for (int idx = 0; idx < MAX_CLIENTS; idx++) {
        if (clients[idx] != NULL) {
            printf("    Client: %d; Game: %d; Socket: %d\n",
                   clients[idx]->id,
                   clients[idx]->active_game_id,
                   clients[idx]->socket);
        }
    }
    pthread_mutex_unlock(&clients_mutex);
}

/**
 * Registers a new client by inserting its reference into the global array,
 * provided the array is not full and no client with the same socket exists.
 *
 * @param socket The client's socket descriptor
 * @param username The chosen username for this client
 * @param thread Reference to the pthread managing this client
 * @return TRUE if successfully added; FALSE otherwise
 */
int register_client(int socket, char *username, pthread_t *thread) {
    // First, ensure no existing client with the same socket
    if (locate_client_by_socket(socket) != NULL) {
        return FALSE;
    }

    pthread_mutex_lock(&clients_mutex);

    // Check the current number of active slots
    int connectedCount = 0;
    for (int idx = 0; idx < MAX_CLIENTS; idx++) {
        if (clients[idx] != NULL) {
            connectedCount++;
        }
    }

    // If array is already at capacity, fail
    if (connectedCount >= MAX_CLIENTS) {
        pthread_mutex_unlock(&clients_mutex);
        return FALSE;
    }

    // Allocate memory for a new client struct
    client *pNewClient = malloc(sizeof(client));
    if (pNewClient == NULL) {
        perror("Failed to allocate memory for client");
        pthread_mutex_unlock(&clients_mutex);
        return FALSE;
    }

    // Initialize fields
    pNewClient->socket = socket;
    strcpy(pNewClient->username, username);
    pNewClient->active_game_id = GAME_NULL_ID;
    pNewClient->is_in_game = FALSE;
    pNewClient->is_connected = TRUE;
    pNewClient->need_reconnect_mess = FALSE;
    pNewClient->last_ping = time(NULL);
    pNewClient->client_char = EMPTY_CHAR;
    pNewClient->opponent = NULL;
    pNewClient->client_thread = thread;

    // Insert the new client into the global array
    for (int idx = 0; idx < MAX_CLIENTS; idx++) {
        if (clients[idx] == NULL) {
            pNewClient->id = idx;
            clients[idx] = pNewClient;
            break;
        }
    }

    pthread_mutex_unlock(&clients_mutex);
    return TRUE;
}

/**
 * Sets whether a client wants to participate in a new match.
 *
 * @param cl The client
 * @param want_game TRUE if the client wants to join a game; FALSE otherwise
 */
void set_game_request(client *cl, int want_game) {
    pthread_mutex_lock(&clients_mutex);
    cl->is_requesting_game = want_game;
    pthread_mutex_unlock(&clients_mutex);
}

/**
 * Clears the client's game-related information so it can be reused or removed cleanly.
 *
 * @param cl The client whose data is reset
 */
void reset_client_game_data(client *cl) {
    pthread_mutex_lock(&clients_mutex);
    cl->active_game_id = GAME_NULL_ID;
    cl->is_in_game = FALSE;
    cl->client_char = EMPTY_CHAR;
    cl->opponent = NULL;
    pthread_mutex_unlock(&clients_mutex);
}

/**
 * Updates the client's connection status and last ping timestamp.
 *
 * @param cl The client
 * @param is_connected TRUE if the client is connected; FALSE otherwise
 */
void update_client_ping(client *cl, int is_connected) {
    pthread_mutex_lock(&clients_mutex);
    cl->is_connected = is_connected;
    cl->last_ping = time(NULL);
    pthread_mutex_unlock(&clients_mutex);
}

/**
 * Attempts to find another client who is waiting for a match. If successful,
 * sets up a new game for both the waiting client and the requesting client.
 *
 * @param cl The client ready to play
 * @return TRUE if a waiting opponent was found and a game starts; FALSE otherwise
 */
int match_waiting_opponent(client *cl) {
    pthread_mutex_lock(&clients_mutex);
    int wasFound = FALSE;

    for (int idx = 0; idx < MAX_CLIENTS; idx++) {
        // Ensure we have a different client, no game in progress, and the client wants a game
        if (clients[idx] != NULL &&
            strcmp(clients[idx]->username, cl->username) != 0 &&
            clients[idx]->active_game_id == GAME_NULL_ID &&
            clients[idx]->is_requesting_game == TRUE) {

            // Create a game with the first waiting client
            game *newMatch = create_new_game(clients[idx], cl);
            if (newMatch == NULL) {
                break;
            }

            // Configure the waiting client (they become 'X')
            clients[idx]->client_char = FIRST_PL_CHAR;
            clients[idx]->is_in_game = TRUE;
            clients[idx]->active_game_id = newMatch->id;
            clients[idx]->opponent = cl;
            clients[idx]->is_requesting_game = FALSE;

            // Configure the new client (they become 'O')
            cl->client_char = SECOND_PL_CHAR;
            cl->is_in_game = FALSE;
            cl->active_game_id = newMatch->id;
            cl->opponent = clients[idx];
            cl->is_requesting_game = FALSE;

            // Notify the waiting client
            char buffer[START_GAME_MESSAGE_SIZE] = {0};
            sprintf(buffer, "START_GAME;%s;%c;%c\n",
                    clients[idx]->opponent->username,
                    clients[idx]->opponent->client_char,
                    clients[idx]->is_in_game ? '1' : '0');
            send_mess(clients[idx], buffer);

            wasFound = TRUE;
            break;
        }
    }

    pthread_mutex_unlock(&clients_mutex);
    return wasFound;
}

/**
 * Locates and returns a reference to a client object by its socket descriptor.
 *
 * @param socket The socket descriptor
 * @return Pointer to the found client, or NULL if none match
 */
client *locate_client_by_socket(int socket) {
    pthread_mutex_lock(&clients_mutex);
    client *pMatch = NULL;
    for (int idx = 0; idx < MAX_CLIENTS; idx++) {
        if (clients[idx] != NULL && clients[idx]->socket == socket) {
            pMatch = clients[idx];
            break;
        }
    }
    pthread_mutex_unlock(&clients_mutex);
    return pMatch;
}

/**
 * Removes a client by locating it via socket descriptor, if it exists.
 *
 * @param socket The socket descriptor of the client
 * @return TRUE if the client was found and removed; FALSE otherwise
 */
int detach_client_by_socket(int socket) {
    client *pClient = locate_client_by_socket(socket);
    if (pClient == NULL) {
        return FALSE;
    }
    return detach_client(pClient);
}


/**
 * Removes the specified client from the global array, closes its socket, and frees its memory.
 *
 * @param cl The client to remove
 * @return TRUE if the client was found and removed; FALSE otherwise
 */
int detach_client(client *cl) {
    pthread_mutex_lock(&clients_mutex);

    for (int idx = 0; idx < MAX_CLIENTS; idx++) {
        if (clients[idx] == cl) {
            printf("Remove client: %d found\n", cl->id);

            // Close the socket
            close(cl->socket);
            printf("Remove client: %d socket closed\n", cl->id);

            // Free the structure and nullify the pointer
            free(clients[idx]);
            clients[idx] = NULL;

            pthread_mutex_unlock(&clients_mutex);

            display_all_clients();
            return TRUE;
        }
    }
    pthread_mutex_unlock(&clients_mutex);

    display_all_clients();
    return FALSE;
}



/**
 * The main thread function that initiates communication with the client,
 * sends an initial LOGIN message, and continuously processes incoming data.
 *
 * @param arg A pointer to the client structure
 * @return A void pointer (unused)
 */
void *client_thread_main(void *arg) {
    client *pClient = (client *)arg;

    char tempBuff[LOGIN_MESSAGE_RESP_SIZE] = {0};
    sprintf(tempBuff, "LOGIN;%s\n", pClient->username);
    send_mess(pClient, tempBuff);

    // This function does not return until the client disconnects or an error occurs
    receive_messages(pClient);
    return NULL;
}


