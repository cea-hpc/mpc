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
#ifdef MPC_Common
#include "sctk_debug.h"
#endif
#include <stdio.h>
#include "sctk_allocator.h"

//compat with NDEBUG system because using this has consequences in MPC sources.
#ifdef NDEBUG
#undef SCTK_ALLOC_DEBUG
#elif ! defined(MPC_Common)
#define SCTK_ALLOC_DEBUG
#endif //NDEBUG

/************************** MACROS *************************/
#if defined(MPC_Common)
	#define SCTK_PDEBUG(...) sctk_debug(__VA_ARGS__)
	#define SCTK_CRASH_DUMP sctk_alloc_crash_dump
#elif defined(SCTK_ALLOC_DEBUG)
	#define SCTK_PDEBUG(...) sctk_alloc_pdebug(__VA_ARGS__)
	#define SCTK_CRASH_DUMP sctk_alloc_crash_dump
#else //SCTK_ALLOC_DEBUG
	#define SCTK_PDEBUG(...)
	#define SCTK_CRASH_DUMP
#endif //SCTK_ALLOC_DEBUG

#if defined(ENABLE_TRACE) && defined(SCTK_ALLOC_DEBUG)
	#define SCTK_PTRACE(m,...) sctk_alloc_ptrace((m),__VA_ARGS__)
#else
	#define SCTK_PTRACE(m,...)
#endif

/************************** MACROS *************************/
#ifdef sctk_warning
#define warning(m) sctk_warning(m)
#else //sctk_warning
#define warning(m) sctk_alloc_pwarning("Warning at %s!%d\n%s\n",__FILE__,__LINE__,m)
#endif //sctk_warning

/************************** MACROS *************************/
#define fatal(m) { sctk_alloc_perror("Fatal error at %s!%d\n%s\n",__FILE__,__LINE__,m); abort(); }

/************************** MACROS *************************/
#ifndef assume_m
#define assume_m(x,m) if (!(x)) { sctk_alloc_perror("Error at %s!%d\n%s\n%s\n",__FILE__,__LINE__,#x,m); abort(); }
#endif

/************************** MACROS *************************/
#ifndef assert
#ifdef SCTK_ALLOC_DEBUG
#define assert(x) if (!(x)) { sctk_alloc_perror("Assertion failure at %s!%d\n%s\n",__FILE__,__LINE__,#x); abort(); }
#else
#define assert(x) while(0){}
#endif
#endif

//Some functions on Windows need to be used "safely" specifying size buffer
#ifdef _WIN32
#define sctk_alloc_sprintf(buffer,buffer_size,...) sprintf_s(buffer,buffer_size,__VA_ARGS__)
#define sctk_alloc_vsprintf(buffer,buffer_size,...) vsprintf_s(buffer,buffer_size,__VA_ARGS__)
#else
#define sctk_alloc_sprintf(buffer,buffer_size,...) sprintf(buffer,__VA_ARGS__)
#define sctk_alloc_vsprintf(buffer,buffer_size,...) vsprintf(buffer,__VA_ARGS__)
#endif

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
