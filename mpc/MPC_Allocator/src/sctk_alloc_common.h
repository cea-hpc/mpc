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

#ifndef SCTK_ALLOC_COMMON_H
#define SCTK_ALLOC_COMMON_H

/************************** HEADERS ************************/
#include <stdlib.h>

#ifdef __cplusplus
extern "C"
{
#endif

/************************* PORTABILITY *********************/
#ifdef _MSC_VER
	#include <errno.h>
	//fix file descriptor
	#ifndef STDIN_FILENO
		#define STDIN_FILENO 0
		#define STDOUT_FILENO 1
		#define STDERR_FILENO 2
	#endif
	#define getpid() _getpid()
	//export dll
	#define SCTK_PUBLIC __declspec(dllexport)
#else
	#define SCTK_PUBLIC
#endif

//spcial inline symbol for VCC
#ifndef __inline__
	#ifdef _MSC_VER
		#define __inline__ __inline
	#else
		#define __inline__ inline
	#endif
#endif

/*************************** ENUM **************************/
/**
 * C unavailability of boolean type sucks.
**/
#ifndef MPC_Common
	#ifndef __cplusplus
		#ifdef HAVE_STDBOOL_H
		#include <stdbool.h>
	#elif !defined(bool)
		#define bool unsigned char
		#define true 1
		#define false 0
		#endif
	#endif //__cplusplus
#else
	#include "sctk_bool.h"
#endif

/************************** CONSTS *************************/
/** Magick value to be used as check in common header. **/
#define SCTK_ALLOC_MAGIK_STATUS 0x10
/**
 * Define the cut size for small chunk, for now it can be up to 256o. It was currently 32 because
 * small bloc support is not fully available.
**/
#define SCTK_ALLOC_SMALL_CHUNK_SIZE 32
/** Define the size of a macro bloc (2Mo by default) **/
#define SCTK_MACRO_BLOC_SIZE (2*1024ULL*1024ULL)
/** Number of size class for the free list. **/
#define SCTK_ALLOC_NB_FREE_LIST 48
/** Minimal size of blocks. **/
#define SCTK_ALLOC_MIN_SIZE 32
/** Basic alignement for large blocs. **/
#define SCTK_ALLOC_BASIC_ALIGN 16
/** Define the considered page size. **/
#define SCTK_ALLOC_PAGE_SIZE 4096
/** Size allocated for region headers. **/
#define SCTK_REGION_HEADER_SIZE (4*1024ULL*1024ULL)
/** Size of a region header (1TB for 2MB macro-blocs and 1 pointer per entry) **/
#define SCTK_REGION_SIZE (1024ULL*1024ULL*1024ULL*1024ULL)
/** Number of entries of a region header. **/
#define SCTK_REGION_HEADER_ENTRIES ((SCTK_REGION_SIZE) / SCTK_MACRO_BLOC_SIZE)
/** Base addresse for the current process heap based on mmap. **/
#define SCTK_ALLOC_HEAP_BASE 0xc0000000UL
/** Maximum size of current process heap based on mmap (128Go by default for now). **/
#define SCTK_ALLOC_HEAP_SIZE (128ULL*1024ULL*1024ULL*1024ULL)
/** Maximum number of regions, need to cover the 256TB available with 48bit addressing. **/
#define SCTK_ALLOC_MAX_REGIONS 256
/**
 * Enable or disable huge chunk segragation. Turn off may cause huge memory consumption as
 * memory may be locked due to presence of non free small block at the end of the macro blocs.
**/
#define SCTK_ALLOC_HUGE_CHUNK_SEGREGATION true
/**
 * Define the smaller size to directly use macro blocs instead of splitting in inner chunks.
 * Usefull only in confunction with SCTK_ALLOC_HUGE_CHUNK_SEGREGATION
 * May be better to be between SCTK_MACRO_BLOC_SIZE / 2 and SCTK_MACRO_BLOC_SIZE.
**/
#define SCTK_HUGE_BLOC_LIMIT (SCTK_MACRO_BLOC_SIZE / 2)
/** Permit to keep the old memory source in chain numa migration. **/
#define SCTK_ALLOC_KEEP_OLD_MM_SOURCE ((void*)-1)
/** It disable some assume which may be OK for stable version of allocator and valid applications. **/
#ifndef SCTK_ALLOC_DEBUG
#define SCTK_ALLOC_FAST_BUT_LESS_SAFE
#endif

/************************** MACROS *************************/
//if have NUMA support
#ifdef HAVE_HWLOC
/** Define the maximum number of numa node supported. **/
#define SCTK_MAX_NUMA_NODE 4
#else
/** Define the maximum number of numa node supported, one if no numa support. **/
#define SCTK_MAX_NUMA_NODE 1
#endif

/** Select the NUMA memory source init function. **/
#define SCTK_DEFAULT_NUMA_MM_SOURCE_ID SCTK_MAX_NUMA_NODE

/************************** MACROS *************************/
//If not in MPC we didn't support internal profiling
#ifndef MPC_Common
#define SCTK_PROFIL_START(x) /** No profiling support **/
#define SCTK_PROFIL_END(x)   /** No profiling support **/
#endif

/************************** MACROS *************************/
/**
 * Macro to align x on required alignement. It internally use a bit per bit AND operation.
 * @param x Define the value to align.
 * @param align Define the target alignement (must be power of 2)
**/
#define SCTK_ALLOC_ALIGN(x,align) ((x) & (~((align) -1 )))
// #define SCTK_ALLOC_ALIGN(x,align) ((x) - ((x)%(align)))
#ifdef MPC_ALLOCATOR_DISABLE_STATIC
	#define SCTK_STATIC /*static*/
#else
	#ifdef _MSC_VER
		#define SCTK_STATIC __inline
	#else
		#define SCTK_STATIC inline
	#endif
#endif // MPC_ALLOCATOR_DISABLE_STATIC
//To mark function which are intern to MPC
#define SCTK_INTERN

/************************** TYPES **************************/
/** Type for size member, must be 64bit type to maintain alignment coherency. **/
typedef size_t sctk_size_t;
/** Type for address member, must be 64bit type to maintain alignement coherency. **/
typedef size_t sctk_addr_t;
/** Type for short size member, must be 8bits type to maintain alignement coherency. **/
typedef unsigned char sctk_short_size_t;

/************************** STRUCT *************************/
struct sctk_alloc_chain;

/************************* FUNCTION ************************/
//mmap/munmap wrappers
void* sctk_mmap(void* addr, size_t length);
void sctk_munmap(void * addr, size_t size);

#ifdef __cplusplus
}
#endif

#endif //SCTK_ALLOC_COMMON_H
