#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#define BUFF_SIZE   1024
const char* pathname = "./myFifos.txt";


int main(int argc, char const *argv[])
{
    int ret;
    int fd;
    char buff[BUFF_SIZE];

    if ((ret = mkfifo(pathname, 0666)))
    {
        fprintf(stderr, "mkfifo(): error\n");
        exit(EXIT_FAILURE);
    }

    while (1)
    {
        fd = open(pathname, O_RDONLY);
        if (fd == -1)
        {
            fprintf(stderr, "open(): error\n");
            exit(EXIT_FAILURE);
        }

        read(fd, buff, BUFF_SIZE);
        printf("msg: %s", buff);

        close(fd);
    }
    
    return 0;
}
