/*
 * player.c
 *
 * An player is a data structure to hold information specific to a player
 * in Nuggets.
 *
 * March 5 2022
 *
 */

#include "player.h"

#include <math.h>
#include <mem.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "log.h"
#include "message.h"

/**************** global types ****************/
typedef struct player {
    char* userName;
    char letterID;
    int py, px;
    bool isSpectator;
    int gridWidth;
    int gridHeight;
    char** map;
    bool** discovered;
    addr_t address;
    bool** visible;
    int gold;
} player_t;

/**************** functions ****************/

/**************** player_newPlayer ****************/
player_t* player_newPlayer(const char* userName, char letterID,
                           bool isSpectator, char** map, int gridWidth,
                           int gridHeight, addr_t address)
{
    char c;      // temp char
    int py, px;  // temp for index of player location

    bool** visible = mem_malloc_assert(gridHeight * sizeof(bool*), "visible");
    bool** discovered =
        mem_malloc_assert(gridHeight * sizeof(bool*), "discovered");
    for (int y = 0; y < gridHeight; y++) {
        visible[y] = mem_malloc_assert(gridWidth * sizeof(bool), "visible row");
        discovered[y] =
            mem_malloc_assert(gridWidth * sizeof(bool), "discovered row");
        for (int x = 0; x < gridWidth; x++) {
            if (isSpectator) {
                visible[y][x] = true;
                discovered[y][x] = true;
            } else {
                visible[y][x] = false;
                discovered[y][x] = false;
            }
        }
    }

    // setting random start in map if not a spectator
    if (!isSpectator) {
        for (bool validLocation = false; !validLocation;) {
            py = rand() % gridHeight;
            px = rand() % gridWidth;
            c = map[py][px];
            if (c == '.') {
                validLocation = true;
            }
        }
    }

    player_t* player = mem_malloc_assert(sizeof(player_t), "player");

    if (!isSpectator) {
        // create a local copy of the username
        player->userName =
            mem_malloc_assert((strlen(userName) + 1), "localUserName");
        strcpy(player->userName, userName);
    } else {
        player->userName = NULL; // no user name for spectator
    }

    // initialize contents of player structure
    player->letterID = letterID;
    player->py = py;
    player->px = px;
    player->isSpectator = isSpectator;
    player->map = map;
    player->gridWidth = gridWidth;
    player->gridHeight = gridHeight;
    player->discovered = discovered;
    player->visible = visible;
    player->gold = 0;
    player->address = address;
    player_setLocation(player, py, px);
    player_updateVisible(player);
    return player;
}

/**************** player_setLocation ****************/
void player_setLocation(player_t* player, int y, int x)
{
    if (player == NULL) {
        log_v("player_setLocation called with NULL player");
        return;  // error in usage
    }

    if (!player->isSpectator) {
        player->py = y;
        player->px = x;
        player_updateVisible(player);
    }
}

/**************** addGold ****************/
int player_addGold(player_t* player, int newGold)
{
    if (player != NULL) {
        return (player->gold += newGold);
    }
    return -1;
}

/**************** getGold ****************/
int player_getGold(player_t* player)
{
    if (player != NULL) {
        return player->gold;
    }
    return -1;
}

/**************** getAddr ****************/
addr_t player_getAddr(player_t* player)
{
    if (player != NULL) {
        return player->address;
    }

    log_v("player_getAddr called with NULL player");
    return message_noAddr();  // error in usage
}

/**************** getPlayerID ****************/
char player_getID(player_t* player)
{
    if (player != NULL) {
        return player->letterID;
    }
    return '\0';
}

/**************** getPlayerName ****************/
char* player_getName(player_t* player)
{
    if (player != NULL) {
        return player->userName;
    }
    return NULL;
}

/**************** getPlayerLocation ****************/
void player_getLocation(player_t* player, int* y, int* x)
{
    if (player == NULL || player->isSpectator == true) {
        return;
    }
    *y = player->py;
    *x = player->px;
}

/**************** isSpectator ****************/
bool player_isSpectator(player_t* player)
{
    if (player != NULL && player->isSpectator == true) {
        return true;
    }
    return false;
}

/**************** deletePlayer ****************/
void player_delete(void* arg)
{
    player_t* p = arg;
    if (p != NULL) {
        mem_free(p->userName);
        for (int y = 0; y < p->gridHeight; y++) {
            mem_free(p->discovered[y]);
            mem_free(p->visible[y]);
        }
        mem_free(p->discovered);
        mem_free(p->visible);
        mem_free(p);
    }
}

static bool blocks(player_t* player, int y, int x)
{
    char** grid = player->map;
    return grid[y][x] == '#' || grid[y][x] == '-' || grid[y][x] == '|' ||
           grid[y][x] == '+' || grid[y][x] == ' ';
}

/**************** updateVisible ****************/
void player_updateVisible(player_t* player)
{
    int px = player->px;
    int py = player->py;
    bool** visible = player->visible;
    // trace from player to every point.
    for (int y = 0; y < player->gridHeight; y++) {
        for (int x = 0; x < player->gridWidth; x++) {
            // ignore previously-discovered points.
            // checkme: this doesn't work well, right? we need to update gold,
            // players in the current vision not the global map
            if (y == py && x == px) {
                continue;
            }

            bool pointVisible = true;

            // we will now trace a ray from the player to the point. depending
            // on the slope of the ray, we will handle it differently.

            // for undefined slope, we need to check verticals manually.
            if (px == x) {
                int dy = py < y ? 1 : -1;
                // check every point in a vertical row from the player to the
                // point.
                for (int iy = py + dy; iy != y; iy += dy) {
                    if (blocks(player, iy, px)) {
                        pointVisible = false;
                        break;
                    }
                }
            } else {
                double slope = (double)(py - y) / (double)(px - x);

                // when the ray is more horizontal than vertical, we increment x
                // values to find intermediate points. this means that for any
                // intermediate ix, iy may lie between two points. if both
                // points are walls, then we cannot see beyond them; vision to
                // our target point is blocked.
                if (fabs(slope) <= 1) {
                    int dx = px < x ? 1 : -1;
                    for (int ix = px + dx; ix != x; ix += dx) {
                        double iy = (slope * (ix - px)) + py;
                        if (blocks(player, ceil(iy), ix) &&
                            blocks(player, floor(iy), ix)) {
                            pointVisible = false;
                            break;
                        }
                    }
                }
                // the exact same process as before, but now we increment y and
                // check ix between two points.
                else {
                    int dy = py < y ? 1 : -1;
                    for (int iy = py + dy; iy != y; iy += dy) {
                        double ix = (double)(iy - py) / slope + px;

                        if (blocks(player, iy, ceil(ix)) &&
                            blocks(player, iy, floor(ix))) {
                            pointVisible = false;
                            break;
                        }
                    }
                }
            }

            if (pointVisible) {
                visible[y][x] = true;
                player->discovered[y][x] = true;
            }
        }
    }
}

void player_compositeDisplay(player_t* player, char** items, char** output)
{
    if (output == NULL || player == NULL || items == NULL) {
        return;
    }
    char* ochar = *output;
    for (int y = 0; y < player->gridHeight; y++) {
        for (int x = 0; x < player->gridWidth; x++) {
            if (!player->isSpectator && items[y][x] == player->letterID) {
                *(ochar++) = '@';
                continue;
            }
            char c = ' ';
            if (player->visible[y][x]) {
                c = items[y][x];
            } else if (player->discovered[y][x]) {
                c = player->map[y][x];
            }
            *(ochar++) = c;
        }
        *(ochar++) = '\n';
    }
}