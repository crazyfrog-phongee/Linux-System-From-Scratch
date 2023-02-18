# DESCRIPTION: What happens when calling the exec() function family!
## Topic: What happens when we use the execute function family (execl/exevp/exepv)? Explain and write a program to verify the answer. Then tell us how the system() command works.

## Related:

* exec(): The exec() family of functions REPLACES the current process image with a new process image.

* system(): The system() library function uses fork(2) to CREATE A CHILD PROCESS that executes the shell command specified in command using execl(3) as follows:

    execl("/bin/sh", "sh", "-c", command, (char *) NULL);

system() returns after the command has been completed.

During execution of the command, SIGCHLD will be blocked, and SIGINT and SIGQUIT will be ignored, in the process that calls
system(). (These signals will be handled according to theirdefaults inside the child process that executes command.)

If command is NULL, then system() returns a status indicating whether a shell is available on the system.

## Advanced Expansion: Combining fork() and exec() system calls

# REFERENCES:
    https://linuxhint.com/linux-exec-system-call/
    https://linux.die.net/man/3/execl
