/**
 * @file match_manager.h
 * @brief Defines high-level logic for Reversi g_gamesArr, including creation and removal of matches.
 */

#ifndef __MATCH_MANAGER_H__
#define __MATCH_MANAGER_H__

#include "def_n_struct.h"
#include <pthread.h>

/**
 * A global mutex used to protect access to the `g_gamesArr` array.
 */
extern pthread_mutex_t g_gamesMutex;

/**
 * Creates a new Reversi game between two clients.
 *
 * @param player_1 The first participant in the new game
 * @param player_2 The second participant in the new game
 * @return Pointer to the newly created game structure, or NULL if creation fails
 */
game *initiate_game_session(client *player_1, client *player_2);

/**
 * Searches for the game in which the specified client is currently participating.
 *
 * @param cl Pointer to a client structure
 * @return Pointer to the corresponding game, or NULL if none is found
 */
game *locate_game_for_client(client *cl);

/**
 * Locates a game by its unique identifier.
 *
 * @param id Integer representing the game's ID
 * @return Pointer to the game if found, or NULL otherwise
 */
game *fetch_game_by_id(int id);

/**
 * Initializes the game board by filling in default Reversi starting positions.
 *
 * @param board A 2D array (BOARD_SIZE x BOARD_SIZE) representing the game board
 */
void setup_initial_board(char board[BOARD_SIZE][BOARD_SIZE]);

/**
 * Removes a game from the global array if it is marked as finished by the specified client.
 *
 * @param cl The client who last interacted with the game
 * @return TRUE if the game was found and removed, FALSE otherwise
 */
int purge_finished_game(client *cl);

/**
 * Prints the details of all active g_gamesArr to stdout.
 */
void display_active_games();

#endif
