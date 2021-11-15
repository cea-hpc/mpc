/* ############################# MPC License ############################## */
/* # Tue Oct 12 10:34:08 CEST 2021                                        # */
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
#ifndef OMP_GOMP_CONSTANTS_H_
#define OMP_GOMP_CONSTANTS_H_

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

/* tasks dependencies flags */
typedef enum    gomp_task_dep_type_e
{
  GOMP_OMP_TASK_DEP_NONE            = 0,
  GOMP_OMP_TASK_DEP_IN              = 1,
  GOMP_OMP_TASK_DEP_OUT             = 2,
  GOMP_OMP_TASK_DEP_INOUT           = 3,
  GOMP_OMP_TASK_DEP_MUTEXINOUTSET   = 4,
  GOMP_OMP_TASK_DEP_INOUTSET        = 5,
  GOMP_OMP_TASK_DEP_COUNT           = 6
}               gomp_task_dep_type_t;

/** depobj */
typedef struct  gomp_omp_depend_t
{
    char __omp_depend_t__[2 * sizeof (void *)];
}               gomp_omp_depend_t;

#endif /* OMP_GOMP_CONSTANTS_H_ */
