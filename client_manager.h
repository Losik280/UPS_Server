#ifndef __CLIENT_MANAGER_H__
#define __CLIENT_MANAGER_H__

#include <pthread.h>
#include "def_n_struct.h"
#include "match_manager.h"

// clients mutex
extern pthread_mutex_t clients_mutex;

extern client *clients[MAX_CLIENTS];

/**
 * Add new client to the array of clients
 * @param socket client's socket
 * @param username client's username
 * @param thread client's thread
 * @return TRUE if client was added, FALSE if client already exists
 */
int add_client(int socket, char *username, pthread_t *thread);

/**
 * Clean client's game data
 * @param cl client to clean
 */
void clean_client_game(client *cl);

/**
 * Client ping
 * @param cl client
 * @param is_connected TRUE if client is connected, FALSE if not
 */
void client_ping(client *cl, int is_connected);

/**
 * Find client who is waiting for another player and create a game if there is no waiting player
 * @param cl client who is ready to play
 * @return TRUE if game can start, FALSE if there is no waiting player or game was created as first player
 */
int find_waiting_player(client *cl);

/**
 * Get client by his socket
 * @param socket socket of the client
 * @return client or NULL if not found
 */
client *get_client_by_socket(int socket);

/**
 * Get count of connected clients
 * @return count of connected clients
 */
int get_connected_clients_count();

/**
 * Print all clients
 */
void print_clients();

/**
 * Remove client
 * @param cl client to remove
 * @return TRUE if client was removed, FALSE if not found
 */
int remove_client(client *cl);

/**
 * Remove client by his socket
 * @param socket socket of the client
 * @return TRUE if client was removed, FALSE if not found
 */
int remove_client_by_socket(int socket);

/**
 * Run client thread
 * @param arg client
 * @return void pointer
 */
void *run_client(void *arg);

/**
 * Set if client is waiting for a game
 * @param cl client
 * @param want_game TRUE if client wants to play, FALSE if not
 */
void set_want_game(client *cl, int want_game);



#endif
