/* ############################# MPC License ############################## */
/* # Tue Oct 12 10:33:59 CEST 2021                                        # */
/* # Copyright or (C) or Copr. Commissariat a l'Energie Atomique          # */
/* #                                                                      # */
/* # IDDN.FR.001.230040.000.S.P.2007.000.10000                            # */
/* # This file is part of the MPC Runtime.                                # */
/* #                                                                      # */
/* # This software is governed by the CeCILL-C license under French law   # */
/* # and abiding by the rules of distribution of free software.  You can  # */
/* # use, modify and/ or redistribute the software under the terms of     # */
/* # the CeCILL-C license as circulated by CEA, CNRS and INRIA at the     # */
/* # following URL http://www.cecill.info.                                # */
/* #                                                                      # */
/* # The fact that you are presently reading this means that you have     # */
/* # had knowledge of the CeCILL-C license and that you accept its        # */
/* # terms.                                                               # */
/* #                                                                      # */
/* # Maintainers:                                                         # */
/* # - CARRIBAULT Patrick patrick.carribault@cea.fr                       # */
/* # - JAEGER Julien julien.jaeger@cea.fr                                 # */
/* # - PERACHE Marc marc.perache@cea.fr                                   # */
/* # - ROUSSEL Adrien adrien.roussel@cea.fr                               # */
/* # - TABOADA Hugo hugo.taboada@cea.fr                                   # */
/* #                                                                      # */
/* # Authors:                                                             # */
/* # - Romain Pereira <romain.pereira@cea.fr>                             # */
/* #                                                                      # */
/* ######################################################################## */
# include "mpcomp_types.h"

# if MPC_OMP_TASK_COMPILE_TRACE

# include "mpc_config.h"
# if MPC_MPI
#  include "mpc_mpi.h"
# endif

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

int
_mpc_omp_task_trace_begun(void)
{
    return mpc_omp_tls && mpc_omp_conf_get()->task_trace && __get_infos()->begun;
}

int
_mpc_omp_task_trace_enabled(mpc_omp_task_trace_record_type_t type)
{
    return mpc_omp_tls && mpc_omp_conf_get()->task_trace && __get_infos()->begun && (mpc_omp_conf_get()->task_trace_mask & (1 << type));
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

    struct timeval tp;
    gettimeofday(&tp, NULL);

    mpc_omp_task_trace_record_t * record = __node_record(node);
    record->type = type;
    record->time = (uint64_t)(tp.tv_sec * 1000000) + (uint64_t) tp.tv_usec;
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

size_t
mpc_omp_task_trace_record_sizeof(int type)
{
    switch (type)
    {
        case (MPC_OMP_TASK_TRACE_TYPE_BEGIN):
        case (MPC_OMP_TASK_TRACE_TYPE_END):
        {
            return sizeof(mpc_omp_task_trace_record_t);
        }

        case (MPC_OMP_TASK_TRACE_TYPE_DEPENDENCY):
        {
            return sizeof(mpc_omp_task_trace_record_dependency_t);
        }

        case (MPC_OMP_TASK_TRACE_TYPE_SCHEDULE):
        {
            return sizeof(mpc_omp_task_trace_record_schedule_t);
        }

        case (MPC_OMP_TASK_TRACE_TYPE_CREATE):
        {
            return sizeof(mpc_omp_task_trace_record_create_t);
        }

        case (MPC_OMP_TASK_TRACE_TYPE_DELETE):
        {
            return sizeof(mpc_omp_task_trace_record_delete_t);
        }

        case (MPC_OMP_TASK_TRACE_TYPE_CALLBACK):
        {
            return sizeof(mpc_omp_task_trace_record_callback_t);
        }

        case (MPC_OMP_TASK_TRACE_TYPE_SEND):
        {
            return sizeof(mpc_omp_task_trace_record_send_t);
        }

        case (MPC_OMP_TASK_TRACE_TYPE_RECV):
        {
            return sizeof(mpc_omp_task_trace_record_recv_t);
        }

        case (MPC_OMP_TASK_TRACE_TYPE_ALLREDUCE):
        {
            return sizeof(mpc_omp_task_trace_record_allreduce_t);
        }

        case (MPC_OMP_TASK_TRACE_TYPE_RANK):
        {
            return sizeof(mpc_omp_task_trace_record_rank_t);
        }

        default:
        {
            assert("invalid record" == NULL);
            return 0;
        }
    }
}

static inline void
__record_flush(
        mpc_omp_task_trace_writer_t * writer,
        mpc_omp_task_trace_record_t * record)
{
    write(writer->fd, record, mpc_omp_task_trace_record_sizeof(record->type));
}

# define MPC_OMP_TASK_TRACE_FLUSH_BUFFER_SIZE 8192

void
_mpc_omp_task_trace_flush(void)
{
    char buffer[MPC_OMP_TASK_TRACE_FLUSH_BUFFER_SIZE];
    mpc_omp_thread_task_trace_infos_t * infos = __get_infos();
    size_t nbytes = 0;
    while (infos->head)
    {
        mpc_omp_task_trace_node_t * node = infos->head;
        mpc_omp_task_trace_node_t * next = infos->head->next;
        mpc_omp_task_trace_record_t * record = __node_record(node);
        size_t size = mpc_omp_task_trace_record_sizeof(record->type);
        if (nbytes + size >= MPC_OMP_TASK_TRACE_FLUSH_BUFFER_SIZE)
        {
            write(infos->writer.fd, buffer, nbytes);
            nbytes = 0;
        }
        memcpy(buffer + nbytes, record, size);
        nbytes += size;
        mpc_common_recycler_recycle(&(infos->recyclers[record->type]), node);
        infos->head = next;
    }
    if (nbytes) write(infos->writer.fd, buffer, nbytes);
    infos->tail = NULL;
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

static inline void
__task_trace(mpc_omp_task_t * task, mpc_omp_task_trace_node_t * node)
{
    mpc_omp_task_trace_record_schedule_t * record = (mpc_omp_task_trace_record_schedule_t *) __node_record(node);
    record->uid                 = task->uid;
    record->persistent_uid      = task->persistent_infos.uid;
    record->priority            = task->priority;
    record->properties          = task->property;
    record->npredecessors       = task->dep_node.npredecessors;
    record->ref_predecessors    = OPA_load_int(&(task->dep_node.ref_predecessors));
    record->schedule_id         = task->schedule_id;
    record->statuses            = task->statuses;
}

void
_mpc_omp_task_trace_schedule(mpc_omp_task_t * task)
{
    mpc_omp_task_trace_node_t * node = __node_new(MPC_OMP_TASK_TRACE_TYPE_SCHEDULE);
    assert(node);
    __task_trace(task, node);
    __node_insert(node);
}

void
_mpc_omp_task_trace_create(mpc_omp_task_t * task)
{
    mpc_omp_task_trace_node_t * node = __node_new(MPC_OMP_TASK_TRACE_TYPE_CREATE);
    assert(node);
    __task_trace(task, node);
    mpc_omp_task_trace_record_create_t * record = (mpc_omp_task_trace_record_create_t *) __node_record(node);
    strncpy(record->label, task->label ? task->label : "(null)", MPC_OMP_TASK_LABEL_MAX_LENGTH);
    record->color = task->color;
    record->parent_uid = task->parent ? task->parent->uid : -1;
    record->omp_priority = task->omp_priority_hint;
    __node_insert(node);
}

void
_mpc_omp_task_trace_delete(mpc_omp_task_t * task)
{
    mpc_omp_task_trace_node_t * node = __node_new(MPC_OMP_TASK_TRACE_TYPE_DELETE);
    assert(node);

    mpc_omp_task_trace_record_delete_t * record = (mpc_omp_task_trace_record_delete_t *) __node_record(node);

    record->uid             = task->uid;
    record->priority        = task->priority;
    record->properties      = task->property;
    record->statuses        = task->statuses;

    __node_insert(node);
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

void
_mpc_omp_task_trace_send(int count, int datatype, int dst, int tag, int comm)
{
    mpc_omp_thread_t * thread = (mpc_omp_thread_t *)mpc_omp_tls;
    assert(thread);

    mpc_omp_task_t * task = MPC_OMP_TASK_THREAD_GET_CURRENT_TASK(thread);
    assert(task);

    mpc_omp_task_trace_node_t * node = __node_new(MPC_OMP_TASK_TRACE_TYPE_SEND);
    assert(node);

    mpc_omp_task_trace_record_send_t * record = (mpc_omp_task_trace_record_send_t *) __node_record(node);
    record->uid = task->uid;
    record->count = count;
    record->datatype = datatype;
    record->dst = dst;
    record->tag = tag;
    record->comm = comm;

    __node_insert(node);
}

void
_mpc_omp_task_trace_recv(int count, int datatype, int src, int tag, int comm)
{
    mpc_omp_thread_t * thread = (mpc_omp_thread_t *)mpc_omp_tls;
    assert(thread);

    mpc_omp_task_t * task = MPC_OMP_TASK_THREAD_GET_CURRENT_TASK(thread);
    assert(task);

    mpc_omp_task_trace_node_t * node = __node_new(MPC_OMP_TASK_TRACE_TYPE_RECV);
    assert(node);

    mpc_omp_task_trace_record_recv_t * record = (mpc_omp_task_trace_record_recv_t *) __node_record(node);
    record->uid = task->uid;
    record->count = count;
    record->datatype = datatype;
    record->src = src;
    record->tag = tag;
    record->comm = comm;

    __node_insert(node);
}

void
_mpc_omp_task_trace_allreduce(int count, int datatype, int op, int comm)
{
    mpc_omp_thread_t * thread = (mpc_omp_thread_t *)mpc_omp_tls;
    assert(thread);

    mpc_omp_task_t * task = MPC_OMP_TASK_THREAD_GET_CURRENT_TASK(thread);
    assert(task);

    mpc_omp_task_trace_node_t * node = __node_new(MPC_OMP_TASK_TRACE_TYPE_ALLREDUCE);
    assert(node);

    mpc_omp_task_trace_record_allreduce_t * record = (mpc_omp_task_trace_record_allreduce_t *) __node_record(node);
    record->uid = task->uid;
    record->count = count;
    record->datatype = datatype;
    record->op = op;
    record->comm = comm;

    __node_insert(node);
}

void
_mpc_omp_task_trace_rank(int comm, int rank)
{
    mpc_omp_thread_t * thread = (mpc_omp_thread_t *)mpc_omp_tls;
    assert(thread);

    mpc_omp_task_t * task = MPC_OMP_TASK_THREAD_GET_CURRENT_TASK(thread);
    assert(task);

    mpc_omp_task_trace_node_t * node = __node_new(MPC_OMP_TASK_TRACE_TYPE_RANK);
    assert(node);

    mpc_omp_task_trace_record_rank_t * record = (mpc_omp_task_trace_record_rank_t *) __node_record(node);
    record->comm = comm;
    record->rank = rank;

    __node_insert(node);
}

static inline int
__task_trace_create_directory(void)
{
    char * tracedir = mpc_omp_conf_get()->task_trace_dir;
    if (tracedir[0] == 0)
    {
        snprintf(tracedir, MPC_CONF_STRING_SIZE, "mpc-omp-task-trace-%ld", (unsigned long) time(NULL));
        if (omp_get_thread_num() == 0) printf("[MPC] Generating trace to %s\n", tracedir);
    }

    char buffer[MPC_CONF_STRING_SIZE];
    strncpy(buffer, tracedir, MPC_CONF_STRING_SIZE);

    struct stat sb;
    char * tmp = buffer;
    while (*tmp)
    {
        if (*tmp == '/')
        {
            *tmp = 0;
            if (mkdir(buffer, S_IRWXU) == -1)
            {
                if (stat(buffer, &sb) == -1 || !S_ISDIR(sb.st_mode))
                {
                    return -1;
                }
            }
            *tmp = '/';
        }
        ++tmp;
    }

    if (mkdir(buffer, S_IRWXU) == -1)
    {
        if (stat(buffer, &sb) == -1 || !S_ISDIR(sb.st_mode))
        {
            return -1;
        }
    }
    return 0;
}

static inline int
__task_trace_create_file(void)
{
    mpc_omp_thread_t * thread = (mpc_omp_thread_t *)mpc_omp_tls;

    char hostname[128];
    gethostname(hostname, sizeof(hostname));

    char filepath[1024];
    char * tracedir = mpc_omp_conf_get()->task_trace_dir;

    sprintf(filepath, "%s/%s-%d-%ld-%d.dat", tracedir, hostname, __getpid(), thread->rank, thread->task_infos.tracer.id++);
    thread->task_infos.tracer.writer.fd = open(filepath, O_WRONLY | O_TRUNC | O_CREAT, 0644);
    assert(thread->task_infos.tracer.writer.fd >= 0);

    mpc_omp_task_trace_file_header_t header;
    header.magic    = MPC_OMP_TASK_TRACE_FILE_MAGIC;
    header.version  = MPC_OMP_TASK_TRACE_FILE_VERSION;
    header.rank     = thread->rank;
    header.pid      = __getpid();
    write(thread->task_infos.tracer.writer.fd, &header, sizeof(mpc_omp_task_trace_file_header_t));
    return 0;
}

void
mpc_omp_task_trace_begin(void)
{
    if (!mpc_omp_conf_get()->task_trace) return ;

    mpc_omp_thread_task_trace_infos_t * infos = __get_infos();
    assert(infos->head == NULL);
    assert(infos->tail == NULL);

    mpc_omp_task_trace_record_type_t type;
    for (type = 0 ; type < MPC_OMP_TASK_TRACE_TYPE_COUNT ; type++)
    {
        //printf("sizeof(type=%d) = %lu\n", type, mpc_omp_task_trace_record_sizeof(type));
        mpc_common_recycler_init(
                &(infos->recyclers[type]),
                mpc_omp_alloc, mpc_omp_free,
                sizeof(mpc_omp_task_trace_node_t) + mpc_omp_task_trace_record_sizeof(type),
                MPC_OMP_TASK_TRACE_RECYCLER_CAPACITY);
    }

    __task_trace_create_directory();
    if (__task_trace_create_file() == 0)
    {
        infos->begun = 1;
    }
    __node_insert(__node_new(MPC_OMP_TASK_TRACE_TYPE_BEGIN));
}

void
mpc_omp_task_trace_end(void)
{
    if (!mpc_omp_conf_get()->task_trace) return ;

    __node_insert(__node_new(MPC_OMP_TASK_TRACE_TYPE_END));
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

#endif /* MPC_OMP_TASK_COMPILE_TRACE */
