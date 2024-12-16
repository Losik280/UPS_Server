/**
* Define logic for the game Reversi|
*/
#ifndef __GAME_MANAGER_H__
#define __GAME_MANAGER_H__

#include "config.h"

/**
 * @brief Mutex for the games array
 */
extern pthread_mutex_t mutex_games;

/**
 * @brief Creates a new game
 * @param player_1 player who started the game
 * @param player_2 player who joined the game
 * @return pointer to the created game
 */
game *create_new_game(client *player_1, client *player_2);

/**
 * @brief Find the game by the client
 * @param cl client
 * @return pointer to the game or NULL if not found
 */
game *find_client_game(client *cl);

/**
 * @brief Finds the game by its ID
 * @param id ID of the game
 * @return pointer to the game or NULL if not found
 */
game *get_game_by_id(int id);

/**
 * @brief Creates and initialize the game board
 * @param board game board
 * @return initialized game board with empty fields
 */
void initialize_board(char board[BOARD_SIZE][BOARD_SIZE]);

/**
 * @brief Check if the game is ended and remove it
 * @param cl client who made last move
 * @return TRUE if the game was removed, FALSE if not found
 */
int remove_game(client *cl);

void print_games();

#endif