#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>

#define handle_error_en(en, msg) \
    do { errno = en; perror(msg); exit(EXIT_FAILURE); } while (0)

void sig_rtmin_handler(int signum, siginfo_t *siginfo, void *context);

int main(int argc, char const *argv[])
{
    struct sigaction act;
    int ret;

    printf("PID of manager: %d\n", getpid());

    memset(&act, 0, sizeof(act));
    act.sa_sigaction = sig_rtmin_handler;
    act.sa_flags = SA_SIGINFO;

    if ((ret = sigaction(SIGRTMIN, &act, NULL)))
    {
        handle_error_en(ret, "sigaction");
    }

    while(1);

    return 0;
}

void sig_rtmin_handler(int signum, siginfo_t *siginfo, void *context)
{
    switch (siginfo -> si_value.sival_int) 
    {
        case 17: 
            printf("Seventeen runs scored.\n");
            break;

        case 6:
            printf("Six runs scored.\n");
            break;

        case 'W': 
            printf("A wicket has fallen.\n");
            break;

        default:
            printf("Unclear communication\n");
    }
}
