#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>

void func(int signum);

int main(int argc, char const *argv[])
{
	pid_t child_pid;

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

			signal(SIGCHLD, func);
			while(1);
			printf("The parent process is doing other exercises\n");
		}
	} else 
	{
		printf("fork() unsuccessfully\n");
	}
	return 0;
}

void func(int signum)
{
	int rv, status;
	printf("Hello from  func()\n");
    	//wait(NULL);
	
	/* Print the end status of the child process */	
	rv = wait(&status);
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
