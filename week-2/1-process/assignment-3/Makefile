.PHONY : all, clean

CC := gcc 
CFLAGS := -Werror -Wall

all:
	$(CC) -o test main.c $(CFLAGS)

debug:
	$(CC) -o test main.c $(CFLAGS) -g

clean:
	rm -rf test