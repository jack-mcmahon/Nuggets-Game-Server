#!/usr/bin/env bash
# Unit testing script for nuggets client
#
# Jordan Mann, February 2022
#
# usage: ./testing.sh hostname port

set -v

HOSTNAME=$1
PORT=$2
NAME=test

################ erroneous arguments ################

# 0 args
./client

# 1 arg
./client $HOSTNAME

# 4 args
./client $HOSTNAME $PORT $NAME extraArgument

# bad hostname - assumes 'badhost' does not exist
./client badhost $PORT $NAME

# bad port - ASSUMES port 99999 is unusable
./client $HOSTNAME 99999 $NAME