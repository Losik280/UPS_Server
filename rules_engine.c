#include "rules_engine.h"
#include "match_manager.h"
#include "client_manager.h"
#include "network_interface.h"
#include <stdio.h>
#include <stdlib.h>


//int validate_game_status(client *cl, int to_x, int to_y) {
////    game *g = get_game_by_id(cl->current_game_id);
////    pthread_mutex_lock(&mutex_games);
////    // look from the last move to all directions
////    int directions[4][2] = {
////            {1, 0},  // horizontal
////            {0, 1},  // vertical
////            {1, 1},  // diagonal (/)
////            {1, -1}  // diagonal (\)
////    };
////
////    for (int i = 0; i < 4; i++) {
////        int dx = directions[i][0];
////        int dy = directions[i][1];
////        int count = count_in_direction(cl->client_char, from_x, from_y, dx, dy, g);
////
////        // if number of player's chars in the direction is equal or
////        //   higher to WINNING_LENGTH, player wins
////        if (count >= WINNING_LENGTH) {
////            g->game_status = GAME_OVER;
////            g->winner = cl;
////            pthread_mutex_unlock(&mutex_games);
////            return GAME_WIN;
////        }
////    }
////
////    // check if the board is full
////    int empty_fields = 0;
////    for (int i = 0; i < BOARD_SIZE; i++) {
////        for (int j = 0; j < BOARD_SIZE; j++) {
////            if (g->board[i][j] == EMPTY_CHAR) {
////                empty_fields++;
////            }
////        }
////    }
////    if (empty_fields == 0) {
////        g->game_status = GAME_OVER;
////        g->winner = NULL;
////        pthread_mutex_unlock(&mutex_games);
////        return GAME_DRAW;
////    }
////
////    pthread_mutex_unlock(&mutex_games);
//
//
//
//    //check if the player on move has available moves
//
//    //if there are no more available moves, the game is over, count the score of both players and decide the result of the game
//
//
//
////TODO check if the game is over
//    return 0;
//}

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

int validate_game_status(client *cl, int to_x, int to_y) {
    game *g = get_game_by_id(cl->current_game_id);
    pthread_mutex_lock(&mutex_games);

    //switch the player for opponent
    cl = get_opponent_client(cl, g);

    // Define directions: horizontal, vertical, and diagonals
    int directions[8][2] = {
            {1, 0}, {-1, 0},  // Horizontal
            {0, 1}, {0, -1},  // Vertical
            {1, 1}, {-1, -1}, // Diagonal (\)
            {1, -1}, {-1, 1}  // Diagonal (/)
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
            pthread_mutex_unlock(&mutex_games);
            return GAME_WIN;
        } else if (score_O > score_X) {
            g->winner = cl->client_char == 'B' ? get_opponent_client(cl, g) : cl;
            pthread_mutex_unlock(&mutex_games);
            return GAME_WIN;
        } else {
            g->winner = NULL; // It's a draw
            pthread_mutex_unlock(&mutex_games);
            return GAME_DRAW;
        }
    }

    // Game continues if the player has available moves
    pthread_mutex_unlock(&mutex_games);

    return 0;
}








int validate_move(client *cl, int to_x, int to_y) {
    // Debugging: print the arguments
//    printf("from_x: %d, from_y: %d, to_x: %d, to_y: %d\n", to_x, to_y);

    // Fetch the game by client ID
    game *g = get_game_by_id(cl->current_game_id);
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

    //print the board
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
