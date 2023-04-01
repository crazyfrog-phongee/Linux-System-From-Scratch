#define _GNU_SOURCE
#define BUILDING_GATEWAY
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <pthread.h>
#include <stdlib.h>
#include <inttypes.h>

#include "datamgr.h"

/**
 * Custom Types: The data manager organizes all sensor nodes in a pointer list data structure.
 **/
typedef struct
{
    uint16_t room;
    sensor_data_t sensor;
    sensor_value_t msrmnts[RUN_AVG_LENGTH];
    unsigned char num_msrmnts;
} node_t;

/**
 * Global Variables
 **/
static int *pfds;
static pthread_mutex_t *ipc_pipe_mutex;

static int *sbuffer_open;
static pthread_rwlock_t *sbuffer_open_rwlock;

static sensor_id_t *connmgr_sensor_to_drop;
static pthread_mutex_t *connmgr_drop_conn_mutex;

static int *storagemgr_failed_flag;
static pthread_rwlock_t *storagemgr_failed_rwlock;

static dplist_t *dplist;
static int readby;
static int *retval;
static int num_parsed_data = 0;

/**
 * Private Prototypes: Callback funcs
 **/

/**
 * Makes a deep copy of the source element into the heap
 */
static void *sensor_copy(void *element);

/**
 * Calls to dplist API, frees used memory
 */
static void sensor_free(void **element);

/**
 * If Room ID is specified uses it to sort in descending order, otherwise uses Sensor ID
 */
static int sensor_compare(void *x, void *y);

/**
 * Functions
 **/
void datamgr_init(datamgr_init_arg_t *arg)
{
    pfds = arg->ipc_pipe_fd;
    ipc_pipe_mutex = arg->pipe_mutex;
    sbuffer_open = arg->sbuffer_flag;
    sbuffer_open_rwlock = arg->sbuffer_rwlock;
    connmgr_sensor_to_drop = arg->connmgr_sensor_to_drop;
    connmgr_drop_conn_mutex = arg->connmgr_drop_conn_mutex;
    storagemgr_failed_flag = arg->storagemgr_fail_flag;
    storagemgr_failed_rwlock = arg->storagemgr_failed_rwlock;
    readby = arg->id;
    retval = arg->status;
}

void datamgr_parse_sensor_files(FILE *fp_sensor_map, sbuffer_t **buffer)
{
    ERROR_HANDLER(fp_sensor_map == NULL || *buffer == NULL, "Error openning streams - NULL\n");
    dplist = dpl_create(&sensor_copy, &sensor_free, &sensor_compare);
    node_t dummy;
    char *send_buf;

    for (int i = 0; i < RUN_AVG_LENGTH; i++)
        dummy.msrmnts[i] = 0;
    dummy.sensor.ts = (sensor_ts_t)0;
    dummy.sensor.value = (sensor_value_t)0;
    dummy.num_msrmnts = (unsigned char)0;

    /** Exer 3 (Lab 5)
     * Reading room_sensor.map containing all room-sensor node mappings
     * The data manager organizes all sensor nodes in a pointer list data structure.
     */
    char line[11];
    while (fgets(line, sizeof(line), fp_sensor_map) != NULL)
    {
        sscanf(line, "%hu%hu", &(dummy.room), &(dummy.sensor.id)); /* %hu: short integer */
        dpl_insert_sorted(dplist, &dummy, true);                   /* Inserts nodes in the list according to sorting criteria, copies dummy to heap */

    #if (DEBUG_LVL > 0)
            printf("\n##### Printing Sensors|Rooms DPLIST Content Summary #####\n");
            dpl_print_heap(dplist); // Prints dplist occupied heap data changes
    #endif
    }
    if (ferror(fp_sensor_map))
    {
        fprintf(stderr, "Error while reading text file\n");
        fflush(stderr);
        *retval = DATAMGR_FILE_PARSE_ERROR;

        asprintf(&send_buf, "%ld Data Manager: failed to read room_sensor.map", time(NULL));
        write_to_pipe(ipc_pipe_mutex, pfds, send_buf);

        return;
    }
    else
    {
        asprintf(&send_buf, "%ld Data Manager: started and parsed room_sensor.map successfully", time(NULL));
        write_to_pipe(ipc_pipe_mutex, pfds, send_buf);
    }

    void *node = NULL;
    dummy.room = 0;                    /* Setting dummy's room ID to NULL to compare elements by sensor ID's instead */
    int sbuffer_ret = SBUFFER_SUCCESS; /* Storing the return value of reading sbuffer */

    pthread_rwlock_rdlock(storagemgr_failed_rwlock);
    pthread_rwlock_rdlock(sbuffer_open_rwlock);
    while ((sbuffer_ret != SBUFFER_NO_DATA || *sbuffer_open) && *storagemgr_failed_flag == 0) /* Use condition variable from writer thread to know when to terminate the readers */
    {
        pthread_rwlock_unlock(sbuffer_open_rwlock);
        pthread_rwlock_unlock(storagemgr_failed_rwlock);

        sbuffer_ret = sbuffer_pop(*buffer, &node, &(dummy.sensor), readby);
        if (sbuffer_ret != SBUFFER_SUCCESS)
            pthread_yield();
        else if (sbuffer_ret == SBUFFER_SUCCESS)
        {
            num_parsed_data++;
            #if (DEBUG_LVL > 1)
                        printf("Data Manager: sbuffer data available %" PRIu16 " %g %ld\n", dummy.sensor.id, dummy.sensor.value, dummy.sensor.ts);
                        fflush(stdout);
            #endif

            dplist_node_t *node_of_element = dpl_get_reference_of_element(dplist, &dummy); /* Use compare() func (compare sensor ID) to find a list item whose data matches */
            if (node_of_element == NULL)                                                   /* If no such list item is found, go back to beginning of while-loop */
            {
                fprintf(stderr, "%" PRIu16 " is not a valid sensor ID\n", dummy.sensor.id);
                fflush(stderr);

                asprintf(&send_buf, "%ld Data Manager: sensor %" PRIu16 " does not exist", time(NULL), dummy.sensor.id);
                write_to_pipe(ipc_pipe_mutex, pfds, send_buf);

                pthread_mutex_lock(connmgr_drop_conn_mutex);
                *connmgr_sensor_to_drop = dummy.sensor.id; /* Signal Connmgr to terminate connection to this socket */
                pthread_mutex_unlock(connmgr_drop_conn_mutex);

                /* Lock and go back while() */
                pthread_rwlock_rdlock(storagemgr_failed_rwlock);
                pthread_rwlock_rdlock(sbuffer_open_rwlock);
                continue;
            }

            node_t *element_of_node = dpl_get_element_of_reference(node_of_element);
            element_of_node->sensor.ts = dummy.sensor.ts; /* Update the "Last Modified" stamp */
            sensor_value_t sum = 0;                       /* Accumulate the measurements and find average reading */
            for (int i = RUN_AVG_LENGTH - 1; i >= 0; i--) /* Shift array elements (circular buffer style) */
            {
                if (i != 0)
                    element_of_node->msrmnts[i] = element_of_node->msrmnts[i - 1];
                if (i == 0)
                    element_of_node->msrmnts[i] = dummy.sensor.value;
                sum += element_of_node->msrmnts[i];
            }
            if (element_of_node->num_msrmnts < RUN_AVG_LENGTH - 1) /* If less than running average of measurements were taken, average is 0 */
            {
                (element_of_node->num_msrmnts)++;
                element_of_node->sensor.value = 0;

                /* Lock and go back while() */
                pthread_rwlock_rdlock(storagemgr_failed_rwlock);
                pthread_rwlock_rdlock(sbuffer_open_rwlock);
                continue; /* If total measurement number is less than RUN_AVG_LENGTH, skip accumulation */
            }
            element_of_node->sensor.value = sum / RUN_AVG_LENGTH;
            if (element_of_node->sensor.value < SET_MIN_TEMP)
            {
                fprintf(stderr, "Sensor %" PRIu16 " in Room %" PRIu16 " detected temperature below %g *C limit of %g *C at %ld\n", element_of_node->sensor.id, element_of_node->room, (double)SET_MIN_TEMP, element_of_node->sensor.value, element_of_node->sensor.ts);
                fflush(stderr);

                asprintf(&send_buf, "%ld Data Manager: sensor %" PRIu16 " in room %" PRIu16 " - too cold %g below %g", time(NULL), element_of_node->sensor.id, element_of_node->room, element_of_node->sensor.value, (double)SET_MIN_TEMP);

                write_to_pipe(ipc_pipe_mutex, pfds, send_buf);
            }
            else if (element_of_node->sensor.value > SET_MAX_TEMP)
            {
                fprintf(stderr, "Sensor %" PRIu16 " in Room %" PRIu16 " detected temperature above %g *C limit of %g *C at %ld\n", element_of_node->sensor.id, element_of_node->room, (double)SET_MAX_TEMP, element_of_node->sensor.value, element_of_node->sensor.ts);
                fflush(stderr);

                asprintf(&send_buf, "%ld Data Manager: sensor %" PRIu16 " in room %" PRIu16 " - too hot %g above %g", time(NULL), element_of_node->sensor.id, element_of_node->room, element_of_node->sensor.value, (double)SET_MAX_TEMP);

                write_to_pipe(ipc_pipe_mutex, pfds, send_buf);
            }
        }

        /* Lock and go back while() */
        pthread_rwlock_rdlock(storagemgr_failed_rwlock);
        pthread_rwlock_rdlock(sbuffer_open_rwlock); /* Lock mutex to sbuffer_open shared data to prevent race condition during checking end of shared buffer */
    }

    pthread_rwlock_unlock(sbuffer_open_rwlock);

    if (*storagemgr_failed_flag)
    {
        pthread_rwlock_unlock(storagemgr_failed_rwlock);

        *retval = DATAMGR_INTERRUPTED_BY_STORAGEMGR;

        asprintf(&send_buf, "%ld Data Manager: signalled to terminate by Storage Manager", time(NULL));
        write_to_pipe(ipc_pipe_mutex, pfds, send_buf);

        datamgr_free();

        fclose(fp_sensor_map);

        #if (DEBUG_LVL > 0)
                printf("Data Manager is stopped\n");
                fflush(stdout);
        #endif

        pthread_exit(retval);
    }
    else
        pthread_rwlock_unlock(storagemgr_failed_rwlock);
}

void datamgr_free()
{
    assert(dplist != NULL);
    dpl_free(&dplist, true);

    char *send_buf;

    asprintf(&send_buf, "%ld Data Manager: successfully cleaned up", time(NULL));

    write_to_pipe(ipc_pipe_mutex, pfds, send_buf);

#if (DEBUG_LVL > 0)
    printf("\nData Manager: parsed data %d times\n", num_parsed_data);
    fflush(stdout);
#endif
}

static void *sensor_copy(void *src_element)
{
    node_t *node = (node_t *)malloc(sizeof(node_t));
    *node = *((node_t *)src_element);
    return node;
}

static void sensor_free(void **element)
{
    free((node_t *)*element);
}

static int sensor_compare(void *x, void *y)
{
    if (((node_t *)x)->room == 0 || ((node_t *)y)->room == 0)
    {
        return (((node_t *)x)->sensor.id > ((node_t *)y)->sensor.id) ? 1 : (((node_t *)x)->sensor.id == ((node_t *)y)->sensor.id) ? 0
                                                                                                                                  : -1;
    }
    return ((((node_t *)x)->room > ((node_t *)y)->room) ? 1 : ((((node_t *)x)->room == ((node_t *)y)->room) ? 0 : -1));
}

void datamgr_print_summary()
{
    for (dplist_node_t *dummy = dpl_get_first_reference(dplist); dummy != NULL; dummy = dpl_get_next_reference(dplist, dummy))
    {
        node_t *node = dpl_get_element_of_reference(dummy);
        printf("\n********Room %" PRIu16 " - Sensor %" PRIu16 "********\nCurrent average reading = %g *C\nLast modified: %ld\nLast measurements (DESC):\n", node->room, node->sensor.id, node->sensor.value, node->sensor.ts);
        fflush(stdout);
        for (int i = 0; i < RUN_AVG_LENGTH; i++)
        {
            printf("%d) %g *C\n", i + 1, node->msrmnts[i]);
            fflush(stdout);
        }
    }
}
