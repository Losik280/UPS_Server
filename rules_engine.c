#include "rules_engine.h"
#include "match_manager.h"
#include <stdio.h>


/**
 * Helper function to check if a move is valid for a player in a specific direction.
 */
int is_valid_move(char player_char, int x, int y, int dx, int dy, game *g) {
    char opponent_char = (player_char == 'R') ? 'B' : 'R';
    int i = x + dx;
    int j = y + dy;
    int found_opponent = 0;

    // Step through the direction
    while (i >= 0 && i < BOARD_SIZE && j >= 0 && j < BOARD_SIZE) {
        if (g->board[i][j] == opponent_char) {
            found_opponent = 1;
        } else if (g->board[i][j] == player_char) {
            return found_opponent; // Valid move if opponent's stones are enclosed
        } else {
            break; // Empty or invalid space
        }
        i += dx;
        j += dy;
    }
    return 0; // No valid move in this direction
}

/**
 * Helper function to get the opponent client.
 */
client *get_opponent_client(client *cl, game *g) {
    return (cl == g->player1) ? g->player2 : g->player1;
}

/**
 * @brief Validates the game status for the given client.
 *
 * This function checks if the current player has any available moves. If no moves are available,
 * it ends the game and determines the winner based on the current board state.
 *
 * @param cl Pointer to the client structure.
 * @param to_x The x-coordinate of the move.
 * @param to_y The y-coordinate of the move.
 * @return int Returns GAME_WIN if the game is won, GAME_DRAW if it's a draw, or 0 if the game continues.
 */
int check_available_moves(client *cl) {
    game *g = fetch_game_by_id(cl->active_game_id);
    pthread_mutex_lock(&g_gamesMutex);

    // Switch the player for opponent
    cl = get_opponent_client(cl, g);

    // Define directions: horizontal, vertical, and diagonals
    int directions[8][2] = {
            {1,  0},
            {-1, 0},  // Horizontal
            {0,  1},
            {0,  -1},  // Vertical
            {1,  1},
            {-1, -1}, // Diagonal (\)
            {1,  -1},
            {-1, 1}  // Diagonal (/)
    };

    // Check if the current player has available moves
    int available_moves = 0;
    for (int i = 0; i < BOARD_SIZE; i++) {
        for (int j = 0; j < BOARD_SIZE; j++) {
            if (g->board[i][j] == EMPTY_CHAR) {
                // Check all directions for a valid move
                for (int d = 0; d < 8; d++) {
                    int dx = directions[d][0];
                    int dy = directions[d][1];
                    if (is_valid_move(cl->client_char, i, j, dx, dy, g)) {
                        available_moves++;
                        break; // No need to check further directions
                    }
                }
            }
        }
    }

    // If the current player has no moves, end the game
    if (available_moves == 0) {
        g->game_status = GAME_OVER;

        // Count the score for both players
        int score_X = 0, score_O = 0;
        for (int i = 0; i < BOARD_SIZE; i++) {
            for (int j = 0; j < BOARD_SIZE; j++) {
                if (g->board[i][j] == 'R') score_X++;
                else if (g->board[i][j] == 'B') score_O++;
            }
        }

        // Determine the winner
        if (score_X > score_O) {
            g->winner = cl->client_char == 'R' ? get_opponent_client(cl, g) : cl;
            pthread_mutex_unlock(&g_gamesMutex);
            return GAME_WIN;
        } else if (score_O > score_X) {
            g->winner = cl->client_char == 'B' ? get_opponent_client(cl, g) : cl;
            pthread_mutex_unlock(&g_gamesMutex);
            return GAME_WIN;
        } else {
            g->winner = NULL; // It's a draw
            pthread_mutex_unlock(&g_gamesMutex);
            return GAME_DRAW;
        }
    }

    // Game continues if the player has available moves
    pthread_mutex_unlock(&g_gamesMutex);

    return 0;
}


/**
 * @brief Validates a move for the given client.
 *
 * This function checks if the move to the specified coordinates is valid for the current player.
 * It ensures the move is within the board, the target field is empty, and the move encloses
 * the opponent's pieces in any direction.
 *
 * @param cl Pointer to the client structure.
 * @param to_x The x-coordinate of the move.
 * @param to_y The y-coordinate of the move.
 * @return int Returns TRUE if the move is valid, INVALID_MOVE if the move is invalid,
 *         GAME_NOT_FOUND if the game is not found, or NOT_MY_TURN if it's not the client's turn.
 */
int validate_move(client *cl, int to_x, int to_y) {
    // Fetch the game by client ID
    game *g = fetch_game_by_id(cl->active_game_id);
    if (g == NULL) {
        return GAME_NOT_FOUND;
    }

    if (g->game_status != GAME_PLAYING) {
        return GAME_NOT_FOUND;
    }

    if (g->current_player != cl) {
        return NOT_MY_TURN;
    }

    // Check if the move is within the board
    if (to_x < 0 || to_x >= BOARD_SIZE || to_y < 0 || to_y >= BOARD_SIZE) {
        return INVALID_MOVE;
    }

    // The target field must be empty
    if (g->board[to_y][to_x] != EMPTY_CHAR) {
        return FIELD_TAKEN;
    }

    char opponent_char = (cl->client_char == FIRST_PL_CHAR) ? SECOND_PL_CHAR : FIRST_PL_CHAR;
    int valid = FALSE;

    // Check all 8 directions
    int directions[8][2] = {
            {0,  1},
            {1,  0},
            {0,  -1},
            {-1, 0}, // Vertical & horizontal
            {1,  1},
            {1,  -1},
            {-1, 1},
            {-1, -1} // Diagonal
    };

    for (int i = 0; i < 8; i++) {
        int dx = directions[i][0];
        int dy = directions[i][1];
        int nx = to_x + dx;
        int ny = to_y + dy;
        int count = 0;

        // Move in the direction and check for opponent's pieces
        while (nx >= 0 && nx < BOARD_SIZE && ny >= 0 && ny < BOARD_SIZE && g->board[ny][nx] == opponent_char) {
            nx += dx;
            ny += dy;
            count++;
        }

        // Ensure we end at the player's own piece after traversing opponent's pieces
        if (count > 0 && nx >= 0 && nx < BOARD_SIZE && ny >= 0 && ny < BOARD_SIZE &&
            g->board[ny][nx] == cl->client_char) {
            valid = TRUE;
            break; // No need to check further directions once a valid move is found
        }
    }

    // If the move is valid, apply it
    if (valid) {
        apply_move(g, cl, to_x, to_y);
    }

    return valid ? TRUE : INVALID_MOVE;
}

/**
 * @brief Applies a move for the given client.
 *
 * This function updates the game board by placing the client's piece at the specified coordinates
 * and flipping the opponent's pieces that are enclosed by the move.
 *
 * @param g Pointer to the game structure.
 * @param cl Pointer to the client structure.
 * @param to_x The x-coordinate of the move.
 * @param to_y The y-coordinate of the move.
 */
void apply_move(game *g, client *cl, int to_x, int to_y) {
    int directions[8][2] = {
            {0,  1},
            {1,  0},
            {0,  -1},
            {-1, 0},
            {1,  1},
            {1,  -1},
            {-1, 1},
            {-1, -1}
    };

    char opponent_char = (cl->client_char == FIRST_PL_CHAR) ? SECOND_PL_CHAR : FIRST_PL_CHAR;

    for (int i = 0; i < 8; i++) {
        int dx = directions[i][0];
        int dy = directions[i][1];
        int nx = to_x + dx;
        int ny = to_y + dy;
        int count = 0;

        while (nx >= 0 && nx < BOARD_SIZE && ny >= 0 && ny < BOARD_SIZE && g->board[ny][nx] == opponent_char) {
            nx += dx;
            ny += dy;
            count++;
        }

        if (count > 0 && nx >= 0 && nx < BOARD_SIZE && ny >= 0 && ny < BOARD_SIZE &&
            g->board[ny][nx] == cl->client_char) {
            nx = to_x + dx;
            ny = to_y + dy;
            while (g->board[ny][nx] == opponent_char) {
                g->board[ny][nx] = cl->client_char;
                nx += dx;
                ny += dy;
            }
        }
    }

    g->board[to_y][to_x] = cl->client_char;

    // Print the board
    for (int i = 0; i < BOARD_SIZE; i++) {
        for (int j = 0; j < BOARD_SIZE; j++) {
            char pom = g->board[i][j];
            if (pom == ' ') {
                printf("-");
            }
            printf("%c", g->board[i][j]);
        }
        printf("\n");
    }
    g->current_player = (cl == g->player1) ? g->player2 : g->player1;
}
