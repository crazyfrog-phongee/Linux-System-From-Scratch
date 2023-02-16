#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>


int main(int argc, char const *argv[])
{
	pid_t child_pid;
	int rv, status;

	child_pid = fork();
	if (child_pid >= 0)
	{
		if (child_pid == 0)
		{
			printf("In the child process\n");
			printf("My PID is %d, my parent PID is %d\n", getpid(), getppid());
			while(1);	/* waiting for the kill signal */
			//return 0;	/* terminated normally */
		} else 
		{
			printf("In the parent process\n");
			printf("My PID is %d, my child PID is %d\n", getpid(), child_pid);

			rv = waitpid(child_pid, &status, 0);
			if (rv == -1)
			{
				printf("waitpid() unsucessful\n");
			} else
			{
				if (WIFEXITED(status))
				{
					printf("The child terminated normally with status %d\n", WEXITSTATUS(status));
				} else if (WIFSIGNALED(status))
				{
					printf("The child was killed by signal, value = %d\n", WTERMSIG(status));
				}
			}
		}
	} else 
	{
		printf("fork() unsuccessfully\n");
	}
	return 0;
}
