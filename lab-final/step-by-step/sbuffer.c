
#define _GNU_SOURCE
#define BUILDING_GATEWAY
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <inttypes.h>
#include <pthread.h>
#include "sbuffer.h"
#include "config.h"

/**
 * Custom Types
 **/
struct sbuffer_data
{
    sensor_data_t data;
    int read_by[READER_THREADS]; /* labeling to identify when the message was read by both threads and can be removed */
};

typedef struct sbuffer_node
{
    struct sbuffer_node *next;
    sbuffer_data_t element;
} sbuffer_node_t;

struct sbuffer
{
    sbuffer_node_t *head;
    sbuffer_node_t *tail;
    pthread_rwlock_t *lock;
};

/**
 * Private Prototypes
 **/

static void _sbuffer_print_content(sbuffer_t *buffer);
static int _is_read_by_all(sbuffer_node_t *node);

/**
 * Public Prototypes
 */

int sbuffer_init(sbuffer_t **buffer)
{
    *buffer = malloc(sizeof(sbuffer_t));

    if (*buffer == NULL)
        return SBUFFER_FAILURE;
    (*buffer)->head = NULL;
    (*buffer)->tail = NULL;
    (*buffer)->lock = malloc(sizeof(pthread_rwlock_t)); /* malloc rwlock so it can be referenced from external object during cleanup */
    pthread_rwlock_init((*buffer)->lock, NULL);

    return SBUFFER_SUCCESS;
}

int sbuffer_free(sbuffer_t **buffer)
{
    pthread_rwlock_t *lock;

    pthread_rwlock_wrlock((*buffer)->lock); /* Apply write lock to avoid interruption during clean up */
    if ((buffer == NULL) || (*buffer == NULL))
    {
        pthread_rwlock_unlock((*buffer)->lock);

        #if (DEBUG_LVL > 0)
                printf("Buffer on failure\n");
                fflush(stdout);
        #endif

        return SBUFFER_FAILURE;
    }

    while (sbuffer_remove(*buffer, NULL) != SBUFFER_NO_DATA)
        ;

    lock = (*buffer)->lock; /* copy pointer to rwlock */
    free(*buffer);          /* free and NULL the buffer */
    *buffer = NULL;
    pthread_rwlock_unlock(lock);  /* release the rwlock to the buffer */
    pthread_rwlock_destroy(lock); /* destroy the rwlock to the buffer */
    free(lock);                   /* free the memory allocated to rwlock */
    printf("Buffer on success\n");
    return SBUFFER_SUCCESS;
}

int sbuffer_remove(sbuffer_t *buffer, sensor_data_t *data)
{
    if (buffer == NULL)
        return SBUFFER_FAILURE;
    if (buffer->head == NULL)
        return SBUFFER_NO_DATA;

    sbuffer_node_t *dummy = buffer->head;

    if (buffer->head == buffer->tail)
        buffer->head = buffer->tail = NULL; /* Buffer has only one node */
    else
        buffer->head = buffer->head->next; /* Buffer has many nodes */

    dummy->next = NULL;

#if (DEBUG_LVL > 1)
    printf("After removing, node pointing to %p points to next at %p\n", dummy, dummy->next);
    fflush(stdout);
#endif

    free(dummy); /* Release node from memory */

    return SBUFFER_SUCCESS;
}

int sbuffer_pop(sbuffer_t *buffer, void **node_ptr, sensor_data_t *data, int readby)
{
    if (buffer == NULL)
        return SBUFFER_FAILURE;

    sbuffer_node_t **node = (sbuffer_node_t **)node_ptr;
    /** Purpuse: Tìm node tiếp theo để đọc dữ liệu
     * Luồng thuật toán: case 3 ---> liên tục xử lý ở case 4 ----> case 1 (đọc hết các node)
     * Case 1: Empty buffer hoặc thread chỉ định đã đọc hết các phần tử trong buffer
     * Case 2: Node head hiện tại đã được đọc bởi 2 thread ---> check and remove (hỗ trợ nếu trước đó case 3 được xử lý)
     * Case 3: Node head hiện tại chưa được đọc bởi thread chỉ định -> gán vào *node để get ra thread xử lý (ứng dụng cho node đầu danh sách chưa được đọc bởi thread chỉ định)
     * Case 4: Đối số truyền vào *node_ptr != NULL, đã được đọc bởi thread chỉ định -> gán vào *node giá trị node kế tiếp (*node)->next để get ra thread xử lý (ứng dụng cho các node ở giữa danh sách chưa được đọc bởi thread chỉ định)
     */
    pthread_rwlock_wrlock(buffer->lock);
    if (buffer->head == NULL || (buffer->tail != NULL && buffer->tail->element.read_by[readby] == 1)) /* If sbuffer is empty or thread already at last element and read it before */
    {
        if (buffer->head == NULL)
            *node = NULL;
        else
            *node = buffer->tail;
        pthread_rwlock_unlock(buffer->lock);

        return SBUFFER_NO_DATA;
    }
    else if (buffer->head != NULL && *node == buffer->head && _is_read_by_all(*node)) /* Before poping data, check to remove current node if it was read by all */
    {
        #if (DEBUG_LVL > 1)
                printf("Thread %d removed node %p\n", readby, *node);
                fflush(stdout);
        #endif

        *node = (*node)->next;        /* set node pointing to next element, allowed to be NULL and will trigger 3rd condition on next iteration */
        sbuffer_remove(buffer, NULL); /* remove the node now that we know no one will definitely use it, applies to only 1 element in buffer, will put head to NULL and force other threads into 2nd condition */
        pthread_rwlock_unlock(buffer->lock);

        return SBUFFER_NODE_ALREADY_CONSUMED;
    }
    else if (buffer->head != NULL && buffer->head->element.read_by[readby] == 0)
        *node = buffer->head; /* Phần tử head chưa được đọc, gán node hiện tại = phần tử head */
    else if (*node != NULL && (*node)->element.read_by[readby] == 1 && (*node)->next != NULL)
        *node = (*node)->next; /* else we treat the node that we point to already */

    #if (DEBUG_LVL > 1)
        printf("Next node now for thread %d - %p vs head %p and tail %p\n", readby, *node, buffer->head, buffer->tail);
        printf("Thread %d marked node %p read\n", readby, *node);
        fflush(stdout);
    #endif

    /* Lấy dữ liệu từ node hiện tại và gán vào con trỏ data để thread chỉ định xử lý */
    *data = (*node)->element.data;
    (*node)->element.read_by[readby] = 1;

    if (_is_read_by_all(*node)) /* If node was read by all threads, remove it from shared buffer */
    {
        #if (DEBUG_LVL > 1)
                printf("Thread %d removed node %p\n", readby, *node);
                fflush(stdout);
        #endif

        sbuffer_remove(buffer, NULL);
        *node = NULL; /* Set node pointing to NULL */
    }
    else if ((*node)->next != NULL)
        *node = (*node)->next;

    #if (DEBUG_LVL > 1)
        _sbuffer_print_content(buffer);
    #endif

    pthread_rwlock_unlock(buffer->lock);

    return SBUFFER_SUCCESS;
}

int sbuffer_insert(sbuffer_t *buffer, sensor_data_t *data)
{
    if (buffer == NULL)
        return SBUFFER_FAILURE;

    sbuffer_node_t *dummy = malloc(sizeof(sbuffer_node_t));
    if (dummy == NULL)
        return SBUFFER_FAILURE;

    dummy->element.data = *data;
    dummy->next = NULL;

    /**
     * Setup the default: the data wasn't read by any threads
     * READER_THREADS = 2 respectively data_manager and storage_manager threads
     */
    for (int i = 0; i < READER_THREADS; i++)
        dummy->element.read_by[i] = 0;

    pthread_rwlock_wrlock(buffer->lock);
    if (buffer == NULL)
    {
        pthread_rwlock_unlock(buffer->lock);
        free(dummy);

        return SBUFFER_FAILURE;
    }

    if (buffer->tail == NULL)
        buffer->head = buffer->tail = dummy; /* buffer empty (buffer->head should also be NULL) */
    else                                     /* Not empty buffer, insert at the end of the list */
    {
        buffer->tail->next = dummy; /* Gán next của node tail trỏ đến dummy */
        buffer->tail = dummy;       /* Cập nhật lại giá trị của node tail */
    }

    #if (DEBUG_LVL > 1)
        printf("\nNew node at %p and next node %p- %" PRIu16 " %g %ld datamgr (%d) storagemgr (%d)\n", buffer->tail, buffer->tail->next, buffer->tail->element.data.id, buffer->tail->element.data.value, buffer->tail->element.data.ts, buffer->tail->element.read_by[0], buffer->tail->element.read_by[1]);
        fflush(stdout);
        _sbuffer_print_content(buffer);
    #endif

    pthread_rwlock_unlock(buffer->lock);

    return SBUFFER_SUCCESS;
}

void sbuffer_print_content(sbuffer_t *buffer)
{
    pthread_rwlock_rdlock(buffer->lock);
    _sbuffer_print_content(buffer);
    pthread_rwlock_unlock(buffer->lock);
}

static void _sbuffer_print_content(sbuffer_t *buffer)
{
    printf("\n##### Printing SBUFFER Content Summary #####\n");
    sbuffer_node_t *dummy = buffer->head;
    for (int i = 0; dummy != NULL; dummy = dummy->next, i++)
    {
        printf("%d: %p | %p | %" PRIu16 " - %g - %ld - [%d, %d]\n", i, dummy, dummy->next, dummy->element.data.id, dummy->element.data.value, dummy->element.data.ts, dummy->element.read_by[0], dummy->element.read_by[1]);
    }
    printf("\n");
    fflush(stdout);
}

static int _is_read_by_all(sbuffer_node_t *node)
{
    for (int i = 0; i < READER_THREADS; i++) /* Check if node has been utilised by all threads */
    {
        if (node->element.read_by[i] == 0)
            return 0; /* If the node was not read by any of the threads, indicate */
    }
    return 1;
}

void write_to_pipe(pthread_mutex_t *pipe_mutex, int *pfds, char *send_buf)
{
    pthread_mutex_lock(pipe_mutex);
    write(*(pfds + 1), send_buf, strlen(send_buf) + 1);
    pthread_mutex_unlock(pipe_mutex);
    free(send_buf);
}