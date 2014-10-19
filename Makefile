CC=gcc
CFLAGS=-Wall

all:
	$(CC) $(CFLAGS) src/lispora.c lib/mpc/mpc.c -ledit -lm -o bin/lispora
