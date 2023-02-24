.PHONY : all, clean

CC := gcc
CFLAGS := -Werror -Wall

all:
	$(CC) -o scorer 	scorer.c 	$(CFLAGS)
	$(CC) -o manager 	manager.c 	$(CFLAGS)
clean:
	rm -rf  scorer
	rm -rf 	manager
