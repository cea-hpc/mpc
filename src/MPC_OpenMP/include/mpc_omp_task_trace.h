/* ############################# MPC License ############################## */
/* # Tue Oct 12 12:25:57 CEST 2021                                        # */
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
# ifndef __MPC_OMP_TASK_TRACE_H__
#  define __MPC_OMP_TASK_TRACE_H__

/* enable trace compiling */
# define MPC_OMP_TASK_COMPILE_TRACE 1

/* enable PAPI tracing per tasks */
# define MPC_OMP_TASK_TRACE_USE_PAPI 1
# if MPC_OMP_TASK_TRACE_USE_PAPI
#  define PAPI_LOG(...) do {                                                \
                            printf("[PAPI] [%d] ", omp_get_thread_num());   \
                            printf(__VA_ARGS__);                            \
                            printf("\n");                                   \
                        } while (0)
# endif

#if MPC_OMP_TASK_COMPILE_TRACE
# define MPC_OMP_TASK_FIBER_ENABLED mpc_omp_conf_get()->task_use_fiber
# define MPC_OMP_TASK_TRACE_ENABLED mpc_omp_conf_get()->task_trace
# else
# define MPC_OMP_TASK_TRACE_ENABLED 0
#endif

# if MPC_OMP_TASK_COMPILE_TRACE

# include "mpc_common_recycler.h"
# include "mpc_omp.h"
# include "mpc_omp_task_property.h"

typedef enum    mpc_omp_task_trace_record_type_e
{
    MPC_OMP_TASK_TRACE_TYPE_BEGIN,          // 0
    MPC_OMP_TASK_TRACE_TYPE_END,            // 1
    MPC_OMP_TASK_TRACE_TYPE_DEPENDENCY,     // 2
    MPC_OMP_TASK_TRACE_TYPE_SCHEDULE,       // 4
    MPC_OMP_TASK_TRACE_TYPE_CREATE,         // 8
    MPC_OMP_TASK_TRACE_TYPE_DELETE,         // 16
    MPC_OMP_TASK_TRACE_TYPE_SEND,           // 32
    MPC_OMP_TASK_TRACE_TYPE_RECV,           // 64
    MPC_OMP_TASK_TRACE_TYPE_ALLREDUCE,      // 128
    MPC_OMP_TASK_TRACE_TYPE_RANK,           // 256
    MPC_OMP_TASK_TRACE_TYPE_BLOCKED,        // 512
    MPC_OMP_TASK_TRACE_TYPE_UNBLOCKED,      // 1024
    MPC_OMP_TASK_TRACE_TYPE_COUNT
}               mpc_omp_task_trace_record_type_t;

#  define MPC_OMP_TASK_SHOULD_TRACE(X)  _mpc_omp_task_trace_enabled(MPC_OMP_TASK_TRACE_TYPE_##X)

#  define MPC_OMP_TASK_TRACE_DEPENDENCY(out, in)                            if (MPC_OMP_TASK_SHOULD_TRACE(DEPENDENCY))  _mpc_omp_task_trace_dependency(out, in)
#  define MPC_OMP_TASK_TRACE_SCHEDULE(task)                                 if (MPC_OMP_TASK_SHOULD_TRACE(SCHEDULE))    _mpc_omp_task_trace_schedule(task)
#  define MPC_OMP_TASK_TRACE_CREATE(task)                                   if (MPC_OMP_TASK_SHOULD_TRACE(CREATE))      _mpc_omp_task_trace_create(task)
#  define MPC_OMP_TASK_TRACE_DELETE(task)                                   if (MPC_OMP_TASK_SHOULD_TRACE(DELETE))      _mpc_omp_task_trace_delete(task)
#  define MPC_OMP_TASK_TRACE_SEND(count, dtype, dst, tag, comm, completed)  if (MPC_OMP_TASK_SHOULD_TRACE(SEND))        _mpc_omp_task_trace_send(count, dtype, dst, tag, comm, completed)
#  define MPC_OMP_TASK_TRACE_RECV(count, dtype, src, tag, comm, completed)  if (MPC_OMP_TASK_SHOULD_TRACE(RECV))        _mpc_omp_task_trace_recv(count, dtype, src, tag, comm, completed)
#  define MPC_OMP_TASK_TRACE_ALLREDUCE(count, dtype, op, comm, completed)   if (MPC_OMP_TASK_SHOULD_TRACE(ALLREDUCE))   _mpc_omp_task_trace_allreduce(count, dtype, op, comm, completed)
#  define MPC_OMP_TASK_TRACE_RANK(comm, rank)                               if (MPC_OMP_TASK_SHOULD_TRACE(RANK))        _mpc_omp_task_trace_rank(comm, rank)
#  define MPC_OMP_TASK_TRACE_BLOCKED(task)                                  if (MPC_OMP_TASK_SHOULD_TRACE(BLOCKED))     _mpc_omp_task_trace_blocked(task)
#  define MPC_OMP_TASK_TRACE_UNBLOCKED(task)                                if (MPC_OMP_TASK_SHOULD_TRACE(UNBLOCKED))   _mpc_omp_task_trace_unblocked(task)

# define MPC_OMP_TASK_TRACE_FILE_VERSION    1
# define MPC_OMP_TASK_TRACE_FILE_MAGIC      (0x6B736174) /* 't' 'a' 's' 'k' */

# define MPC_OMP_TASK_TRACE_RECYCLER_CAPACITY mpc_omp_conf_get()->task_trace_recycler_capacity

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

/**
 * A generic trace file entry
 */
typedef struct  mpc_omp_task_trace_record_s
{
    /* event time */
    uint64_t time;

    /* the record type */
    mpc_omp_task_trace_record_type_t type;
}               mpc_omp_task_trace_record_t;

# define MPC_OMP_TASK_TRACE_MAX_HW_COUNTERS (4)

typedef struct  mpc_omp_task_trace_record_schedule_s
{
    /* inheritance */
    mpc_omp_task_trace_record_t parent;

    /* the task uid */
    int uid;

    /* the task internal priority */
    int priority;

    /* the task properties */
    int properties;

    /* number of tasks that were scheduled before this one */
    int schedule_id;

    /* the task statuses */
    int statuses;

    /* the task hardware counters (4 maximum) */
    long long hwcounters[MPC_OMP_TASK_TRACE_MAX_HW_COUNTERS];

}               mpc_omp_task_trace_record_schedule_t;

typedef struct  mpc_omp_task_trace_record_create_s
{
    /* inheritance */
    mpc_omp_task_trace_record_t parent;

    /* the task uid */
    int uid;

    /* task persistent uid */
    int persistent_uid;

    /* the task properties */
    int properties;

    /* the task statuses */
    int statuses;

    /* the task label */
    char label[MPC_OMP_TASK_LABEL_MAX_LENGTH];

    /* the task color id */
    int color;

    /* control parent task */
    int parent_uid;

    /* openmp constructor priority */
    int omp_priority;

}               mpc_omp_task_trace_record_create_t;

typedef struct  mpc_omp_task_trace_record_delete_s
{
    /* inheritance */
    mpc_omp_task_trace_record_t parent;

    /* the task uid */
    int uid;

    /* the task internal priority */
    int priority;

    /* the task properties */
    int properties;

    /* the task statuses */
    int statuses;

}               mpc_omp_task_trace_record_delete_t;

typedef struct  mpc_omp_task_trace_record_dependency_s
{
    /* inheritance */
    mpc_omp_task_trace_record_t parent;

    /* out task uid */
    int out_uid;

    /* in task uid */
    int in_uid;
}               mpc_omp_task_trace_record_dependency_t;

typedef struct  mpc_omp_task_trace_record_send_s
{
    /* inheritance */
    mpc_omp_task_trace_record_t parent;

    /* task uid */
    int uid;

    /* mpi informations */
    int count;
    int datatype;
    int dst;
    int tag;
    int comm;

    /* 0 = the request just started */
    /* 1 = the request completed */
    int completed;
}               mpc_omp_task_trace_record_send_t;

typedef struct  mpc_omp_task_trace_record_recv_s
{
    /* inheritance */
    mpc_omp_task_trace_record_t parent;

    /* task uid */
    int uid;

    /* mpi informations */
    int count;
    int datatype;
    int src;
    int tag;
    int comm;

    /* 0 = the request just started */
    /* 1 = the request completed */
    int completed;
}               mpc_omp_task_trace_record_recv_t;

typedef struct  mpc_omp_task_trace_record_allreduce_s
{
    /* inheritance */
    mpc_omp_task_trace_record_t parent;

    /* task uid */
    int uid;

    /* mpi informations */
    int count;
    int datatype;
    int op;
    int comm;

    /* 0 = the request just started */
    /* 1 = the request completed */
    int completed;
}               mpc_omp_task_trace_record_allreduce_t;

typedef struct  mpc_omp_task_trace_record_rank_s
{
    /* inheritance */
    mpc_omp_task_trace_record_t parent;

    /* current process mpi communicator and rank */
    int comm;
    int rank;
}               mpc_omp_task_trace_record_rank_t;

typedef struct  mpc_omp_task_trace_record_blocked_s
{
    /* inheritance */
    mpc_omp_task_trace_record_t parent;

    /* the task uid */
    int uid;

}               mpc_omp_task_trace_record_blocked_t;

typedef mpc_omp_task_trace_record_blocked_t mpc_omp_task_trace_record_unblocked_t;

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

# if MPC_OMP_TASK_TRACE_USE_PAPI
    /* the papi event set */
    int papi_eventset;

    /* number of event traced */
    int papi_nevents;

# endif /* MPC_OMP_TASK_TRACE_USE_PAPI */
}               mpc_omp_thread_task_trace_infos_t;

#ifdef __cplusplus
extern "C" {
#endif

    /**
     *  Flush the thread events to the file descriptor
     */
    void mpc_omp_task_trace_flush(void);

    /**
     *  Return true if between a 'begin' and 'end' trace section
     */
    int mpc_omp_task_trace_begun(void);

    /**
     * Return true if the given event must be traced by the runtime
     */
    int _mpc_omp_task_trace_enabled(mpc_omp_task_trace_record_type_t type);

    /**
     * Add events to the thread event queue
     */
    void _mpc_omp_task_trace_dependency(struct mpc_omp_task_s * out, struct mpc_omp_task_s * in);
    void _mpc_omp_task_trace_schedule(struct mpc_omp_task_s * task);
    void _mpc_omp_task_trace_create(struct mpc_omp_task_s * task);
    void _mpc_omp_task_trace_delete(struct mpc_omp_task_s * task);
    void _mpc_omp_task_trace_send(int count, int datatype, int dst, int tag, int comm, int completed);
    void _mpc_omp_task_trace_recv(int count, int datatype, int src, int tag, int comm, int completed);
    void _mpc_omp_task_trace_allreduce(int count, int datatype, int op, int comm, int completed);
    void _mpc_omp_task_trace_rank(int comm, int rank);
    void _mpc_omp_task_trace_blocked(struct mpc_omp_task_s * task);
    void _mpc_omp_task_trace_unblocked(struct mpc_omp_task_s * task);

#ifdef __cplusplus
}
#endif /* __cplusplus */

# else  /* MPC_OMP_TASK_COMPILE_TRACE */
#  define MPC_OMP_TASK_TRACE_DEPENDENCY(...)
#  define MPC_OMP_TASK_TRACE_SCHEDULE(...)
#  define MPC_OMP_TASK_TRACE_CREATE(...)
#  define MPC_OMP_TASK_TRACE_DELETE(...)
#  define MPC_OMP_TASK_TRACE_SEND(...)
#  define MPC_OMP_TASK_TRACE_RECV(...)
#  define MPC_OMP_TASK_TRACE_ALLREDUCE(...)
#  define MPC_OMP_TASK_TRACE_RANK(...)
#  define MPC_OMP_TASK_TRACE_BLOCKED(...)
#  define MPC_OMP_TASK_TRACE_UNBLOCKED(...)
# endif /* MPC_OMP_TASK_COMPILE_TRACE */

# endif /* __MPC_OMP_TASK_TRACE_H__ */
