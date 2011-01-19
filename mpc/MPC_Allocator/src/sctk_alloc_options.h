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
/* #   - PERACHE Marc marc.perache@cea.fr                                 # */
/* #                                                                      # */
/* ######################################################################## */
#ifndef __SCTK__ALLOC__OPTIONS__
#define __SCTK__ALLOC__OPTIONS__

#ifdef __cplusplus
extern "C"
{
#endif
#ifndef __SCTK__ALLOC__C__
#error "For internal use only"
#endif

#include <stdlib.h>
#include "sctk_config.h"

/* #define SCTK_MEM_DEBUG */
/* #define SCTK_MEM_LOG */
/* #define SCTK_MEMINFO */
/* #undef SCTK_ALL_OPTS */
/* #define SCTK_USE_VALGRIND */

/******************************** OPTION START ********************************/

/*Compression buffer size for checkpoint*/
#define SCTK_COMPRESSION_BUFFER_SIZE 4*SCTK_PAGE_SIZE

/* Taille du plus petit bloc MUST BE >= chunks[0]*/
#define SCTK_SPLIT_THRESHOLD 32

/* Taille du buffer utilise pour la migration de pages */
#define SCTK_RELOCALISE_BUFFER_SIZE (1024*1024)

/* Doit être une puissance de 2*/
#define SCTK_PAGES_ALLOC_SIZE ((size_t)(2*1024*1024))

#define SCTK_SMALL_PAGES_ALLOC_SIZE (1*SCTK_PAGES_ALLOC_SIZE - (1024))

/* Doit être une puissance de 2*/
#ifdef SCTK_32_BIT_ARCH
#define SCTK_xMO_TAB_SIZE 16
#else
#define SCTK_xMO_TAB_SIZE 32
#endif

#define SCTK_FULL_PAGE_NUMBER_DEFAULT 4

/* #define SCTK_ALLOC_INFOS */
/* #define SCTK_TRACE_ALLOC_BEHAVIOR */
#define SCTK_SWITCH_MODE_INFO
/* #define SCTK_USE_BRK_INSTED_MMAP */

/* #define SCTK_DO_NOT_FREE */

/******************************** OPTION  END  ********************************/

#define sctk_align SCTK_ALIGNEMENT
#define SCTK_ALIGNEMENT_MASK (SCTK_ALIGNEMENT - 1)
#define SCTK_IS_ALIGNED_OK(m)  (((sctk_size_t)(m) & SCTK_ALIGNEMENT_MASK) == 0)
#define sctk_aligned_size(a) (((a) | SCTK_ALIGNEMENT_MASK) + 1)

#ifdef __cplusplus
}				/* end of extern "C" */
#endif
#endif
