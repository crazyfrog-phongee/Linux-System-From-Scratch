all:
	gcc -c main.c 			-o obj/static/main.o 		-I./inc
	gcc -c src/helloWorld.c 	-o obj/static/helloWorld.o 	-I./inc
	gcc -c src/helloPhong.c 	-o obj/static/helloPhong.o 	-I./inc
	
	gcc -c -fPIC main.c		-o obj/shared/main.o		-I./inc
	gcc -c -fPIC src/helloWorld.c 	-o obj/shared/helloWorld.o 	-I./inc
	gcc -c -fPIC src/helloPhong.c 	-o obj/shared/helloPhong.o 	-I./inc

static:
	ar rcs lib/static/libhello.a obj/helloWorld.o obj/helloPhong.o

shared:
	gcc -shared obj/shared/helloWorld.o obj/shared/helloPhong.o -o lib/shared/libhello.so
	
link:
	gcc obj/static/main.o -Llib/static -lhello -o bin/statically-linked
	gcc obj/shared/main.o -Llib/shared -lhello -o bin/dynamically-linked

LD_LIBRARY_PATH=$(pwd)/lib/shared bin/dynamically-linked
