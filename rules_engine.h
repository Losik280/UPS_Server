#ifndef __GAME_LOGIC_H__
#define __GAME_LOGIC_H__

#include "def_n_struct.h"

/**
 * @brief Error states constants
 */
// Move states
#define GAME_NOT_FOUND 5
#define NOT_MY_TURN 6
#define INVALID_MOVE 7
#define FIELD_TAKEN 8

/**
 * @brief Validates the move of the player and updates the game board
 * @param cl client who made the move
 * @param from_x from_x coordinate
 * @param from_y from_y coordinate
 * @return TRUE if the move was successful, Error states for move otherwise
 */
int validate_move(client *cl, int to_x, int to_y);

/**
 * @brief Validates the game status
 * @param cl client who made the move
 * @param x x coordinate (last move)
 * @param y y coordinate (last move)
 * @return 0 game is not finished, 1 game wins someone, 2 game is draw
 */
int check_available_moves(client *cl, int to_x, int to_y);

void apply_move(game *g, client *cl, int to_x, int to_y);

#endif