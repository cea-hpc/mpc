/* # Wed Nov 19 15:19:19 CET 2008                                         # */
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
/* #   - PERACHE Marc marc.perache@cea.fr                                 # */
/* #   - CARRIBAULT Patrick patrick.carribault@cea.fr                     # */
/* #   - CAPRA Antoine capra@paratools.com                                # */
/* #                                                                      # */
/* ######################################################################## */

#ifndef __MPCOMP_TASK_GOMP_CONSTANTS_H__
#define __MPCOMP_TASK_GOMP_CONSTANTS_H__

#define GOMP_TASK_FLAG_FINAL (1 << 1)
#define GOMP_TASK_FLAG_MERGEABLE (1 << 2)
#define GOMP_TASK_FLAG_DEPEND (1 << 3)
#define GOMP_TASK_FLAG_PRIORITY (1 << 4)
#define GOMP_TASK_FLAG_UP (1 << 8)
#define GOMP_TASK_FLAG_GRAINSIZE (1 << 9)
#define GOMP_TASK_FLAG_IF (1 << 10)
#define GOMP_TASK_FLAG_NOGROUP (1 << 11)

#define MPCOMP_TASK_FLAG_FINAL GOMP_TASK_FLAG_FINAL
#define MPCOMP_TASK_FLAG_MERGEABLE GOMP_TASK_FLAG_MERGEABLE
#define MPCOMP_TASK_FLAG_DEPEND GOMP_TASK_FLAG_DEPEND
#define MPCOMP_TASK_FLAG_PRIORITY GOMP_TASK_FLAG_PRIORITY
#define MPCOMP_TASK_FLAG_UP GOMP_TASK_FLAG_UP
#define MPCOMP_TASK_FLAG_GRAINSIZE GOMP_TASK_FLAG_GRAINSIZE
#define MPCOMP_TASK_FLAG_IF GOMP_TASK_FLAG_IF
#define MPCOMP_TASK_FLAG_NOGROUP GOMP_TASK_FLAG_NOGROUP

#endif /* __MPCOMP_TASK_GOMP_CONSTANTS_H__ */
