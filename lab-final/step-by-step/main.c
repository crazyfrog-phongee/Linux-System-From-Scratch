#define _GNU_SOURCE
#define BUILDING_GATEWAY /* This definition for using some macros in config.h file */

#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/wait.h>
#include <string.h>
#include <pthread.h>

#include "errmacros.h"
#include "config.h"
#include "lib/tcpsock.h"
#include "lib/dplist.h"
#include "sbuffer.h"

/**
 * Global Variables Definition
 */

pthread_t pthread_id[NUM_THREADS];
void* exit_codes[NUM_THREADS]; /* array of pointers to thread returns */

static int fds[2];
static pthread_mutex_t ipc_pipe_mutex; /* Use as mutex to IPC pipe for logging */

static sbuffer_t *buffer;              /* incomplete data type, all logic and synchronization of buffer is taken care of in sbuffer implementation */ 
static int sbuffer_open = 1;
static pthread_rwlock_t sbuffer_open_rwlock;

static pthread_mutex_t connmgr_drop_conn_mutex;
static sensor_id_t connmgr_sensor_to_drop = 0;

static int storagemgr_failed = 0;
static pthread_rwlock_t storagemgr_failed_rwlock;

void *connection_manager(void *arg);

int main(int argc, char const *argv[])
{
    pid_t child_pid, parent_pid;
    int ret;
    int server_port; /* Store port number of the server */

    if (argc != 2)
    {
        printf("No port provided\nUsage: ./server <port_number>\n");
        exit(EXIT_FAILURE);
    }
    else
        server_port = atoi(argv[1]);

    parent_pid = getpid();

    ret = pipe(fds);
    SYSCALL_ERROR(ret);

    pthread_mutex_init(&ipc_pipe_mutex, NULL);

    child_pid = fork();
    SYSCALL_ERROR(child_pid);

    if (child_pid == 0) /* Child process code */
    {
        ret = close(fds[1]);    /* Close the write end of pipe */
        SYSCALL_ERROR(ret);

        char rcv_buf[PIPE_SIZE];    /* Rcv buffer for storing data read from pipe file descriptor */
        int num_read;
        int sequence = 0;
        char *buffer_to_write; /* Buffer for saving output format data and writing to log file*/
        char *msg;             /* Using for saving the error message returning from reading pipe and writing to log file */

        FILE *log_data = fopen("gateway.log", "w"); /* Open the log file with write mode */
        FILE_OPEN_ERROR(log_data);

        child_pid = getpid();
        parent_pid = getppid();

        #if (DEBUG_LVL > 0)
                printf(CHILD_POS "Child process (%d) of parent (%d) is started...\n", child_pid, parent_pid);
                fflush(stdout);
        #endif

        while (1)
        {
            num_read = read(fds[0], rcv_buf, PIPE_SIZE);

            if (num_read > 0)
            {
                asprintf(&buffer_to_write, "%d %s\n", sequence++, rcv_buf);
                fwrite(buffer_to_write, strlen(buffer_to_write), 1, log_data); /* Writing data from pipe to log file */

                free(buffer_to_write);
            }
            else if (num_read == 0)
            {
                asprintf(&msg, "%d %ld (Pipe end of pipe): Pipe between parent (%d) and child (%d) terminated normally\n", sequence++, time(NULL), parent_pid, child_pid);
                break;
            }
            else
            {
                asprintf(&msg, "%d %ld Error reading from pipe, pipe closed\n", sequence++, time(NULL));
                break;
            }
        }

        fwrite(msg, strlen(msg), 1, log_data); /* Writing error message returning from reading pipe to the log file */
        free(msg);

        close(fds[0]); /* Close the read end of pipe */
        fclose(log_data); /* Close log file */

        #if (DEBUG_LVL > 0)
            printf(CHILD_POS "Child process (%d) of parent (%d) is terminating...\n", child_pid, parent_pid);
            fflush(stdout);
        #endif

        exit(EXIT_SUCCESS);
    }
    /* Parent process code */
    ret = close(fds[0]);
    SYSCALL_ERROR(ret);

    #if (DEBUG_LVL > 0)
        printf("Parent process (%d) has created child logging process (%d)...\n", parent_pid, child_pid);
        fflush(stdout);
    #endif

    /* Initialize shared data buffer */
    sbuffer_init(&buffer);
    pthread_rwlock_init(&sbuffer_open_rwlock);
    pthread_rwlock_init(&storagemgr_failed_rwlock, NULL);
    pthread_mutex_init(&connmgr_drop_conn_mutex, NULL);

    /* Create 3 threads: the connection, the data and the storage manager threads. */
    ret = pthread_create(&thread_id[0], NULL, connection_manager, &port_no);
    SYSCALL_ERROR(ret);
    // ret = pthread_create(&thread_id[1], NULL, data_manager, NULL);
    // SYSCALL_ERROR(ret);
    // rv = pthread_create(&thread_id[2], NULL, storage_manager, NULL);
    // SYSCALL_ERROR(ret);

    for(int i = 0; i < NUM_THREADS; i++) pthread_join(threads[i], &exit_codes[i]); /* blocks until all threads terminate */ 
    
    #if (DEBUG_LVL > 0)
    printf("Threads stopped. Cleaning up\nThread exit result:\n"CHILD_POS"Data Manager: %d\n"CHILD_POS"Storage Manager %d\n"CHILD_POS"Connection Manager: %d\n", *((int *) exit_codes[0]), *((int *) exit_codes[1]), *((int *) exit_codes[2]));
    fflush(stdout);
    #endif

    close(fds[1]); /* First, let threads terminate, then shut down IPC pipe -> ensures all data in pipe is written before closing */ 
    wait(NULL); /* Wait on child process changing state */

    #if (DEBUG_LVL > 0)
    printf("Child process stopped. Cleaning up\n");
    fflush(stdout);
    #endif
    
    /* Free memory and destroy mutex, rwlock */
    sbuffer_free(&buffer);
    pthread_rwlock_destroy(&sbuffer_open_rwlock);
    pthread_mutex_destroy(&ipc_pipe_mutex);
    pthread_mutex_destroy(&connmgr_drop_conn_mutex);
    pthread_rwlock_destroy(&storagemgr_failed_rwlock);
    
    for(int i = 0; i < NUM_THREADS; i++) free(exit_codes[i]); // release memory alloc'ed to exit codes

    exit(EXIT_SUCCESS);
}

void *connection_manager(void *arg)
{
    #if (DEBUG_LVL > 0)
        printf("Connection Manager Thread is started\n");
        fflush(stdout);
    #endif

}