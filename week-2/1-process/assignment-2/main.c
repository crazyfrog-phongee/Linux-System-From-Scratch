#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

int main(int argc, char const *argv[])
{
    const char *args = "Hello C Programing";
    int rv;

    printf("PID of the main process which contains main.c program: %d\n", getpid());

    /* Calling execl() function */
    rv = execl("./hello", args, NULL);
    if (rv == -1)
    {
        fprintf(stderr, "execl() function is called on failure\n");
        exit(EXIT_FAILURE);
    }
    
    /* After calling execl() function */
    printf("Back to main.c\n");
    return 0;
}
