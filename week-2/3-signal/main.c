#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

void sig_handler1(int signum);

int main()
{
	sigset_t newset, oldset;

	if (signal(SIGINT, sig_handler1) == SIG_ERR)
	{
		fprintf(stderr, "Cannot handle SIGINT\n");
		exit(EXIT_FAILURE);
	}
	

	sigemptyset(&newset);
	sigemptyset(&oldset);

	printf("Current signal mask of the process: %x", oldset);

	sigaddset(&newset, SIGINT);

	if (sigprocmask(SIG_SETMASK, &newset, &oldset) == 0)
	{
		if (sigismember(&newset, SIGINT) == 1)
		{
			printf("SIGINT is a member of set\n");
		}
		else if (sigismember(&newset, SIGINT) == 0)
		{
			printf("SIGINT is not a member of set\n");
		}
		else if (sigismember(&newset, SIGINT) == -1)
		{
			printf("On error\n");
		}
	}

	/* Print current signal mask of the process */
	printf("Current signal mask of the process: %x", oldset);
	
	while(1);

	return 0;
}

void sig_handler1(int signum)
{
	printf("\nIm signal handler1: %d\n", signum);
	exit(EXIT_SUCCESS);
}
