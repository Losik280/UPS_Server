#ifndef __PLAYER_MANAGER_H__
#define __PLAYER_MANAGER_H__

#include <pthread.h>
#include "def_n_struct.h"
#include "match_manager.h"

/**
 * Global mutex protecting operations on the global clients array.
 */
extern pthread_mutex_t clients_mutex;

/**
 * Global array holding pointers to all connected clients (up to MAX_CLIENTS).
 */
extern client *clients[MAX_CLIENTS];

/**
 * Attempts to register a new client into the global clients array.
 *
 * @param socket The socket descriptor for the new client
 * @param username The username of the new client
 * @param thread A reference to the thread handling this client
 * @return TRUE if the client was successfully added; FALSE if it already exists or the array is full
 */
int register_client(int socket, char *username, pthread_t *thread);

/**
 * Resets the specified client's game-related fields, such as current game ID.
 *
 * @param cl The client whose data should be cleared
 */
void reset_client_game_data(client *cl);

/**
 * Updates ping information for the specified client.
 *
 * @param cl The client to be updated
 * @param is_connected TRUE if the client is currently connected, FALSE otherwise
 */
void update_client_ping(client *cl, int is_connected);

/**
 * Searches for a client who is already waiting for an opponent; if found, creates a new game.
 *
 * @param cl The client ready to play
 * @return TRUE if a waiting opponent was found and a game can start; FALSE otherwise
 */
int match_waiting_opponent(client *cl);

/**
 * Retrieves a client that has the given socket descriptor.
 *
 * @param socket The socket descriptor
 * @return The client pointer if found; NULL otherwise
 */
client *locate_client_by_socket(int socket);

/**
 * Returns the number of currently connected clients.
 *
 * @return The count of connected clients
 */
int get_connected_clients_count();

/**
 * Prints a summary of all registered clients to stdout.
 */
void display_all_clients();

/**
 * Removes a specific client from the global clients array and closes its socket.
 *
 * @param cl The client to be removed
 * @return TRUE if the client was removed; FALSE if it wasn't found
 */
int detach_client(client *cl);

/**
 * Removes the client associated with the provided socket descriptor.
 *
 * @param socket The socket descriptor
 * @return TRUE if the client was removed; FALSE if it was not found
 */
int detach_client_by_socket(int socket);

/**
 * Entry point for the client handling thread.
 *
 * @param arg A pointer to the client
 * @return A void pointer (unused)
 */
void *client_thread_main(void *arg);

/**
 * Sets whether the client wants to participate in a game.
 *
 * @param cl The client
 * @param want_game TRUE if the client wants a game; FALSE otherwise
 */
void set_game_request(client *cl, int want_game);

#endif
