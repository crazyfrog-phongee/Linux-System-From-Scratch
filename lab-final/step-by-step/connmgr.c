#define _GNU_SOURCE
#define BUILDING_GATEWAY

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <inttypes.h>
#include <sys/types.h>

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
/**
 * Callback functions for socket_list
*/
static void *socket_copy(void *element);
static void socket_free(void **element);
static int socket_compare(void *x, void *y);

