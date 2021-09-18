# ifndef __MPC_OMP_TASK_TRACE_H__
#  define __MPC_OMP_TASK_TRACE_H__

# define MPC_OMP_TASK_TRACE_FILE_VERSION    1
# define MPC_OMP_TASK_TRACE_FILE_MAGIC      (0x6B736174) /* 't' 'a' 's' 'k' */

# define MPC_OMP_TASK_TRACE_RECYCLER_CAPACITY   131072

# include "mpc_common_recycler.h"
# include "mpc_omp.h"
# include "mpc_omp_task_property.h"

/**
 *  *  A trace writer (1 per thread)
 *   */
typedef struct  mpc_omp_task_trace_writer_s
{
    /* the fd */
    int fd;
}               mpc_omp_task_trace_writer_t;

/**
 *  * A trace file header
 *   */
typedef struct  mpc_omp_task_trace_file_header_s
{
    /* magic */
    int magic;

    /* trace file version */
    int version;

    /* Process ID : kernel PID one (or MPI rank if under a MPI context) */
    int pid;

    /* omp rank */
    int rank;
}               mpc_omp_task_trace_file_header_t;

typedef enum    mpc_omp_task_trace_record_type_e
{
    MPC_OMP_TASK_TRACE_TYPE_DEPENDENCY,
    MPC_OMP_TASK_TRACE_TYPE_SCHEDULE,
    MPC_OMP_TASK_TRACE_TYPE_CREATE,
    MPC_OMP_TASK_TRACE_TYPE_CALLBACK,
    MPC_OMP_TASK_TRACE_TYPE_COUNT
}               mpc_omp_task_trace_record_type_t;

/**
 *  * A generic trace file entry
 *   */
typedef struct  mpc_omp_task_trace_record_s
{
    /* event time */
    double time;

    /* the record type */
    mpc_omp_task_trace_record_type_t type;
}               mpc_omp_task_trace_record_t;

typedef struct  mpc_omp_task_trace_record_schedule_s
{
    /* inherithance */
    mpc_omp_task_trace_record_t parent;

    /* the task label */
    char label[MPC_OMP_TASK_LABEL_MAX_LENGTH];

    /* the task uid */
    int uid;

    /* the task->priority attribute */
    int priority;

    /* the task->omp_priority attribute */
    int omp_priority;

    /* the task properties */
    int properties;

    /* number of predecessors (data dependencies) */
    int predecessors;

    /* number of tasks that were scheduled before this one */
    int schedule_id;

    /* the task statuses */
    mpc_omp_task_statuses_t statuses;

}               mpc_omp_task_trace_record_schedule_t;

/* task creation record is the same as schedule one */
typedef mpc_omp_task_trace_record_schedule_t mpc_omp_task_trace_record_create_t;

typedef struct  mpc_omp_task_trace_record_dependency_s
{
    /* inherithance */
    mpc_omp_task_trace_record_t parent;

    /* out task uid */
    int out_uid;

    /* in task uid */
    int in_uid;
}               mpc_omp_task_trace_record_dependency_t;

typedef struct  mpc_omp_task_trace_record_famine_overlap_s
{
    /* inherithance */
    mpc_omp_task_trace_record_t parent;

    /* 0 on start, 1 on finish */
    int status;
}               mpc_omp_task_trace_record_famine_overlap_t;

typedef struct  mpc_omp_task_trace_record_callback_s
{
    /* inherithance */
    mpc_omp_task_trace_record_t parent;

    /* 0 on start, 1 on finish */
    int status;

    /* when the call occured */
    int when;
}               mpc_omp_task_trace_record_callback_t;

/**
 *  A record node
 */
typedef struct  mpc_omp_task_trace_node_s
{
    /* next node */
    struct mpc_omp_task_trace_node_s * next;

    /* the record is allocated right after this struct */
}               mpc_omp_task_trace_node_t;

struct mpc_omp_task_s;

/**
 *  Per thread trace informations
 */
typedef struct  mpc_omp_thread_task_trace_infos_s
{
    /* the trace writer */
    mpc_omp_task_trace_writer_t writer;

    /* the head node */
    mpc_omp_task_trace_node_t * head;

    /* the tail node */
    mpc_omp_task_trace_node_t * tail;

    /* node recycler */
    mpc_common_recycler_t recyclers[MPC_OMP_TASK_TRACE_TYPE_COUNT];

    /* 1 if between 'begin' / 'end' calls */
    int begun;

    /* id of the current traced code section */
    int id;
}               mpc_omp_thread_task_trace_infos_t;

/**
 *  Flush the thread events to the file descriptor
 */
void _mpc_omp_task_trace_flush(void);

/**
 *  Return true if between a 'begin' and 'end' trace section
 */
int _mpc_omp_task_trace_begun(void);

/**
 * Add events to the thread event queue
 */
void _mpc_omp_task_trace_dependency(struct mpc_omp_task_s * out, struct mpc_omp_task_s * in);
void _mpc_omp_task_trace_schedule(struct mpc_omp_task_s * task);
void _mpc_omp_task_trace_create(struct mpc_omp_task_s * task);
void _mpc_omp_task_trace_callback(int when, int status);

# endif
