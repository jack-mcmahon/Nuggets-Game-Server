# CS50 Nuggets
## Implementation Spec
### Palmer's Scholars, Winter 2022

According to the [Requirements Spec](REQUIREMENTS.md), the Nuggets game requires two standalone programs: a client program and a server program.

Our design also includes provided message and log modules and the nCurses library. We describe each program and module separately.

We do not describe the `support` library nor the modules that enable features that go beyond the spec. We avoid repeating information that is provided in the requirements spec.

## Plan for division of labor

Server: Jack(parseArgs, loadGame), Marvin(runNetwork), Sam(main)

Handler module: Marvin

Player module: Jack (movement, gold), Sam (vision)

Client: Jordan

Each team member will test/document the modules they develop.
The client and server programs will be play-tested as a group.

## playerClient

### Data structures

We do not anticipate any major data structures used by the client, with the exception of a potential gameState containing any pertinent values shared between functions/modules (these could include a reference to the server, error/connection status, etc.)

We use one primary data structure: a `gameState_t` which contains:
* A reference to the `server` address
* Whether the player `isSpectator`
* The `playerID` if not a spectator and having successfully joined the game.
* statusIndex
* hostname
* port

```c
typedef struct gameState {
    addr_t server;
    bool isSpectator;
    char playerID;
    int statusIndex;
    char* hostname;
    char* port;
} gameState_t;
```

A single instance of the game state is stored as a singular global variable:

```c
gameState_t* state;
```

### Definition of function prototypes

Log a formatted error message (wraps `vfprintf`) and newline `\n` to stderr.
```c
static void plogf(const char* format, ...);
```

Prints a new message to the status line on row 0.
```c
static void printStatus(const char* format, ...);
```

Clears the status line on row 0.
```c
static void clearStatus();
```

Parse the command-line arguments, set up the TUI and server connection, start the game, and run the game.
```c
int main(const int argc, char* argv[]);
```

Parse and validate the command-line arguments.
```c
static bool parseArgs(const int argc, char* argv[], char** hostname, char** port, char** playerName);
```

Initialize the game state struct. Set up the TUI and server connection.
```c
static gameState_t* setup(char* hostname, char* port, char* playerName);
```

Join and run the game, then close the TUI.
```c
static bool game(char* playerName);
```

Send a message to the server.
```c
static void sendMsg(char* type, char* body);
```

Handle a message from the server.
```c
static bool handleEvent(void* arg, const addr_t server, const char* message);
```

Ensure that the client's screen is the correct size.
```c
static bool setupGrid(const char* body);
```

Show the game grid as sent by the server.
```c
static void showGrid(const char* body);
```

Show statistics regarding the player's gold as sent from the server.
```c
static bool showGold(const char* body);
```

Close the TUI and show a reason for exiting the game as given by the server.
```c
static void showQuit(const char* body);
```

Handle client input - a keypress.
If the key corresponds with a command valid for the current game state, forward the command to the server.
```c
static bool handleAction();
```

### Detailed pseudo code

#### `plogf(format, ...)`
```
with format, ...args
vfprintf(stderr, format, args)
fputc('\n', stderr)
fflush(stderr)
```

#### `printStatus(format, ...)`
```
clearStatus()
format new text on status line
plogf()
refresh
```


#### `clearStatus()`
```
delete the text on the status line
refresh
```


#### `main(argc, argv)`
```
with argc, argv

let hostname, port, playerName?

guard parseArgs(argc, argv, &hostname, &port, &playerName) else return errParse

guard let global state = setup(hostname, port, playerName) else return errSetup

let gameStatus = game()

(clean up)
message_done()

return gameStatus
```

#### `parseArgs(argc, argv, hostname, port, playername)`
```
with argc, argv, &hostname, &port, &playerName

guard argc == 3 + 1 else return false

*hostname = argv[1]
*port = argv[2]
*playerName = argv[3]

return true;
```

#### `setup(hostname, port, playername)`
```
with hostname, port, playerName

let server;
guard message_init(stderr) else return NULL
guard message_setAddr(hostname, port, &server) else return NULL

let state

state.server = server
state.isSpectator = playerName == NULL
state.playerID = NULL

(setup/clear TUI)

return state
```

#### `game(playerName)`
```
with global state, playerName

(show 'connecting')

if state.isSpectator sendMsg(SPECTATE)
else sendMsg(PLAY, playerName)

return message_loop(NULL, 0, NULL, handleAction, handleEvent)
```

#### `sendMsg(type, body)`
```
with global state, type, body?

message_send(state.server, { type, body })
```

#### `handleEvent(arg, server, message)`
```
with global state, server, message

let { type, body } = message

int status;

switch (type) {
	case QUIT -> showQuit(body); return false
	case OK && !state.isSpectator && !state.playerID ->
		state.playerID = body
	case GRID -> setupGrid(body)
	case DISPLAY -> showGrid(body)
	case GOLD -> showGold(body)
	case ERROR -> showError(body)
}
return true
```

#### `setupGrid(body)`
```
with body

let { gridRows, gridCols } = body

while (display.rows - 1 != gridRows, display.cols != gridCols) (print error)

return true
```

#### `showGrid(body)`
```
with gridString
(move to grid)
(print gridString)
```

#### `showGold(body)`
```
with body, global state
let { collected, purse, remaining } = body

(move to status)
(print state.player has purse of remaining)
(if collected > 0 print collected)
```

#### `showQuit(body)`
```
with body
(close TUI)
(print body)
```

#### `handleAction(arg)`
```
with global state
let key = (get key)
if state.playerID == NULL return true
if (key is of accepted keys) sendMsg({ KEY, key })
```

---

## Server

### Data structures

We anticipate four three data structures:  
* `Player` -  A struct representing a player in the game. 
* `playerSet` - A _Set_ representing all of the _Player_ structs in the game. 
* `game_t` - A struct encapsulating all current information regarding the game. 

#### `player`
The player data structure will contain all of the information particular to a certain player. A player.c file will be used to handle making player data structures and modifying/retreiving information about them.

```c
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
```

A single instance of the game state is stored as a singular global variable:

```c
player_t* player;
```

#### `game`
The game struct represents a universal game state containing the number of players, the base map and live game map, game map information, gold positions and gold remaining, and a set of the players.

```c
typedef struct game {
  int numPlayers;
  char** baseMap;
  char** liveGameMap
  int goldRemaining;
  set_t* players;
  int gridWidth;
  int gridHeight;
  int mapStringLength;
  player_t* spectator;
  int* goldPiles;
  int pilesFound;
} game_t;
```

A single instance of the game state is stored as a singular global variable:

```c
game_t* game;
```

### Definition of function prototypes

A function to handle the main flow of the client program and any errors bubbled up from other modules. Responsible for parsing command-line arguments, initializing data structures, and running the game.
```c
int main(const int argc, char* argv[]);
```

A function to parse the command-line arguments and seed the random number generator.
```c
static bool parseArgs(const int argc, char* argv[], int* randomSeed, const char** mapPathFile);
```

A function to initialize the map with gold and create the game object for play.
```c
static bool loadGame(const char* mapPathFile);
```

A function to initialize the network, initialize the message module, announce the port number, and handle the execution of the game until completion.
```c
static bool runNetwork();
```

A function to end the game, print the scoreboard, and free memory.
```c
static bool gameOver();
```

A helper function to convert a string to an integer (taken from CS50 Knowledge Units)
```c
static bool str2int(const char string[], int* number);
```

A function to parse a message from the client, and calls a corresponding helper function to handle that specific message. Returns false to keep looping for more messages from client; returns true to terminate loop. 
```c
static bool handleMessage(void* arg, const addr_t from, const char* message);
```

A function to handle what the server should do upon receiving PLAY message from client.
```c
static bool handlePLAY(void* arg, const addr_t from, const char* userName);
```

A function to handle what the server should do upon receiving SPECTATE message from client.
```c
static bool handleSPECTATE(void* arg, const addr_t from);
```

A function to handle what the server should do upon receiving KEY message from client.
```c
static bool handleKEY(void* arg, const addr_t from, const char* keyStroke);
```

A function to build a two-dimensional representation of the game map given a string.
```c
static bool buildMap(char* mapString);
```

A function for building and sending messages to the client.
```c
static void sendMsg(addr_t to, char* type, char* body);
```

A function for sending an OK message to the client.
```c
static void sendOK(addr_t to, char playerLetter);
```

A function for sending a grid representation of the game map to the client.
```c
static void sendGRID(addr_t to);
```

A function for sending a gold collection message to the client.
```c
static void sendGOLD(addr_t to, player_t* player, int n);
```

A helper function for calling sendGOLD on all clients.
```c
static void sendGoldAll();
```

A wrapper function passed to set iterate to call sendGOLD on each client.
```c
static void playerSendGold(void* arg, const char* key, void* item);
```

A function for sending the game display view to the client, for that specific client.
```c
static void sendDISPLAY(addr_t to, player_t* player);
```

A helper function for calling sendDISPLAY on all clients.
```c
static void sendDisplayAll();
```

A wrapper function passed to set iterate to call sendDISPLAY on each client.
```c
static void playerSendDisplay(void* arg, const char* key, void* item);
```

A function for sending an error message to the client.
```c
static void sendERROR(addr_t to, char* explanation);
```

A function for sending a quit message to the client.
```c
static void sendQUIT(addr_t to, char* explanation);
```

A helper function for calling sendQUIT on all clients.
```c
static void sendQuitAll();
```

A wrapper function passed to set iterate to call sendQUIT on each client.
```c
static void playerSendQUIT(void* arg, const char* key, void* item);
```

A function for handling player movement based on incoming messages from clients.
```c
static bool movePlayer(player_t* player, int y, int x);
```

A function to TBD
```c
static player_t* playerFromAddr(addr_t address);
```

A helper function to initialize adrs object in playerFromAddr.
```c
static void matchAddress(void* arg, const char* key, void* item);
```


### Detailed pseudo code

#### `main()`:

	initialize game variables, begin logging
	call parseArgs()
	      check that arguments are non-NULL
	      seed random number generator
	      if error, return to main and exit non-zero 
	call loadGame()
	      check that arguments are non-NULL
	      load map file, build game object
	      place gold into map, update game state
	      if error, return to main and exit non-zero
	call runNetwork()
	      initialize network, announce port number
	      listen for messages and handle changing game state
	      exit when game ends
	      if error, terminate game and return to main and exit non-zero                
	call gameOver()
	      communicate end of game to clients
	      display leaderboard
	      free memory used in game
	      if errors from gameOver, exit non-zero  
	end logging, exit zero


#### `parseArgs(argc, argv, randomSeed, mapPathFile)`:

	validate parameters are non-NULL
	check number of arguments
	if argument number incorrect:
		return to main and exit w/ error
	open map file
	if map file cannot be opened:
		close file
		return to main and exit with error
	save map file
		if random seed provided:
			if the random seed is a valid number:
				seed the random-number generator
			else:
				return to main and exit w/ error
		else:
			seed the random-number generator with getpid
	close map file
	return to main and continue


#### `loadGame(mapPathFile)`:

	validate paramters are non-NULL
	open map file
	if map file cannot be loaded:
		close file
		return to main and exit w/ error
	initialize game struct, members, grid dimensions
	initialize player set
	calculate number of gold piles to drop
	initialize an array to store value of gold piles at locations
	while all gold hasn't been dropped:
		choose a random coordinate
		if valid character, drop gold
		update state
	add gold symbols to map based on where the gold was dropped
	update game struct
	close file
	return to main and continue

	
#### `runNetwork()`:

	initialize message module
	if cannot initialize:
		return to main and exit
	announce port number
	while game has not executed:
		listen for messages from clients and handle them, update game state
	close message stream
	return to main and continue


#### `gameOver()`:

	send quit message to clients
	free player set, player maps, spectator
	free game struct

	
#### `handleMessage(arg, from, message)`:
	
	if game is null:
		return error to caller
	if from address is not properly initialized:
		return error to caller
	parse message
		if message is PLAY:
			collect given username from client
			return handlePLAY(userName)
		else if message is SPECTATE:
			return handleSPECTATE()
		else if message is KEY:
			parse keystroke
			return handleKEY(key)
		else:
			sendERROR()
			return false


#### `handlePLAY(arg, from, username)`:
	
	if game is null:
		return error to caller
	if from address is not properly initialized:
		return error to caller
	parse username
		if username not provided:
			send QUIT message
			return false
		else if max players in game:
			send QUIT message
			return false
		else:
			normalizeUsername(userName)
			store userName
			create player struct for player
			add to playerSet
			send OK [playerLetter] to client
			send GRID, GOLD, and DISPLAY messages to client
	return false


#### `handleSPECTATE(arg, from)`:
	
	if game is null
		return error to caller
	if from address is not properly initialized
		return error to caller
	initialize new spectator
	if spectator already exists:
		send QUIT message to prior spectator
		spectator is updated to new spectator
	else:
		initialize spectator
		send GRID, GOLD, and DISPLAY messages to spectator
	return false

		
#### `handleKEY(arg, from, keystroke)`:

    if client is a spectator:
        switch(key):
            case 'Q':
                send QUIT to client with explanation
                return false
            default:
                return handleError()
            
    if client is a player:
        switch (key):
            case 'Q':
                send QUIT to client with explanation
                return false
            case 'h':
                move player left
                update player map
                return false
            case 'l':
                move player right
                update player map
                return false
            case 'j':
                move player down
                update player map
                return false
            case 'k':
                move player up
                update player map
                return false
            case 'y':
                move player diagonally up and left
                update player map
                return false
            case 'u':
                move player diagonally up and right
                update player map
                return false
            case 'b':
                move player diagonally down and left
                update player map
                return false
            case 'n':
                move player diagonally down and right
                update player map
                return false
            default:
                return handleError()

     
#### `buildMap(char* mapString)`:

	if game is null:
		return error to caller
	if mapString is null:
		return error to caller
	initialize gridWidth
	initialize mapStringLength
	initialize baseMap, liveGameMap
	for each char in the baseMap:
		add to liveGameMap


#### `sendMsg(to, type, body)`:
	
	if to is not a valid address
		return error to caller
	if type or body are NULL
		return error to caller
	message_send(to, message)

	
#### `sendOK(to, playerLetter)`:
	
	if to is not a valid address
		return error to caller
	sendMsg(to, "OK", playerLetter)

	
#### `sendGOLD(to, player, n)`:

	if to is not a valid address
		return error to caller
	if player or n is null
		return error to caller
	build gold message
		set n length
		set p length
		set r length
	sendMsg(to, "GOLD", goldMsg)


#### `sendGoldAll()`:

	if players set in game are NULL
		return error to caller
	iterate through set, pass playerSendGold function


#### `playerSendGold(arg, key, item)`:

	if player is null:
		return error to caller
	sendGOLD(player's address, player, 0)


#### `sendDISPLAY(to, player)`:

	if to address is not valid:
		return error to caller
	if player is null:
		return error to caller
	build updated display view for client
	sendMsg(to, displayMsg, NULL)


#### `sendDisplayAll()`:

	if players set in game are NULL:
		return error to caller
	iterate through set, pass playerSendDisplay function


#### `playerSendDisplay(arg, key, item)`:

	if player is null:
		return error to caller
	sendDISPLAY(player's address, player)


#### `sendERROR(to, explanation)`:

	if to address is not valid:
		return error to caller
	if explanation is null:
		return error to caller
	sendMsg(to, "ERROR", explanation)


#### `sendQUIT(to, explanation)`:

	if to address is not valid:
		return error to caller
	if explanation is null:
		return error to caller
	sendMsg(to, "QUIT", explanation)
	

#### `sendQuitAll()`:

	if players set in game are NULL:
		return error to caller
	iterate through set, pass playerSendQuit function


#### `playerSendQUIT(arg, key, item)`:
	
	if player is null:
		return error to caller
	build leaderboard
	sendQUIT(player address, leaderboard)


#### `movePlayer(player, y, x)`:

	if player, y, or x are null:
		return error to caller
	get current location of player
	if character is a wall character:
		update live game map
	else if character is open spot:
		set new location of player
		update live game map
		return
	else if character is gold:
		set new location of player
		add gold to player's count
		sendGoldAll()
		sendGOLD(player's address, player, goldFound)
		update live game map
	else:
		set new location of player
		set new location of player moved into
		update live game map


#### `playerFromAddr(address)`:

	if address is not valid:
		return error to caller
	initialize new address
	iterate through game's plaers set
	return the matched player

#### `matchAddress(arg, key, item)`:

	if arg (adrs) is null:
		return error to caller
	if item (player) is null:
		return error to caller
	get player address
	return matched address from game's player set


## Player module

### Definition of function prototypes

Add a new player (client) to the game.
```c
player_t* newPlayer(const char* userName, char letterID, bool isSpectator, char** map, int gridWidth, int gridHeight, addr_t address);
```

Move a player to a given index in the map.
```c
void player_setLocation(player_t* play, int y, int x);
```

Add gold to a player's total.
```c
int player_addGold(player_t* player, int newGold);
```

Get the amount of gold a player has.
```c
int player_getGold(player_t* player);
```

Get player's address.
```c
addr_t player_getAddr(player_t* player);
```

Get player's ID.
```c
char player_getID(player_t* player);
```

Get player's name.
```c
char* player_getName(player_t* player);
```

Get player's location.
```c
void player_getLocation(player_t* player, int* y, int* x);
```

Check if the player is a spectator.
```c
bool player_isSpectator(player_t* player);
```

Delete the player object.
```c
void player_delete(void* arg);
```

Update the visibility of the player in the current game.
```c
void player_updateVisibile(player_t* player);
```

Produce the display for the player.
```c
void player_compositeDisplay(player_t* player, char** items, char** output);
```

### Detailed pseudo code

#### `player_newPlayer(userName, letterID, isSpectator, map, gridWidth, gridHeight, address)`:

    create a new player struct
	    set player userName
	    set player letterID
	    set player game mode
	    set player map
	    set gridWidth, gridHeight
	    set gridHeight
	    set player gold to 0
	    set player address
	    player_playerLocation(player, py, px)
	    player_updateVisible(player)
    return player


#### `player_setLocation(player, y, x)`:

	if player is null:
		return error to caller
	if the player is not a spectator:
		set new x
		set new y
		player_updateVisibile(player)


#### `player_addGold(player, newGold)`:

	if player is null:
		return error to caller
	set player's gold value to newGold
	
	
#### `player_getGold(player)`:

	if player is null:
		return error to caller
	return value of player's gold value


#### `player_getAddr(player)`:

	if player is null:
		return error to caller
	return value of player's address


#### `player_getID(player)`:

	if player is null:
		return error to caller
	return value of player's ID

	
#### `player_getName(player)`:

	if player is null:
		return error to caller
	return value of player's name
	

#### `player_getLocation(player, y, x)`:

	if player is null:
		return error to caller
	if player is spectator:
		return error to caller
	set values of y and x from player's py and px


#### `player_isSpectator(player)`:

	if player is null:
		return error to caller
	if player is spectator:
		return true
	return false


#### `player_delete(arg)`:

	if player is null:
		return error to caller
	free memory for userName, map, visible, and player struct
	

#### `player_updateVisible(player)`:

	if player is null:
		return error to caller
	for each point in the map:
		if point is visible:
			continue:
		else:
			if point is player:
				do not change visibility continue
			else if same column or row:
				check each point (exclusive) between player and point on shared axis
				if a wall char is one of the points:
					change visibility, continue
				else:
					do not change visibility
			else:
				calculate slope between the player and point
					check each point (exclusive) between player on line
					if point is non-integer:
						check if floor is wall char
						check if ceiling is wall char
					else:
						check if wall char
					if wall char is one of the points/point combinations (non-integer):
						change visibility


#### `player_compositeDisplay(player, items, output)`:

	if player, items, or output NULL:
		return error to caller
	for each point in the map:
		add point based on char, item, player, and visibility
		

## Username module

A module commited to helping a user create a username for themselves in the game.

### Definition of function prototypes

A helper function that truncates an over-length username to maxNameLength characters and replaces underscore for any character where isgraph() and isblank() are false

```c
static void normalizeUsername(char* userName, int maxNameLength);
```

A helper function that returns whether a given string is only whitespace.

```c
static bool isEmpty(char* str);
```

### Detailed pseudo code

#### `normalizeUsername`:

	if userName is not NULL:
	    insert '\0' into userName[maxNameLength]
		loop through all characters in new string:
			for character c where !isgraph() and !isblank():
				replace that character with '_'
	

#### `isEmpty`:

	if str is NULL:
		return false
	loop through every character in string:
		if character is not blank:
			return false
	return true


## Testing plan


### Client

The client will be thoroughly play-tested to perform integration testing; a shell script `testing.sh` will serve as the root for all automated testing.

1. `testing.sh` will run either on the sample server `~/cs50dev/shared/nuggets/linux/server` or on the server implementation.

2. The program is then tested to see whether erroneous arguments (invalid count, invalid address) are handled properly.

3. Two regression tests (one with erroneous arguments, one general) are performed against sample log output from the example client. The the results can be directly `diff`ed with sample log outputs.

4. The general test is run again using `valgrind` to check for memory leaks.

### Server

The server's proper functionality will be thoroughly play-tested to perform integration testing; a shell script `testing.sh` will serve as the root for all automated testing.

1. : We will write a `testing.sh` script specifically for the server, testing different numbers of clients, too many clients, invalid messages etc to see if the server handles them properly.

2. The general test is run again using `valgrind` to check for memory leaks.

### System testing

The client and server will be play-tested together, with TUI output and log outputs being compared/analyzed for discrepancies.

## Limitations

This implementation has no known limitations at this time.
