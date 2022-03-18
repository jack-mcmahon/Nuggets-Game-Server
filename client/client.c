/*
 * ┌───────────────────────┐
 * │ ┌┐┌┬ ┬┌─┐┌─┐┌─┐┌┬┐┌─┐ │
 * │ ││││ ││ ┬│ ┬├┤  │ └─┐ │
 * │ ┘└┘└─┘└─┘└─┘└─┘ ┴ └─┘ │
 * └───────────────────────┘
 *    CS50 nuggets client
 *
 * The client provides an interactive text user interface (TUI)
 * to the nuggets game. It connects to a server as either a named
 * player or spectator depending on the presence of a playerName
 * argument; it then forwards keystrokes to the server and handles
 * display update events as necessary before exiting on error, quit,
 * or game end.
 *
 * Jordan Mann, February 2022
 *
 * Usage: client hostname port [playerName]
 */

#include <log.h>
#include <message.h>
#include <ncurses.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mem.h"

/**************** definitions and prototypes ****************/

// Enable capital movement keys
#define USE_SPRINT

// Make output look more like the assignment's sample client output
// #define USE_COMPAT

// Global game state.
typedef struct gameState {
    addr_t server;
    bool isSpectator;
    char playerID;
    int statusIndex;
    char* hostname;
    char* port;
} gameState_t;

gameState_t* state;

static void plogf(const char* format, ...);
static void printStatus(const char* format, ...);
static void clearStatus();
int main(const int argc, char* argv[]);
static bool parseArgs(const int argc, char* argv[], char** hostname,
                      char** port, char** playername);
static gameState_t* setup(char* hostname, char* port, char* playername);
static bool game(char* playername);
static void sendMsg(char* type, char* body);
static bool handleEvent(void* arg, const addr_t server, const char* message);
static bool setupGrid(const char* body);
static void showGrid(const char* body);
static bool showGold(const char* body);
static void showQuit(const char* body);
static bool handleAction();

/**************** plogf ****************/
/*
 * Write a line to the client log (stderr), formatted.
 */
static void plogf(const char* format, ...)
{
    va_list argp;
    va_start(argp, format);
    vfprintf(stderr, format, argp);
    va_end(argp);
    fputc('\n', stderr);
    fflush(stderr);
}

/**************** printStatus ****************/
/*
 * Add a message to the status line on row 0.
 */
static void printStatus(const char* format, ...)
{
    clearStatus();
    move(0, state->statusIndex);
    va_list argp;
    va_start(argp, format);
    vw_printw(stdscr, format, argp);
    plogf(format, argp);
    va_end(argp);
    refresh();
}

static void clearStatus()
{
    move(0, state->statusIndex);
    clrtoeol();
    refresh();
}

/**************** main ****************/
/*
 * Parse the command-line arguments, set up the TUI and server connection,
 * start the game, and run the game.
 */
int main(const int argc, char* argv[])
{
    plogf("START OF LOG");
    char* hostname = NULL;
    char* port = NULL;
    char* playerName = NULL;

    if (!parseArgs(argc, argv, &hostname, &port, &playerName)) {
        return EXIT_FAILURE;
    }
    state = setup(hostname, port, playerName);
    if (state == NULL) {
        plogf("%s: can't form address from %s %s", argv[0], hostname, port);
        return EXIT_FAILURE;
    }
    bool gameStatus = game(playerName);
    /* clean up */
    message_done();
    if (!gameStatus) {
        plogf("client: exiting on error in game");
        endwin();
        return EXIT_FAILURE;
    }
    mem_free(state->hostname);
    mem_free(state->port);
    mem_free(state);
    plogf("END OF LOG");
    return EXIT_SUCCESS;
}

/**************** parseArgs ****************/
/*
 * Parse and validate the command-line arguments.
 */
static bool parseArgs(const int argc, char* argv[], char** hostname,
                      char** port, char** playername)
{
    if (!(argc == 3 + 1 || argc == 2 + 1)) {
        plogf("usage: too few (or too many) arguments %d", argc - 1);
        plogf("usage: %s hostname port [yourname]", argv[0]);
        return false;
    }

    *hostname = argv[1];
    *port = argv[2];
    *playername = argv[3];

    return true;
}

/**************** setup ****************/
/*
 * Initialize the game state struct. Set up the TUI and server connection.
 */
static gameState_t* setup(char* hostname, char* port, char* playername)
{
    state = mem_malloc_assert(sizeof(gameState_t), "state");

    int clientPort = 0;
    if ((clientPort = message_init(NULL)) == 0) {
        plogf("couldn't init message module.");
        return NULL;
    }

    plogf("client on port %d", clientPort);

    if (!message_setAddr(hostname, port, &(state->server))) {
        plogf("possible problem with serverHost %s", hostname);
        plogf("possible problem with serverPort %s", port);
        mem_free(state);
        return NULL;
    }
    state->isSpectator = playername == NULL;
    state->playerID = '\0';
    state->statusIndex = 0;
    state->hostname =
        mem_malloc_assert(sizeof(char) * strlen(hostname) + 1, "hostname copy");
    strcpy(state->hostname, hostname);
    state->port =
        mem_malloc_assert(sizeof(char) * strlen(port) + 1, "port copy");
    strcpy(state->port, port);
    if (state->isSpectator) {
        plogf("spectate");
    } else {
        plogf("interactive player mode; name '%s'", playername);
    }

    return state;
}

/**************** game ****************/
/*
 * Join and run the game, then close the TUI.
 */
static bool game(char* playerName)
{
    //     (show 'connecting')

    if (state->isSpectator) {
        sendMsg("SPECTATE", NULL);
    } else {
        sendMsg("PLAY", playerName);
    }

    return message_loop(&state, 0, NULL, handleAction, handleEvent);
}

/**************** sendMsg ****************/
/*
 * Send a message to the server.
 */
static void sendMsg(char* type, char* body)
{
    if (type == NULL) {
        plogf("sendMsg was given NULL type");
        return;
    }
    if (body == NULL) {
        message_send(state->server, type);
        return;
    }
    char* message = mem_malloc_assert(
        sizeof(char) * (strlen(type) + 1 + strlen(body) + 1), "message");
    sprintf(message, "%s %s", type, body);
    message_send(state->server, message);
    mem_free(message);
}

/**************** handleEvent ****************/
/*
 * Handle a message from the server.
 */
static bool handleEvent(void* arg, const addr_t server, const char* message)
{
    int typeLength = -1;
    for (int i = 0; i < strlen(message); i++) {
        if (message[i] == ' ' || message[i] == '\n') {
            typeLength = i;
            break;
        }
    }
    if (typeLength == -1) {
        plogf("error: could not find space delimiter in message '%s'", message);
        return true;
    }
    char* type = mem_malloc_assert(sizeof(char) * (typeLength + 1), "type");
    char* body = mem_malloc_assert(
        sizeof(char) * (strlen(message) - typeLength - 1 + 1), "body");
    if (type == NULL || body == NULL) {
        plogf("couldn't malloc type, body buffers");
        return true;
    }
    strncpy(type, message, typeLength);
    type[typeLength] = '\0';  // terminate the string
    // include the null terminator here:
    strncpy(body, message + typeLength + 1, strlen(message) - typeLength);

    bool terminate = false;

    if (strcmp(type, "DISPLAY") == 0) {
        showGrid(body);
    } else {
        plogf("%s message received: %s", type, message);
        if (strcmp(type, "QUIT") == 0) {
            showQuit(body);
            terminate = true;
        } else if (strcmp(type, "OK") == 0) {
            if (!state->isSpectator && state->playerID == '\0') {
                state->playerID = body[0];
            } else {
                plogf("warning: ignoring unexpected OK");
            }
        } else if (strcmp(type, "GRID") == 0) {
            if (!state->isSpectator && state->playerID == '\0') {
                plogf("warning: ignoring unexpected GRID");
            } else {
                if (!setupGrid(body)) {
                    terminate = true;
                }
            }
        } else if (strcmp(type, "GOLD") == 0) {
            if (!state->isSpectator && state->playerID == '\0') {
                plogf("warning: ignoring unexpected GOLD");
            } else {
                if (!showGold(body)) {
                    terminate = true;
                }
            }
        } else if (strcmp(type, "ERROR") == 0) {
            printStatus(body);
        }
    }

    mem_free(type);
    mem_free(body);
    return terminate;
}

/**************** setupGrid ****************/
/*
 * Ensure that the client's screen is the correct size.
 */
static bool setupGrid(const char* body)
{
    initscr();      // initialize the screen
    cbreak();       // accept keystrokes immediately
    noecho();       // don't echo characters to the screen
    start_color();  // turn on color controls
    init_pair(1, COLOR_YELLOW, COLOR_BLACK);
    init_pair(2, COLOR_BLACK, COLOR_YELLOW);
    attron(COLOR_PAIR(1));  // set the screen color using color 1

    int gridRows = 0;
    int gridCols = 0;
    int currRows = getmaxy(stdscr);
    int currCols = getmaxx(stdscr);
    if (sscanf(body, "%d %d", &gridRows, &gridCols) != 2) {
        plogf("error: couldn't parse GRID body: %s", body);
        return false;
    }
    while (currRows < gridRows + 1 || currCols < gridCols) {
        clear();
#ifdef USE_COMPAT
        mvprintw(0, 0,
                 "Your window must be at least %d high\n"
                 "Your window must be at least %d wide\n"
                 "Resize your window, and press Enter to continue.",
                 gridRows, gridCols);
#else
        mvprintw(0, 0,
                 "Window is:\n"
                 "%2d/%2d chars tall %s\n"
                 "%2d/%2d chars wide %s\n"
                 "resize to continue.",
                 currRows, gridRows + 1,
                 currRows < gridRows + 1 ? "(make it taller!)" : "", currCols,
                 gridCols, currCols < gridCols ? "(make it wider!)" : "");
#endif
        refresh();
        const int returnKey = 10;
        for (int c = getch(); c != returnKey
#ifndef USE_COMPAT
                              && c != KEY_RESIZE
#endif
             ;
             c = getch()) {
        }
        currRows = getmaxy(stdscr);
        currCols = getmaxx(stdscr);
    }
    return true;
}

/**************** showGrid ****************/
/*
 * Show the game grid as sent by the server.
 */
static void showGrid(const char* body)
{
    mvprintw(1, 0, body);
    refresh();
}

/**************** showGold ****************/
/*
 * Show statistics regarding the player's gold as sent from the server.
 */
static bool showGold(const char* body)
{
    int collected = 0;
    int purse = 0;
    int remaining = 0;
    if (sscanf(body, "%d %d %d", &collected, &purse, &remaining) != 3) {
        plogf("error: couldn't parse GOLD body: %s", body);
        return false;
    }
    if (state->isSpectator) {
        mvprintw(0, 0, "Spectator: %d nuggets unclaimed.  ", remaining);
    } else {
        mvprintw(0, 0, "Player %c has %d nuggets (%d nuggets unclaimed).  ",
                 state->playerID, purse, remaining);
    }
    state->statusIndex = getcurx(stdscr);

    if (state->isSpectator) {
        attron(COLOR_PAIR(2));
        printStatus("Play at %s %s", state->hostname, state->port);
        attron(COLOR_PAIR(1));
    }
    if (collected > 0) {
        printStatus("GOLD received: %d", collected);
    }
    refresh();
    // (move to status)
    // (print state.player has purse of remaining)
    // (if collected > 0 print collected)
    return true;
}

/**************** showQuit ****************/
/*
 * Close the TUI and show a reason for exiting the game as given by the server.
 */
static void showQuit(const char* body)
{
    endwin();
    printf("%s", body);
    printf("\n");
}

/**************** handleAction ****************/
/*
 * Handle client input - a keypress.
 * If the key corresponds with a command valid for the current game state,
 * forward the command to the server.
 */
static bool handleAction(void* arg)
{
    clearStatus();
    char key = getch();
    char* buf = malloc(sizeof(char) * 2);
    if (buf == NULL) {
        plogf("could not allocate KEY body buffer");
        return true;
    }
    buf[0] = key;
    buf[1] = '\0';

    if (key == -1) {
        return true;
    }

    if (state->isSpectator && key != 'Q') {
        printStatus("usage: unknown spectator keystroke");
    } else {
        switch (key) {
            case 'Q':
            case 'h':
            case 'j':
            case 'k':
            case 'l':
            case 'b':
            case 'n':
            case 'y':
            case 'u':
#ifdef USE_SPRINT
            case 'H':
            case 'J':
            case 'K':
            case 'L':
            case 'B':
            case 'N':
            case 'Y':
            case 'U':
#endif
                sendMsg("KEY", buf);
                break;
            default:
                printStatus("usage: unknown keystroke");
                break;
        }
    }
    mem_free(buf);
    return false;
    // with global state
    // let key = (get key)
    // if state.playerID == NULL return true
    // if (key is of accepted keys) send(state, { KEY, key })
}