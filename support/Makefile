#
# Makefile for 'support.a'
# CS50 project 'Nuggets'
#
# David Kotz, May 2019, 2021
#

LIB = support.a
TESTS = miniclient messagetest usernametest

CFLAGS = -Wall -pedantic -std=c11 -ggdb
CC = gcc
MAKE = make

.PHONY: all clean

############# default rule ###########
all: $(LIB) $(TESTS) 

$(LIB): message.o log.o username.o
	ar cr $(LIB) $^

usernametest: username.h
	$(CC) $(CFLAGS) -DUNIT_TEST username.c -o usernametest

messagetest: message.c message.h log.h log.o
	$(CC) $(CFLAGS) -DUNIT_TEST message.c log.o -o messagetest

miniclient: miniclient.o message.o log.o
	$(CC) $(CFLAGS) $^ $(LIBS) -o $@

miniclient.o: message.h
message.o: message.h
log.o: log.h
username.o: username.h

############# clean ###########
clean:
	rm -f core
	rm -rf *~ *.o *.gch *.dSYM
	rm -f *.log
	rm -f $(LIB)
	rm -f $(TESTS)
