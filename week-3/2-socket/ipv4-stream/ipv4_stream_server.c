#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <errno.h>

#define BUFFER_SIZE 256
#define BACKLOG 5 /* The maximum length to which the queue of pending connections for server_fd may grow */

void dostuff(int sock, char* buffer);

int main(int argc, char const *argv[])
{
    int server_fd, new_sock_fd;
    int port_no, cli_len, opt;
    struct sockaddr_in serv_addr, cli_addr;

    pid_t child_pid;

    memset(&serv_addr, 0, sizeof(serv_addr));
    memset(&cli_addr, 0, sizeof(cli_addr));

    if (argc != 2)
    {
        printf("No port provided\nUsage: ./server <port_number>\n");
        exit(EXIT_FAILURE);
    }
    else
        port_no = atoi(argv[1]);

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1)
    {
        fprintf(stderr, "socket(): error\n");
        exit(EXIT_FAILURE);
    }
    
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)))
    {
        fprintf(stderr, "setsockopt(): error\n");
        perror("Error");
        exit(EXIT_FAILURE);
    }
    
    /* Initialize server address */
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port_no);
    serv_addr.sin_addr.s_addr = INADDR_ANY;

    /*
        Binding a socket to the an address
        bind() assigns the address specified by serv_addr to the socket referred to by the file descriptor server_fd
    */
    if (bind(server_fd, (struct sockaddr *)(&serv_addr), sizeof(serv_addr)) == -1)
    {
        fprintf(stderr, "bind(): error\n");
        perror("Error");
        exit(EXIT_FAILURE);
    }

    /* Allows the process to listen on the socket for connections */
    printf("Server is listening at port : %d \n....\n", port_no);
    listen(server_fd, BACKLOG);

    /*
        The accept() system call causes the process to block until a client connects to the server.
        Thus, it wakes up the process when a connection from a client has been successfully established
    */
    while (1)
    {
        cli_len = sizeof(cli_addr);
        new_sock_fd = accept(server_fd, (struct sockaddr *)(&cli_addr), (socklen_t *)(&cli_len));
        if (new_sock_fd == -1)
        {
            fprintf(stderr, "accept(): error\n");
            exit(EXIT_FAILURE);
        }
        else
            system("clear");

        printf("Server : got connection\n");

        child_pid = fork();
        if (child_pid == -1)
        {
            fprintf(stderr, "Error while calling fork(). No child process is created\n");
            exit(EXIT_FAILURE);
        }
        else if (child_pid == 0)
        {
            printf("Hello from Child Process with PID: %d\n", getpid());
            close(server_fd);

            char buffer[BUFFER_SIZE];

            while (1)
            {
                dostuff(new_sock_fd, buffer);
                if (strncmp(buffer, "bye", 3) == 0)
                    break;
            }
            
            close(new_sock_fd);
            exit(0);
        }
        else if (child_pid > 0)
        {
            printf("Hello from Parent Process with PID: %d\n", getpid());
            wait(NULL);
            printf("Kill the zombie process successfuly\n");

            close(new_sock_fd);
        }
    }

    close(server_fd);

    return 0;
}

void dostuff(int sock_fd, char* buffer)
{
    int ret;
    
    memset(buffer, '\0', BUFFER_SIZE);
    ret = read(sock_fd, buffer, BUFFER_SIZE);
    if (ret == -1)
    {
        fprintf(stderr, "read(): error\n");
        exit(EXIT_FAILURE);
    }
    else
        printf("Message from Client: %s\n", buffer);

    if (strncmp(buffer, "bye", 3) == 0)
        ret = write(sock_fd, "exit", 4);
    else
        ret = write(sock_fd, "I got your message", 18);
    if (ret == -1)
    {
        fprintf(stderr, "write: error\n");
        exit(EXIT_FAILURE);
    }
}