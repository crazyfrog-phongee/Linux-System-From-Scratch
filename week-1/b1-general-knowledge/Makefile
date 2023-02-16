.PHONY: all, static, shared, link

all:
	gcc -c -Werror -Wall  main.c 		-o obj/static/main.o 		-I./inc
	gcc -c -Werror -Wall src/helloWorld.c 	-o obj/static/helloWorld.o 	-I./inc
	gcc -c -Werror -Wall src/helloPhong.c 	-o obj/static/helloPhong.o 	-I./inc
	
	gcc -c -fPIC -Werror -Wall main.c		-o obj/shared/main.o		-I./inc
	gcc -c -fPIC -Werror -Wall src/helloWorld.c 	-o obj/shared/helloWorld.o 	-I./inc
	gcc -c -fPIC -Werror -Wall src/helloPhong.c 	-o obj/shared/helloPhong.o 	-I./inc

static:
	ar rcs lib/static/libhello.a obj/static/helloWorld.o obj/static/helloPhong.o

shared:
	gcc -shared -Werror -Wall obj/shared/helloWorld.o obj/shared/helloPhong.o -o lib/shared/libhello.so
	
link:
	gcc -Werror -Wall obj/static/main.o -Llib/static -lhello -o bin/statically-linked
	gcc -Werror -Wall obj/shared/main.o -Llib/shared -lhello -o bin/dynamically-linked

