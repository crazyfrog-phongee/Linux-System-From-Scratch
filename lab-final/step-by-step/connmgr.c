#define _GNU_SOURCE
#define BUILDING_GATEWAY

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <inttypes.h>
#include <sys/types.h>
#include <poll.h>

#include "config.h"
#include "connmgr.h"
#include "lib/tcpsock.h"
#include "lib/dplist.h"

/**
 * Custom Types: Custom element to use with my dplist implementation to be able to use compare() call
 * to and create dummy to compare with (tcpsock_t is incomplete data type otherwise, hence
 * can't create a dummy with a given sd to use in compare())
 */
struct tcpsock_dpl_el
{
    tcpsock_t *sock_ptr;
    int sd;
    sensor_ts_t last_active;
    sensor_id_t sensor;
};

/**
 * Global variables definition
 */
static int *pfds;
static pthread_mutex_t *ipc_pipe_mutex;

static int *sbuffer_open;
static pthread_rwlock_t *sbuffer_open_rwlock;

static pthread_mutex_t *connmgr_drop_conn_mutex;
static sensor_id_t *connmgr_sensor_to_drop;

static int *storagemgr_failed_flag;
static pthread_rwlock_t *storagemgr_failed_rwlock;

static dplist_t *socket_list; /* List of info clients are accepted by server */
static tcpsock_t *server;
static struct pollfd *poll_fds;
static int *retval;

/**
 * Callback functions for socket_list
 */
static void *socket_copy(void *element);
static void socket_free(void **element);
static int socket_compare(void *x, void *y);
void connmgr_init(connmgr_init_arg_t *arg)
{
    pfds = arg->ipc_pipe_fd;
    ipc_pipe_mutex = arg->pipe_mutex;
    sbuffer_open = arg->sbuffer_flag;
    sbuffer_open_rwlock = arg->sbuffer_rwlock;
    connmgr_drop_conn_mutex = arg->connmgr_drop_conn_mutex;
    connmgr_sensor_to_drop = arg->connmgr_sensor_to_drop;
    storagemgr_failed_flag = arg->storagemgr_fail_flag;
    storagemgr_failed_rwlock = arg->storagemgr_failed_rwlock;
    retval = arg->status;
}

void connmgr_listen(int port_number, sbuffer_t **buffer)
{
    char *send_buf; /* Using for saving the error message returning from reading pipe and writing to log file */

    /* Checking valid port */
    if (port_number < MIN_PORT || port_number > MAX_PORT)
    {
        asprintf(&send_buf, "%ld Connection Manager: invalid PORT", time(NULL));
        write_to_pipe(ipc_pipe_mutex, pfds, send_buf);

        /* Set pointers to NULL to avoid freeing unallocated space in call to 'free' (initial value may not be NULL)*/
        poll_fds = NULL;
        socket_list = NULL;
        server = NULL;
        *retval = CONNMGR_INCORRECT_PORT;

        return;
    }

    socket_list = dpl_create(&socket_copy, &socket_free, &socket_compare);
    poll_fds = (struct pollfd *) malloc(sizeof(struct pollfd));

    /* TCP Passive Open (Server): Create a new socket and open this socket in passive listening mode */
    if (tcp_passive_open(&server, port_number) != TCP_NO_ERROR)
    {
        asprintf(&send_buf, "%ld Connection Manager: failed to start server", time(NULL));
        write_to_pipe(ipc_pipe_mutex, pfds, send_buf);

        *retval = CONNMGR_SERVER_OPEN_ERROR;

        return;
    }
    asprintf(&send_buf, "%ld Connection Manager: started server successfully", time(NULL));
    write_to_pipe(ipc_pipe_mutex, pfds, send_buf);

    tcp_get_sd(server, &(poll_fds[0].fd)); /* Add the socket file descriptor refer to the server in poll_fds[0] element */
    poll_fds[0].events = POLLIN;           /* Setup event data to read for server socket fds */

    struct tcpsock_dpl_el *client; /* Storing the incoming client connection */
    struct tcpsock_dpl_el dummy;
    dplist_node_t *node;
    sensor_data_t data;
    int num_conn = 0, sbuffer_insertions = 0;
    int bytes;
    int poll_ret, tcp_conn_ret, tcp_ret, sbuffer_ret;

    /**
     * Loop for waiting incoming connection or incoming data 
     * On success, poll() returns a nonnegative value which is the number of elements in the 
     * pollfds whose revents fields have been set to a nonzero value (indicating an event or an error).
     * poll() return the number of available file description, or 0 if timeout, or -1 if on error 
     * \note it waits for one of a set of file descriptors to become ready to perform I/O. 
    */
    while ((poll_ret = poll(poll_fds, (num_conn + 1), TIMEOUT * 1000)) || num_conn)
    {
        pthread_rwlock_rdlock(storagemgr_failed_rwlock);    /* Putting inside the loop does not force storagemgr to hang until poll elapses TIMEOUT seconds */
        if (*storagemgr_failed_flag)   /* Connmgr instead treats the signal asynchronously whenever it is done polling */
        {
            pthread_rwlock_unlock(storagemgr_failed_rwlock);

            *retval = CONNMGR_INTERRUPTED_BY_STORAGEMGR;

            asprintf(&send_buf, "%ld Connection Manager: signalled to terminate by Storage Manager", time(NULL));
            write_to_pipe(ipc_pipe_mutex, pfds, send_buf);

            connmgr_free();

            #if (DEBUG_LVL > 0)
            printf("Connection Manager is stopped\n");
            fflush(stdout);
            #endif

            pthread_exit(retval);
        }
        pthread_rwlock_unlock(storagemgr_failed_rwlock);
        
        if (poll_ret == -1)
            break;
        if((poll_fds[0].revents & POLLIN) && num_conn < MAX_CONN) /* When an event is received from Server socket, create new socket unless limit is reached */
        {
            #if (DEBUG_LVL > 1)
                printf("Incoming client connection\n");
                fflush(stdout);
            #endif

            client = (struct tcpsock_dpl_el *) malloc(sizeof(struct tcpsock_dpl_el));
            if ((tcp_conn_ret = tcp_wait_for_connection(server, &(client->sock_ptr))) != TCP_NO_ERROR) /* Blocks until a connection is processed */
            {
                *retval = CONNMGR_SERVER_CONNECTION_ERROR;

                asprintf(&send_buf, "%ld Connection Manager: failed to accept new connection (%d)", time(NULL), tcp_conn_ret);
                write_to_pipe(ipc_pipe_mutex, pfds, send_buf);
            }
            else
            {
                num_conn++;
                poll_fds = (struct pollfd *) realloc(poll_fds, sizeof(struct pollfd) * (num_conn + 1)); /* Increase poll_fds array size */
                tcp_get_sd(client->sock_ptr, &(poll_fds[num_conn].fd)); /* For adding client socket fds to poll elements */
                poll_fds[num_conn].events = POLLIN | POLLHUP; /* Choose poll events */

                client->sd = poll_fds[num_conn].fd;
                client->last_active = (sensor_ts_t)time(NULL);
                client->sensor = 0;
                
                dpl_insert_sorted(socket_list, client, false);    /* Insert client connection into socket_list */ 

                asprintf(&send_buf, "%ld Connection Manager: new connection received", time(NULL));
                write_to_pipe(ipc_pipe_mutex, pfds, send_buf);

            #if (DEBUG_LVL > 0)
                printf("\n##### Printing Socket DPLIST Content Summary #####\n");
                dpl_print_heap(socket_list);
            #endif
            }
            poll_ret--;
        }
        /* Determine which the client's file descriptor is readable */
        for (int i = 1; i < (num_conn + 1) && poll_ret > 0; i++) /* poll_ret indicates number of structures, stop looping when that number is reached */
        {
            dummy.sd = poll_fds[i].fd;                                /* Find corresponding client, based on the sd */ 
            node = dpl_get_reference_of_element(socket_list, &dummy); /* Get corresponding element from socket_list */ 
            client = (node != NULL) ? (struct tcpsock_dpl_el *)dpl_get_element_of_reference(node) : NULL;
            if (client != NULL && ((client->last_active + (sensor_ts_t)TIMEOUT) > (sensor_ts_t)time(NULL)) && (poll_fds[i].revents & POLLIN)) /* If there is data available to read from client socket and socket is non NULL and has not timed out yet */
            {
                #if (DEBUG_LVL > 1)
                    printf("Receiving data from %d peer of %d total\n", i, num_conn);
                    fflush(stdout);
                #endif

                bytes = sizeof(data.id); /* read sensor ID */
                tcp_ret = tcp_receive(client->sock_ptr, (void *)&data.id, &bytes);
                bytes = sizeof(data.value); /* read temperature */
                tcp_ret = tcp_receive(client->sock_ptr, (void *)&data.value, &bytes);
                bytes = sizeof(data.ts); /* read timestamp */
                tcp_ret = tcp_receive(client->sock_ptr, (void *)&data.ts, &bytes);

                if ((tcp_ret == TCP_NO_ERROR) && bytes)
                {
                    client->last_active = (sensor_ts_t)time(NULL); /* Make sure to update last_active only when receiving is successful */
                    if (client->sensor == 0)
                        client->sensor = data.id; /* Update Sensor ID Client for the first incoming data */

                    sbuffer_ret = sbuffer_insert(*buffer, &data); /* Sbuffer implementation takes care of thread safety */
                    
                    if (sbuffer_ret == SBUFFER_SUCCESS) /* This block is purely for debugging */
                    {
                        sbuffer_insertions++;

                        #if (DEBUG_LVL > 1)
                            printf("Inserted new in shared buffer: %" PRIu16 " %g %ld\n", data.id, data.value, data.ts);
                            fflush(stdout);
                        #endif
                    }
                    else
                    {
                        #if (DEBUG_LVL > 1)
                            printf("Failed to insert in shared buffer: %" PRIu16 " %g %ld\n", data.id, data.value, data.ts);
                            fflush(stdout);
                        #endif
                    }
                }
                else if (tcp_ret == TCP_CONNECTION_CLOSED)
                {
                    poll_fds[i].events = -1;

                    asprintf(&send_buf, "%ld Connection Manager: lost connection with", time(NULL));
                    write_to_pipe(ipc_pipe_mutex, pfds, send_buf);
                }
            }

            /* Drop connection */
            pthread_mutex_lock(connmgr_drop_conn_mutex);
            if ((client != NULL && client->sensor == *connmgr_sensor_to_drop) || (poll_fds[i].revents & POLLHUP) || (poll_fds[i].events == -1) || (client != NULL && ((client->last_active + (sensor_ts_t)TIMEOUT) < (sensor_ts_t)time(NULL))) || client == NULL) // If peer terminated connection or connection timed out for existing socket or no element was found stop listening to this descriptor, remove file descriptor from the list
            {
                if (client != NULL && client->sensor == *connmgr_sensor_to_drop)
                {
                    *connmgr_sensor_to_drop = 0;
                    pthread_mutex_unlock(connmgr_drop_conn_mutex);

                    asprintf(&send_buf, "%ld Connection Manager: signalled to drop connection to %" PRIu16, time(NULL), client->sensor);
                    write_to_pipe(ipc_pipe_mutex, pfds, send_buf);
                }
                else
                    pthread_mutex_unlock(connmgr_drop_conn_mutex);

                #if (DEBUG_LVL > 1)
                    printf("Peer closed connection or timed out - %d of %d\n", i, num_conn);
                    fflush(stdout);
                #endif

                if (client != NULL)
                {
                    asprintf(&send_buf, "%ld Connection Manager: connection to %" PRIu16 " closed", time(NULL), client->sensor);
                    write_to_pipe(ipc_pipe_mutex, pfds, send_buf);

                    dpl_remove_node(socket_list, node, true); /* Close and remove connection from dplist if element exists */ 
                }
                /* Delete element poll_fds[i] using skip deleted element algorithm */
                for (int id1 = 0, id2 = 0; id1 < num_conn; id1++, id2++)
                {
                    id2 += (id2 == i) ? 1 : 0;     
                    poll_fds[id1] = poll_fds[id2];
                }
                poll_fds = realloc(poll_fds, sizeof(struct pollfd) * num_conn);
                num_conn--; /* Decrement number of sockets. Decremented after realloc because array contains server socket at index 0, hence new_connections+1 elements for new array */
                i--;            /* Ensures when an element is removed from poll_fds, incrementation won't skip over the following element */ 

                #if (DEBUG_LVL > 0)
                    printf("\n##### Printing Socket DPLIST Content Summary #####\n");
                    dpl_print_heap(socket_list);
                #endif
            }
            else
                pthread_mutex_unlock(connmgr_drop_conn_mutex);
        }
    }

    if (poll_ret == -1)
    {
        *retval = CONNMGR_SERVER_POLL_ERROR;

        asprintf(&send_buf, "%ld Connection Manager: error polling sockets", time(NULL));
        write_to_pipe(ipc_pipe_mutex, pfds, send_buf);
    }

    #if (DEBUG_LVL > 0)
        printf("Connection Manager: total %d messages processed during session\n", sbuffer_insertions);
        fflush(stdout);
    #endif
}

void connmgr_free()
{
    char *send_buf;

    if(server != NULL && tcp_close(&server) != TCP_NO_ERROR) 
    {
        *retval = CONNMGR_SERVER_CLOSE_ERROR; /* close master socket if any */
        
        asprintf(&send_buf, "%ld Connection Manager: failed to stop", time(NULL));
        write_to_pipe(ipc_pipe_mutex, pfds, send_buf);
    } else
    {
        asprintf(&send_buf, "%ld Connection Manager: stopped successfully", time(NULL));        
        write_to_pipe(ipc_pipe_mutex, pfds, send_buf);
    }
    if(poll_fds != NULL) free(poll_fds); /* Clean up allocated socket descriptor array if any */
    if(socket_list != NULL) dpl_free(&socket_list, true); /* Clean up allocated tcpsock_dpl_el dplist if any */

    pthread_rwlock_wrlock(sbuffer_open_rwlock); /* lock mutex to sbuffer_open shared data */
    #if (DEBUG_LVL > 0)
    printf("Server is shutting down. Closing shared buffer\n");
    fflush(stdout);
    #endif
    *sbuffer_open = 0; /* Indicate reader threads the end of buffer */
    pthread_rwlock_unlock(sbuffer_open_rwlock);
}

static void *socket_copy(void *element)
{
    struct tcpsock_dpl_el *dummy = (struct tcpsock_dpl_el *) malloc(sizeof(struct tcpsock_dpl_el));
    dummy->sock_ptr = ((struct tcpsock_dpl_el *)element)->sock_ptr;
    dummy->sd = ((struct tcpsock_dpl_el *)element)->sd;
    dummy->last_active = ((struct tcpsock_dpl_el *)element)->last_active;
    dummy->sensor = ((struct tcpsock_dpl_el *)element)->sensor;
    return dummy;
}

static void socket_free(void **element)
{
    tcp_close(&(((struct tcpsock_dpl_el *)*element)->sock_ptr)); /* Close connection to that socket */
    free((struct tcpsock_dpl_el *)*element);                     /* Free allocated space of the dplist element */
    *element = NULL;                                             /* Set pointer to a dplist element pointing to NULL */
}

static int socket_compare(void *x, void *y)
{
    return ((((struct tcpsock_dpl_el *)x)->sd == ((struct tcpsock_dpl_el *)y)->sd) ? 0 : ((((struct tcpsock_dpl_el *)x)->sd > ((struct tcpsock_dpl_el *)y)->sd) ? -1 : 1));
}