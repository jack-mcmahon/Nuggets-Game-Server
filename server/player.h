/*
 * player.h
 *
 * An player is a data structure to hold information specific to a player
 * in Nuggets.
 *
 * March 5 2022
 */

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include "message.h"

/**************** global types ****************/
typedef struct player player_t;

/**************** functions ****************/

/**************** newPlayer ****************/
/* Create a new player
 *
 * Caller provides:
 *   userName
 *   letterID
 *   true if spectator
 *   gameMap
 * We return:
 *   pointer to the new player; return NULL if error.
 * Caller is responsible for:
 *   later calling player_delete.
 */
player_t* player_newPlayer(const char* userName, char letterID, bool isSpectator, char** map,
                    int gridWidth, int gridHeight, addr_t address);
/**************** player_setLocation ****************/
/* Move them player on  the map
 *
 * Caller provides:
 *   player
 *   change in index value, for example if the player is to move directly
 * down, the change in index would be exactly the width of a single line in
 * the game map. We return: the new index of the player if successful -1 if
 * move was invalid
 *
 * We also:
 *   update the player gold value if the player encounters gold.
 *
 * Caller is respoinsible for:
 *   Checking new index of the player
 *   if it overlaps with another player then switch that player into the
 * other spot if it overlaps with a gold location then set it to zero in
 * counters and all player maps
 */
void player_setLocation(player_t* player, int y, int x);

/**************** player_addGold ****************/
/* Add gold to a player
 *
 * Caller provides:
 *   player
 *   new amount of gold to add
 * We return:
 *   player's total gold value
 *
 */
int player_addGold(player_t* player, int newGold);

/**************** player_getGold ****************/
/* Get gold value for a player
 *
 * Caller provides:
 *   player
 * We return:
 *   player's total gold value
 */
int player_getGold(player_t* player);

/**************** getAddr ****************/
addr_t player_getAddr(player_t* player);
/* Get gold value for a player
 *
 * Caller provides:
 *   player
 * We return:
 *   player addr_t
 */


/**************** player_updateVisible ****************/
/* Update a player's map
 *
 * Caller provides:
 *   player
 * We return:
 *   nothing
 */
void player_updateVisible(player_t* player);

/**************** player_getID ****************/
/* Get a player's ID
 *
 * Caller provides:
 *   player
 * We return:
 *   char for the player's letter ID
 */
char player_getID(player_t* player);

/**************** player_getName ****************/
/* Get a player's name
 *
 * Caller provides:
 *   player
 * We return:
 *   char* for the player's username
 */
char* player_getName(player_t* player);

/**************** player_getLocation ****************/
/* Get a player's name
 *
 * Caller provides:
 *   player
 * We return:
 *   int for player's location
 */
void player_getLocation(player_t* player, int* y, int* x);

/**************** player_isSpectator ****************/
/* Check if player is a spectator
 *
 * Caller provides:
 *   player
 * We return:
 *   true if they are a spectator
 *   false if otherrwise
 */
bool player_isSpectator(player_t* player);

/**************** player_delete ****************/
/* delete a player
 *
 * Caller provides:
 *   player
 * We free the memory story the player and the local information inside
 * We return:
 *   nothing
 */
void player_delete(void* arg);

/**************** player_compositeDisplay ****************/
/* build the char* display to send to the client
 *
 * Caller provides:
 *   player, live game map, pointer to a char* for the output
 * We build the correct composited map of player vision, discovered and blank space.
 * We return:
 *   nothing
 */
void player_compositeDisplay(player_t* player, char** items, char** output);