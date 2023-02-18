#include <stdio.h>
#include <stdlib.h>


int main(int argc, char const *argv[])
{
    char* command = "ls -lah";

    /* 
        Nature: After calling system() function
        Use fork() to create a child process that excutes the shell command specified in "command" using execl() as follows:
            execl("/bin/sh", "sh", "-c", command, (char *) NULL);
    */
    system(command);

    /* Since creating a child process that excutes the shell command specified in "command" using execl(), the printf() fucntionis executed */
    printf("Back to test.c\n");
    return 0;
}
