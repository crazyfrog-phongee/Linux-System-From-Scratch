#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <sys/types.h>
#include <errno.h>

#define handle_error_en(en, msg) \
    do { errno = en; perror(msg); exit(EXIT_FAILURE); } while (0)

int main(int argc, char const *argv[])
{
    if (argc != 2)
    {
        printf("Usage: ./scorer manager_pid\n");
        exit(EXIT_FAILURE);
    }

    pid_t manager_pid = atoi(argv[1]);
    int ret, option; 
    
    while (1)
    {   
        printf ("Action: \n");
        printf ("\t1\tBatsman hit 17\n");
        printf ("\t2\tBatsman hit 6\n");
        printf ("\t3\tWicket falls\n\n");
        printf ("\t0\tQuit\n\n\n");
        printf ("Enter action: ");

        scanf ("%d", &option);

        if (option == 0)
            break;

        if (option >= 1 && option <= 3) 
        {   
            union sigval sig_val;    
            switch (option) 
            {
                case 1: 
                    sig_val.sival_int = 17;
                    break;

                case 2: 
                    sig_val.sival_int = 6;
                    break;

                case 3: 
                    sig_val.sival_int = 'W';
                    break;
            }
            
            /* Sending signal */
            if ((ret = sigqueue(manager_pid, SIGRTMIN, sig_val)))
                handle_error_en(ret, "sigqueue");
        }
        else
            printf("Illegal option, try again\n\n");
    }

    return 0;
}

