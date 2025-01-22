#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <stdlib.h>

#include "def_n_struct.h"
#include "match_manager.h"

pthread_mutex_t g_gamesMutex = PTHREAD_MUTEX_INITIALIZER;
game *g_gamesArr[MAX_GAMES] = {0};

/**
 * @brief Count the number of g_gamesArr that are played
 * @return number of playing g_gamesArr
 */
int count_games() {
    int count = 0;
    pthread_mutex_lock(&g_gamesMutex);
    for (int i = 0; i < MAX_GAMES; i++) {
        if (g_gamesArr[i] != NULL) {
            count++;
        }
    }
    pthread_mutex_unlock(&g_gamesMutex);

    return count;
}

game *initiate_game_session(client *player_1, client *player_2) {
    if (count_games() >= MAX_GAMES) {
        printf("Maximum number of g_gamesArr reached\n");
        return NULL;
    }

    game *new_game = malloc(sizeof(game));
    if (new_game == NULL) {
        perror("Failed to allocate memory for the game");
        return NULL;
    }

    pthread_mutex_lock(&g_gamesMutex);

    // Initialize the game
    new_game->id = rand();
    new_game->player1 = player_1;
    setup_initial_board(new_game->board);
    new_game->player2 = player_2;
    new_game->current_player = player_1;
    new_game->game_status = GAME_PLAYING;
    new_game->winner = NULL;

    // Add the game to the list of g_gamesArr
    for (int i = 0; i < MAX_GAMES; i++) {
        if (g_gamesArr[i] == NULL) {
            g_gamesArr[i] = new_game;
            break;
        }
    }
    pthread_mutex_unlock(&g_gamesMutex);

    return new_game;
}

game *locate_game_for_client(client *cl) {
    pthread_mutex_lock(&g_gamesMutex);
    game *result = NULL;
    for (int i = 0; i < MAX_GAMES; i++) {
        if (g_gamesArr[i] != NULL && (g_gamesArr[i]->player1 == cl || g_gamesArr[i]->player2 == cl)) {
            result = g_gamesArr[i];
            break;
        }
    }
    pthread_mutex_unlock(&g_gamesMutex);

    return result;
}

game *fetch_game_by_id(int id) {
    pthread_mutex_lock(&g_gamesMutex);
    game *result = NULL;
    for (int i = 0; i < MAX_GAMES; i++) {
        if (g_gamesArr[i] != NULL && g_gamesArr[i]->id == id) {
            result = g_gamesArr[i];
            break;
        }
    }
    pthread_mutex_unlock(&g_gamesMutex);

    return result;
}

void setup_initial_board(char board[BOARD_SIZE][BOARD_SIZE]) {
    for (int i = 0; i < BOARD_SIZE; i++) {
        for (int j = 0; j < BOARD_SIZE; j++) {
            board[i][j] = ' ';
        }
    }
    int mid = BOARD_SIZE / 2;
    board[mid - 1][mid - 1] = FIRST_PL_CHAR;
    board[mid - 1][mid] = SECOND_PL_CHAR;
    board[mid][mid - 1] = SECOND_PL_CHAR;
    board[mid][mid] = FIRST_PL_CHAR;
}

void display_active_games() {
    pthread_mutex_lock(&g_gamesMutex);
    printf("Games: \n");
    for (int i = 0; i < MAX_GAMES; i++) {
        if (g_gamesArr[i] != NULL) {
            printf("    Game: %d; Player 1: %d; Player 2: %d\n", g_gamesArr[i]->id, g_gamesArr[i]->player1->id, g_gamesArr[i]->player2->id);
        }
    }
    pthread_mutex_unlock(&g_gamesMutex);
}

int purge_finished_game(client *cl) {
    pthread_mutex_lock(&g_gamesMutex);

    for (int i = 0; i < MAX_GAMES; i++) {
        if (g_gamesArr[i] != NULL && g_gamesArr[i]->id == cl->active_game_id && g_gamesArr[i]->game_status == GAME_OVER) {
            free(g_gamesArr[i]);
            g_gamesArr[i] = NULL;
            pthread_mutex_unlock(&g_gamesMutex);
            display_active_games();
            return TRUE;
        }
    }

    pthread_mutex_unlock(&g_gamesMutex);
    return FALSE;
}
