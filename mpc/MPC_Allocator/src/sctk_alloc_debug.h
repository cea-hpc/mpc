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
#ifndef __SCTK__ALLOC__DEBUG__
#define __SCTK__ALLOC__DEBUG__

#ifndef __SCTK__ALLOC__C__
#error "For internal use only"
#endif

#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include "sctk_config.h"
#include "sctk_debug.h"

#define SCTK_DEBUG(a) (void)(0)
#define SCTK_FORCE_DEBUG(a) a
#define LOG_IN() (void)(0)
#define LOG_OUT() (void)(0)
#define LOG_ARG(a,b) (void)(0)
#define sctk_mem_assert(a) assume(a)

#ifdef NO_INTERNAL_ASSERT
#define sctk_alloc_assert(a) (void)(0)
#define sctk_check_chunk_coherency(a) (void)(0)
#define sctk_check_alignement(a) (void)(0)
#else
#warning "Enable memory checks"
#define sctk_alloc_assert(a) assume(a)
#define sctk_check_chunk_coherency(a) sctk_check_chunk_coherency_intern(((sctk_malloc_chunk_t *)(a)),__FILE__,__LINE__)
#define sctk_check_alignement(a) sctk_alloc_assert(SCTK_IS_ALIGNED_OK(a))
#define SCTK_COHERENCY_CHECK
#endif

static inline void
sctk_mem_error (const char *format, ...)
{
  char tmp[4096];
  va_list ap;
  sprintf (tmp, "[%d] %s", getpid (), format);
  va_start (ap, format);
  sctk_noalloc_vfprintf (stderr, tmp, ap);
  va_end (ap);
}


#define sctk_check_list(list) do{					\
    sctk_page_t *__sctk_check_list__pos;				\
    __sctk_check_list__pos = (list);					\
    if(__sctk_check_list__pos != NULL){					\
      do{								\
	sctk_mem_assert((unsigned long)__sctk_check_list__pos->next_chunk >= (unsigned long)__sctk_check_list__pos); \
	__sctk_check_list__pos = __sctk_check_list__pos->next_chunk;	\
      }while ((list)->prev_chunk != __sctk_check_list__pos);		\
    }									\
  }while(0)
#define sctk_print_list(list) do{					\
    sctk_page_t *__sctk_check_list__pos;				\
    __sctk_check_list__pos = (list);					\
    if(__sctk_check_list__pos != NULL){					\
      sctk_mem_error("Begin page list\n");				\
      do{								\
	sctk_mem_error("  Page in list %p\n",__sctk_check_list__pos);	\
	__sctk_check_list__pos = __sctk_check_list__pos->next_chunk;	\
      }while ((list)->prev_chunk != __sctk_check_list__pos);		\
      sctk_mem_error("End page list\n");				\
    }									\
  }while(0)

#endif
