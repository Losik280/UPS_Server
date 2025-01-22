#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <string.h>
#include <stdlib.h>
#include "network_interface.h"
#include "player_manager.h"
#include "rules_engine.h"

/**
 * A helper function that sends RECONNECT details if the client was in a game.
 * Not declared in the header, as it's used only internally here.
 *
 * @param cl Pointer to the client
 */
void reconnect_message(client *cl) {
    game *theGame = locate_game_for_client(cl);

    if (theGame == NULL) {
        printf("Game not found\n");
    } else {
        // Rebuild the board state for both players
        char response[RECONNECT_MESSAGE_SIZE] = {0};
        sprintf(response, "RECONNECT;");

        for (int row = 0; row < BOARD_SIZE; row++) {
            for (int col = 0; col < BOARD_SIZE; col++) {
                sprintf(response + strlen(response), "%c", theGame->board[row][col]);
            }
        }

        sprintf(response + strlen(response), ";%s;%s;", theGame->current_player->username, cl->opponent->username);

        char oppResponse[RECONNECT_MESSAGE_SIZE] = {0};
        strcpy(oppResponse, response);

        sprintf(response + strlen(response), "%c\n", cl->opponent->client_char);
        sprintf(oppResponse + strlen(oppResponse), "%c\n", cl->client_char);

        printf("Response: %s\n", response);
        transmit_message(cl, response);
        transmit_message(cl->opponent, oppResponse);
    }
}

/**
 * Declares that a client wants to participate in a game
 * and sends a response to the client if an opponent is found.
 *
 * @param cl Pointer to the client struct
 */
void handle_game_request(client *cl) {
    printf("Client %d wants to play\n", cl->id);
    set_game_request(cl, TRUE);
    int matchFound = match_waiting_opponent(cl);

    char response[WANT_GAME_RESP_SIZE] = {0};
    if (matchFound == FALSE) {
        sprintf(response, "JOIN_GAME;%c\n", FIRST_PL_CHAR);
    } else {
        sprintf(response, "JOIN_GAME;%c\n", SECOND_PL_CHAR);
    }
    transmit_message(cl, response);

    if (matchFound == TRUE) {
        // Start the game
        char startMsg[START_GAME_MESSAGE_SIZE] = {0};
        sprintf(startMsg, "START_GAME;%s;%c;%c\n",
                cl->opponent->username,
                cl->opponent->client_char,
                cl->is_in_game ? '1' : '0');
        transmit_message(cl, startMsg);
    }
}

/**
 * A local helper function (not in .h) used for quickly sending partial game status
 * messages (e.g., for "WAIT" or forced draw). Not exposed in the header.
 */
void ping_game_status_response(client *cl, int status) {
    char response[GAME_STATUS_RESP_SIZE] = {0};

    if (status == GAME_WIN) {
        sprintf(response, "GAME_STATUS;%s\n", cl->username);
        transmit_message(cl, response);
    } else if (status == GAME_DRAW) {
        sprintf(response, "GAME_STATUS;DRAW\n");
        transmit_message(cl, response);
    }
}

/**
 * Processes an incoming message from a particular client and executes the necessary logic.
 *
 * @param cl Pointer to the client struct that sent the message
 * @param message The message itself
 */
void process_client_message(client *cl, char *message) {
    // Read the first token to determine the type of message
    char *token = strtok(message, MESS_DELIMITER);

    if (strcmp(token, "MOVE") == 0) {
        // Extract coordinates
        int toX = atoi(strtok(NULL, MESS_DELIMITER));
        int toY = atoi(strtok(NULL, MESS_DELIMITER));

        int moveStatus = validate_move(cl, toX, toY);
        int finalStatus = check_available_moves(cl);

        respond_to_move(cl, moveStatus, toX, toY);
        notify_game_status(cl, finalStatus);

    } else if (strcmp(token, "JOIN_GAME") == 0) {
        handle_game_request(cl);

    } else if (strcmp(token, "LOGOUT") == 0) {
        if (cl->opponent != NULL) {
            game *clientGame = locate_game_for_client(cl);
            clientGame->game_status = GAME_OVER;
            notify_game_status(cl->opponent, GAME_WIN);
        }
        pthread_t localThread = *cl->client_thread;
        detach_client(cl);
        pthread_join(localThread, NULL);

    } else if (strcmp(token, "PONG") == 0) {
        printf("PONG - Client %d is connected\n", cl->id);
        update_client_ping(cl, 1);

    } else if (strcmp(token, "WAIT_REPLY") == 0) {
        token = strtok(NULL, MESS_DELIMITER);
        // handle WAIT or NOT_WAIT
        if (strncmp(token, "WAIT", 4) == 0) {
            // The client chooses to wait
            printf("Client %d waits for opponent %d\n", cl->id, cl->opponent->id);
        } else {
            // The client does not wait
            ping_game_status_response(cl, GAME_DRAW);
            pthread_mutex_lock(&clients_mutex);
            if (cl->opponent != NULL) {
                cl->opponent->opponent = NULL;
            }
            pthread_mutex_unlock(&clients_mutex);

            reset_client_game_data(cl);
            printf("Serve opp disconnected: %d game cleaned\n", cl->id);
        }

    } else {
        // Invalid message, remove the client
        printf("Invalid message -> remove\n");
        pthread_t localThread = *cl->client_thread;
        detach_client(cl);
        pthread_join(localThread, NULL);
    }
}

/**
 * Sends feedback to the client (and possibly the opponent) after a move attempt,
 * indicating whether the move was valid and where it occurred.
 *
 * @param cl Pointer to the client that made the move
 * @param status Indicates success (TRUE) or an invalid move
 * @param x The x-coordinate of the move
 * @param y The y-coordinate of the move
 */
void respond_to_move(client *cl, int status, int x, int y) {
    char response[MOVE_MESS_RESP_SIZE] = {0};

    if (status == TRUE) {
        sprintf(response, "MOVE;%c;%c;%c\n", status + '0', x + '0', y + '0');
        transmit_message(cl, response);

        if (cl->opponent != NULL) {
            char oppMsg[OPP_MOVE_MESSAGE_SIZE] = {0};
            sprintf(oppMsg, "OPP_MOVE;%c;%c\n", x + '0', y + '0');
            transmit_message(cl->opponent, oppMsg);
        }
    } else {
        sprintf(response, "MOVE;%c;0;0\n", status + '0');
        transmit_message(cl, response);
    }
}

/**
 * Sends a game status notification to the client (e.g., draw or win).
 *
 * @param cl Pointer to the client to notify
 * @param status The final status of the game (e.g., GAME_DRAW, GAME_WIN)
 */
void notify_game_status(client *cl, int status) {
    char response[GAME_STATUS_RESP_SIZE] = {0};

    if (status == GAME_WIN) {
        sprintf(response, "GAME_STATUS;%s\n", cl->username);
        if (cl->opponent != NULL) {
            transmit_message(cl->opponent, response);
            reset_client_game_data(cl->opponent);
        }
        transmit_message(cl, response);
        purge_finished_game(cl);
        reset_client_game_data(cl);

    } else if (status == GAME_DRAW) {
        sprintf(response, "GAME_STATUS;DRAW\n");
        if (cl->opponent != NULL) {
            transmit_message(cl->opponent, response);
            reset_client_game_data(cl->opponent);
        }
        transmit_message(cl, response);
        purge_finished_game(cl);
        reset_client_game_data(cl);
    }
}


/**
 * Sends a message to a client using its client structure.
 *
 * @param client Pointer to the recipient's client struct
 * @param mess The text to send
 * @return A void pointer (unused)
 */
void *transmit_message(client *client, char *mess) {
    printf("Sending client: %d -> message: %s", client->id, mess);
    int sockDesc = client->socket;
    send(sockDesc, mess, strlen(mess), 0);
    return NULL;
}

/**
 * Sends a message to a client, identified only by its socket descriptor.
 *
 * @param socket The socket descriptor of the destination client
 * @param mess The text to send
 * @return A void pointer (unused)
 */
void *transmit_message_by_socket(int socket, char *mess) {
    printf("Sending message: %s", mess);
    send(socket, mess, strlen(mess), 0);
    return NULL;
}

/**
 * Continuously listens for and processes incoming messages from the given client.
 *
 * @param cl Pointer to the client
 */
void listen_for_messages(client *cl) {
    int client_socket = cl->socket;
    char buffer[MESSAGE_SIZE] = {0};
    int bytesRead;

    while (1) {
        bytesRead = recv(client_socket, buffer, sizeof(buffer), 0);
        if (bytesRead <= 0) {
            printf("Client must be disconnected...\n");
            close(client_socket);
            detach_client(cl);
            break;
        }
        printf("Client: %d sent message", cl->id);
        process_client_message(cl, buffer);

        memset(buffer, 0, sizeof(buffer));
    }
    printf("Client thread ends\n");
}

/**
 * Periodically checks whether clients are still responsive and handles reconnection or removal.
 *
 * @return A void pointer (unused)
 */
void *monitor_client_pings() {
    pthread_mutex_lock(&clients_mutex);

    while (1) {
        // First pass: set is_connected to 0, send PING to each client
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (clients[i] != NULL) {
                clients[i]->is_connected = 0;
                transmit_message_by_socket(clients[i]->socket, "PING\n");
            }
        }
        pthread_mutex_unlock(&clients_mutex);

        sleep(PING_SLEEP);

        // Second pass: check who responded
        pthread_mutex_lock(&clients_mutex);
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (clients[i] != NULL) {
                printf("Run ping: client %d is connected: %d\n", clients[i]->id, clients[i]->is_connected);

                // If the client is unresponsive for too long -> remove
                if (clients[i]->is_connected == 0 && clients[i]->last_ping + PING_ZOMBIE < time(NULL)) {
                    printf("Run ping: Client %d disconnected\n", clients[i]->id);

                    pthread_t localThread = *clients[i]->client_thread;

                    // Possibly inform the opponent
                    if (clients[i]->opponent != NULL) {
                        printf("Run ping: MUST SEND GAME STATUS TO OPPONENT\n");
                        pthread_mutex_unlock(&clients_mutex);
                        ping_game_status_response(clients[i]->opponent, GAME_WIN);
                        pthread_mutex_lock(&clients_mutex);
                    }

                    // Remove the game if not already removed
                    if (clients[i]->active_game_id != GAME_NULL_ID) {
                        game *cGame = locate_game_for_client(clients[i]);
                        pthread_mutex_lock(&g_gamesMutex);
                        cGame->game_status = GAME_OVER;
                        pthread_mutex_unlock(&g_gamesMutex);
                        purge_finished_game(clients[i]);

                        pthread_mutex_unlock(&clients_mutex);
                        if (clients[i]->opponent != NULL) {
                            reset_client_game_data(clients[i]->opponent);
                        }
                        reset_client_game_data(clients[i]);
                        pthread_mutex_lock(&clients_mutex);
                    }

                    // Finally remove the client
                    pthread_mutex_unlock(&clients_mutex);
                    detach_client(clients[i]);
                    pthread_cancel(localThread);
                    pthread_mutex_lock(&clients_mutex);
                    printf("Run ping:  Client removed\n");

                    // If the client is connected but needs a reconnection message
                } else if (clients[i]->is_connected && clients[i]->need_reconnect_mess == 1) {
                    printf("Run ping: NEED RECONNECT MESSAGE\n");
                    clients[i]->need_reconnect_mess = 0;

                    if (clients[i]->opponent != NULL) {
                        pthread_mutex_unlock(&clients_mutex);
                        reconnect_message(clients[i]);
                        pthread_mutex_lock(&clients_mutex);

                    } else if (clients[i]->is_requesting_game == FALSE) {
                        // Opponent is not there -> remove the game and notify client
                        char tmpResponse[GAME_STATUS_RESP_SIZE] = {0};
                        sprintf(tmpResponse, "GAME_STATUS;OPP_DISCONNECTED\n");
                        pthread_mutex_unlock(&clients_mutex);
                        transmit_message_by_socket(clients[i]->socket, tmpResponse);

                        game *cGame = locate_game_for_client(clients[i]);
                        pthread_mutex_lock(&g_gamesMutex);
                        cGame->game_status = GAME_OVER;
                        pthread_mutex_unlock(&g_gamesMutex);
                        purge_finished_game(clients[i]);

                        reset_client_game_data(clients[i]);
                        pthread_mutex_lock(&clients_mutex);
                    }
                }
                    // If client didn't respond yet, but not zombie timed out, set need_reconnect_mess = 1
                else if (clients[i]->is_connected == 0) {
                    printf("Run ping: SET NEED RECONNECT MESSAGE\n");
                    if (clients[i]->opponent != NULL && clients[i]->need_reconnect_mess == 0) {
                        transmit_message_by_socket(clients[i]->opponent->socket, "OPP_DISCONNECTED\n");
                    }
                    clients[i]->need_reconnect_mess = 1;
                }
            }
        }
        pthread_mutex_unlock(&clients_mutex);
    }
}



