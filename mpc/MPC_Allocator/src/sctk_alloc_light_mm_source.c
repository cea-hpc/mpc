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

/************************** HEADERS ************************/
#include <sys/mman.h>
#include "sctk_alloc_inlined.h"
#include "sctk_alloc_lock.h"
#include "sctk_alloc_debug.h"
#include "sctk_alloc_light_mm_source.h"
#include "sctk_alloc_topology.h"

#ifdef HAVE_LIBNUMA
#include <hwloc.h>
#endif

/************************* FUNCTION ************************/
#ifdef HAVE_LIBNUMA
SCTK_STATIC  hwloc_nodeset_t sctk_alloc_mm_source_light_init_nodeset(int numa_node)
{
	//allocate node set
	hwloc_nodeset_t nodeset = hwloc_bitmap_alloc();

	//set value
	hwloc_bitmap_set(nodeset,numa_node);

	//return it
	return nodeset;
}
#endif

/************************* FUNCTION ************************/
void sctk_alloc_mm_source_light_init(struct sctk_alloc_mm_source_light * source,int numa_node,enum sctk_alloc_mm_source_light_flags mode)
{
	//errors
	assert(source != NULL);
	assert(numa_node >= 0 || numa_node == SCTK_ALLOC_MM_SOURCE_LIGHT_NUMA_NODE_IGNORE);
	
	//setup functions
	source->source.cleanup = sctk_alloc_mm_source_light_cleanup;
	source->source.free_memory = sctk_alloc_mm_source_light_free_memory;
	source->source.request_memory = sctk_alloc_mm_source_light_request_memory;
	#ifdef HAVE_LIBNUMA
	source->nodeset = NULL;
	#endif

	//init spinlock
	sctk_alloc_spinlock_init(&source->spinlock,PTHREAD_PROCESS_PRIVATE);

	//if numa is disable we don't know anything, so remove info
	#ifndef HAVE_LIBNUMA
	if (mode |= SCTK_ALLOC_MM_SOURCE_LIGHT_NUMA_STRICT)
		sctk_alloc_pwarning("Caution, you request NUMA strict but allocator was compiled without NUMA support.");
	mode &= ~SCTK_ALLOC_MM_SOURCE_LIGHT_NUMA_STRICT;
	numa_node = SCTK_ALLOC_MM_SOURCE_LIGHT_NUMA_NODE_IGNORE;
	#else
	if (mode |= SCTK_ALLOC_MM_SOURCE_LIGHT_NUMA_STRICT && numa_node != -1)
		source->nodeset = sctk_alloc_mm_source_light_init_nodeset(numa_node);
	else
		source->nodeset = NULL;
	#endif

	//mark the mode and numa node
	source->numa_node = numa_node;
	source->mode = mode;

	//setup blocs counter to know if we can safetely delete. (more for debug)
	source->counter = 0;

	//start with empty local cache
	source->cache = NULL;
}


/************************* FUNCTION ************************/
SCTK_STATIC void sctk_alloc_mm_source_light_reg_in_cache(struct sctk_alloc_mm_source_light * light_source,struct sctk_alloc_mm_source_light_free_macro_bloc * free_bloc)
{
	//taks the lock
	sctk_alloc_spinlock_lock(&light_source->spinlock);

	//insert in list at beginin pos
	free_bloc->next = light_source->cache;
	light_source->cache = free_bloc;

	//taks the lock
	sctk_alloc_spinlock_unlock(&light_source->spinlock);
}

/************************* FUNCTION ************************/
SCTK_STATIC struct sctk_alloc_mm_source_light_free_macro_bloc * sctk_alloc_mm_source_light_setup_free_macro_bloc(void * ptr,sctk_size_t size)
{
	struct sctk_alloc_mm_source_light_free_macro_bloc * free_bloc = ptr;
	free_bloc->next = NULL;
	free_bloc->size = size;
	return free_bloc;
}

/************************* FUNCTION ************************/
SCTK_STATIC struct sctk_alloc_mm_source_light_free_macro_bloc * sctk_alloc_mm_source_light_to_free_macro_bloc(struct sctk_alloc_macro_bloc * macro_bloc)
{
	return sctk_alloc_mm_source_light_setup_free_macro_bloc(macro_bloc,macro_bloc->header.size);
}

/************************* FUNCTION ************************/
SCTK_STATIC void sctk_alloc_mm_source_light_insert_segment(struct sctk_alloc_mm_source_light* light_source, void* base, sctk_size_t size)
{
	//vars
	struct sctk_alloc_mm_source_light_free_macro_bloc * free_bloc = base;

	//errors
	assert(light_source != NULL);
	assert(base != NULL);
	assert(size > 0);

	//some runtime errors
	if (base == NULL || size == 0)
		return;

	//Check requirements on params, need page aligned address, size smaller or equals to SCTK_ALLOC_MACRO_BLOC_SIZE
	assume((sctk_addr_t)base % SCTK_ALLOC_PAGE_SIZE == 0,"Buffer base adderss must be aligned to page size.");
	assume(size % SCTK_MACRO_BLOC_SIZE == 0,"Buffer size must be aligned to page size.");

	//if need to force binding, as we didn't now the origin, call meme binding method
	#ifdef HAVE_LIBNUMA
	if (light_source->mode | SCTK_ALLOC_MM_SOURCE_LIGHT_NUMA_STRICT && light_source->numa_node != SCTK_ALLOC_MM_SOURCE_LIGHT_NUMA_NODE_IGNORE)
		sctk_alloc_force_segment_binding(light_source,base,size,light_source->numa_node);
	#endif //HAVE_LIBNUMA

	//setup header
	free_bloc->size = size;
	free_bloc->next = NULL;

	//insert
	sctk_alloc_mm_source_light_reg_in_cache(light_source,free_bloc);
}

/************************* FUNCTION ************************/
#ifdef HAVE_LIBNUMA
SCTK_STATIC void sctk_alloc_force_segment_binding(struct sctk_alloc_mm_source_light * light_source,void* base, sctk_size_t size,int numa_node)
{
	//vars
	int res;

	//errors
	assert(base != NULL);
	assert(size > SCTK_ALLOC_PAGE_SIZE);
	assert(size % SCTK_ALLOC_PAGE_SIZE == 0);
	assert(numa_node >= 0);

	//use hwloc to bind the segment
	if (light_source->nodeset != NULL)
		res = hwloc_set_area_membind_nodeset(sctk_get_topology_object(),base,size,light_source->nodeset,HWLOC_MEMBIND_BIND ,HWLOC_MEMBIND_THREAD);
}
#endif //HAVE_LIBNUMA

/************************* FUNCTION ************************/
SCTK_STATIC void sctk_alloc_mm_source_light_cleanup(struct sctk_alloc_mm_source* source)
{
	//vars
	struct sctk_alloc_mm_source_light_free_macro_bloc * free_bloc;
	struct sctk_alloc_mm_source_light_free_macro_bloc * free_bloc_next;
	struct sctk_alloc_mm_source_light * light_source = (struct sctk_alloc_mm_source_light *)source;

	//errors
	assert(source != NULL);

	//take the lock
	sctk_alloc_spinlock_lock(&light_source->spinlock);

	//check counters to know if all blocs are returned to the mm source.
	assume(light_source->counter == 0, "Invalid counter status, must be 0 to call cleanup.");

	//loop on the blocs in cache to free them
	free_bloc = light_source->cache;
	while (free_bloc != NULL)
	{
		free_bloc_next = free_bloc->next;
		sctk_munmap(free_bloc,free_bloc->size);
		free_bloc = free_bloc_next;
	}

	//free the nodeset
	#ifdef HAVE_LIBNUMA
	if (light_source->nodeset != NULL)
		hwloc_bitmap_free(light_source->nodeset);
	#endif

	//never free the lock as this structure may go away
}

/************************* FUNCTION ************************/
SCTK_STATIC struct sctk_alloc_macro_bloc * sctk_alloc_mm_source_light_to_macro_bloc(struct sctk_alloc_mm_source_light_free_macro_bloc * free_bloc)
{
	//vars
	struct sctk_alloc_macro_bloc * macro_bloc = (struct sctk_alloc_macro_bloc * )free_bloc;
	
	if (free_bloc != NULL)
	{
		sctk_alloc_get_large(sctk_alloc_setup_chunk(free_bloc,free_bloc->size,NULL));
		macro_bloc->chain = NULL;
	}

	return macro_bloc;
}

/************************* FUNCTION ************************/
SCTK_STATIC struct sctk_alloc_macro_bloc* sctk_alloc_mm_source_light_find_in_cache(struct sctk_alloc_mm_source_light * light_source,sctk_size_t size)
{
	//vars
	struct sctk_alloc_mm_source_light_free_macro_bloc * free_bloc = NULL;
	struct sctk_alloc_mm_source_light_free_macro_bloc * prev_bloc = NULL;

	//errors
	assert(light_source != NULL);
	assert(size > 0);

	//trivial case, we know that we didn't start multi-macro bloc chunks in the cache so return directory
	if (size > SCTK_MACRO_BLOC_SIZE)
		return NULL;

	//take the lock
	sctk_alloc_spinlock_lock(&light_source->spinlock);

	//loop in the list the find the first on which is good.
	free_bloc = light_source->cache;
	prev_bloc = NULL;
	while (free_bloc != NULL && free_bloc->size < size)
	{
		prev_bloc = free_bloc;
		free_bloc = free_bloc->next;
	}

	//update counter and remove from list
	if (free_bloc != NULL)
	{
		if (prev_bloc == NULL)
			light_source->cache = free_bloc->next;
		else
			prev_bloc->next = free_bloc->next;
		light_source->counter++;
	}

	//unlock
	sctk_alloc_spinlock_unlock(&light_source->spinlock);

	//return
	return sctk_alloc_mm_source_light_to_macro_bloc(free_bloc);
}

/************************* FUNCTION ************************/
SCTK_STATIC struct sctk_alloc_macro_bloc* sctk_alloc_mm_source_light_request_memory(struct sctk_alloc_mm_source* source, sctk_size_t size)
{
	//vars
	struct sctk_alloc_mm_source_light * light_source = (struct sctk_alloc_mm_source_light *)source;
	struct sctk_alloc_macro_bloc * macro_bloc = NULL;

	//errors
	assert(source != NULL);
	assert(size > 0);

	//runtime errors
	if (size == 0)
		return NULL;

	//try to find memory in source local cache if smaller that one macro bloc (we now that we didn't store such blocs here)
	if (size <= SCTK_MACRO_BLOC_SIZE)
		macro_bloc = sctk_alloc_mm_source_light_find_in_cache(light_source,size);

	//allocate a new one
	if (macro_bloc == NULL)
		macro_bloc = sctk_alloc_mm_source_light_mmap_new_segment(light_source,size);

	//warn if out of memory
	#ifndef NDEBUG
	if (macro_bloc == NULL)
		SCTK_PDEBUG("Memory source get out of memory and can't request more to system.");
	#endif

	//return if
	return macro_bloc;
}

/************************* FUNCTION ************************/
SCTK_STATIC struct sctk_alloc_macro_bloc * sctk_alloc_mm_source_light_mmap_new_segment(struct sctk_alloc_mm_source_light* light_source,sctk_size_t size)
{
	//vars
	struct sctk_alloc_macro_bloc * macro_bloc = NULL;

	//errors
	assert(size % SCTK_ALLOC_PAGE_SIZE == 0);

	//call mmap to get a macro blocs
	macro_bloc = sctk_mmap(NULL,size);

	//setup header
	macro_bloc = sctk_alloc_setup_macro_bloc(macro_bloc,size);

	//increment counters
	sctk_alloc_spinlock_lock(&light_source->spinlock);
	light_source->counter++;
	sctk_alloc_spinlock_unlock(&light_source->spinlock);

	//return
	return macro_bloc;
}

/************************* FUNCTION ************************/
SCTK_STATIC void sctk_alloc_mm_source_light_free_memory(struct sctk_alloc_mm_source * source,struct sctk_alloc_macro_bloc * bloc)
{
	//vars
	struct sctk_alloc_mm_source_light * light_source = (struct sctk_alloc_mm_source_light *)source;
	struct sctk_alloc_mm_source_light_free_macro_bloc * free_bloc;

	//errors
	assert(source != NULL);
	assert(bloc != NULL);

	//free bloc
	free_bloc = sctk_alloc_mm_source_light_to_free_macro_bloc(bloc);

	//if larger than basic macro bloc size, return to system immediately
	if (bloc->header.size > SCTK_MACRO_BLOC_SIZE)
		sctk_munmap(bloc,bloc->header.size);
	else
		sctk_alloc_mm_source_light_reg_in_cache(light_source,free_bloc);

	//increment counters
	sctk_alloc_spinlock_lock(&light_source->spinlock);
	light_source->counter--;
	sctk_alloc_spinlock_unlock(&light_source->spinlock);
}
