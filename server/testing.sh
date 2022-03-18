#!/usr/bin/env bash 
# Unit testing script for nuggets server 
# usage: ./testing.sh map.txt [seed] 
set -v 
MAPFILE=$1 
SEED=$2 

########## Erroneous Arguments ######### 
# 0 args (invalid number of arguments)
./server 

# 3 args (invalid number of arguments)
./server $MAPFILE $SEED "this is a random argument" 

# 1 args (invalid file path) 
./server "./potato.txt" 

# 2 args (invalid file path) 
./server "./potato.txt" $SEED 

# 2 args (invalid seed) .
/server $MAPFILE "this is not a proper seed" 

# 2 args (seed less than 0) 
./server $MAPFILE -10
