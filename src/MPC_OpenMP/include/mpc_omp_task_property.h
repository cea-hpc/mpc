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

/* MPCOMP bit flags */
# define MPCOMP_TASK_PROP_UNDEFERRED    (1 << 0)
# define MPCOMP_TASK_PROP_UNTIED        (1 << 1)
# define MPCOMP_TASK_PROP_EXPLICIT      (1 << 2)
# define MPCOMP_TASK_PROP_IMPLICIT      (1 << 3)
# define MPCOMP_TASK_PROP_INITIAL       (1 << 4)
# define MPCOMP_TASK_PROP_INCLUDED      (1 << 5)
# define MPCOMP_TASK_PROP_FINAL         (1 << 6)
# define MPCOMP_TASK_PROP_STARTED       (1 << 7)
# define MPCOMP_TASK_PROP_YIELDED       (1 << 8)
# define MPCOMP_TASK_PROP_COMPLETED     (1 << 9)
# define MPCOMP_TASK_PROP_MERGED        (1 << 10)
# define MPCOMP_TASK_PROP_MERGEABLE     (1 << 11)
# define MPCOMP_TASK_PROP_DEPEND        (1 << 12)
# define MPCOMP_TASK_PROP_PRIORITY      (1 << 13)
# define MPCOMP_TASK_PROP_UP            (1 << 14)
# define MPCOMP_TASK_PROP_GRAINSIZE     (1 << 15)
# define MPCOMP_TASK_PROP_IF            (1 << 16)
# define MPCOMP_TASK_PROP_NOGROUP       (1 << 17)
# define MPCOMP_TASK_PROP_BLOCKED       (1 << 18)
# define MPCOMP_TASK_PROP_UNBLOCKED     (1 << 19)
# define MPCOMP_TASK_PROP_HAS_FIBER     (1 << 20)

/** Property of an OpenMP task */
typedef unsigned int mpcomp_task_property_t;

/*** Task property primitives ***/
static inline void
_mpc_omp_task_reset_property(mpcomp_task_property_t *property)
{
    *property = 0;
}

static inline void
_mpc_omp_task_set_property(mpcomp_task_property_t *property,
        mpcomp_task_property_t mask)
{
    *property |= mask;
}

static inline void
_mpc_omp_task_unset_property(mpcomp_task_property_t *property,
        mpcomp_task_property_t mask)
{
    *property &= ~(mask);
}

static inline int
_mpc_omp_task_property_isset(mpcomp_task_property_t property,
        mpcomp_task_property_t mask)
{
    return (property & mask);
}

# endif /* __MPC_OMP_TASK_PROPERTY_H__ */
