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

#  define MPC_OMP_TASK_TRACE_DEPENDENCY(out, in)    if (_mpc_omp_task_trace_begun()) _mpc_omp_task_trace_dependency(out, in)
#  define MPC_OMP_TASK_TRACE_SCHEDULE(task)         if (_mpc_omp_task_trace_begun()) _mpc_omp_task_trace_schedule(task)
#  define MPC_OMP_TASK_TRACE_CREATE(task)           if (_mpc_omp_task_trace_begun()) _mpc_omp_task_trace_create(task)
#  define MPC_OMP_TASK_TRACE_DELETE(task)           if (_mpc_omp_task_trace_begun()) _mpc_omp_task_trace_delete(task)
#  define MPC_OMP_TASK_TRACE_CALLBACK(when, status) if (_mpc_omp_task_trace_begun()) _mpc_omp_task_trace_async(when, status)
#  define MPC_OMP_TASK_TRACE_SEND(count, dtype, dst, tag, comm) if (_mpc_omp_task_trace_begun()) _mpc_omp_task_trace_send(count, dtype, dst, tag, comm)
#  define MPC_OMP_TASK_TRACE_RECV(count, dtype, src, tag, comm) if (_mpc_omp_task_trace_begun()) _mpc_omp_task_trace_recv(count, dtype, src, tag, comm)

# define MPC_OMP_TASK_TRACE_FILE_VERSION    1
# define MPC_OMP_TASK_TRACE_FILE_MAGIC      (0x6B736174) /* 't' 'a' 's' 'k' */

# define MPC_OMP_TASK_TRACE_RECYCLER_CAPACITY   131072

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
    MPC_OMP_TASK_TRACE_TYPE_DELETE,
    MPC_OMP_TASK_TRACE_TYPE_CALLBACK,
# if MPC_MPI
    MPC_OMP_TASK_TRACE_TYPE_SEND,
    MPC_OMP_TASK_TRACE_TYPE_RECV,
# endif /* MPC_MPI */
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

// TODO("optimize records : for instance, label should only be present on CREATE event");

typedef struct  mpc_omp_task_trace_record_schedule_s
{
    /* inheritance */
    mpc_omp_task_trace_record_t parent;

    /* the task uid */
    int uid;

    /* the task->priority attribute */
    int priority;

    /* the task properties */
    int properties;

    /* number of predecessors */
    int npredecessors;

    /* number of tasks that were scheduled before this one */
    int schedule_id;

    /* the task statuses */
    mpc_omp_task_statuses_t statuses;

}               mpc_omp_task_trace_record_schedule_t;

typedef struct  mpc_omp_task_trace_record_create_s
{
    /* inheritance */
    mpc_omp_task_trace_record_schedule_t parent;

    /* the task label */
    char label[MPC_OMP_TASK_LABEL_MAX_LENGTH];

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

    /* the task->priority attribute */
    int priority;

    /* the task properties */
    int properties;

    /* the task statuses */
    mpc_omp_task_statuses_t statuses;

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

typedef struct  mpc_omp_task_trace_record_famine_overlap_s
{
    /* inheritance */
    mpc_omp_task_trace_record_t parent;

    /* 0 on start, 1 on finish */
    int status;
}               mpc_omp_task_trace_record_famine_overlap_t;

typedef struct  mpc_omp_task_trace_record_callback_s
{
    /* inheritance */
    mpc_omp_task_trace_record_t parent;

    /* 0 on start, 1 on finish */
    int status;

    /* when the call occured */
    int when;
}               mpc_omp_task_trace_record_callback_t;

# if MPC_MPI
# include <mpc_mpi.h>

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
}               mpc_omp_task_trace_record_recv_t;
# endif /* MPC_MPI */

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
void _mpc_omp_task_trace_delete(struct mpc_omp_task_s * task);
void _mpc_omp_task_trace_callback(int when, int status);
void _mpc_omp_task_trace_send(int count, int datatype, int dst, int tag, int comm);
void _mpc_omp_task_trace_recv(int count, int datatype, int src, int tag, int comm);

# else  /* MPC_OMP_TASK_COMPILE_TRACE */
#  define MPC_OMP_TASK_TRACE_DEPENDENCY(...)
#  define MPC_OMP_TASK_TRACE_SCHEDULE(...)
#  define MPC_OMP_TASK_TRACE_CREATE(...)
#  define MPC_OMP_TASK_TRACE_DELETE(...)
#  define MPC_OMP_TASK_TRACE_CALLBACK(...)
#  define MPC_OMP_TASK_TRACE_SEND(...)
#  define MPC_OMP_TASK_TRACE_RECV(...)
# endif /* MPC_OMP_TASK_COMPILE_TRACE */

# endif /* __MPC_OMP_TASK_TRACE_H__ */
