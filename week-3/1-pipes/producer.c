#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>

#define BUFF_SIZE   1024
const char* pathname = "./myFifos.txt";

int main(int argc, char const *argv[])
{
    int fd;
    char buff[BUFF_SIZE];

    while (1)
    {
        printf("Message to named_pipe: "); fflush(stdin);
        fgets(buff, BUFF_SIZE, stdin);

        fd = open(pathname, O_WRONLY);
        if (fd == -1)
        {
            fprintf(stderr, "open(): error\n");
            exit(EXIT_FAILURE);
        }

        write(fd, buff, strlen(buff) + 1);

        close(fd);
    }
    
    return 0;
}
