#ifndef __CONFIG_H__
#define __CONFIG_H__

#include <pthread.h>

/**
 * @brief Messages constants
 */
#define MESSAGE_SIZE 1024
#define PLAYER_NAME_SIZE 21
#define MESS_DELIMITER ";"
#define MESS_END_CHAR "\n"
#define LOGIN_MESSAGE_SIZE (8 + PLAYER_NAME_SIZE)
#define LOGIN_MESSAGE_RESP_SIZE (9 + PLAYER_NAME_SIZE)
#define WANT_GAME_RESP_SIZE 13
#define START_GAME_MESSAGE_SIZE (16 + PLAYER_NAME_SIZE)
#define MOVE_MESS_RESP_SIZE 12
#define OPP_MOVE_MESSAGE_SIZE 14
#define GAME_STATUS_RESP_SIZE (14 + PLAYER_NAME_SIZE)
#define RECONNECT_MESSAGE_SIZE (BOARD_SIZE * BOARD_SIZE + 20 + PLAYER_NAME_SIZE + PLAYER_NAME_SIZE)


/**
 * @brief Game constants
 */
#define BOARD_SIZE 4
#define MAX_GAMES 10
#define MAX_CLIENTS 20
#define GAME_PLAYING 1
#define GAME_WAITING 0
#define GAME_OVER 2
#define GAME_WIN 1
#define GAME_DRAW 2
#define GAME_NULL_ID 0
#define FIRST_PL_CHAR 'R'
#define SECOND_PL_CHAR 'B'
#define EMPTY_CHAR ' '
#define WINNING_LENGTH 4

#define PING_SLEEP 5
#define PING_ZOMBIE 20



#define TRUE 1
#define FALSE 0


/**
 * @brief Server constants
 */
#define PORT 10000


typedef struct client client;

/**
 * @brief Struct for client
 */
typedef struct client {
    int socket;
    int id;
    char username[PLAYER_NAME_SIZE];
    int active_game_id;
    int is_in_game;
    int is_connected;
    int need_reconnect_mess;
    time_t last_ping;
    char client_char;
    int is_requesting_game;
    client *opponent;
    pthread_t *client_thread;
} client;

/**
 * @brief Struct for game
 */
typedef struct {
    int id;
    char board[BOARD_SIZE][BOARD_SIZE];
    client *player1;
    client *player2;
    client *current_player;
    int game_status;
    client *winner;
} game;

/**
 * @brief Struct for server address
 */
typedef struct {
    char ip_address[17];
    int port;
} server_address;

#endif