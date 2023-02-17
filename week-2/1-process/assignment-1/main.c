#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>

void sig_handler1(int signum);

int main(int argc, char const *argv[])
{
    sigset_t oldset, newset;
    pid_t child_pid;
    int status;

    if (signal(SIGCHLD, sig_handler1) == SIG_ERR)
    {
        fprintf(stderr, "Cannot handle SIGCHLD\n");
        exit(EXIT_FAILURE);
    }

    sigemptyset(&oldset);
    sigemptyset(&newset);

    sigaddset(&newset, SIGCHLD);

    if (sigprocmask(SIG_SETMASK, &newset, NULL) == 0)
    {
        // sigprocmask(SIG_SETMASK, NULL, &oldset);
        // printf("%lu\n", oldset.__val[0]);
        printf("sigprocmask() called successfully. Main process start blocking SIGCHLD signal\n");
    }
    else
    {
        fprintf(stderr, "sigprocmask() called on failure.\n");
        exit(EXIT_FAILURE);
    }

    child_pid = fork();

    if (child_pid == -1)
    {
        fprintf(stderr, "Error while calling fork(). No child process is created\n");
        exit(EXIT_FAILURE);
    }
    else if (child_pid == 0)
    {
        printf("Hello from Child Process\n");
        while (1);
    }
    else if (child_pid > 0)
    {
        printf("Hello from Parent Process\n");
        int rv = wait(&status);

        if (rv == -1)
        {
            fprintf(stderr, "wait() is called unsucessfully\n");
            exit(EXIT_FAILURE);
        }
        else
        {
            printf("Im the parent process, PID child process: %d\n", rv);
        }

        sigdelset(&newset, SIGCHLD);

        if (sigprocmask(SIG_SETMASK, &newset, NULL) == 0)
        {
            sigprocmask(SIG_SETMASK, NULL, &oldset);
            printf("%lu\n", oldset.__val[0]);
            printf("sigprocmask() called successfully. Main process stop blocking SIGCHLD signal\n");
        }
        else
        {
            fprintf(stderr, "sigprocmask() called on failure.\n");
            exit(EXIT_FAILURE);
        }

        while (1);
    }

    return 0;
}

void sig_handler1(int signum)
{
    printf("\nIm signal handler1: %d\n", signum);
	// exit(EXIT_SUCCESS);
}