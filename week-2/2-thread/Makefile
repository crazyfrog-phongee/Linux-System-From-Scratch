.PHONY : all, clean

CC := gcc
CFLAGS := -pthread -Werror -Wall

all:
	$(CC) -o test main.c $(CFLAGS)
	
clean:
	rm -rf test