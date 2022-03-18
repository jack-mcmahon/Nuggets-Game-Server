# Makefile for nuggets client
#
# Based on CS50 sample makefiles
# Jordan Mann, February 2022

LIBS = ../support/support.a ../libcs50/libcs50.a
INCLS = -I../support -I../libcs50

# BUILDENV is a placeholder for environment-variable-defined
# build arguments - namely -D #define flags.
CFLAGS = -Wall -pedantic -std=c11 -ggdb $(TESTING) $(INCLS) $(BUILDENV)
CC = gcc
MAKE = make
# for memory-leak tests
VALGRIND = valgrind --leak-check=full --show-leak-kinds=all

all: client

client: client.o
	$(CC) $(CFLAGS) $^ $(LIBS) -o $@ -lcurses

fulltest: unittest memtest

unittest: client
	./testing.sh localhost $(PORT) 2>&1 | tee testing.out

memtest: client
	$(VALGRIND) ./client localhost $(PORT) jordan 2>>testing.out
	$(VALGRIND) ./client localhost $(PORT) 2>>testing.out

clean:
	rm -rf *.dSYM  # MacOS debugger info
	rm -f *~ *.o
	rm -f vgcore.*
	rm -f client