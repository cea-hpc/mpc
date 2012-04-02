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
/* #   - Valat Sébastien sebastien.valat@cea.fr                           # */
/* #                                                                      # */
/* ######################################################################## */

#ifndef SCTK_ALLOC_DEBUG_H
#define SCTK_ALLOC_DEBUG_H

/************************** HEADERS ************************/
#include <stdio.h>
#include <assert.h>
#include "sctk_allocator.h"

/************************** MACROS *************************/
#ifdef NDEBUG
	#define SCTK_PDEBUG //sctk_alloc_pdebug
	#define SCTK_CRASH_DUMP //sctk_alloc_crash_dump
#else //NDEBUG
	#define SCTK_PDEBUG sctk_alloc_pdebug
	#define SCTK_CRASH_DUMP sctk_alloc_crash_dump
	#ifndef ENABLE_TRACE
		#define ENABLE_TRACE
	#endif
#endif //NDEBUG

#ifndef ENABLE_TRACE
	#define SCTK_PTRACE sctk_alloc_ptrace
#else
	#define SCTK_PTRACE //sctk_alloc_ptrace
#endif

/************************** MACROS *************************/
#define warning(m) sctk_alloc_pwarning("Warning at %s!%d\n%s\n",__FILE__,__LINE__,m);
#define assume(x,m) if (!(x)) { sctk_alloc_perror("Error at %s!%d\n%s\n%s\n",__FILE__,__LINE__,#x,m); abort(); }
#define fatal(m) { sctk_alloc_perror("Fatal error at %s!%d\n%s\n",__FILE__,__LINE__,m); abort(); }
#define sctk_alloc_assert(x) if (!(x)) { sctk_alloc_perror("Assertion failure at %s!%d\n%s\n",__FILE__,__LINE__,#x); abort(); }

/************************* FUNCTION ************************/
void sctk_alloc_ptrace_init(void);

/************************* FUNCTION ************************/
void sctk_alloc_pdebug(const char * format,...);
void sctk_alloc_ptrace(const char * format,...);
void sctk_alloc_perror(const char * format,...);
void sctk_alloc_pwarning(const char * format,...);
void sctk_alloc_fprintf(int fd,const char * format,...);

/************************* FUNCTION ************************/
void sctk_alloc_debug_dump_segment(int fd,void * base_addr,void * end_addr);
void sctk_alloc_debug_dump_free_lists(int fd,struct sctk_alloc_free_chunk * free_lists);
void sctk_alloc_debug_dump_thread_pool(int fd,struct sctk_thread_pool * pool);
void sctk_alloc_debug_dump_alloc_chain(struct sctk_alloc_chain * chain);

/************************* FUNCTION ************************/
void sctk_alloc_debug_init(void);
void sctk_alloc_crash_dump(void);

#endif
