/* ############################# MPC License ############################## */
/* # Wed Nov 10 13:55:00 CET 2020                                         # */
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
/* # Authors:                                                             # */
/* #   - PEREIRA Romain romain.pereira.ocre@cea.fr                        # */
/* #                                                                      # */
/* ######################################################################## */

#ifndef __MPC_OMP_TASK_PROPERTY_H__ 
# define __MPC_OMP_TASK_PROPERTY_H__

/* GOMP bit flags */
/* https://github.com/gcc-mirror/gcc/blob/master/include/gomp-constants.h#L202 */
# define GOMP_TASK_FLAG_UNTIED      (1 << 0)
# define GOMP_TASK_FLAG_FINAL       (1 << 1)
# define GOMP_TASK_FLAG_MERGEABLE   (1 << 2)
# define GOMP_TASK_FLAG_DEPEND      (1 << 3)
# define GOMP_TASK_FLAG_PRIORITY    (1 << 4)
# define GOMP_TASK_FLAG_UP          (1 << 8)
# define GOMP_TASK_FLAG_GRAINSIZE   (1 << 9)
# define GOMP_TASK_FLAG_IF          (1 << 10)
# define GOMP_TASK_FLAG_NOGROUP     (1 << 11)

/* MPC_OMP bit flags */
# define MPC_OMP_TASK_PROP_UNDEFERRED    (1 << 0)
# define MPC_OMP_TASK_PROP_UNTIED        (1 << 1)  /* if the task is untied */
# define MPC_OMP_TASK_PROP_EXPLICIT      (1 << 2)
# define MPC_OMP_TASK_PROP_IMPLICIT      (1 << 3)
# define MPC_OMP_TASK_PROP_INITIAL       (1 << 4)
# define MPC_OMP_TASK_PROP_INCLUDED      (1 << 5)
# define MPC_OMP_TASK_PROP_FINAL         (1 << 6)  /* if the task is final */
# define MPC_OMP_TASK_PROP_MERGED        (1 << 7)
# define MPC_OMP_TASK_PROP_MERGEABLE     (1 << 8)
# define MPC_OMP_TASK_PROP_DEPEND        (1 << 9)
# define MPC_OMP_TASK_PROP_PRIORITY      (1 << 10)
# define MPC_OMP_TASK_PROP_UP            (1 << 11)
# define MPC_OMP_TASK_PROP_GRAINSIZE     (1 << 12)
# define MPC_OMP_TASK_PROP_IF            (1 << 13)
# define MPC_OMP_TASK_PROP_NOGROUP       (1 << 14)
# define MPC_OMP_TASK_PROP_HAS_FIBER     (1 << 15) /* if the task has it own fiber */

 /* the properties to consider when comparing to a critical task */
# define MPC_OMP_TASK_PROP_PROFILE_MASK \
    (MPC_OMP_TASK_PROP_UNDEFERRED | MPC_OMP_TASK_PROP_UNTIED | MPC_OMP_TASK_PROP_EXPLICIT | MPC_OMP_TASK_PROP_INCLUDED | MPC_OMP_TASK_PROP_FINAL | MPC_OMP_TASK_PROP_MERGEABLE | MPC_OMP_TASK_PROP_DEPEND | MPC_OMP_TASK_PROP_PRIORITY | MPC_OMP_TASK_PROP_IF)

# include <stdbool.h>

/** the task statuses */
typedef struct  mpc_omp_task_statuses_s
{
    volatile bool started;           /* if the task started */
    volatile bool completed;         /* if the task completed */
    volatile bool blocking;          /* if the task is blocking (still running but will be blocked) */
    volatile bool blocked;           /* if the task is blocked (suspended) */
    volatile bool unblocked;         /* if the task was unblocked */
    volatile bool in_blocked_list;   /* if the task is in a blocked list */
}               mpc_omp_task_statuses_t;

/** Property of an OpenMP task */
typedef unsigned int mpc_omp_task_property_t;

/*** Task property primitives ***/
static inline void
mpc_omp_task_reset_property(mpc_omp_task_property_t *property)
{
    *property = 0;
}

static inline void
mpc_omp_task_set_property(mpc_omp_task_property_t *property,
        mpc_omp_task_property_t mask)
{
    *property |= mask;
}

static inline void
mpc_omp_task_unset_property(mpc_omp_task_property_t *property,
        mpc_omp_task_property_t mask)
{
    *property &= ~(mask);
}

static inline int
mpc_omp_task_property_isset(mpc_omp_task_property_t property,
        mpc_omp_task_property_t mask)
{
    return (property & mask);
}

# endif /* __MPC_OMP_TASK_PROPERTY_H__ */
