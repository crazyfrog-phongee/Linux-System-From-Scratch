#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>

void sig_handler1(int signum);
int sigisemptyset(sigset_t *setp);
void print_signal_mask(unsigned long int num);
void print_set_word(sigset_t *setp);

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

	sigaddset(&newset, SIGINT);

	if (sigprocmask(SIG_SETMASK, &newset, NULL) == 0)
	{
		sigprocmask(SIG_SETMASK, NULL, &oldset); /* Get the current value of the signal mask returned in oldset */

		printf("Current signal mask of the process in HEX: ");
		print_set_word(&oldset); /* Print the current signal mask of the process in HEX */
		printf("Current signal mask of the process in BINARY: ");
		print_signal_mask(oldset.__val[0]); /* Print the current signal mask of the process in BINARY */

		/* Check SIGINT is member of set */
		if (sigismember(&oldset, SIGINT) == 1)
		{
			printf("SIGINT is a member of set\n");
		}
		else if (sigismember(&oldset, SIGINT) == 0)
		{
			printf("SIGINT is not a member of set\n");
		}
	}

	while (1);

	return 0;
}

void sig_handler1(int signum)
{
	printf("\nIm signal handler1: %d\n", signum);
	exit(EXIT_SUCCESS);
}

int sigisemptyset(sigset_t *setp)
{
	return setp->__val[0] == 0;
}

void print_signal_mask(unsigned long int num)
{
	uint8_t binary_num[64];
	int count_bit = 0;

	while (num > 0)
	{
		binary_num[count_bit] = num % 2;
		num = num / 2;
		count_bit++;
	}

	for (int j = count_bit - 1; j >= 0; j--)
	{
		printf("%d ", binary_num[j]);
	}
	printf("\n");
}

void print_set_word(sigset_t *setp)
{
	int i;

	if (!setp)
	{
		fprintf(stderr, "print_set_word(): NULL parameter\n");
		return;
	}

	/* Print the current value of signal mask of the process */
	for (i = 0; i < 1; i++)
	{
		printf("%lu", setp->__val[i]);
	}

	printf("\n[%s]\n\n", sigisemptyset(setp) ? "Empty Set" : "Non-empty Set");
}
