/* ############################# MPC License ############################## */
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
/* #   - CARRIBAULT Patrick patrick.carribault@cea.fr                     # */
/* #   - CAPRA Antoine capra@paratools.com                                # */
/* #                                                                      # */
/* ######################################################################## */
#ifndef __MPCOMP_ENUM_MACROS_H__
#define __MPCOMP_ENUM_MACROS_H__

#define SCTK_OMP_VERSION_MAJOR 3
#define SCTK_OMP_VERSION_MINOR 1

#define MPCOMP_TASK 1
#define MPCOMP_USE_INTEL_ABI 1

#define MPCOMP_OPENMP_3_0

/* Activate tasking support if OMP 3.0 is on */
#if defined( MPCOMP_OPENMP_3_0 )
#define MPCOMP_TASK 1
#endif /* MPCOMP_OPENMP_3_0 */

#define MPCOMP_USE_TASKDEP 1


/* Enable/Disable OMPT support */
#define OMPT_SUPPORT 0



/* Maximum number of alive 'for dynamic' and 'for guided'  construct */
#define MPCOMP_MAX_ALIVE_FOR_DYN 3

#define MPCOMP_NOWAIT_STOP_SYMBOL (-2)

/* Uncomment to enable coherency checking */
#define MPCOMP_COHERENCY_CHECKING 0
#define MPCOMP_OVERFLOW_CHECKING 0

/* MACRO FOR PERFORMANCE */
#define MPCOMP_MALLOC_ON_NODE 1
#define MPCOMP_TRANSFER_INFO_ON_NODES 0
#define MPCOMP_ALIGN 1

#define MPCOMP_CHUNKS_NOT_AVAIL 1
#define MPCOMP_CHUNKS_AVAIL 2

/* Define macro MPCOMP_MIC in case of Xeon Phi compilation */
#if __MIC__ || __MIC2__
#define MPCOMP_MIC 1
#endif /* __MIC__ || __MIC2__ */

#endif /* __MPCOMP_ENUM_MACROS_H__ */
