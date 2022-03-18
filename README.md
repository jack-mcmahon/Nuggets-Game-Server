# Nuggets

This repository contains the code for the CS50 "Nuggets" game, in which players explore a set of rooms and passageways in search of gold nuggets.
The rooms and passages are defined by a *map* loaded by the server at the start of the game.
The gold nuggets are randomly distributed in *piles* within the rooms.
Up to 26 players, and one spectator, may play a given game.
Each player is randomly dropped into a room when joining the game.
Players move about, collecting nuggets when they move onto a pile.
When all gold nuggets are collected, the game ends and a summary is printed.

## Project Overview

This was the final project for CS 50: Software Design and Implementation. I worked with a team of 3 other students to design and implement the game by building server and client modules. My role in the project largely focused on building out code in the main server program as well as creating a "player" module to store information for each player, their game statistics, and their field of vision.
