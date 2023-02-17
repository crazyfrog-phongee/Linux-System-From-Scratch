#Description: Block SIGINT signal

#Error Resolved:
    1. Tracing the header files included in signal.h, I found that sigset_t is a struct defined in /usr/include/x86_64-linux-gnu/bits/types/__sigset_t.h as __sigset_t.h and typedefed in /usr/include/x86_64-linux-gnu/bits/types/sigset_t.h:

        #define _SIGSET_NWORDS (1024 / (8 * sizeof (unsigned long int)))
        typedef struct
        {
        unsigned long int __val[_SIGSET_NWORDS];
        } __sigset_t;
    
    2. As you can see, sigemptyset() and sigfillset() manipulate all the 1024 bits of the set. Meanwhile, sigprocmask() manipulates only the first 64 bits (one unsigned long int)

    3. A few points to NOTE:

    int sigprocmask(int how, const sigset_t *restrict set, sigset_t *restrict oldset);

    The behavior of the call is dependent on the value of how, as follows:
        * SIG_SETMASK: 
            If oldset is non-NULL, THE PREVIOUS VALUE of the signal mask is
            stored in oldset (NOTE)
    So, if you wanna get the current value of signal mask of the process after FETCHING the signal mask, i think:
    Firstly, call:
        sigprocmask(SIG_SETMASK, &newset, NULL);    // Fetching the signal mask with the value of newset
    Secondly, call:
        sigprocmask(SIG_SETMASK, NULL, &oldset);    // Get the current value of signal mask of the process

#References:
    Resolve (1. and 2. Error Resolved): https://stackoverflow.com/questions/33579854/sigset-t-unix-using-sigprocmask   
    Source: https://elixir.bootlin.com/linux/v4.2/source/include/linux/signal.h#L40
            https://man7.org/linux/man-pages/man2/sigprocmask.2.html


