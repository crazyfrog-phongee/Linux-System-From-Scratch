#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/wait.h>

#define MSG_SIZE    30

char* msg = "Nguyen Trong Phong";

void sigchld_handler(int signum);

int main(int argc, char const *argv[])
{
    pid_t child_pid;
    int fds[2] = {0};
    char in_buffer[MSG_SIZE];
    int num_read;

    if (pipe(fds) == -1)
    {
        fprintf(stderr, "pipe(): error\n");
        exit(EXIT_FAILURE);
    }

    child_pid = fork();
    if (child_pid == 0)
    {
        printf("Hello from Child Process\n");

        if (close(fds[1]) == -1)
        {
            fprintf(stderr, "close(): error\n");
            exit(EXIT_FAILURE);
        }

        while (1)
        {
            num_read = read(fds[0], in_buffer, MSG_SIZE);
            if (num_read > 0)
            printf("msg: %s with the number of bytes read: %d\n", in_buffer, num_read);
            else if (num_read == 0)
            {
                printf("Pipes: write end of pipe\n");
                break;
            }
            else
            {
                fprintf(stderr, "read(): error");
                exit(EXIT_FAILURE);
            }
        }
    }
    else if (child_pid > 0)
    {
        printf("Hello from Parent Process\n");

        signal(SIGCHLD, sigchld_handler);

        if (close(fds[1]) == -1)
        {
            fprintf(stderr, "close(): error\n");
            exit(EXIT_FAILURE);
        }
        
        write(fds[1], (void *)(msg), MSG_SIZE);

        if (close(fds[0]) == -1)
        {
            fprintf(stderr, "close(): error\n");
            exit(EXIT_FAILURE);
        }
        
        while(1);
    }
    else
    {
        fprintf(stderr, "Cannot handle fork()\n");
        exit(EXIT_FAILURE);
    }

    return 0;
}

void sigchld_handler(int signum)
{
    int rv, status;

    /* Print the end status of the child process */	
	rv = wait(&status);
	if (rv == -1)
    {
		fprintf(stderr, "wait() error\n");
        exit(EXIT_FAILURE);
    }
	else
	{
		if (WIFEXITED(status))  /* WIFEXITED() return true if the child terminated normally */
			printf("The child terminated normally with status %d\n", WEXITSTATUS(status));  /* WEXITSTATUS(status) returns the exit status of the child. */
		else if (WIFSIGNALED(status)) /* WIFSIGNALED(status) returns true if the child process was terminated by a signal */
			printf("The child was killed by signal, value = %d\n", WTERMSIG(status)); /* WTERMSIG(status) returns the number of the signal that caused the child process to terminate.*/
	}
    
    return;
}