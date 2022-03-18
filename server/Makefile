# Makefile for Nuggets Server
#
# Marvin Escobar Barajas, March 2022

LIBS = -lm ../support/support.a ../libcs50/libcs50.a
INCLS = -I../support -I../libcs50

# BUILDENV is a placeholder for environment-variable-defined
# build arguments - namely -D #define flags.
CFLAGS = -Wall -pedantic -std=c11 -g -ggdb $(TESTING) $(INCLS) $(BUILDENV)
CC = gcc
MAKE = make
# for memory-leak tests
VALGRIND = valgrind --leak-check=full --show-leak-kinds=all

# default build
all: server 

server: server.o player.o 
	$(CC) $(CFLAGS) $^ $(LIBS) -o $@

unittest: server
	./testing.sh 2>&1 | tee testing.out


clean:
	rm -rf *.dSYM  # MacOS debugger info
	rm -f *~ *.o
	rm -f vgcore.*
	rm -f server
	rm -f player