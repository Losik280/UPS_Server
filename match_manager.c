#include <stdio.h>
#include <malloc.h>
#include <string.h>
#include <pthread.h>
#include <stdlib.h>

#include "def_n_struct.h"
#include "match_manager.h"

pthread_mutex_t mutex_games = PTHREAD_MUTEX_INITIALIZER;
game *games[MAX_GAMES] = {0};

/**
 * @brief Count the number of games that are played
 * @return number of playing games
 */
int count_games() {
    int count = 0;
    pthread_mutex_lock(&mutex_games);
    for (int i = 0; i < MAX_GAMES; i++) {
        if (games[i] != NULL) {
            count++;
        }
    }
    pthread_mutex_unlock(&mutex_games);

    return count;
}

game *create_new_game(client *player_1, client *player_2) {
    if (count_games() >= MAX_GAMES) {
        printf("Maximum number of games reached\n");
        return NULL;
    }

    game *new_game = malloc(sizeof(game));
    if (new_game == NULL) {
        perror("Failed to allocate memory for the game");
        return NULL;
    }

    pthread_mutex_lock(&mutex_games);

    // Initialize the game
    new_game->id = rand();
    new_game->player1 = player_1;
    initialize_board(new_game->board);
    new_game->player2 = player_2;
    new_game->current_player = player_1;
    new_game->game_status = GAME_PLAYING;
    new_game->winner = NULL;

    // Add the game to the list of games
    for (int i = 0; i < MAX_GAMES; i++) {
        if (games[i] == NULL) {
            games[i] = new_game;
            break;
        }
    }
    pthread_mutex_unlock(&mutex_games);

    return new_game;
}

game *find_client_game(client *cl) {
    pthread_mutex_lock(&mutex_games);
    game *result = NULL;
    for (int i = 0; i < MAX_GAMES; i++) {
        if (games[i] != NULL && (games[i]->player1 == cl || games[i]->player2 == cl)) {
            result = games[i];
            break;
        }
    }
    pthread_mutex_unlock(&mutex_games);

    return result;
}

game *get_game_by_id(int id) {
    pthread_mutex_lock(&mutex_games);
    game *result = NULL;
    for (int i = 0; i < MAX_GAMES; i++) {
        if (games[i] != NULL && games[i]->id == id) {
            result = games[i];
            break;
        }
    }
    pthread_mutex_unlock(&mutex_games);

    return result;
}

void initialize_board(char board[BOARD_SIZE][BOARD_SIZE]) {
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

void print_games() {
    pthread_mutex_lock(&mutex_games);
    printf("Games: \n");
    for (int i = 0; i < MAX_GAMES; i++) {
        if (games[i] != NULL) {
            printf("    Game: %d; Player 1: %d; Player 2: %d\n", games[i]->id, games[i]->player1->id, games[i]->player2->id);
        }
    }
    pthread_mutex_unlock(&mutex_games);
}

int remove_game(client *cl) {
    pthread_mutex_lock(&mutex_games);

    for (int i = 0; i < MAX_GAMES; i++) {
        if (games[i] != NULL && games[i]->id == cl->active_game_id && games[i]->game_status == GAME_OVER) {
            free(games[i]);
            games[i] = NULL;
            pthread_mutex_unlock(&mutex_games);
            print_games();
            return TRUE;
        }
    }

    pthread_mutex_unlock(&mutex_games);
    return FALSE;
}
