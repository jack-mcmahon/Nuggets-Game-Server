# CS50 Nuggets
## Design Spec
### Palmer's Scholars, Winter 2022

According to the [Requirements Spec](https://github.com/cs50winter2022/nuggets-info/blob/main/REQUIREMENTS.md), the Nuggets game requires two standalone programs: a client program and a server program.

Our design also includes provided `message` and `log` modules and the nCurses library.
We describe each program and module separately.

We do not describe the `support` library nor the modules that enable features that go beyond the spec.
We avoid repeating information that is provided in the requirements spec.

## Client (Player)

The *client* acts in one of two modes:

 1. *spectator*, the passive spectator mode described in the requirements spec.
 2. *player*, the interactive game-playing mode described in the requirements spec.

## User interface

See the requirements spec for both the command-line and interactive UI.

## Inputs and outputs

* The client takes command-line arguments as described in the requirements spec.

* Keystrokes (`Q` and movement keys `HJKLYUBN` if in player mode) are listened for and sent directly to the server.

* Server-sent information is displayed in the TUI (see "user interface" above).
	This includes the grid/map, which is displayed constantly during gameplay, along with temporary status messages.

* The client will log useful information to sterr; i.e. game state, client actions, server-sent events, and errors.

## Functional decomposition into modules

We anticipate the following major modules or functions:

* `main` - Parse arguments; setup data structures; run game.
	All critical errors should be passed up to be handled on the main thread.
* `parseArgs` - Parse and validate command-line arguments.
* `setup` - Connect to the server; init TUI and any game state data structures.
* `game` - Loop for events and actions; display the game state in the TUI.

Additional helper methods will be used throughout the program.

## Pseudo code for logic/algorithmic flow

The client will run as follows:

	with argv
	hostname, port, playername? = parseArgs(argv)
	setup(hostname, port): connect to server, setup TUI
	game(server, playername):
		if (playername != NULL): send(PLAY, playername)
		else: send(SPECTATE)
		loop for action (keypress) or server-sent event:
			on action with key: if already received OK: send(KEY, key)
			on event with message: display message accordingly, or break game loop on QUIT/ERROR

### Major data structures

We do not anticipate any major data structures used by the client, with the exception of a potential `gameState` containing any pertinent values shared between functions/modules (these could include a reference to the server, error/connection status, etc.)

---


## Server

### User interface

There is no direct interaction with the user after Server starts running. Per the requirements spec, Server takes an argument for the map and an optional argument for a random seed.

### Inputs and outputs

This program takes command-line arguments as input, as described in the requirements spec.

The server will log useful information to sterr; i.e. game state, client actions, server-sent events, and errors.

### Functional decomposition into modules

We anticipate the following major modules or functions:

* `main` - Parse arguments; calls `loadGame()`, `runNetwork()`, and `gameOver()` if given valid args. All errors that require an exit should be passed to the main thread to be handled.
* `parseArgs` - Parse and validate the command-line arguments.
* `loadGame` - Interprets the map, drops gold in random piles across the map, and initializes the game struct.
* `runNetwork` - Start network, announce port number, handle game execution.
* `gameOver` - Clean up and end the game, display leaderboard.
* `handleMessage`- Parses and handles client message, relaying to proper logic.
* `movePlayer`- Updates the position of the player in the map. updates all relevant game logic.

And some helper modules that provide data structures:

* `player` - This module handles the player structs including adding new players, computing player vision, etc.
* `username` - This module helps the client create a username for the game.

Additional helper methods will be used throughout the program.

### Pseudo code for logic/algorithmic flow

The server will run as follows:
	
	parse and validate command-line arguments
	build game struct
	build map from text file, drop gold into map, update the game struct
	initialize network
	announce port number
	while game has not ended:
		on player connect:
			add player object to set
			increase number of players by one
			send START message to client
		on spectator connect:
			add spectator 
			send SPECTATOR message
		on message from client:
			parse message
			send proper reply to client
		when number of gold left is 0:
			end game
			send QUIT to each player

### Major data structures

The `player` struct holds information regarding each player, including their game mode, username, position in the map, letter ID, amount of gold collected, and a record of the map they can see.

The `game` struct represents a universal game state containing the number of players, map, gold remaining, collection of gold locations, and a set of the players.

---
