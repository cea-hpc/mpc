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
/* #   - Adam Julien julien.adam.ocre@cea.fr                              # */
/* #                                                                      # */
/* ######################################################################## */

/************************** HEADERS ************************/
#include <stdio.h>
#include <stdlib.h>
#include "sctk_alloc_numa_stat.h"
#include "sctk_alloc_lock.h"
#include "sctk_alloc_debug.h"
#include "sctk_alloc_topology.h"

//optional
#ifdef HAVE_NUMAIF_H
#include <numaif.h>
#endif

/************************* GLOBALS *************************/
/** Global pointer to store the file pointer to /proc/self/pagemap. **/
static FILE * sctk_alloc_glob_pagemap_fp = NULL;
/** Lock to ensure serial access to the file pointer. **/
static SCTK_ALLOC_INIT_LOCK_TYPE sctk_alloc_glob_pagemap_fp_spinlock = SCTK_ALLOC_INIT_LOCK_INITIALIZER;
/** We consider 4K pages **/
/** Define the page shifting to compute page size (considering static 4K page). **/
#define SCTK_ALLOC_NUMA_STAT_PAGE_SHIFT 12
#define SCTK_ALLOC_PAGEMAP_NOT_AVAIL ((FILE*)0x1)

/************************* FUNCTION ************************/
/**
 * Internal function used to open the page map file. This file is provided by
 * Linux kernel >= 2.6.25.
 * This file is thread safe as it was protected by a global spinlock.
**/
SCTK_STATIC void sctk_alloc_numa_stat_open_pagemap(void )
{
	//otherwise take the lock
	SCTK_ALLOC_INIT_LOCK_LOCK(&sctk_alloc_glob_pagemap_fp_spinlock);

	//if not setup
	if (sctk_alloc_glob_pagemap_fp == NULL)
	{
		//open the file
		sctk_alloc_glob_pagemap_fp = fopen("/proc/self/pagemap","r");
		if ( sctk_alloc_glob_pagemap_fp == NULL )
		{
			perror("/proc/self/pagemap");
			warning("Check you have Linux kernel newer than 2.6.25, and check your PID is valid.\n");
			sctk_alloc_glob_pagemap_fp = SCTK_ALLOC_PAGEMAP_NOT_AVAIL;
		} else {
			//register to close the file
			atexit(sctk_alloc_numa_stat_at_exit);
		}
	}
	
	//unlock
	SCTK_ALLOC_INIT_LOCK_UNLOCK(&sctk_alloc_glob_pagemap_fp_spinlock);
}

/************************* FUNCTION ************************/
SCTK_STATIC void sctk_alloc_numa_stat_at_exit(void )
{
	//otherwise take the lock
	SCTK_ALLOC_INIT_LOCK_LOCK(&sctk_alloc_glob_pagemap_fp_spinlock);

	//close the file
	if (sctk_alloc_glob_pagemap_fp != NULL)
	{
		fclose(sctk_alloc_glob_pagemap_fp);
		sctk_alloc_glob_pagemap_fp = NULL;
	}

	//unlock
	SCTK_ALLOC_INIT_LOCK_UNLOCK(&sctk_alloc_glob_pagemap_fp_spinlock);
}

/************************* FUNCTION ************************/
#ifdef HAVE_NUMAIF_H
int sctk_alloc_numa_stat_get_node_of_page(void* ptr)
{
	//vars
	int res;
	int status = -1;

	//errors
	assert(ptr != NULL);

	//move_pages is supported since Linux kernel 2.6.18
	//This si a trick, we can to a request with this function, see Linux manpage
	res = move_pages(0 , 1, &ptr,NULL, &status, 0);

	//check status and return
	if (res == -1)
	{
		return SCTK_MAX_NUMA_NODE;
	} else if (status >= 0 && status < SCTK_MAX_NUMA_NODE) {
		return status;
	} else {
		return SCTK_MAX_NUMA_NODE;
	}
}
#else
int sctk_alloc_numa_stat_get_node_of_page(void* ptr)
{
	warning("Caution, get_node_of_page is supported only on Linux with libnuma support.");
	return -1;
}
#endif

/************************* FUNCTION ************************/
void sctk_alloc_numa_stat_init(struct sctk_alloc_numa_stat_s* stat)
{
	//errors
	assert(stat != NULL);

	//ensure opening of pagemap
	if (sctk_alloc_glob_pagemap_fp == NULL)
		sctk_alloc_numa_stat_open_pagemap();

	//check number of nodes
	if (sctk_is_numa_node())
	{
		stat->numa_nodes = sctk_get_numa_node_number();
	} else {
		stat->numa_nodes = 1;
		warning("No NUMA node availables\n");
	}

	//reset
	sctk_alloc_numa_stat_reset(stat);
}

/************************* FUNCTION ************************/
void sctk_alloc_numa_stat_reset(struct sctk_alloc_numa_stat_s* stat)
{
	//vars
	int i;

	//errors
	assert(stat != NULL);

	//other parameters are set to 0.
	stat->total_pages  = 0;
	stat->total_mapped = 0;

	//reset NUMA counters
	for ( i = 0 ; i <= SCTK_MAX_NUMA_NODE ; i++)
		stat->numa_pages[i] = 0;
}

/************************* FUNCTION ************************/
void sctk_alloc_numa_stat_print(const struct sctk_alloc_numa_stat_s* stat,void * ptr,sctk_size_t size)
{
	//vars
	int i;

	//errors
	assert(stat != NULL);

	//printf("============== NUMA STAT ================\n");
	if (ptr != NULL && size != 0)
		printf("%-20s : %p - %p [%lu]\n","Range",ptr,(void*)((sctk_addr_t)ptr+size),size);
	printf("%-20s : %d\n","NUMA nodes",stat->numa_nodes);
	printf("%-20s : %-10lu (%.01f Mo)\n","Total pages",stat->total_pages,(float)(stat->total_pages * 4) / 1024.0);
	printf("%-20s : %-10lu (%.01f Mo / %.01f %%)\n","Total mapped",stat->total_mapped,(float)(stat->total_mapped * 4) / 1024.0,100.0*(float)stat->total_mapped/(float)stat->total_pages);
	printf("%-20s : %lu\n","Non mapped",stat->total_pages - stat->total_mapped);
	for (i = 0 ; i < stat->numa_nodes ; i++)
		printf("NUMA %-15d : %-10lu (%.01f Mo / %.01f %%)\n",i,stat->numa_pages[i],(float)(stat->numa_pages[i] * 4) / 1024.0,100.0*(float)stat->numa_pages[i] / (float)stat->total_mapped);
	if (stat->numa_pages[SCTK_MAX_NUMA_NODE] != 0)
		printf("%-20s : %lu\n","NUMA unknown",stat->numa_pages[SCTK_MAX_NUMA_NODE]);
}

/************************* FUNCTION ************************/
struct sctk_alloc_numa_stat_linux_page_entry_s * sctk_alloc_numa_stat_read_pagemap(sctk_size_t first_page, sctk_size_t last_page)
{
	//vars
	int status;
	struct sctk_alloc_numa_stat_linux_page_entry_s * table = NULL;

	//errors
	assert(first_page > 0);
	assert(last_page > 0);
	assert(first_page < last_page);
	assert(sctk_alloc_glob_pagemap_fp != NULL);

	//if not supported
	if (sctk_alloc_glob_pagemap_fp == SCTK_ALLOC_PAGEMAP_NOT_AVAIL)
		return NULL;

	//compute the size of the table
	//alloc buffer and read part of page table and read it
	table = calloc(sizeof(struct sctk_alloc_numa_stat_linux_page_entry_s),(last_page-first_page));

	//take the lock
	SCTK_ALLOC_INIT_LOCK_LOCK(&sctk_alloc_glob_pagemap_fp_spinlock);

	//seek into the table
	status = fseek(sctk_alloc_glob_pagemap_fp,first_page*sizeof(struct sctk_alloc_numa_stat_linux_page_entry_s),SEEK_SET);
	if (status != 0)
	{
		perror("fseek");
		fatal("Error while seeking over /proc/self/pagemap to read the Linux page table.");
	}

	//read the section which interest us
	status = fread(table,sizeof(struct sctk_alloc_numa_stat_linux_page_entry_s),last_page-first_page,sctk_alloc_glob_pagemap_fp);
	if (status == 0)
		perror("fread");
	assume_m(status == last_page-first_page,"Failed to read the required entries of Linux page table in /proc/self/pagemap.");

	//unlock
	SCTK_ALLOC_INIT_LOCK_UNLOCK(&sctk_alloc_glob_pagemap_fp_spinlock);

	return table;
}

/************************* FUNCTION ************************/
void sctk_alloc_numa_stat_cumul(struct sctk_alloc_numa_stat_s* stat, void* ptr, size_t size)
{
	//vars
	int i;
	size_t first_page = (size_t)ptr >> SCTK_ALLOC_NUMA_STAT_PAGE_SHIFT;
	size_t last_page = first_page + (size >> SCTK_ALLOC_NUMA_STAT_PAGE_SHIFT);
	struct sctk_alloc_numa_stat_linux_page_entry_s * table = NULL;
	
	//errors
	assert(stat != NULL);
	assert(ptr != NULL);
	assert(size > 0);

	//read the table
	table = sctk_alloc_numa_stat_read_pagemap(first_page,last_page);

	//if not supported
	if (table == NULL)
	{
		stat->total_mapped += (size >> SCTK_ALLOC_NUMA_STAT_PAGE_SHIFT);
	} else {
		//count the pages
		for (i = 0 ; i < last_page - first_page ; i++)
		{
			stat->total_pages++;
			if (table[i].present)
			{
				assume_m(table[i].order == SCTK_ALLOC_NUMA_STAT_PAGE_SHIFT,"Caution we support only 4K pages up to now, but get another size.");
				stat->total_mapped++;
				stat->numa_pages[sctk_alloc_numa_stat_get_node_of_page(ptr + (i << SCTK_ALLOC_NUMA_STAT_PAGE_SHIFT))]++;
			}
		}

		//free the temporaray segment
		free(table);
	}
}

/************************* FUNCTION ************************/
void sctk_alloc_numa_stat_get(struct sctk_alloc_numa_stat_s* stat, void* ptr, size_t size)
{
	sctk_alloc_numa_stat_reset(stat);
	sctk_alloc_numa_stat_cumul(stat,ptr,size);
}

/************************* FUNCTION ************************/
void sctk_alloc_numa_stat_print_detail(void* ptr, size_t size)
{
	//vars
	int i;
	size_t first_page = (size_t)ptr >> SCTK_ALLOC_NUMA_STAT_PAGE_SHIFT;
	size_t last_page = first_page + (size >> SCTK_ALLOC_NUMA_STAT_PAGE_SHIFT);
	struct sctk_alloc_numa_stat_linux_page_entry_s * table = NULL;

	//errors
	assert(ptr != NULL);
	assert(size > 0);

	//no supported
	if (sctk_alloc_glob_pagemap_fp == SCTK_ALLOC_PAGEMAP_NOT_AVAIL)
	{
		warning("/prof/self/pagemap isn't supported on your platforme or disabled at compile time.");
		return;
	}

	//print
	printf("NUMA mapping of %p - %p [%lu] : ",ptr,(void*)((sctk_addr_t)ptr + size),size);

	//read the table
	table = sctk_alloc_numa_stat_read_pagemap(first_page,last_page);

	//count the pages
	for (i = 0 ; i < last_page - first_page ; i++)
	{
		if (table[i].present == 1)
		{
			assume_m(table[i].order == SCTK_ALLOC_NUMA_STAT_PAGE_SHIFT,"Caution we support only 4K pages up to now, but get another size.");
			printf(" %d ",sctk_alloc_numa_stat_get_node_of_page(ptr + (i << SCTK_ALLOC_NUMA_STAT_PAGE_SHIFT)));
		} else  {
			printf(" - ");
		}
	}

	//end line
	printf("\n");

	//free the temporaray segment
	free(table);
}
