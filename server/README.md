# Nuggets/Server

This repository contains the server code for the CS50 "Nuggets" game. The directory includes:

* The main logic for the client in `server.c`.
* The player helper module and corresponding header file in `player.c` and `player.h`.
* A makefile `Makefile` for building the server program, cleaning up the directory, and running different tests. 
* A shell test script `testing.sh` for conducting unit testing on `server.c`.
* A `.gitignore` file for version control.

## Limitations

Server runs perfectly with myValgrind, when running the program outside of
Valgrind we are getting memory corruption with malloc on some occasions when
building the leaderboard.