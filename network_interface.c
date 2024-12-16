#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <string.h>
#include <stdlib.h>
#include "network_interface.h"
#include "client_manager.h"
#include "rules_engine.h"

void game_status_response(client *cl, int status) {
    char response[GAME_STATUS_RESP_SIZE] = {0};

    if (status == GAME_WIN) {
        sprintf(response, "GAME_STATUS;%s\n", cl->username);
        if (cl->opponent != NULL) {
            send_mess(cl->opponent, response);
            clean_client_game(cl->opponent);
        }
//        send_mess(cl->opponent, response);
        send_mess(cl, response);
        remove_game(cl);
        clean_client_game(cl);
    } else if (status == GAME_DRAW) {
        sprintf(response, "GAME_STATUS;DRAW\n");
        if (cl->opponent != NULL) {
            send_mess(cl->opponent, response);
            clean_client_game(cl->opponent);
        }
        send_mess(cl, response);
        remove_game(cl);
        clean_client_game(cl);
    }
}

void ping_game_status_response(client *cl, int status) {
    char response[GAME_STATUS_RESP_SIZE] = {0};
    if (status == GAME_WIN) {
        sprintf(response, "GAME_STATUS;%s\n", cl->username);
        send_mess(cl, response);
    } else if (status == GAME_DRAW) {
        sprintf(response, "GAME_STATUS;DRAW\n");
        send_mess(cl, response);
    }
}

void move_response(client *cl, int status, int x, int y) {
    char response[MOVE_MESS_RESP_SIZE] = {0};

    if (status == TRUE) {
        sprintf(response, "MOVE;%c;%c;%c\n", status + '0', x + '0', y + '0');
        send_mess(cl, response);

        if (cl->opponent != NULL) {
            char mess[OPP_MOVE_MESSAGE_SIZE] = {0};
            sprintf(mess, "OPP_MOVE;%c;%c\n", x + '0', y + '0');
            send_mess(cl->opponent, mess);
        }
    } else {
        sprintf(response, "MOVE;%c;0;0\n", status + '0');
        send_mess(cl, response);
    }
}

void serve_opp_disconnected(client *cl, char *token) {
    if (strncmp(token, "WAIT", 4) == 0) {
        // client wants to wait for opponent -> do nothing
        printf("Client %d waits for opponent %d\n", cl->id, cl->opponent->id);
    } else {
        ping_game_status_response(cl, GAME_DRAW);

        // remove client connection to the game and his opponent
        pthread_mutex_lock(&clients_mutex);
        cl->opponent->opponent = NULL;
        pthread_mutex_unlock(&clients_mutex);

        printf("Serve opp disconnected: %d ping status response has been sent\n", cl->id);

        clean_client_game(cl);
        printf("Serve opp disconnected: %d game cleaned\n", cl->id);
    }
}


void receive_messages(client *cl) {
//    client *cl = (client *) arg;
    int client_socket = cl->socket;
    char message[MESSAGE_SIZE] = {0};
    int valread;
    while (1) {
        valread = recv(client_socket, message, sizeof(message), 0);
        if (valread <= 0) {
            printf("Client must be disconnected...\n");
            close(client_socket);
            remove_client(cl);
            break;
        }
        printf("Client: %d sent message", cl->id);
        serve_message(cl, message);
        memset(message, 0, sizeof(message));
        valread = 0;
    }
    printf("Client thread ends\n");
}

void reconnect_message(client *cl) {
    game *cl_game = find_client_game(cl);

    // game not found -> send want game message
    if (cl_game == NULL) {
        printf("Game not found\n");
    } else {
        // game found -> send reconnect message
        char response[RECONNECT_MESSAGE_SIZE] = {0};
        // format: RECONNECT;board;current_player;opponent
        sprintf(response, "RECONNECT;");

        for (int i = 0; i < BOARD_SIZE; i++) {
            for (int j = 0; j < BOARD_SIZE; j++) {
                sprintf(response + strlen(response), "%c", cl_game->board[i][j]);
            }
        }

        sprintf(response + strlen(response), ";%s;%s;", cl_game->current_player->username, cl->opponent->username);
        char opp_response[RECONNECT_MESSAGE_SIZE] = {0};
        strcpy(opp_response, response);

        sprintf(response + strlen(response), "%c\n", cl->opponent->client_char);
        sprintf(opp_response + strlen(opp_response), "%c\n", cl->client_char);

        printf("Response: %s\n", response);
        send_mess(cl, response);
        send_mess(cl->opponent, opp_response);
    }
}

void *send_mess(client *client, char *mess) {
    printf("Sending client: %d -> message: %s", client->id, mess);
    int client_socket = client->socket;
    send(client_socket, mess, strlen(mess), 0);
    return NULL;
}

void serve_message(client *cl, char *message) {
    // get the first token
    char *token = strtok(message, MESS_DELIMITER);

    // check message type
    if (strcmp(token, "MOVE") == 0) {
        // get the second token
        int to_x = atoi(strtok(NULL, MESS_DELIMITER));
        int to_y = atoi(strtok(NULL, MESS_DELIMITER));
        int move_status = validate_move(cl, to_x, to_y);

        int game_status = validate_game_status(cl,  to_x, to_y);

        move_response(cl, move_status,  to_x, to_y);


        game_status_response(cl, game_status);
    } else if (strcmp(token, "WANT_GAME") == 0) {
        want_game_response(cl);
    } else if (strcmp(token, "LOGOUT") == 0) {
        if (cl->opponent != NULL) {
            game *cl_game = find_client_game(cl);
            cl_game->game_status = GAME_OVER;
            game_status_response(cl->opponent, GAME_WIN);
        }

        pthread_t thread = *cl->client_thread;
        remove_client(cl);
        pthread_join(thread, NULL);
    } else if (strcmp(token, "PONG") == 0) {
        printf("PONG - Client %d is connected\n", cl->id);
        client_ping(cl, 1);
    } else if (strcmp(token, "OPP_DISCONNECTED") == 0) {
        token = strtok(NULL, MESS_DELIMITER);
        serve_opp_disconnected(cl, token);
    } else {
        printf("Invalid message -> remove\n");
        pthread_t thread = *cl->client_thread;
        remove_client(cl);
        pthread_join(thread, NULL);
    }
}

void *send_mess_by_socket(int socket, char *mess) {
    printf("Sending message: %s", mess);
    send(socket, mess, strlen(mess), 0);
    return NULL;
}

void *run_ping() {
    pthread_mutex_lock(&clients_mutex);
    int i;
    while (1) {
        for (i = 0; i < MAX_CLIENTS; i++) {
            if (clients[i] != NULL) {
                clients[i]->is_connected = 0;
                send_mess_by_socket(clients[i]->socket, "PING\n");
            }
        }
        pthread_mutex_unlock(&clients_mutex);

        sleep(PING_SLEEP);

        // control whether clients response
        pthread_mutex_lock(&clients_mutex);
        for (i = 0; i < MAX_CLIENTS; i++) {
            if (clients[i] != NULL) {
                printf("Run ping: client %d is connected: %d\n", clients[i]->id, clients[i]->is_connected);
                // if client is not connected and last ping was more than PING_ZOMBIE seconds ago -> remove client
                if (clients[i]->is_connected == 0 && clients[i]->last_ping + PING_ZOMBIE < time(NULL)) {
                    printf("Run ping: Client %d disconnected\n", clients[i]->id);

                    pthread_t thread = *clients[i]->client_thread;

                    if (clients[i]->opponent != NULL) {
                        // opponent waits for the client -> send game status message
                        printf("Run ping: MUST SEND GAME STATUS TO OPPONENT\n");
                        pthread_mutex_unlock(&clients_mutex);
                        ping_game_status_response(clients[i]->opponent, GAME_WIN);
                        pthread_mutex_lock(&clients_mutex);
                    }

                    if (clients[i]->current_game_id != GAME_NULL_ID) {
                        // game is not removed -> remove it
                        game *cl_game = find_client_game(clients[i]);
                        pthread_mutex_lock(&mutex_games);
                        cl_game->game_status = GAME_OVER;
                        pthread_mutex_unlock(&mutex_games);
                        remove_game(clients[i]);

                        pthread_mutex_unlock(&clients_mutex);
                        if (clients[i]->opponent != NULL) {
                            clean_client_game(clients[i]->opponent);
                        }
                        clean_client_game(clients[i]);
                        pthread_mutex_lock(&clients_mutex);
                    }

                    pthread_mutex_unlock(&clients_mutex);
                    remove_client(clients[i]);
                    pthread_cancel(thread);
                    pthread_mutex_lock(&clients_mutex);
                    printf("Run ping:  Client removed\n");
                }

                    // if client is connected and need reconnect message
                else if (clients[i]->is_connected && clients[i]->need_reconnect_mess == 1) {
                    printf("Run ping: NEED RECONNECT MESSAGE\n");
                    clients[i]->need_reconnect_mess = 0;

                    // opponent is connected (waits) -> send reconnect message
                    if (clients[i]->opponent != NULL) {
                        pthread_mutex_unlock(&clients_mutex);

                        reconnect_message(clients[i]);

                        pthread_mutex_lock(&clients_mutex);
                    } else if (clients[i]->want_game == FALSE) {
                        // opponent is not connected in game (did not want to wait) -> send game status message
                        char response[GAME_STATUS_RESP_SIZE] = {0};
                        sprintf(response, "GAME_STATUS;OPP_DISCONNECTED\n");
                        pthread_mutex_unlock(&clients_mutex);
                        send_mess_by_socket(clients[i]->socket, response);

                        game *cl_game = find_client_game(clients[i]);
                        pthread_mutex_lock(&mutex_games);
                        cl_game->game_status = GAME_OVER;
                        pthread_mutex_unlock(&mutex_games);
                        remove_game(clients[i]);

                        clean_client_game(clients[i]);
                        pthread_mutex_lock(&clients_mutex);
                    }
                    // else -> client is in waiting state -> do nothing
                }

                    // if client is not connected, but still no zombie time, set need_reconnect_mess to 1
                else if (clients[i]->is_connected == 0) {
                    printf("Run ping: SET NEED RECONNECT MESSAGE\n");
                    if (clients[i]->opponent != NULL && clients[i]->need_reconnect_mess == 0) {
                        send_mess_by_socket(clients[i]->opponent->socket, "OPP_DISCONNECTED\n");
                    }
                    clients[i]->need_reconnect_mess = 1;
                }
            }
        }
        pthread_mutex_unlock(&clients_mutex);
    }
}

void want_game_response(client *cl) {
    printf("Client %d wants to play\n", cl->id);
    set_want_game(cl, TRUE);
    int found = find_waiting_player(cl);

    char response[WANT_GAME_RESP_SIZE] = {0};
    if (found == FALSE) {
        sprintf(response, "WANT_GAME;%c\n", FIRST_PL_CHAR);
    } else {
        sprintf(response, "WANT_GAME;%c\n", SECOND_PL_CHAR);
    }
    send_mess(cl, response);

    if (found == TRUE) {
        // Start the game
        char game_response[START_GAME_MESSAGE_SIZE] = {0};
        sprintf(game_response, "START_GAME;%s;%c;%c\n", cl->opponent->username, cl->opponent->client_char,
                cl->is_playing ? '1' : '0');
        send_mess(cl, game_response);
    }
}