#ifndef __COMMUNICATION_H__
#define __COMMUNICATION_H__

#include "config.h"

/**
 * @brief Send message to the client
 * @param cl client to send message to
 * @param status status of the game
 */
void game_status_response(client *cl, int status);

/**
 * @brief Serve client playing game according to status
 * @param cl client that made move
 * @param status status of the move
 * @param x x coordinate
 * @param y y coordinate
 */
void move_response(client *cl, int status, int x, int y);

/**
 * @brief Consider message from the client
 * @param cl client that sent the message
 * @param message message from the client
 */
void serve_message(client *cl, char *message);

/**
 * @brief Send message to the client by socket
 * @return void
 */
void *send_mess_by_socket(int socket, char *mess);

/**
 * @brief Send message to the client
 * @return
 */
void *send_mess(client *client, char *mess);

/**
 * @brief Receive message from the client
 * @param client pointer
 * @return void pointer
 */
//void *receive_messages(void *arg);
void receive_messages(client *cl);

/**
 * @brief Controls clients connection
 * @return void pointer
 */
void *run_ping();

/**
 * @brief Send response to the client that wants to play
 * @param cl client that wants to play
 */
void want_game_response(client *cl);


#endif