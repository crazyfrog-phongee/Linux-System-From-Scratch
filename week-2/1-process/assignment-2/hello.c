#include <stdio.h>
#include <unistd.h>

int main(int argc, char const *argv[])
{
    printf("Hello from hello.c\n");
    printf("PID current of the process contains hello.c program: %d\n", getpid());
    return 0;
}
