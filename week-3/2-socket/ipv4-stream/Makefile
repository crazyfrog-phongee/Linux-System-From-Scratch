.PHONY : all, clean

CC := gcc
CFLAG := -Wall -Werror

all:
	$(CC) -o server ipv4_stream_server.c $(CFLAG)
	$(CC) -o client ipv4_stream_client.c $(CFLAG)
	$(CC) -o poll_server poll_server.c $(CFLAG)
clean:
	rm -rf server
	rm -rf client
	rm -rf poll_server