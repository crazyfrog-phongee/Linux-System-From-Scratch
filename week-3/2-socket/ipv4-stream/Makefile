.PHONY : all, clean

CC := gcc
CFLAG := -Wall -Werror

all:
	$(CC) -o server ipv4_stream_server.c $(CFLAG)
	$(CC) -o client ipv4_stream_client.c $(CFLAG)
	
clean:
	rm -rf server
	rm -rf client