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

#ifndef SCTK_ALLOC_NUMA_STAT_H
#define SCTK_ALLOC_NUMA_STAT_H

#ifdef __cplusplus
extern "C"
{
#endif

/************************** HEADERS ************************/
#include "sctk_alloc_common.h"

/************************** STRUCT *************************/
/** Structure to return the stats of a memory segment. **/
struct sctk_alloc_numa_stat_s
{
	/** Number of numa nodes of the current running station. **/
	int numa_nodes;
	/** Total number of pages in the segment. **/
	size_t total_pages;
	/** Number of mapped pages in the segment. **/
	size_t total_mapped;
	/** Number of pages used in each NUMA nodes (indexed by NUMA node ID, starting by 0). **/
	size_t numa_pages[SCTK_MAX_NUMA_NODE+1];
};

/************************** STRUCT *************************/
/**
 * Structure to read the linux page table on /proc/self/pagemap, see linux documentation
 * for more detailes on it (Documentation / vm / pagemap.txt).
 * Available since Linux 2.6.25.
**/
#ifdef HAVE_LINUX_PAGEMAP
struct sctk_alloc_numa_stat_linux_page_entry_s
{
	/** Page frame number if present and not swaped. **/
	unsigned long pfn : 55;
	/** Page shift (1<<page_shift provide the size). **/
	unsigned int order         :  6;
	/** Reserved for future usage. **/
	unsigned char reserved     :  1;
	/** Set to 1 if the page is swapped. **/
	unsigned char swapped      :  1;
	/** Set to 1 if the page if present. **/
	unsigned char present      :  1;
};
#endif //HAVE_LINUX_PAGEMAP

/************************* FUNCTION ************************/
//internal functions
int sctk_alloc_numa_stat_get_node_of_page(void *ptr);
extern void sctk_alloc_numa_stat_at_exit(void);
void sctk_alloc_numa_stat_open_pagemap(void);
#ifdef HAVE_LINUX_PAGEMAP
struct sctk_alloc_numa_stat_linux_page_entry_s *
sctk_alloc_numa_stat_read_pagemap(sctk_size_t first_page,
                                  sctk_size_t last_page);
#endif //HAVE_LINUX_PAGEMAP

/************************* FUNCTION ************************/
//user interface
SCTK_PUBLIC void sctk_alloc_numa_stat_init(struct sctk_alloc_numa_stat_s * stat);
SCTK_PUBLIC void sctk_alloc_numa_stat_print(const struct sctk_alloc_numa_stat_s * stat,void * ptr,sctk_size_t size);
SCTK_PUBLIC void sctk_alloc_numa_stat_reset(struct sctk_alloc_numa_stat_s * stat);
SCTK_PUBLIC void sctk_alloc_numa_stat_get(struct sctk_alloc_numa_stat_s * stat,void * ptr,size_t size);
SCTK_PUBLIC void sctk_alloc_numa_stat_cumul(struct sctk_alloc_numa_stat_s * stat,void * ptr,size_t size);
SCTK_PUBLIC void sctk_alloc_numa_stat_print_detail(void * ptr,size_t size);

/************************* FUNCTION ************************/
//helpers for NUMA debugging on Linux only
#ifdef SCTK_ALLOC_DEBUG
#define assert_numa(ptr,size,required_numa,min_ratio,message) sctk_alloc_numa_check(true,__FILE__,__LINE__,(ptr),(size),(required_numa),(min_ratio),(message))
#define warn_numa(ptr,size,required_numa,min_ratio,message) sctk_alloc_numa_check(false,__FILE__,__LINE__,(ptr),(size),(required_numa),(min_ratio),(message))
#else
#define assert_numa(ptr,size,required_numa,min_ratio,message) do {} while(0)
#define warning_numa(ptr,size,required_numa,min_ratio,message) do {} while(0)
#endif
SCTK_PUBLIC void sctk_alloc_numa_check(bool fatal_on_fail,const char * filename,int line,void * ptr,size_t size,int required_numa,int min_ratio,const char * message);

#ifdef __cplusplus
}
#endif

#endif //SCTK_ALLOC_NUMA_STAT_H
