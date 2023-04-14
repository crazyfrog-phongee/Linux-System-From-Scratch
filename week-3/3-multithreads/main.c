#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

void *runner(void *param)
{
    /* some operations ... */
    printf("Hello from threadID: %ld with param\n", pthread_self());
    pthread_exit(NULL);
}

int n;

int main(void)
{

    int i;
    pthread_t *threadIdArray;

    n = 2; /* for example */

    threadIdArray = malloc((n + n - 1) * sizeof(pthread_t));

    for (i = 0; i < (n + n - 1); i++)
    {
        if (pthread_create(&threadIdArray[i], NULL, runner, NULL) != 0)
        {
            printf("Couldn't create thread %d\n", i);
            exit(1);
        }
    }

    for (i = 0; i < (n + n - 1); i++)
    {
        pthread_join(threadIdArray[i], NULL);
        printf("Terminate threadID index: %d\n", i);
    }

    free(threadIdArray);

    return (0);
}