#define _GNU_SOURCE
#define BUILDING_GATEWAY
#include <stdio.h>
#include <inttypes.h>

#include "sensor_db.h"

/**
 * Global variables definition
*/
static int *pfds;
static pthread_mutex_t *ipc_pipe_mutex;

static int *sbuffer_open;
static pthread_rwlock_t *sbuffer_open_rwlock;

static int readby;
static int *retval;
static int num_parsed_data = 0;

/**
 * Functions
*/
void storagemgr_init(storagemgr_init_arg_t *arg)
{
    pfds = arg->ipc_pipe_fd;
    ipc_pipe_mutex = arg->pipe_mutex;
    sbuffer_open = arg->sbuffer_flag;
    sbuffer_open_rwlock = arg->sbuffer_rwlock;
    readby = arg->id;
    retval = arg->status;
}

DBCONN *init_connection(char clear_up_flag)
{
    sqlite3 *db; /* The sqlite3 structure defines a database handle */
    char *send_buf;
    char *errmsg;
    int rc = sqlite3_open(TO_STRING(DB_NAME), &db); /* Sensor.db SQLite Database is represented by "db" database hanlde */

    if (rc != SQLITE_OK)
    {
        fprintf(stderr, "Cannot open database: %s\n", sqlite3_errmsg(db)); /* The sqlite3_errmsg function returns a description of the error */
        fflush(stderr);
        
        asprintf(&send_buf, "%ld Storage Manager: Unable to connect to SQL server", time(NULL));
        
        sqlite3_close(db); /* Release the resources associated with the database connection handle */
    } else
    {
        asprintf(&send_buf, "%ld Storage Manager: Connected to SQL server", time(NULL));
    }
    write_to_pipe(ipc_pipe_mutex, pfds, send_buf);

    if (db != NULL)
    {
        char *sql;

        /* Create a SensorData table */
        if(clear_up_flag == 1)
            sql =   "DROP TABLE IF EXISTS "TO_STRING(TABLE_NAME)";"
                    "CREATE TABLE "TO_STRING(TABLE_NAME)"(id INTEGER PRIMARY KEY ASC AUTOINCREMENT, sensor_id INTEGER, sensor_value DECIMAL(4,2), timestamp TIMESTAMP);";
        else
            sql =   "CREATE TABLE IF NOT EXISTS "TO_STRING(TABLE_NAME)"(id INTEGER PRIMARY KEY ASC AUTOINCREMENT, sensor_id INTEGER, sensor_value DECIMAL(4,2), timestamp TIMESTAMP);";

        /* Run multiple statements of SQL */
        rc = sqlite3_exec(db, sql, NULL, NULL, &errmsg);

        if(rc != SQLITE_OK) /* If query failed, print to stderr and pass DB error message to child process */
        {
            fprintf(stderr, "SQL error: %s\n", errmsg); /* If an error occurs then the last parameter("errmsg") points to the allocated error message. */
            fflush(stderr);

            asprintf(&send_buf, "%ld Storage Manager: %s", time(NULL), errmsg);
            
            sqlite3_free(errmsg); /* The allocated message string must be freed*/
            db = NULL;
        } else
        {
            asprintf(&send_buf, "%ld Storage Manager: New table "TO_STRING(TABLE_NAME)" created", time(NULL));
        }
        
        write_to_pipe(ipc_pipe_mutex, pfds, send_buf);
    }

    return db;
}

void disconnect(DBCONN *conn)
{
    char *send_buf;
    int rc = sqlite3_close(conn); /* Close the database connection */

    if(rc != SQLITE_OK)
    {
        fprintf(stderr, "Failed to close. Database still busy\n");
        fflush(stderr);

        asprintf(&send_buf, "%ld Storage Manager: Unable to disconnect from SQL server - server busy", time(NULL));
    } else
    {
        asprintf(&send_buf, "%ld Storage Manager: Disconnected from SQL server", time(NULL));
    }

    write_to_pipe(ipc_pipe_mutex, pfds, send_buf);

    #if (DEBUG_LVL > 0)
    printf("\nStorage Manager: parsed data %d times\n", num_parsed_data);
    fflush(stdout);
    #endif
}

void storagemgr_parse_sensor_data(DBCONN *conn, sbuffer_t **buffer)
{
    void *node;
    sensor_data_t data;
    int sbuffer_ret = SBUFFER_SUCCESS; /* Storing the return value of reading sbuffer */

    pthread_rwlock_rdlock(sbuffer_open_rwlock); /* Lock mutex to sbuffer_open shared data to prevent race condition during checking end of shared buffer */
    while (sbuffer_ret != SBUFFER_NO_DATA || *sbuffer_open) /* Use condition variable from writer thread to know when to terminate the readers */ 
    {
        pthread_rwlock_unlock(sbuffer_open_rwlock);

        sbuffer_ret = sbuffer_pop(*buffer, &node, &data, readby); /* Non-blocking, implementation takes care of thread-safety */
        if(sbuffer_ret != SBUFFER_SUCCESS) pthread_yield();
        else if(sbuffer_ret == SBUFFER_SUCCESS) 
        {
            num_parsed_data++;

            #if (DEBUG_LVL > 1)
            printf("Storage Manager: sbuffer data available %"PRIu16" %g %ld\n", data.id, data.value, data.ts);
            fflush(stdout);
            #endif

            insert_sensor(conn, data.id, data.value, data.ts);
        }

        pthread_rwlock_rdlock(sbuffer_open_rwlock); /* Lock mutex to sbuffer_open shared data to prevent race condition during checking end of shared buffer */
    }
    pthread_rwlock_unlock(sbuffer_open_rwlock);
}

int insert_sensor(DBCONN *conn, sensor_id_t id, sensor_value_t value, sensor_ts_t ts)
{
    char *send_buf;
    char *errmsg;
    char *sql;
    int rc;

    asprintf(&sql, "INSERT INTO "TO_STRING(TABLE_NAME)"(sensor_id, sensor_value, timestamp) VALUES(%hu, %e, %ld);", id, value, ts);
    rc = sqlite3_exec(conn, sql, NULL, NULL, &errmsg);
    if(rc != SQLITE_OK)
    {
        fprintf(stderr, "SQL error: %s\n", errmsg);
        fflush(stderr);

        asprintf(&send_buf, "%ld Storage Manager: Data insertion failed::%s", ts, errmsg);
        
        sqlite3_free(errmsg);
    } else
        asprintf(&send_buf, "%ld Storage Manager: Inserted new reading in %s", ts, TO_STRING(TABLE_NAME));

    write_to_pipe(ipc_pipe_mutex, pfds, send_buf);
    free(sql); /* Release the allocated storage */
    
    return rc;
}

int find_sensor_all(DBCONN * conn, callback_t f)
{
    char *errmsg;
    char *send_buf;
    char *sql = "SELECT * FROM "TO_STRING(TABLE_NAME)" ORDER BY id ASC;";
    int rc = sqlite3_exec(conn, sql, f, NULL, &errmsg);
    
    if(rc != SQLITE_OK)
    {
        fprintf(stderr, "SQL error: %s\n", errmsg);
        fflush(stderr);

        asprintf(&send_buf, "%ld Storage Manager: All sensor query failed::%s", time(NULL), errmsg);
        
        sqlite3_free(errmsg);
    } else
    {
        asprintf(&send_buf, "%ld Storage Manager: All sensor query complete", time(NULL));
    }
    
    write_to_pipe(ipc_pipe_mutex, pfds, send_buf);

    return rc;
}

int find_sensor_exceed_value(DBCONN * conn, sensor_value_t value, callback_t f)
{
    char * errmsg;
    char * send_buf;
    char * sql;
    
    asprintf(&sql, "SELECT * FROM "TO_STRING(TABLE_NAME)" WHERE sensor_value > %g ORDER BY id ASC;", value);
    
    int rc = sqlite3_exec(conn, sql, f, NULL, &errmsg);
    
    if(rc != SQLITE_OK)
    {
        fprintf(stderr, "SQL error: %s\n", errmsg);
        fflush(stderr);

        asprintf(&send_buf, "%ld Storage Manager: Sensor query GT value failed::%s", time(NULL), errmsg);

        sqlite3_free(errmsg);
    } else 
    {
        asprintf(&send_buf, "%ld Storage Manager: Sensor query GT value complete", time(NULL));
    }
    
    write_to_pipe(ipc_pipe_mutex, pfds, send_buf);
    free(sql);

    return rc;
}

int find_sensor_by_timestamp(DBCONN * conn, sensor_ts_t ts, callback_t f)
{
    char * errmsg;
    char * send_buf;
    char * sql;

    asprintf(&sql, "SELECT * FROM "TO_STRING(TABLE_NAME)" WHERE timestamp = %ld ORDER BY id ASC;", ts);
    
    int rc = sqlite3_exec(conn, sql, f, NULL, &errmsg);
    
    if(rc != SQLITE_OK)
    {
        fprintf(stderr, "SQL error: %s\n", errmsg);
        fflush(stderr);

        asprintf(&send_buf, "%ld Storage Manager: Sensor query by timestamp failed::%s", time(NULL), errmsg);
        
        sqlite3_free(errmsg);
    } else
    {
        asprintf(&send_buf, "%ld Storage Manager: Sensor query by timestamp complete", time(NULL));    
    }
    
    write_to_pipe(ipc_pipe_mutex, pfds, send_buf);
    free(sql);

    return rc;
}

int find_sensor_after_timestamp(DBCONN * conn, sensor_ts_t ts, callback_t f)
{
    char * errmsg;
    char * send_buf;
    char * sql;

    asprintf(&sql, "SELECT * FROM "TO_STRING(TABLE_NAME)" WHERE timestamp > %ld ORDER BY id ASC;", ts);
    
    int rc = sqlite3_exec(conn, sql, f, NULL, &errmsg);
    
    if(rc != SQLITE_OK)
    {
        fprintf(stderr, "SQL error: %s\n", errmsg);
        fflush(stderr);

        asprintf(&send_buf, "%ld Storage Manager: Sensor query GT timestamp failed::%s", time(NULL), errmsg);

        sqlite3_free(errmsg);
    } else
    {
        asprintf(&send_buf, "%ld Storage Manager: Sensor query GT timestamp complete", time(NULL));
    }

    write_to_pipe(ipc_pipe_mutex, pfds, send_buf);
    free(sql);

    return rc;
}