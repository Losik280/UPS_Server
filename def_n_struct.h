#ifndef __DEF_N_STRUCT__
#define __DEF_N_STRUCT__

#include <pthread.h>

/* -------------------------------------------------------------------------
 *                              MESSAGE CONSTANTS
 * ------------------------------------------------------------------------- */
/**
 * The maximum size (in bytes) for incoming/outgoing messages.
 */
#define MESSAGE_SIZE            1024

/**
 * The maximum length for a player's name (including trailing '\0').
 */
#define PLAYER_NAME_SIZE        21

/**
 * The delimiter used to split fields in communication.
 */
#define MESS_DELIMITER          ";"

/**
 * The ending character indicating the end of a message.
 */
#define MESS_END_CHAR           "\n"

/**
 * The length of the login message, which includes an 8-char prefix and the player's name.
 */
#define LOGIN_MESSAGE_SIZE      (8 + PLAYER_NAME_SIZE)

/**
 * The size for a response to a login message (9 + max name length).
 */
#define LOGIN_MESSAGE_RESP_SIZE (9 + PLAYER_NAME_SIZE)

/**
 * The size allocated for responding to a "want game" query.
 */
#define WANT_GAME_RESP_SIZE     13

/**
 * The maximum length for a "start game" message (16 + player's name).
 */
#define START_GAME_MESSAGE_SIZE (16 + PLAYER_NAME_SIZE)

/**
 * The size used when responding to a move attempt.
 */
#define MOVE_MESS_RESP_SIZE     12

/**
 * The size of a message indicating an opponent's move.
 */
#define OPP_MOVE_MESSAGE_SIZE   14

/**
 * The size for messages containing the game status, including the winner's name.
 */
#define GAME_STATUS_RESP_SIZE   (14 + PLAYER_NAME_SIZE)

/**
 * The size for a message used when a client reconnects (depends on board size and names).
 */
#define RECONNECT_MESSAGE_SIZE  (BOARD_SIZE * BOARD_SIZE + 20 + PLAYER_NAME_SIZE + PLAYER_NAME_SIZE)


/* -------------------------------------------------------------------------
 *                              GAME CONSTANTS
 * ------------------------------------------------------------------------- */

/**
 * The width and height of the Reversi board.
 */
#define BOARD_SIZE              4

/**
 * The maximum number of simultaneous games allowed by the server.
 */
#define MAX_GAMES              10

/**
 * The maximum number of clients that can be connected concurrently.
 */
#define MAX_CLIENTS            20

/**
 * Indicates that a game is active/ongoing.
 */
#define GAME_PLAYING           1

/**
 * Indicates that a game is in a waiting or pending state.
 */
#define GAME_WAITING           0

/**
 * Indicates that a game has concluded.
 */
#define GAME_OVER              2

/**
 * Indicates a winning state in the game.
 */
#define GAME_WIN               1

/**
 * Indicates a draw situation in the game.
 */
#define GAME_DRAW              2

/**
 * Represents a "null" or invalid game ID.
 */
#define GAME_NULL_ID           0

/**
 * Represents the character used by the first player on the board (e.g., 'R' = Red).
 */
#define FIRST_PL_CHAR          'R'

/**
 * Represents the character used by the second player on the board (e.g., 'B' = Blue).
 */
#define SECOND_PL_CHAR         'B'

/**
 * Represents an empty cell on the board.
 */
#define EMPTY_CHAR             ' '

/**
 * This can be used if a "winning line" rule is needed (although it might not be used).
 */
#define WINNING_LENGTH         4

/**
 * When pinging clients, how many seconds we sleep before evaluating responses.
 */
#define PING_SLEEP             5

/**
 * Number of seconds after which an unresponsive client is considered a "zombie."
 */
#define PING_ZOMBIE            20


/* -------------------------------------------------------------------------
 *                              BOOLEAN SHORTCUTS
 * ------------------------------------------------------------------------- */
#define TRUE                   1
#define FALSE                  0


/* -------------------------------------------------------------------------
 *                              SERVER DEFAULTS
 * ------------------------------------------------------------------------- */

/**
 * The default port on which the server listens.
 */
#define PORT                   10000


/* -------------------------------------------------------------------------
 *                           FORWARD DECLARATIONS
 * ------------------------------------------------------------------------- */
typedef struct client client;  /* Forward declaration to allow self-referencing. */

/* -------------------------------------------------------------------------
 *                             CLIENT STRUCTURE
 * ------------------------------------------------------------------------- */
/**
 * Represents an individual player's connection and status.
 */
struct client {
    int         socket;                 /**< File descriptor representing the client's connection. */
    int         id;                     /**< A unique identifier for the client. */
    char        username[PLAYER_NAME_SIZE];  /**< The player's chosen name, up to 20 chars. */
    int         active_game_id;         /**< The ID of the game in which the client participates. */
    int         is_in_game;            /**< Flag indicating if the user is actively playing. */
    int         is_connected;          /**< Flag showing if the user is currently connected. */
    int         need_reconnect_mess;   /**< Indicator that a reconnect message is needed. */
    time_t      last_ping;             /**< Timestamp of the last ping response. */
    char        client_char;           /**< The character used by this client in Reversi (e.g., 'R' or 'B'). */
    int         is_requesting_game;    /**< Flag indicating if the client wants to join a new game. */
    client      *opponent;            /**< A pointer to the client's current opponent, or NULL if none. */
    pthread_t   *client_thread;       /**< Reference to the thread that handles this client. */
};

/* -------------------------------------------------------------------------
 *                             GAME STRUCTURE
 * ------------------------------------------------------------------------- */
/**
 * Holds information about an individual Reversi match, including the participants,
 * board state, and current game status.
 */
typedef struct {
    int         id;                        /**< The unique identifier for this game. */
    char        board[BOARD_SIZE][BOARD_SIZE];  /**< A 2D array holding the Reversi board. */
    client      *player1;                  /**< Pointer to the first player. */
    client      *player2;                  /**< Pointer to the second player. */
    client      *current_player;           /**< Pointer to whichever client is currently moving. */
    int         game_status;               /**< Tracks whether it's playing, waiting, or over. */
    client      *winner;                   /**< Pointer to the winning client, or NULL if no winner yet. */
} game;

/* -------------------------------------------------------------------------
 *                            SERVER STRUCTURE
 * ------------------------------------------------------------------------- */
/**
 * Captures the server's address and port for binding.
 */
typedef struct {
    char    ip_address[17];  /**< Holds the IPv4 address as a dotted-decimal string. */
    int     port;            /**< The port on which the server is set to listen. */
} server_address;

#endif /* __CONFIG_H__ */
