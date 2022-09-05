# ifndef __MPC_OMP_TASK_SCHEDULE_H__
#  define __MPC_OMP_TASK_SCHEDULE_H__

/**
 * This file describe various data structures to export/import an
 * entire application « task scheduling »
 *
 * -------------------------------------------
 *                  VERSION 1
 * -------------------------------------------
 *  This version assumes that
 *      - all the tasks are created by the same threads and in the same order
 *      - the scheduling files entries are sorted by their order of execution on the associated thread
 */

# define MPC_OMP_TASK_SCHEDULE_MAGIC        0x7461736b
# define MPC_OMP_TASK_SCHEDULE_VERSION      1
# define MPC_OMP_TASK_SCHEDULE_BUFF_SIZE    8192

/* a scheduling file header */
typedef struct
{
    /* magic */
    int magic;

    /* file version */
    int version;

    /* 'omp_num_thread' */
    int thread_num;

} mpc_omp_task_scheduling_file_header_t;

/* scheduling events */
typedef enum
{
    MPC_OMP_TASK_SCHEDULING_EVENT_CREATE,
    MPC_OMP_TASK_SCHEDULING_EVENT_SCHEDULE,
    MPC_OMP_TASK_SCHEDULING_EVENT_MAX
} mpc_omp_task_scheduling_event_t;

/* a scheduling file entry */
typedef struct
{
    /* event (0 = create, 1 = schedule) */
    unsigned event : 1;

    /* the task ID */
    unsigned uid : 31;

} mpc_omp_task_scheduling_file_entry_t;

/* a scheduling file */
typedef struct
{
    /* the file header */
    mpc_omp_task_scheduling_file_header_t header;

} mpc_omp_task_scheduling_file_t;

/* a scheduling file writer */
typedef struct
{
    /* the file */
    mpc_omp_task_scheduling_file_t file;

    /* the file descriptor */
    int fd;

    /* the read/write buffer */
    char buffer[MPC_OMP_TASK_SCHEDULE_BUFF_SIZE];

    /* the cursor position in the buffer */
    int cursor;

} mpc_omp_task_scheduling_file_writer_t;

/* a scheduling file reader */
typedef mpc_omp_task_scheduling_file_reader_t mpc_omp_task_scheduling_file_writer_t;

/** a task scheduling to be replayed */
typedef struct
{
    /* the scheduling files directory */
    char * directory;

}               mpc_omp_task_scheduling_t;

/***************** FILE FUNCTIONS ***********************/
void mpc_omp_task_scheduling_file_new(mpc_omp_task_scheduling_t * scheduling, char * directory);

/***************** WRITER FUNCTIONS ********************/
void mpc_omp_task_scheduling_file_writer_open(
    mpc_omp_task_scheduling_t * scheduling,
    mpc_omp_task_scheduling_file_reader_t * reader,
    int nthread);

void mpc_omp_task_scheduling_file_writer_close(mpc_omp_task_scheduling_file_reader_t * reader);

void mpc_omp_task_scheduling_file_writer_next_entry(
    mpc_omp_task_scheduling_file_reader_t * reader,
    mpc_omp_task_scheduling_file_entry_t * entry);

/***************** READER FUNCTIONS ********************/
void mpc_omp_task_scheduling_file_reader_open(
    mpc_omp_task_scheduling_t * scheduling,
    mpc_omp_task_scheduling_file_reader_t * reader,
    int nthread);

void mpc_omp_task_scheduling_file_reader_close(mpc_omp_task_scheduling_file_reader_t * reader);

void mpc_omp_task_scheduling_file_reader_next_entry(
    mpc_omp_task_scheduling_file_reader_t * reader,
    mpc_omp_task_scheduling_file_entry_t * entry);

# endif /* __MPC_OMP_TASK_SCHEDULE_H__ */
