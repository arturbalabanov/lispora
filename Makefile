CC=gcc
CFLAGS=-Wall -std=c11

all:
	$(CC) $(CFLAGS) src/lispora.c lib/mpc/mpc.c -ledit -lm -o bin/lispora

run:
	bin/lispora
