#include "client_manager.h"
#include <stdio.h>
#include <malloc.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include "config.h"
#include "communication.h"

// Mutex for clients array
pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;
// Array of all clients
client *clients[MAX_CLIENTS] = {0};

int add_client(int socket, char *username, pthread_t *thread) {
    if (get_client_by_socket(socket) != NULL) {
        return FALSE;
    }

    pthread_mutex_lock(&clients_mutex);

    // check capacity
    int count = 0;
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i] != NULL /*&& clients[i]->is_connected*/) {
            count++;
        }
    }
    if (count >= MAX_CLIENTS) {
        pthread_mutex_unlock(&clients_mutex);
        return FALSE;
    }

    client *cl = malloc(sizeof(client));
    if (cl == NULL) {
        perror("Failed to allocate memory for client");
        pthread_mutex_unlock(&clients_mutex);
        return FALSE;
    }


    cl->socket = socket;
    strcpy(cl->username, username);
    cl->current_game_id = GAME_NULL_ID;
    cl->is_playing = FALSE;
    cl->is_connected = TRUE;
    cl->need_reconnect_mess = FALSE;
    cl->last_ping = time(NULL);
    cl->client_char = EMPTY_CHAR;
    cl->opponent = NULL;
    cl->client_thread = thread;

    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i] == NULL) {
            cl->id = i;
            clients[i] = cl;
            break;
        }
    }

    pthread_mutex_unlock(&clients_mutex);
    return TRUE;
}

void clean_client_game(client *cl) {
    pthread_mutex_lock(&clients_mutex);
    cl->current_game_id = GAME_NULL_ID;
    cl->is_playing = FALSE;
    cl->client_char = EMPTY_CHAR;
    cl->opponent = NULL;
//    cl->want_game = FALSE;
    pthread_mutex_unlock(&clients_mutex);
}

void client_ping(client *cl, int is_connected) {
    pthread_mutex_lock(&clients_mutex);
    cl->is_connected = is_connected;
    cl->last_ping = time(NULL);
    pthread_mutex_unlock(&clients_mutex);
}

int find_waiting_player(client *cl) {
    pthread_mutex_lock(&clients_mutex);
    int found = FALSE;
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i] != NULL && strcmp(clients[i]->username, cl->username) != 0 && /*clients[i]->is_connected &&*/ clients[i]->current_game_id == GAME_NULL_ID && clients[i]->want_game == TRUE) {
            game *new_game = create_new_game(clients[i], cl);
            if (new_game == NULL) {
                break;
            }

            // clients[i] already waits -> has 'X'
            clients[i]->client_char = FIRST_PL_CHAR;
            clients[i]->is_playing = TRUE;
            clients[i]->current_game_id = new_game->id;
            clients[i]->opponent = cl;
            clients[i]->want_game = FALSE;

            // Start the game
            cl->client_char = SECOND_PL_CHAR;
            cl->is_playing = FALSE;
            cl->current_game_id = new_game->id;
            cl->opponent = clients[i];
            cl->want_game = FALSE;

            char response[START_GAME_MESSAGE_SIZE] = {0};
            sprintf(response, "START_GAME;%s;%c;%c\n", clients[i]->opponent->username, clients[i]->opponent->client_char, clients[i]->is_playing ? '1' : '0');
            send_mess(clients[i], response);

            found = TRUE;
            break;
        }
    }
    pthread_mutex_unlock(&clients_mutex);
    return found;
}


client *get_client_by_socket(int socket) {
    pthread_mutex_lock(&clients_mutex);
    client *result = NULL;
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i] != NULL && clients[i]->socket == socket) {
            result = clients[i];
            break;
        }
    }
    pthread_mutex_unlock(&clients_mutex);
    return result;
}

int get_connected_clients_count() {
    int count = 0;
    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i] != NULL /*&& clients[i]->is_connected*/) {
            count++;
        }
    }
    pthread_mutex_unlock(&clients_mutex);

    return count;
}

void print_clients() {
    pthread_mutex_lock(&clients_mutex);
    printf("Connected clients: \n");
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i] != NULL /*&& clients[i]->is_connected*/) {
            printf("    Client: %d; Game: %d; Socket: %d\n", clients[i]->id, clients[i]->current_game_id, clients[i]->socket);
        }
    }
    pthread_mutex_unlock(&clients_mutex);
}

int remove_client(client *cl) {
    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i] == cl) {
            printf("Remove client: %d found\n", cl->id);
            // close client socket
            close(cl->socket);
            printf("Remove client: %d socket closed\n", cl->id);

            free(clients[i]);
            clients[i] = NULL;
            pthread_mutex_unlock(&clients_mutex);

            //pthread_cancel(thread);
//            pthread_join(thread, NULL);
            print_clients();
            return TRUE;
        }
    }

    pthread_mutex_unlock(&clients_mutex);
    print_clients();

    return FALSE;
}

int remove_client_by_socket(int socket) {
    client *cl = get_client_by_socket(socket);
    if (cl == NULL) {
        return FALSE;
    }
    return remove_client(cl);
}

void *run_client(void *arg) {
    client *cl = (client *) arg;

//    int found = find_waiting_player(cl);

    char response[LOGIN_MESSAGE_RESP_SIZE] = {0};
    sprintf(response, "LOGIN;%s\n", cl->username);
//    if (found == FALSE) {
//        sprintf(response, "LOGIN;%s;%c\n", cl->username, FIRST_PL_CHAR);
//    } else {
//        sprintf(response, "LOGIN;%s;%c\n", cl->username, SECOND_PL_CHAR);
//    }
    send_mess(cl, response);

//    if (found == TRUE) {
//        // Start the game
//        char game_response[START_GAME_MESSAGE_SIZE] = {0};
//        sprintf(game_response, "START_GAME;%s;%c;%c\n", cl->opponent->username, cl->opponent->client_char, cl->is_playing ? '1' : '0');
//        send_mess(cl, game_response);
//    }

    receive_messages(cl);
}

void set_want_game(client *cl, int want_game) {
    pthread_mutex_lock(&clients_mutex);
    cl->want_game = want_game;
    pthread_mutex_unlock(&clients_mutex);
}
