#ifndef __NETWORK_INTERFACE_H__
#define __NETWORK_INTERFACE_H__

#include "def_n_struct.h"

/**
 * Declares that a client wants to participate in a game
 * and sends a response to the client if an opponent is found.
 *
 * @param cl Pointer to the client struct
 */
void handle_game_request(client *cl);

/**
 * Processes an incoming message from a particular client and executes the necessary logic.
 *
 * @param cl Pointer to the client struct that sent the message
 * @param message The message itself
 */
void process_client_message(client *cl, char *message);

/**
 * Sends feedback to the client (and possibly the opponent) after a move attempt,
 * indicating whether the move was valid and where it occurred.
 *
 * @param cl Pointer to the client that made the move
 * @param status Indicates success (TRUE) or an invalid move
 * @param x The x-coordinate of the move
 * @param y The y-coordinate of the move
 */
void respond_to_move(client *cl, int status, int x, int y);

/**
 * Sends a game status notification to the client (e.g., draw or win).
 *
 * @param cl Pointer to the client to notify
 * @param status The final status of the game (e.g., GAME_DRAW, GAME_WIN)
 */
void notify_game_status(client *cl, int status);

/**
 * Sends a message to a client using its client structure.
 *
 * @param client Pointer to the recipient's client struct
 * @param mess The text to send
 * @return A void pointer (unused)
 */
void *transmit_message(client *client, char *mess);

/**
 * Sends a message to a client, identified only by its socket descriptor.
 *
 * @param socket The socket descriptor of the destination client
 * @param mess The text to send
 * @return A void pointer (unused)
 */
void *transmit_message_by_socket(int socket, char *mess);

/**
 * Continuously listens for and processes incoming messages from the given client.
 *
 * @param cl Pointer to the client
 */
void listen_for_messages(client *cl);

/**
 * Periodically checks whether clients are still responsive and handles reconnection or removal.
 *
 * @return A void pointer (unused)
 */
void *monitor_client_pings();

#endif
