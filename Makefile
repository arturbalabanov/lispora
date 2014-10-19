all:
	gcc src/lispora.c lib/mpc/mpc.c -ledit -lm -o bin/lispora
