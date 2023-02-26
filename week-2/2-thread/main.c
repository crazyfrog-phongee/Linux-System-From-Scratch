#include <stdio.h>
#include <signal.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <errno.h>
#include <string.h>

#define handle_error_en(en, msg) \
    do                           \
    {                            \
        errno = en;              \
        perror(msg);             \
        exit(EXIT_FAILURE);      \
    } while (0)

pthread_t thread_id1, thread_id2;

void print_signal_mask(unsigned long int num)
{
    uint8_t binary_num[64] = {0};
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

/* Function check the specific signal of the pending signal. */
void check_pending(pthread_t tid, int sig, char *signame)
{
    sigset_t sigset;

    if (sigpending(&sigset) != 0)
        perror("sigpending() error\n");
    else if (sigismember(&sigset, sig))
        printf("%ld: a %s signal is pending\n", tid, signame);
    else
        printf("%ld: no %s signals are pending\n", tid, signame);
}

void *thread_handle1(void *arg)
{
    printf("Hello from ThreadID1 with %ld\n", pthread_self());

    sigset_t *set = (sigset_t *)(arg);
    sigset_t sig_pending;
    int rv, count = 0;

    sigaddset(set, SIGUSR1);
    sigprocmask(SIG_BLOCK, set, NULL);

    for (;;)
    {
        if ((rv = sigpending(&sig_pending)))
            handle_error_en(rv, "sigpending");

        if (sig_pending.__val[0] == 0)
            printf("ThreadID1: No signals are pending\n");
        else
        {
            printf("ThreadID1: Signals are pending\n");
            print_signal_mask(sig_pending.__val[0]); /* Print current signal pending */
        }

        count++;
        if (count == 5)
            pthread_exit(NULL);

        sleep(5);
    }
}

void *thread_handle2(void *arg)
{
    printf("Hello from ThreadID2 with %ld\n", pthread_self());

    sigset_t *set = (sigset_t *)(arg);
    sigset_t sig_pending;
    int rv, count = 0;

    sigaddset(set, SIGUSR2);
    sigprocmask(SIG_BLOCK, set, NULL);

    for (;;)
    {
        if ((rv = sigpending(&sig_pending)))
            handle_error_en(rv, "sigpending");

        if (sig_pending.__val[0] == 0)
            printf("ThreadID2: No signals are pending\n");
        else
        {
            printf("ThreadID2: Signals are pending\n");
            print_signal_mask(sig_pending.__val[0]); /* Print current signal pending */
        }

        count++;
        if (count == 5)
            pthread_exit(NULL);

        sleep(2);
    }
}

int main(int argc, char const *argv[])
{
    int ret;
    sigset_t oldset, newset;

    sigemptyset(&newset);
    sigemptyset(&oldset);

    sigaddset(&newset, SIGTERM);

    if ((ret = pthread_sigmask(SIG_BLOCK, &newset, NULL)))
        handle_error_en(ret, "pthread_sigmask");

    if ((ret = pthread_create(&thread_id1, NULL, thread_handle1, &newset)))
        handle_error_en(ret, "pthread_create");

    if ((ret = pthread_create(&thread_id2, NULL, thread_handle2, &newset)))
        handle_error_en(ret, "pthread_create");

    if (sigprocmask(SIG_SETMASK, NULL, &oldset) == 0)
    {
        printf("Signal mask in main thread: ");
        print_signal_mask(oldset.__val[0]);
    }

    sleep(2);

    pthread_kill(thread_id1, SIGTERM);

    sleep(5);

    pthread_kill(thread_id2, SIGUSR2);
    pthread_kill(thread_id1, SIGUSR1);

    pthread_join(thread_id1, NULL);
    pthread_join(thread_id2, NULL);

    return 0;
}
