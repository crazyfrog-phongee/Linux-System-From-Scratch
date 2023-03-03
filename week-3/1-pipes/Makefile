.PHONY : all, clean

CC := gcc
CFLAGS := -Wall -Werror

all:
	$(CC) -o related_pipe 	related_pipe.c 	$(CFLAGS)
	$(CC) -o named_pipe 	named_pipe.c 	$(CFLAGS)
	$(CC) -o producer		producer.c 		$(CFLAGS)
clean:
	rm -rf related_pipe
	rm -rf named_pipe
	rm -rf producer