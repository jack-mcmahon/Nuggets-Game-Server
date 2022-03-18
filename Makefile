# Makefile for CS50 Nuggets
#
# Jordan Mann, February 2021

.DEFAULT_GOAL := all
L = libcs50
.PHONY: all clean

############## default: make all libs and programs ##########
# If libcs50 contains set.c, we build a fresh libcs50.a;
# otherwise we use the pre-built library provided by instructor.
all:
	(cd libcs50 && if [ -r set.c ]; then make $L.a; else cp $L-given.a $L.a; fi)
	make -C support
	make -C client
	make -C server

############## clean  ##########
clean:
	make -C support clean
	make -C client clean
	make -C libcs50 clean
	make -C server clean