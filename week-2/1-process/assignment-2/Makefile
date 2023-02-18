.PHONY : clean, all

CC := gcc
CFLAGS := -Werror -Wall

all:
	${CC} -o test 	main.c $(CFLAGS)
	${CC} -o hello 	hello.c $(CFLAGS)
	${CC} -o demo 	test.c $(CFLAGS)

clean:
	rm -rf test
	rm -rf hello
	rm -rf demo