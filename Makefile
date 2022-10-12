#
# Sample Makefile for the ring buffer lab.
#

#
# Explicitly specify the shell to work around inconsistencies in "make".
#
SHELL   =       /bin/sh

#
# Ask for debugging and warnings, and enable pthreads.
#
CFLAGS  =       -g -Og -Wall -pthread

all:    ringbuf

ringbuf:        ringbuf.c
	$(CC) $(CFLAGS) -o ringbuf ringbuf.c

clean:
	rm -f ringbuf
