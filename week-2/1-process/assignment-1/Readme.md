# Description: Blocking and Unblocking Signal 
## Topic: Assume that a parent process has set up a handler for SIGCHLD and also blocked this signal. Then one of its child processes exits and the parrent process excutes wait() for collecting the end state of the child process. What happens when the parent process unblocks the SIGCHLD?
## Relate to: Signal Lifecycle (especially DELIVERY stage)


## Research:
1. struct pid_t: integer datatype originally.
In linux/types.h, it is defined:

    typedef __kernel_pid_t		pid_t;

In posix_types.h, it is defined:

    #ifndef __kernel_pid_t
    typedef int		__kernel_pid_t;
    #endif

2. system call fork()

3. system call wait()

    pid_t wait(int *wstatus);
    
RETURN VALUE: on success, returns the process ID of the terminated child; on failure, -1 is returned.

# REFERENCES:
(Research 1.):
    https://elixir.bootlin.com/linux/latest/source/include/linux/types.h#L22

(Research 2.):
    https://man7.org/linux/man-pages/man2/fork.2.html
    https://linuxhint.com/fork-system-call-linux/
(Research 3.)
    https://man7.org/linux/man-pages/man2/wait.2.html


