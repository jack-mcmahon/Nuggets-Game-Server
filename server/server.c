/*
 * ┌───────────────────────┐
 * │ ┌┐┌┬ ┬┌─┐┌─┐┌─┐┌┬┐┌─┐ │
 * │ ││││ ││ ┬│ ┬├┤  │ └─┐ │
 * │ ┘└┘└─┘└─┘└─┘└─┘ ┴ └─┘ │
 * └───────────────────────┘
 *    CS50 nuggets server
 *
 *
 * The server handles the creation of a new nuggets game,
 * the gameplay and state during the game, and the completion
 * of the game.
 *
 *
 * Palmer's Scholars, February 2022
 *
 * Usage: ./server map.txt [seed]
 */

/*********** Include ***********/

#include <assert.h>
#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "counters.h"
#include "file.h"
#include "log.h"
#include "mem.h"
#include "message.h"
#include "player.h"
#include "set.h"
#include "username.h"

// TODO: write gameOver() marvin
// TODO: write createLeaderBoard() Jack
// TODO: Sam map
// TODO: revise design & implementation

/**************** global types ****************/
typedef struct game {
    int numPlayers;
    char** baseMap;      // game map that is loaded in the beginning
    char** liveGameMap;  // game map that is updated to include players + gold
    int goldRemaining;
    set_t* players;
    int gridWidth;
    int gridHeight;
    int mapStringLength;
    player_t* spectator;
    int* goldPiles;
    int pilesFound;
} game_t;

typedef struct {
    player_t* p;   // player with corresponding addr -- could be NULL
    addr_t match;  // address we are searching for in players
} addrStruct;

/* Global variables */
game_t* game;  // represents a universal game state

/* Global constants */
const int maxNameLength = 10;  // maximum name length for player name
const int maxPlayers = 26;     // maximum number of players allowed

/* Function prototypes */
static bool parseArgs(const int argc, const char* argv[], int* randomSeed,
                      const char** mapPathFile);
static bool str2int(const char string[], int* number);
static bool loadGame(const char* mapPathFile);
static bool runNetwork();
static bool gameOver();
static bool buildMap(char* mapString);

static bool handleMessage(void* arg, const addr_t from, const char* message);
static bool handlePLAY(void* arg, const addr_t from, const char* userName);
static bool handleSPECTATE(void* arg, const addr_t from);
static bool handleKEY(void* arg, const addr_t from, const char keyStroke);

static void sendMsg(addr_t to, char* type, char* body);
static void sendOK(addr_t to, char* playerKey);
static void sendGRID(addr_t to);
static void sendGOLD(addr_t to, player_t* player, int n);
static void sendDISPLAY(addr_t to, player_t* player);
static void sendQUIT(addr_t to, char* explanation);
static void sendERROR(addr_t to, char* explanation);

static void sendDisplayAll();
static void sendGoldAll();
static void sendQuitAll();

static bool movePlayer(player_t* player, int y, int x);
static player_t* playerFromAddr(addr_t address);
static void matchAddress(void* arg, const char* key, void* item);
static void playerSendDisplay(void* arg, const char* key, void* item);
static void playerSendGold(void* arg, const char* key, void* item);
static void playerSendQUIT(void* arg, const char* key, void* item);
char* playerLeaderBoard();

/************* main ************/
/*
 * Parse the command-line arguments, load the game, run the game,
 * and end the game upon completion.
 *
 * Logs errors
 */
int main(const int argc, const char* argv[])
{
    // Variables
    const char* mapPathFile = NULL;
    int randomSeed;
    const char* progName = argv[0];
    // Begin logging
    log_init(stderr);
    log_s("Beginning logging of %s program. \n", progName);

    // Handle parseArgs()
    log_s("Parsing arguments of %s to parseArgs \n", progName);
    if (!parseArgs(argc, argv, &randomSeed, &mapPathFile)) {
        log_s("Usage: %s map.txt [seed] \n", progName);
        return EXIT_FAILURE;
    }

    // Handle loadGame()
    log_s("Loading game for %s \n", argv[0]);
    if (!loadGame(mapPathFile)) {
        log_s("Error in loadGame() in %s \n", progName);
        return EXIT_FAILURE;
    }

    // Handle runNetwork()
    log_s("Running network for %s \n", progName);
    if (!runNetwork()) {
        log_s("Error in runNetwork() in %s \n", progName);
        return EXIT_FAILURE;
    }

    // Handle endGame()
    // TODO: write endGame()
    // log_s("Ending game for %s \n", argv[0]);
    // if (!endGame()) {
    //      log_s("Error in endGame() in %s \n", progName);
    //      return EXIT_FAILURE;
    // }

    // End logging
    log_done();

    // Exit
    exit(0);
}

/************ parseArgs *********/
/*
 * Parse and validate the command line arguments
 *
 * Logs errors
 */
static bool parseArgs(const int argc, const char* argv[], int* randomSeed,
                      const char** mapPathFile)
{
    const char* progName = argv[0];

    // CHECK: Parameters are non-NULL
    if (argv == NULL) {
        log_s("Parameters must be non-NULL in %s. \n", progName);
        return false;
    }

    // CHECK: Proper argument count
    if (argc != 1 + 1 && argc != 2 + 1) {
        log_s("Incorrect number of parameters in %s. \n", progName);
        return false;
    }

    FILE* fp;
    // CHECK: Can open provided file
    if ((fp = fopen(argv[1], "r")) == NULL) {
        log_s("File %s does not exist. \n", argv[1]);
        return false;
    }

    *mapPathFile = argv[1];
    fclose(fp);
    log_s("mapFilePath '%s' successfully opened and closed. \n", argv[1]);

    // Seeding with random seed
    if (argv[2] != NULL) {
        // CHECK: If randomSeed is an int
        if (!str2int(argv[2], randomSeed)) {
            log_s("Seed %s is not a valid integer.", argv[2]);
            return false;
        }
        // CHECK: If randomSeed is positive integer
        if (*randomSeed <= 0) {
            log_d("randomSeed '%d' must be a positive integer. \n",
                  *randomSeed);
            return false;
        }
        srand(*randomSeed);
        log_d("Random number generator seeded with %d. \n", *randomSeed);
    }

    // Seeding with getPid()
    else {
        srand(getpid());
        log_v("Random number generator seeded. \n");
    }

    // Return true
    log_v("Arguments parsed successfully. \n");
    return true;
}
/************ loadGame *********/
/*
 * Opens map, initializes gold in map, prepares game
 *
 * Logs errors
 */
static bool loadGame(const char* mapPathFile)
{
    // Variables
    const int goldMinNumPiles = 10;
    const int goldMaxNumPiles = 30;
    const int goldTotalToDrop = 300;
    int goldDropped = 0;
    int currentXPos = 0;
    int currentYPos = 0;
    int toDrop = 0;

    // CHECK: Parameters are non-NULL
    if (mapPathFile == NULL) {
        log_v("Parameters must be non-NULL. \n");
        return false;
    }
    // Load game map
    FILE* fp = fopen(mapPathFile, "r");
    if (fp == NULL) {
        log_s("Could not open mapPathFile: %s. \n", mapPathFile);
        return false;
    }

    // init game struct
    game = mem_malloc_assert(sizeof(game_t),
                             "Game state could not be allocated. \n");

    // initialize game struct members
    game->numPlayers = 0;

    // init game->gridHeight
    game->gridHeight = file_numLines(fp);

    // init player set
    set_t* players =
        mem_assert(set_new(), "players set failed to be created. \n");
    game->players = players;

    // TODO: figure out how this creates unitialized value
    char* mapString = file_readFile(fp);  // string representation of map
    // Build map from function
    // build map initializes game->baseMap, game->liveGameMap,
    // game->mapStringLengh, game->gridWidth
    if (!buildMap(mapString)) {
        log_v("Map could not be built. \n");
        return false;
    }

    free(mapString);
    
    // Calculate number of piles of gold to drop based on max and min
    int difference = (goldMaxNumPiles - goldMinNumPiles);
    int pilesToDrop = goldMinNumPiles + (rand() % difference);

    // create an array of gold piles
    int* goldPiles = mem_malloc_assert((sizeof(int) * pilesToDrop),
                                       "could not make piles array\n");

    for (int i = 0; i < pilesToDrop; i++) {
        goldPiles[i] = 0;
    }

    // loop of the array several times to insure even distribution of gold
    while (goldTotalToDrop > goldDropped) {
        for (int i = 0; i < pilesToDrop; i++) {
            toDrop = (rand() % 5);
            if (goldDropped + toDrop > goldTotalToDrop) {
                toDrop = goldTotalToDrop - goldDropped;
                goldPiles[i] += toDrop;
                goldDropped += toDrop;
                break;
            } else {
                goldPiles[i] += toDrop;
                goldDropped += toDrop;
            }
        }
    }

    // create the right number of '*' symbols on the map
    for (int i = 0; i < pilesToDrop;) {
        currentXPos = (rand() % (game->gridWidth - 1));
        currentYPos = (rand() % (game->gridHeight - 1));
        char c = game->liveGameMap[currentYPos][currentXPos];

        // Only place gold on '.'
        if (c == '.') {
            game->liveGameMap[currentYPos][currentXPos] = '*';
            i++;
        }
    }
    game->goldPiles = goldPiles;
    game->pilesFound = 0;
    game->goldRemaining = goldDropped;
    log_v("Gold successfully dropped into the map. \n");

    fclose(fp);
    log_v("Game successfully loaded. \n");
    return true;
}

/************ runNetwork ********/
/*
 * A function to initialize the network, announce the port number,
 * and handle game execution until completion.
 */
static bool runNetwork()
{
    // Initialize network
    int portNumber = message_init(NULL);
    if (portNumber == 0) {
        log_v("Could not initialize message module. \n");
        return false;
    }

    // Announce port number
    printf("Waiting on port %d for contact...\n", portNumber);
    log_v("Port number announced to players. \n");

    // Listen for messages and handle game execution
    log_v("Listening for messages from players. \n");
    message_loop(NULL, 0, NULL, NULL, handleMessage);

    // Close messaging stream
    message_done();
    return true;
}

/************* gameOver *********/
/*
 * A function to end the game
 */
static bool gameOver()
{
    if (game->goldRemaining <= 0) {
        sendQuitAll();

        // free all data used
        // free player set
        set_delete(game->players, player_delete);

        // free maps
        for (int y = 0; y < game->gridHeight; y++) {
            mem_free(game->baseMap[y]);
            mem_free(game->liveGameMap[y]);
        }

        mem_free(game->baseMap);
        mem_free(game->liveGameMap);

        // free spectator (if any)
        if (game->spectator != NULL) {
            player_delete(game->spectator);
        }

        // free gold piles array
        mem_free(game->goldPiles);

        // free game struct
        mem_free(game);
        return true;  // game over
    }
    return false;  // game not over -- continue looping
}

/* ***************** str2int ********************** */
/*
 * Citation: Taken from CS50 Knowledge units.
 *
 * Convert a string to an integer, updating that integer using a pointer
 * Returns true if successful, or false if any error.
 * It is an error if there is any additional character beyond the integer.
 * Assumes number is a valid pointer.
 */
static bool str2int(const char string[], int* number)
{
    char nextchar;
    return (sscanf(string, "%d%c", number, &nextchar) == 1);
}

/**************** handleMessage() ****************/
/* handleMessage: Parses message from the client and calls a corresponding
 * helper function to handle that specific message.
 *
 * Caller provides: A pointer to anything, a NON-NULL, VALID address from
 *                  correspondent, and the message
 *
 * Function returns: true if error -- stops the server from looping for more
 * messages false if successful -- server continues to loop for more messages
 *
 * Logs: nothing.
 */
static bool handleMessage(void* arg, const addr_t from, const char* message)
{
    if (game == NULL) {  // defensive
        log_v("Game struct has not been initialized.");
        return true;  // error in usage
    }

    if (!message_isAddr(from)) {  // defensive
        log_v("handleMessage called without a correspondent.");
        return true;  // error in usage
    }

    // parse type of message
    if (strncmp(message, "PLAY ", strlen("PLAY ")) == 0) {  // PLAY
        const char* realName = message + strlen("PLAY ");

        return handlePLAY(arg, from, realName);
    } else if (strncmp(message, "SPECTATE", strlen("SPECTATE")) ==
               0) {  // SPECTATE
        return handleSPECTATE(arg, from);
    } else if (strncmp(message, "KEY ", strlen("KEY ")) == 0) {  // KEY
        char keyStroke = *(message + strlen("KEY "));
        return handleKEY(arg, from, keyStroke);
    } else {  // ERROR
        sendERROR(from, "Unknown command.");
        return false;  // continue looping
    }
}

/******************************************/
/* handlePLAY: Handles what the server should do upon receiving PLAY message
 * from client.
 *
 * Caller provides: A pointer to anything, an address from correspondent, and
 * the player's username
 *
 * Function returns: true if error -- stops the server from looping for more
 * messages false if successful -- server continues to loop for more messages
 *
 * Logs: nothing.
 */
static bool handlePLAY(void* arg, const addr_t from, const char* userName)
{
    if (game == NULL) {  // defensive
        log_v("Game struct has not been initialized. \n");
        return true;
    }

    if (!message_isAddr(from)) {  // defensive
        log_v("handlePLAY called without a correspondent. \n");
        return true;
    }

    if (isEmpty(userName)) {  // no name provided
        message_send(from, "QUIT Sorry - you must provide player's name.");
        return false;
    } else if (game->numPlayers == maxPlayers) {  // game full
        message_send(from, "QUIT Game is full: no more players can join.");
        return false;
    } else {  // add player to game
        // normalize username
        char* name = mem_assert(normalizeUserName(userName, maxNameLength),
                                "Player name could not be allocated.");

        log_s("Player name initialized as: %s \n", name);

        // add player to game
        game->numPlayers++;
        char playerLetter = 'A' + game->numPlayers - 1;

        // init new player
        player_t* player = mem_assert(
            player_newPlayer(userName, playerLetter, false, game->baseMap,
                             game->gridWidth, game->gridHeight, from),
            "Player could not be allocated.");
        free(name);
        int x = -1, y = -1;
        player_getLocation(player, &y, &x);

        char c;
        c = game->liveGameMap[y][x];
        if (c == '*'){
            int goldFound = game->goldPiles[game->pilesFound];
            game->goldRemaining -= goldFound;
            game->pilesFound += 1;
            player_addGold(player, goldFound);
            // update gold for everyone
            sendGoldAll();

            // update gold to show the player the amount they picked up
            sendGOLD(from, player, goldFound);
        }
        game->liveGameMap[y][x] = playerLetter;

        // add player to set
        if (game->players != NULL) {
            char* playerKey = mem_malloc_assert(sizeof(char) * 2, "set key");
            playerKey[0] = playerLetter;
            playerKey[1] = '\0';
            if (set_insert(game->players, playerKey, player)) {
                log_c("Player %c inserted successfully. \n", playerLetter);

                // send OK (playerLetter)
                sendOK(from, playerKey);

                mem_free(playerKey);

                // send GRID nrows ncols
                sendGRID(from);

                // send GOLD n p r
                sendGOLD(from, player, 0);

                // send DISPLAY\nstring
                sendDisplayAll();
            } else {
                log_v("Player could not be inserted. \n");
            }

            return false;
        } else {  // defensive
            log_v("Player set has not been initialized. \n");
            free(name);
            free(player);
            return true;  // stop looping
        }
    }
    return false;
}

/******************************************/
/* handleSPECTATE: handles what the server should do upon receiving SPECTATE
 *                 message from client.
 *
 * Caller provides: A pointer to anything, an address from correspondent
 *
 * Function returns: true if error -- stops the server from looping for more
 * messages false if successful -- server continues to loop for more messages
 *
 * Logs: Errors.
 */
static bool handleSPECTATE(void* arg, const addr_t from)
{
    if (game == NULL) {  // defensive
        log_v("Game struct has not been initialized. \n");
        return true;
    }

    if (!message_isAddr(from)) {  // defensive
        log_v("handleSPECTATE called without a correspondent. \n");
        return true;
    }

    // init new player
    player_t* spectator =
        mem_assert(player_newPlayer(NULL, '\0', false, game->baseMap,
                                    game->gridWidth, game->gridHeight, from),
                   "Spectator could not be allocated.");

    // send GRID nrows ncols
    sendGRID(from);

    // send GOLD n p r
    sendGOLD(from, spectator, 0);

    // send DISPLAY\nstring
    sendDISPLAY(from, spectator);

    if (game->spectator == NULL) {
        game->spectator = spectator;
        return false;
    } else {
        player_t* oldSpectator = game->spectator;
        game->spectator = spectator;
        addr_t oldAddress = player_getAddr(oldSpectator);
        sendQUIT(oldAddress, "You have been replaced by a new spectator.");
        return false;
    }
}

/******************************************/
/* handleKEY: handles what the server should do upon receiving KEY message from
 * client. Either calls the helper functions to move or quit the Player. A
 * Spectator client can only quit.
 *
 * Read the specs for more info on what keystrokes are possible for
 * player or client.
 *
 * Caller provides: A pointer to anything, an address from correspondent, and
 * the keyStroke
 *
 * Function returns: true if error -- stops the server from looping for more
 * messages false if successful -- server continues to loop for more messages
 *
 * Logs: nothing.
 */
static bool handleKEY(void* arg, const addr_t from, const char keyStroke)
{
    if (!message_isAddr(from)) {
        log_v("handleKEY called without a correspondent. \n");
        return true;  // error in usage
    }

    player_t* player = NULL;
    if (game->spectator != NULL &&
        message_eqAddr(from, player_getAddr(game->spectator))) {
        player = game->spectator;
    } else {
        player = playerFromAddr(from);  // player we are updating

        if (player == NULL) {  // player not found or error
            log_v("Player not found in set. \n");
            return false;  // error in usage
        }
    }

    if (player_isSpectator(player)) {  // player is spectator
        switch (keyStroke) {
            case 'Q':  // spectator quits
                sendQUIT(from, "Thanks for watching!");
                break;

            default:  // error
                sendERROR(from, "Unknown keystroke.");
                break;
        }
    } else {     // player is regular player
        int px;  // players current x
        int py;  // players current y

        player_getLocation(player, &py, &px);

        switch (tolower(keyStroke)) {
            case 'q':  // player quits
                if (keyStroke == 'Q') {
                    sendQUIT(from, "Thanks for playing!");
                } else {
                    sendERROR(from, "Unknown keystroke.");
                }
                break;
            case 'h':  // move left
                px--;
                while (movePlayer(player, py, px)) {
                    sendDisplayAll();  // update everyone's screen
                    px--;
                    if (keyStroke != 'H') {
                        break;
                    }
                }
                break;

            case 'l':  // move right
                px++;
                while (movePlayer(player, py, px)) {
                    sendDisplayAll();
                    px++;
                    if (keyStroke != 'L') {
                        break;
                    }
                }
                break;

            case 'j':  // move down
                py++;
                while (movePlayer(player, py, px)) {
                    sendDisplayAll();
                    py++;
                    if (keyStroke != 'J') {
                        break;
                    }
                }
                break;

            case 'k':  // move up
                py--;
                while (movePlayer(player, py, px)) {
                    sendDisplayAll();
                    py--;
                    if (keyStroke != 'K') {
                        break;
                    }
                }
                break;

            case 'y':  // move up and left
                py--;
                px--;
                while (movePlayer(player, py, px)) {
                    sendDisplayAll();
                    py--;
                    px--;
                    if (keyStroke != 'Y') {
                        break;
                    }
                }
                break;

            case 'u':  // move up and right
                py--;
                px++;
                while (movePlayer(player, py, px)) {
                    sendDisplayAll();
                    py--;
                    px++;
                    if (keyStroke != 'U') {
                        break;
                    }
                }
                break;

            case 'b':  // move down and left
                py++;
                px--;
                while (movePlayer(player, py, px)) {
                    sendDisplayAll();
                    py++;
                    px--;
                    if (keyStroke != 'B') {
                        break;
                    }
                }
                break;

            case 'n':  // move down and right
                py++;
                px++;
                while (movePlayer(player, py, px)) {
                    sendDisplayAll();
                    py++;
                    px++;
                    if (keyStroke != 'N') {
                        break;
                    }
                }
                break;

            default:  // error
                sendERROR(from, "Unknown keystroke.");
                break;
        }
    }

    return gameOver();  // check if game is over
}

/******************************************/
/* buildMap: given a string representing a map, updates game->baseMap to contain
 * a two-dimensional representation of the map.
 * Also updates game->gridWidth to match the map width
 * Also initializes game->mapStringLength
 * Initializes game->liveMap to be the same as game->baseMap (at start)
 *
 * Caller provides: A pointer to a string representing map.txt
 *
 * Function returns: true if two-dimensional array has been initialized
 * successfully false if error
 *
 * Assumptions: We assume that mapString is in valid map format;
 * We assume game->gridHeight has already been initialized
 *
 * User must remember to free() game->baseMap and free() game->liveGameMap
 */
static bool buildMap(char* mapString)
{
    if (game == NULL) {
        log_v("buildMap: game state cannot be NULL. \n");
        return false;  // error in usage
    }
    if (mapString == NULL) {
        log_v("buildMap: mapString cannot be NULL. \n");
        return false;  // error in usage
    }

    // init game->gridWidth
    int gridWidth = 0;
    for (char* c = mapString; *c != '\0'; c++) {
        if (*c == '\n') {
            break;
        }
        gridWidth++;
    }
    game->gridWidth = gridWidth;

    // update game->mapStringLength
    game->mapStringLength = strlen(mapString);

    // malloc memory for array of pointers
    game->baseMap = mem_malloc_assert(game->gridHeight * sizeof(char*),
                                      "baseMap could not be allocated. \n");

    game->liveGameMap =
        mem_malloc_assert(game->gridHeight * sizeof(char*),
                          "liveGameMap could not be allocated. \n");

    for (int y = 0; y < game->gridHeight; y++) {
        game->baseMap[y] = mem_malloc_assert(game->gridWidth * sizeof(char) + 1,
                                             "baseMap row");
        game->liveGameMap[y] = mem_malloc_assert(
            game->gridWidth * sizeof(char) + 1, "liveGameMap row");
    }

    // loop through mapString char by char
    char* startPointer = mapString;  // pointer to start of string
    int i = 0;                       // current index in baseMap array
    for (char* c = mapString; *c != '\0'; c++) {
        if (*c == '\n') {  // new line encountered
            // squash with '\0'
            *c = '\0';
            strcpy(game->baseMap[i], startPointer);
            i++;
            c++;
            startPointer = c;
        }
    }

    for (int y = 0; y < game->gridHeight; y++) {
        strcpy(game->liveGameMap[y], game->baseMap[y]);
    }

    return true;  // success
}

/**************** sendMsg ****************/
/*
 * Send a message to the client.
 */
static void sendMsg(addr_t to, char* type, char* body)
{
    if (!message_isAddr(to)) {
        log_v("sendMsg called without correspondent. \n");
        return;  // error in usage
    }

    if (type == NULL) {
        log_v("sendMsg was given NULL type. \n");
        return;  // error in usage
    }
    if (body == NULL) {  // no body
        message_send(to, type);
        return;
    }

    // + 1 for player letter
    // + 1 for space between type and body
    // + 1 for termining null character
    char* message =
        mem_malloc_assert(sizeof(char) * (strlen(type) + strlen(body)) + 1 + 1,
                          "sendMsg: System out of memory.");
    sprintf(message, "%s %s", type, body);
    message_send(to, message);
    mem_free(message);
}

/**************** sendOK ****************/
/*
 * sends OK [player letter] to client.
 * returns nothing
 *
 * Logs errors
 */
static void sendOK(addr_t to, char* playerKey)
{
    if (!message_isAddr(to)) {
        log_v("sendOK called without correspondent. \n");
        return;  // error in usage
    }

    sendMsg(to, "OK", playerKey);
}

/**************** sendGRID ****************/
/*
 * send GRID [nrows] [ncols] to client.
 * returns nothing
 *
 * Logs errors
 */
static void sendGRID(addr_t to)
{
    // build gridMsg
    int gridHeightLength = snprintf(NULL, 0, "%d", game->gridHeight);  // nrows
    int gridWidthLength = snprintf(NULL, 0, "%d", game->gridWidth);    // ncols

    // + 1 for space between nrows and ncols
    // + 1 for termining null character
    char* gridMsg =
        mem_malloc_assert(gridHeightLength + gridWidthLength + 1 + 1,
                          "GridMsg could not be allocated.");

    sprintf(gridMsg, "%d %d", game->gridHeight, game->gridWidth);
    sendMsg(to, "GRID", gridMsg);
    mem_free(gridMsg);
}

/**************** sendGOLD ****************/
/*
 * send GOLD [n] [p] [r] to client.
 * where n is the number of gold nuggets the player has just collected,
 * p is the player's purse
 * and r is the remaining number of nuggets left to be found
 *
 * returns nothing
 *
 * Logs errors
 */
static void sendGOLD(addr_t to, player_t* player, int n)
{
    if (!message_isAddr(to)) {
        log_v("sendGOLD called without a correspondent. \n");
        return;  // error in usage
    }

    if (player == NULL) {
        log_v("sendGOLD called with NULL player. \n");
        return;  // error in usage
    }

    // build goldMsg
    int nLength = snprintf(NULL, 0, "%d", n);                       // n
    int pLength = snprintf(NULL, 0, "%d", player_getGold(player));  // p
    int rLength = snprintf(NULL, 0, "%d", game->goldRemaining);     // r

    // + 1 for space between n and p
    // + 1 for space between p and r
    // + 1 for termining null character
    char* goldMsg = mem_malloc_assert(nLength + pLength + rLength + 1 + 1 + 1,
                                      "goldMsg could not be allocated.");

    sprintf(goldMsg, "%d %d %d", n, player_getGold(player),
            game->goldRemaining);

    sendMsg(to, "GOLD", goldMsg);
    mem_free(goldMsg);
}

/**************** sendGoldAll ****************/
/*
 * iterates through each player in player set and calls sendGOLD
 *
 * each client receives GOLD n p r
 *
 * returns nothing
 *
 * Logs errors
 */
static void sendGoldAll()
{
    if (game->players != NULL) {  // defensive
        set_iterate(game->players, NULL, playerSendGold);
    }
}

/**************** playerSendGold ****************/
/*
 * calls sendGOLD on player object
 *
 */
static void playerSendGold(void* arg, const char* key, void* item)
{
    player_t* player = item;
    if (player == NULL) {
        log_v("playerSendGold called with NULL player. \n");
        return;  // error in usage
    }

    sendGOLD(player_getAddr(player), player, 0);
}

/**************** sendDISPLAY ****************/
/*
 * send DISPLAY\n[string] to client.
 * where string is a multi-line textual representation of the grid as known/seen
 * by this client.
 *
 * each client receives a different version of map
 *
 * spectator sees all but is not represented on the map
 * players only see what is in their field of vision and the player's own
 * position is represented by an '@'
 *
 * Assumes that loadGame() has been called and game, game->mapStringLength, and
 * game->liveGameMap have been initialized
 *
 * returns nothing
 *
 * Logs errors
 */
static void sendDISPLAY(addr_t to, player_t* player)
{
    if (!message_isAddr(to)) {
        log_v("sendDISPLAY called without a correspondent. \n");
        return;  // error in usage
    }

    if (player == NULL) {
        log_v("sendDISPLAY called with NULL player. \n");
        return;  // error in usage
    }

    char* output = mem_malloc_assert(sizeof(char) * game->mapStringLength + 1,
                                     "Display could not be allocated.");

    player_compositeDisplay(player, game->liveGameMap, &output);

    if (output != NULL) {
        char* displayMsg = mem_malloc_assert(
            sizeof(char) * (strlen("DISPLAY\n") + game->mapStringLength) + 1,
            "displayMsg could not be allocated.");
        sprintf(displayMsg, "%s\n%s", "DISPLAY", output);
        sendMsg(to, displayMsg, NULL);
        mem_free(displayMsg);
        mem_free(output);
    }
}

/**************** sendDisplayAll ****************/
/*
 * iterates through each player in player set and calls sendDISPLAY
 *
 * each client receives a different version of map
 *
 * returns nothing
 *
 * Logs errors
 */
static void sendDisplayAll()
{
    if (game->players != NULL) {  // defensive
        set_iterate(game->players, NULL, playerSendDisplay);
    }
}

/**************** playerSendDisplay ****************/
/*
 * calls sendDISPLAY on player object
 *
 */
static void playerSendDisplay(void* arg, const char* key, void* item)
{
    player_t* player = item;
    if (player == NULL) {
        log_v("playerSendDisplay called with NULL player");
        return;  // error in usage
    }
    sendDISPLAY(player_getAddr(player), player);
}

/**************** sendERROR ****************/
/*
 * send ERROR [explanation] to client.
 *
 * returns nothing
 *
 * Logs errors
 */
static void sendERROR(addr_t to, char* explanation)
{
    if (!message_isAddr(to)) {
        log_v("sendERROR called without a correpsondent. \n");
        return;  // error in usage
    }

    if (explanation == NULL) {
        log_v("sendERROR called without an explanation. \n");
        return;  // error in usage
    }

    sendMsg(to, "ERROR", explanation);
}

/**************** sendQUIT ****************/
/*
 * send QUIT [explanation] to client.
 *
 * returns nothing
 *
 * Logs errors
 */
static void sendQUIT(addr_t to, char* explanation)
{
    if (!message_isAddr(to)) {
        log_v("sendQUIT called without a correpsondent. \n");
        return;  // error in usage
    }

    if (explanation == NULL) {
        log_v("sendQUIT called without an explanation. \n");
        return;  // error in usage
    }

    sendMsg(to, "QUIT", explanation);
}

/**************** sendQuitAll ****************/
/*
 * send QUIT GAME OVER:
 *      A         4 Alice*
 *      B         16 Bob*
 *      C         230 Carol*
 *
 * Alice, Bob, and Carol are all example player names
 *
 * returns nothing
 *
 * Logs errors
 */
static void sendQuitAll()
{
    if (game->players != NULL) {  // defensive
        set_iterate(game->players, NULL, playerSendQUIT);
    }
}

/**************** playerSendQUIT ****************/
/*
 * helper function for sendQuitAll
 *
 * returns nothing
 *
 * Logs errors
 */
static void playerSendQUIT(void* arg, const char* key, void* item)
{
    player_t* player = item;
    if (player == NULL) {
        log_v("playerSendQUIT called with NULL player");
        return;  // error in usage
    }

    char* leaderBoard = playerLeaderBoard();
    sendQUIT(player_getAddr(player), leaderBoard);
    mem_free(leaderBoard);
}

/**************** playerLeaderBoard ****************/
/*
 * creates a formatted leaderboard
 *
 */
char* playerLeaderBoard()
{
    char playerLetter;      // temp for iterating
    player_t* player;       // loads a player
    char ID;                // char for player's ID
    int gold;               // int for player golf
    char* name;             // char* for player's name
    char* header = "QUIT GAME OVER:\n";
    const int maxlineLength = 28;

    char* leaderBoard = mem_malloc_assert(
        sizeof(char) * ((game->numPlayers + 1) * (maxlineLength) + 1), "leaderboard could not be allocated.");

    strncpy(leaderBoard, header, maxlineLength);

    for (int i = 0; i < game->numPlayers; i++) {
        // iterate through each player letter in the players set
        playerLetter = 'A' + i;
        char* playerLetterPtr = mem_malloc_assert(
            sizeof(char) + 1, "playerLetterPtr could not be allocated.");
        playerLetterPtr[0] = playerLetter;
        playerLetterPtr[1] = '\0';
        player = set_find(game->players, playerLetterPtr);
        mem_free(playerLetterPtr);
        
        ID = player_getID(player);
        fflush(stdout);
        gold = player_getGold(player);
        name = player_getName(player);

        char* nextLine = mem_malloc_assert(sizeof(char) * maxlineLength,
                                     "NextLine could not be allocated.");
        // format output line for a player's results
        snprintf(nextLine, maxlineLength, "%c%10d %s\n", ID, gold, name);

        // add the new line to the leaderboard
        strcat(leaderBoard, nextLine);

        // free memory
        mem_free(nextLine);
    }
    return leaderBoard;
}

/**************** movePlayer ****************/
/*
 * takes care of player movement and associated events
 */
static bool movePlayer(player_t* player, int y, int x)
{
    char c = game->liveGameMap[y][x];    // temp char for game character at
                                         // intended move index
    char thisID = player_getID(player);  // holds letterID of the current player
    char otherID;  // holds letterID of a player that is run into
    int py, px;    // holds player coordinates
    player_getLocation(player, &py, &px);

    if (c == ' ' || c == '|' || c == '-' || c == '+') {
        // can't move into a wall
        return false;
    } else if (c == '.' || c == '#') {
        // normal valid move
        player_setLocation(player, y, x);
        game->liveGameMap[y][x] = thisID;
        game->liveGameMap[py][px] = game->baseMap[py][px];
        return true;
    } else if (c == '*') {
        // if hitting gold, remove it from the map, decrease the global gold
        // count, and add gold to the player's count
        player_setLocation(player, y, x);
        game->liveGameMap[y][x] = thisID;
        game->liveGameMap[py][px] = game->baseMap[py][px];

        // find how much gold is in the next pile
        int goldFound = game->goldPiles[game->pilesFound];
        game->goldRemaining -= goldFound;
        game->pilesFound += 1;
        player_addGold(player, goldFound);
        // update gold for everyone
        sendGoldAll();

        // update gold to show the player the amount they picked up
        addr_t address = player_getAddr(player);
        sendGOLD(address, player, goldFound);

        return true;
    } else {
        // the move spot overlaps with another player's location
        otherID = game->liveGameMap[y][x];
        player_setLocation(player, y, x);
        game->liveGameMap[y][x] = thisID;

        // swap the other player into the current player's location
        char* otherKey = mem_malloc_assert(sizeof(char) * 2, "set key");
        otherKey[0] = otherID;
        otherKey[1] = '\0';
        player_t* other = set_find(game->players, otherKey);
        mem_free(otherKey);
        player_setLocation(other, py, px);
        game->liveGameMap[py][px] = otherID;
        return true;
    }
}

/**************** sendQUIT ****************/
/*
 * returns pointer to player object associated with given address
 *
 * returns NULL if player not found or error
 *
 * Assumes loadGame() has been called and thus game and game->players have been
 * initialized.
 *
 * Logs errors
 */
static player_t* playerFromAddr(addr_t address)
{
    if (!message_isAddr(address)) {
        log_v("playerFromAddr called without a correspondent. \n");
        return NULL;  // error in usage
    }

    addrStruct* adrs = mem_malloc_assert(sizeof(addrStruct),
                                         "addrStruct could not be allocated.");
    adrs->p = NULL;
    adrs->match = address;

    set_iterate(game->players, adrs, matchAddress);

    player_t* matchPlayer = adrs->p;
    mem_free(adrs);
    return matchPlayer;
}

/**************** matchAddress ****************/
/*
 * initializes adrs->p and adrs->match
 *
 * where adrs->p is the player we are looking for in playerFromAddr
 * and adrs->match is the address we are looking for in players
 *
 * Logs errors
 */
static void matchAddress(void* arg, const char* key, void* item)
{
    addrStruct* adrs = arg;
    if (adrs == NULL) {
        log_v("matchAddress called with NULL addrStruct. \n");
        return;  // error in usage
    }

    player_t* player = item;
    if (player == NULL) {
        log_v("matchAddress called with NULL player. \n");
        return;  // error in usage
    }

    addr_t playerAddr = player_getAddr(player);

    if (message_eqAddr(adrs->match, playerAddr)) {  // addresses match
        // update adrs
        adrs->p = player;
    }
}
