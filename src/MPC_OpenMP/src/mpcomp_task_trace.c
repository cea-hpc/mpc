# include "mpc_config.h"
# if MPC_MPI
#  include "mpc_mpi.h"
# endif

# include "mpcomp_types.h"
# include "mpcomp_core.h"
# include "mpcomp_task.h"

# include <sys/types.h>
# include <sys/stat.h>
# include <fcntl.h>

static int
__getpid(void)
{
# if MPC_MPI
    int mpi_context;
    MPI_Initialized(&mpi_context);
    if (mpi_context)
    {
        int rank;
        MPI_Comm_rank(MPI_COMM_WORLD, &rank);
        return rank;
    }
# endif /* MPC_MPI */
    return (int) getpid();
}

static inline mpc_omp_thread_task_trace_infos_t *
__get_infos(void)
{
    mpc_omp_thread_t * thread = (mpc_omp_thread_t *)mpc_omp_tls;
    assert(thread);
    return &(thread->task_infos.tracer);
}

static inline double
__get_global_time(void)
{
    return mpc_omp_timestamp();
}

int
_mpc_omp_task_trace_begun(void)
{
    return __get_infos()->begun;
}

static inline mpc_omp_task_trace_record_t *
__node_record(mpc_omp_task_trace_node_t * node)
{
    return (mpc_omp_task_trace_record_t *) (node + 1);
}

static inline mpc_omp_task_trace_node_t *
__node_new(mpc_omp_task_trace_record_type_t type)
{
    mpc_omp_thread_task_trace_infos_t * infos = __get_infos();
    mpc_omp_task_trace_node_t * node = (mpc_omp_task_trace_node_t *) mpc_common_recycler_alloc(&(infos->recyclers[type]));
    assert(node);

    mpc_omp_task_trace_record_t * record = __node_record(node);
    record->type = type;
    record->time = __get_global_time();
    return node;
}

static inline void
__node_insert(mpc_omp_task_trace_node_t * node)
{
    mpc_omp_thread_task_trace_infos_t * infos = __get_infos();
    assert(infos->begun);
    if (infos->head == NULL)
    {
        assert(infos->tail == NULL);
        infos->head = node;
    }
    else
    {
        assert(infos->tail);
        infos->tail->next = node;
    }
    infos->tail = node;
    node->next = NULL;

}

static inline size_t
__record_sizeof(mpc_omp_task_trace_record_type_t type)
{
    switch (type)
    {
        case (MPC_OMP_TASK_TRACE_TYPE_DEPENDENCY):
        {
            return sizeof(mpc_omp_task_trace_record_dependency_t);
        }
        
        case (MPC_OMP_TASK_TRACE_TYPE_SCHEDULE):
        case (MPC_OMP_TASK_TRACE_TYPE_CREATE):
        {
            return sizeof(mpc_omp_task_trace_record_schedule_t);
        }

        case (MPC_OMP_TASK_TRACE_TYPE_CALLBACK):
        {
            return sizeof(mpc_omp_task_trace_record_callback_t);
        }

        default:
        {
            printf("invalid record : type=%d\n", type);
            assert(0);
            return 0;
        }
    }
}

static inline void
__record_flush(
        mpc_omp_task_trace_writer_t * writer,
        mpc_omp_task_trace_record_t * record)
{
    write(writer->fd, record, __record_sizeof(record->type));
}

void
_mpc_omp_task_trace_flush(void)
{
    mpc_omp_thread_task_trace_infos_t * infos = __get_infos();
    while (infos->head)
    {
        mpc_omp_task_trace_node_t * node = infos->head;
        mpc_omp_task_trace_node_t * next = infos->head->next;
        mpc_omp_task_trace_record_t * record = __node_record(node);
        __record_flush(&(infos->writer), record);
        mpc_common_recycler_recycle(&(infos->recyclers[record->type]), node);
        infos->head = next;
    }
}

void
_mpc_omp_task_trace_dependency(mpc_omp_task_t * out, mpc_omp_task_t * in)
{
    mpc_omp_task_trace_node_t * node = __node_new(MPC_OMP_TASK_TRACE_TYPE_DEPENDENCY);
    assert(node);

    mpc_omp_task_trace_record_dependency_t * record = (mpc_omp_task_trace_record_dependency_t *) __node_record(node);
    record->out_uid = out->uid;
    record->in_uid  = in->uid;
   
    __node_insert(node); 
}

static void
__task_trace(mpc_omp_task_t * task, mpc_omp_task_trace_record_type_t type)
{
    mpc_omp_task_trace_node_t * node = __node_new(type);
    assert(node);

    mpc_omp_task_trace_record_schedule_t * record = (mpc_omp_task_trace_record_schedule_t *) __node_record(node);
    strncpy(record->label, task->label ? task->label : "(null)", MPC_OMP_TASK_LABEL_MAX_LENGTH);
    record->uid             = task->uid;
    record->priority        = task->priority;
    record->omp_priority    = task->omp_priority_hint;
    record->properties      = task->property;
    record->predecessors    = OPA_load_int(&(task->dep_node.ref_predecessors));
    record->schedule_id     = task->schedule_id;

    __node_insert(node);
}

void
_mpc_omp_task_trace_schedule(mpc_omp_task_t * task)
{
    __task_trace(task, MPC_OMP_TASK_TRACE_TYPE_SCHEDULE);
}

void
_mpc_omp_task_trace_create(mpc_omp_task_t * task)
{
    __task_trace(task, MPC_OMP_TASK_TRACE_TYPE_CREATE);
}

void
_mpc_omp_task_trace_callback(int when, int status)
{
    mpc_omp_task_trace_node_t * node = __node_new(MPC_OMP_TASK_TRACE_TYPE_CALLBACK);
    assert(node);

    mpc_omp_task_trace_record_callback_t * record = (mpc_omp_task_trace_record_callback_t *) __node_record(node);
    record->when    = when;
    record->status  = status;

    __node_insert(node);
}

static inline void
__task_trace_create_file(void)
{
    mpc_omp_thread_t * thread = (mpc_omp_thread_t *)mpc_omp_tls;
    char filepath[64];
    sprintf(filepath, "mpc_omp_trace_%d_%ld.dat", __getpid(), thread->rank);
    thread->task_infos.tracer.writer.fd = open(filepath, O_WRONLY | O_TRUNC | O_CREAT, 0644);
    assert(thread->task_infos.tracer.writer.fd >= 0);

    mpc_omp_task_trace_file_header_t header;
    header.magic    = MPC_OMP_TASK_TRACE_FILE_MAGIC;
    header.version  = MPC_OMP_TASK_TRACE_FILE_VERSION;
    header.rank     = thread->rank;
    header.pid      = __getpid();
    write(thread->task_infos.tracer.writer.fd, &header, sizeof(mpc_omp_task_trace_file_header_t));
}

void
mpc_omp_task_trace_begin(void)
{
    if (!MPC_OMP_TASK_TRACE_ENABLED) return ;

    mpc_omp_thread_task_trace_infos_t * infos = __get_infos();

    mpc_omp_task_trace_record_type_t type;
    for (type = 0 ; type < MPC_OMP_TASK_TRACE_TYPE_COUNT ; type++)
    {
        mpc_common_recycler_init(
                &(infos->recyclers[type]),
                mpc_omp_alloc, mpc_omp_free,
                sizeof(mpc_omp_task_trace_node_t) + __record_sizeof(type),
                MPC_OMP_TASK_TRACE_RECYCLER_CAPACITY);
    }

    __task_trace_create_file();
    infos->begun = 1;
}

void
mpc_omp_task_trace_end(void)
{
    if (!MPC_OMP_TASK_TRACE_ENABLED) return ;
    _mpc_omp_task_trace_flush();
    
    mpc_omp_thread_task_trace_infos_t * infos = __get_infos();
    close(infos->writer.fd);

    mpc_omp_task_trace_record_type_t type;
    for (type = 0 ; type < MPC_OMP_TASK_TRACE_TYPE_COUNT ; type++)
    {
        mpc_common_recycler_deinit(&(infos->recyclers[type]));
    }
    infos->begun = 0;
}
