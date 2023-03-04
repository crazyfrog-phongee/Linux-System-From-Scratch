#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>

#define BUFFER_SIZE 256

void dostuff(int sock_fd);

int main(int argc, char const *argv[])
{
    int server_fd, port_no;
    struct sockaddr_in serv_addr;

    /* Reading server address and port number from command line */
    if (argc < 3)
    {
        printf("Usage: ./client <server_address> <port_number>\n");
        exit(EXIT_FAILURE);
    }
    else
        port_no = atoi(argv[2]);
    
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1)
    {
        fprintf(stderr, "socket(): error\n");
        exit(EXIT_FAILURE);
    }

    /* Initialize server address for client to connect to */
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port_no);
    if (inet_pton(AF_INET, argv[1], &serv_addr.sin_addr) == -1)
    {
        fprintf(stderr, "inet_pton(): error\n");
        exit(EXIT_FAILURE);
    }

    /* The connect function is called by the client to establish a connection to the server. */
    if (connect(server_fd, (struct sockaddr *)(&serv_addr), sizeof(serv_addr)) == -1)
    {
        fprintf(stderr, "connect(): error\n");
        exit(EXIT_FAILURE);
    }    
    
    /* Communicating */
    dostuff(server_fd);

    close(server_fd);
    return 0;
}

void dostuff(int sock_fd)
{
    int ret;
    char buffer[BUFFER_SIZE];

    while (1)
    {
        memset(buffer, '\0', BUFFER_SIZE);
        printf("Please enter the message : ");
        fgets(buffer, BUFFER_SIZE, stdin);

        /* Write to a socket file descriptor */
        ret = write(sock_fd, buffer, strlen(buffer));
        if (ret == -1)
        {
            fprintf(stderr, "write: error\n");
            exit(EXIT_FAILURE);
        }

        memset(buffer, '\0', BUFFER_SIZE);
        ret = read(sock_fd, buffer, BUFFER_SIZE);
        if (ret == -1)
        {
            fprintf(stderr, "read(): error\n");
            exit(EXIT_FAILURE);
        }
        else
            printf("Message from Server: %s\n", buffer);
        
        if (strncmp(buffer, "exit", 4) == 0)
        {
            printf("Server exit ...\n");
            break;
        }
    }
}
