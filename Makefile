CC=gcc
CFLAGS=-std=gnu99 -Wall -Wextra -Werror -pedantic -pthread

bus: bus.c
	@$(CC) $(CFLAGS) -o bus bus.c

clean:
	@rm bus

zip:
	zip bus bus.c Makefile
