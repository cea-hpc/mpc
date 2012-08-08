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
/* #   - Adam Julien julien.adam@cea.fr                                   # */
/* #                                                                      # */
/* ######################################################################## */

/************************** HEADERS ************************/
#include <stdlib.h>
#include <stdio.h>
#ifndef _WIN32
	#include <unistd.h>
	#include <sys/types.h>
#else
	#include <process.h>
#endif

#include <stdarg.h>
#include <string.h>
#include "sctk_alloc_lock.h"
#include "sctk_alloc_debug.h"
#include "sctk_allocator.h"
#include "sctk_alloc_inlined.h"
#include "sctk_alloc_topology.h"
#include "sctk_alloc_numa_stat.h"
#include "sctk_alloc_light_mm_source.h"

//for getpid
//optional header
#ifdef MPC_Common
#include "sctk.h"
#endif

/************************* PORTABILITY *************************/
#ifdef _WIN32
	#define MAP_FAILED NULL
	#define write _write
#endif

//usage of safe write in MPC
#ifdef MPC_Common
	#include "sctk_io_helper.h"
#else //MPC_Common
	#define sctk_safe_write(fd,buf,count) write((fd),(buf),(count))
#endif //MPC_Common

/************************* GLOBALS *************************/
/** @todo to move to a clean global structure, avoid spreading global elements everywheres **/
/**
 * Global pointer to maintain the list of available region. It point the region header which is
 * allocated dynamically. None initialized regions will be represented by a NULL pointer.
**/
static struct sctk_alloc_region * sctk_alloc_glob_regions[SCTK_ALLOC_MAX_REGIONS];
/** Lock to protect sctk_alloc_glob_regions. **/
static sctk_alloc_spinlock_t sctk_alloc_glob_regions_lock;
/** To know if region spinlock was init. **/
static bool sctk_alloc_glob_regions_init = false;
/** @todo  To remove or move elseware **/
/** It serve to find chain list on crash dump in debug mode. **/
struct sctk_alloc_chain * sctk_alloc_chain_list[2] = {NULL,NULL};

/************************** CONSTS *************************/
/**
 * Define the size classes used for the free list for thread pools.
 * As is follow a linear + exp analytic model, we can reverse it
 * with analytic function sctk_alloc_reverse_analytic_free_size()
 * CAUTION, if change values here, you need to update this function.
**/
const sctk_size_t SCTK_ALLOC_FREE_SIZES[SCTK_ALLOC_NB_FREE_LIST] = {
	32,    64,   96,  128,  160,   192,   224,   256,    288,    320,
	352,  384,  416,  448,  480,   512,   544,   576,    608,    640,
	672,  704,  736,  768,  800,   832,   864,   896,    928,    960,
	992, 1024, 2048, 4096, 8192, 16384, 32768, 65536, 131072, 262144,
	524288, 1048576, SCTK_MACRO_BLOC_SIZE, -1, -1, -1, -1, -1
};
/** Define the size classes used for the free list of default memory source. **/
const sctk_size_t SCTK_ALLOC_MMS_FREE_SIZES[SCTK_ALLOC_NB_FREE_LIST] = {
	     1*SCTK_MACRO_BLOC_SIZE,     2*SCTK_MACRO_BLOC_SIZE,     4*SCTK_MACRO_BLOC_SIZE,
	     8*SCTK_MACRO_BLOC_SIZE,    16*SCTK_MACRO_BLOC_SIZE,    32*SCTK_MACRO_BLOC_SIZE,
	    64*SCTK_MACRO_BLOC_SIZE,   128*SCTK_MACRO_BLOC_SIZE,   256*SCTK_MACRO_BLOC_SIZE,
	   512*SCTK_MACRO_BLOC_SIZE,  1024*SCTK_MACRO_BLOC_SIZE,  2048*SCTK_MACRO_BLOC_SIZE,
	  4096*SCTK_MACRO_BLOC_SIZE,  8192*SCTK_MACRO_BLOC_SIZE, 16384*SCTK_MACRO_BLOC_SIZE,
	 32768*SCTK_MACRO_BLOC_SIZE, 65536*SCTK_MACRO_BLOC_SIZE,131072*SCTK_MACRO_BLOC_SIZE,
	262144*SCTK_MACRO_BLOC_SIZE,524288*SCTK_MACRO_BLOC_SIZE,       -1
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1
	-1,-1,-1,-1, -1
};
/** Default value to initialize the vchunk values. **/
const sctk_alloc_vchunk SCTK_ALLOC_DEFAULT_CHUNK = NULL;

/************************* FUNCTION ************************/
/**
 * Permit to wrap a standar (small or large) chunk to add padding rules. Caution, it work only on allocated
 * chunk, free chunks are not accepted here due to larger header. Caution, we didn't cascading of padding
 * header.
 * @param chunk Define the chunk to wrap with padding rules.
 * @param boundary Define the boudary limit to apply for padding.
**/
SCTK_STATIC sctk_alloc_vchunk sctk_alloc_setup_chunk_padded(sctk_alloc_vchunk vchunk,sctk_size_t boundary)
{
	//vars
	struct sctk_alloc_chunk_header_padded * chunk_padded;
	struct sctk_alloc_chunk_info * chunk_padded_info;
	sctk_addr_t ptr;
	sctk_addr_t ptr_orig;
	//errors
	assert(vchunk != NULL);
	assert(vchunk->state == SCTK_ALLOC_CHUNK_STATE_ALLOCATED);

	//get body addr
	ptr = (sctk_addr_t)sctk_alloc_chunk_body(vchunk);
	ptr_orig = ptr;

	//bound <= 1 : trivial if boundary if 0 or 1 nothing to do, we are already aligned.
	//vchunk.info == NULL : invalid chunk, so we can simply return it, no padding to do.
	//ptr % boundary == 0 : already aligned
	if (boundary > 1 && vchunk != NULL && ptr % boundary != 0)
	{
		//need to pad, so start to add padding header size
		ptr += sizeof(struct sctk_alloc_chunk_header_padded);
		//calc next aligned address if not already aligned
		if (ptr % boundary != 0)
			ptr += boundary - (ptr %boundary);
		//setup padded header
		chunk_padded = (struct sctk_alloc_chunk_header_padded *)((char*)ptr - sizeof(struct sctk_alloc_chunk_header_padded));
		sctk_alloc_set_chunk_header_padded_padding(chunk_padded, (sctk_addr_t)chunk_padded - ptr_orig);
		chunk_padded_info = sctk_alloc_get_chunk_header_padded_info(chunk_padded);
		chunk_padded_info->state = vchunk->state;
		chunk_padded_info->type = SCTK_ALLOC_CHUNK_TYPE_PADDED;
		chunk_padded_info->unused_magik = SCTK_ALLOC_MAGIK_STATUS;
		//setup final vchunk
		vchunk = sctk_alloc_padded_to_vchunk(chunk_padded);
	}
	return vchunk;
}

/************************* FUNCTION ************************/
/** @todo To document **/
SCTK_STATIC void sctk_alloc_create_stopper(void * ptr,void * prev)
{
	sctk_alloc_vchunk res;
	res = sctk_alloc_setup_chunk(ptr,128,prev);
	sctk_alloc_set_chunk_header_large_size(sctk_alloc_get_large(res), 0);
}

/************************* FUNCTION ************************/
SCTK_STATIC sctk_size_t sctk_alloc_calc_chunk_size(sctk_size_t user_size)
{
	/** @todo  As we do not support small blocs now **/
	//if (user_size < SCTK_ALLOC_SMALL_CHUNK_SIZE - sizeof(struct sctk_alloc_chunk_header_small))
	//	return user_size + sizeof(struct sctk_alloc_chunk_header_small);
	//else
	//	return user_size + sizeof(struct sctk_alloc_chunk_header_large);
	user_size += sizeof(struct sctk_alloc_chunk_header_large);
	if (user_size <= 32)
		return 32;
	else
		return user_size;
}

/************************* FUNCTION ************************/
/**
 * Initialize the thead pool. For now it only initialized the free lists.
 * @param pool Define the thread pool to initialize.
 * @param alloc_free_sizes Define the size which determine the class of a free bloc for sorting
 * free lists. If NULL, it use a default value : SCTK_ALLOC_FREE_SIZES.
**/
SCTK_STATIC void sctk_alloc_thread_pool_init(struct sctk_thread_pool* pool,const sctk_size_t alloc_free_sizes[SCTK_ALLOC_NB_FREE_LIST])
{
	int i;

	//default value
	if (alloc_free_sizes == NULL)
		alloc_free_sizes = SCTK_ALLOC_FREE_SIZES;

	//errors
	assert(pool != NULL);
	assert(alloc_free_sizes[SCTK_ALLOC_NB_FREE_LIST-1]==-1);

	//setup the link lists as a cyclic list, by default it is selflinked on its handle element.
	//it permit to avoid managing NULL/non NULL cases for border elements.
	for (i = 0 ; i < SCTK_ALLOC_NB_FREE_LIST ; ++i)
	{
		sctk_alloc_set_chunk_header_large_addr(&pool->free_lists[i].header,0);
		sctk_alloc_get_chunk_header_large_info(&pool->free_lists[i].header)->unused_magik = 0;
		sctk_alloc_set_chunk_header_large_size(&pool->free_lists[i].header, 0);
		pool->free_lists[i].next = pool->free_lists+i;
		pool->free_lists[i].prev = pool->free_lists+i;
		if (alloc_free_sizes[i] != -1)
			pool->nb_free_lists = i;
	}

	//keep the list -1
	if (pool->nb_free_lists < SCTK_ALLOC_NB_FREE_LIST)
	{
		if (pool->nb_free_lists%2 == 0)
			pool->nb_free_lists+=2;
		else
			pool->nb_free_lists++;
		assume_m(pool->nb_free_lists < SCTK_ALLOC_NB_FREE_LIST,"Error while calculating number of free lists.");
	} else {
		warning("Caution, the last free list size must be -1, you didn't follow this requirement, this may leed to errors.");
	}

	//set analitic reverse
	if (alloc_free_sizes == SCTK_ALLOC_FREE_SIZES)
		pool->reverse_analytic_free_size = sctk_alloc_reverse_analytic_free_size;
	else
		pool->reverse_analytic_free_size = NULL;

	//setup free list bitmap
	memset(pool->free_list_status,0,SCTK_ALLOC_NB_FREE_LIST);

	//setup class size
	pool->alloc_free_sizes = alloc_free_sizes;
}

/************************* FUNCTION ************************/
int sctk_alloc_optimized_log2_size_t(sctk_size_t value)
{
	//vars
	sctk_size_t res = 0;

	#if defined(__GNUC__) && defined(__x86_64__)
		if (value != 0)
			asm volatile ("bsr %1, %0":"=r" (res):"r"(value));
	#else
		/** @TODO find equivalent for others compiler. Maybe arch x86 is also OK as for x86_64, but need to check. **/
		#ifndef _MSC_VER
		#warning "ASM bsr was tested only on gcc x64_64, fallback on default slower C implementation."
		#endif
		while (value > 1) {value = value >> 1 ; res++;};
	#endif

	return (int)res;
}

/************************* FUNCTION ************************/
/**
 * Use analytic way to reverse the position of a free list for the case of values
 * provided by internal SCTK_ALLOC_FREE_SIZES.
 * It use a linear + exp model so we can compute easily the position
 * by inverting the mathematical formula.
 * @param size Define the size for which to find the list.
 * @param size_list Define the list this is only to check in debug mode.
**/
SCTK_STATIC int sctk_alloc_reverse_analytic_free_size(sctk_size_t size,const sctk_size_t * size_list)
{
	//errors
	assert(size_list == SCTK_ALLOC_FREE_SIZES);
	assert(64 >> 5 == 2);
	assert(size_list[43] == -1);

	if (size <= 1024)
		//divide by 32 and fix first element ID as we start to indexes by 0
		return (int)((size >> 5) - 1);
	else if (size > SCTK_MACRO_BLOC_SIZE)
		return 43;
	else
		//1024/32 :  starting offset of the exp zone
		// >> 10: ( - log2(1024)) remote the start of the exp
		return 1024/32 + sctk_alloc_optimized_log2_size_t(size >> 10) - 1;
}

/************************* FUNCTION ************************/
/**
 * Return the free list of the given pool which correspond to the size class related the the given
 * size. The size classes are determined by pool->alloc_free_sizes which define the minimal size of
 * bloc contained in the list (greater or equal).
 * @param pool Define the thread pool in which to search the free list.
 * @param size Define the size in which we are interested. We consider the full buffer size.
 * header comprised. For blocs larger than a macro bloc, it will return NULL.
**/
SCTK_STATIC sctk_alloc_free_list_t * sctk_alloc_get_free_list_slow(struct sctk_thread_pool* pool, sctk_size_t size)
{
	/** @TODO maybe this can be optimized by using uin32_t, only ok if refuse usage of old memory source as it manage segment of large size. **/
	sctk_size_t seg_size = pool->nb_free_lists;
	sctk_size_t i = seg_size >> 1;
	const sctk_size_t * ptr = pool->alloc_free_sizes;
	//int j = 0;

	//errors
	assert(pool->alloc_free_sizes != NULL);
	assert(pool != NULL);
	assert(size > 0);
	assert(4 >> 1 == 2);//required property to quicly divide by 2

	/** @todo  Remove this old code **/
	//find the correct size class, none if too large by dicotomic search.
	/*while (pool->alloc_free_sizes[j] != -1 && pool->alloc_free_sizes[j] < size)
		++j;*/
	if (ptr[0] >= size)
	{
		i = 0;
	} else {
		//use dicotomic search, it's faster as we know the list sizes are sorted.
		while((ptr[i-1] >= size || ptr[i] < size))
		{
			if (ptr[i] < size)
			{
				seg_size -= i;
				ptr += i;
			} else {
				seg_size = i;
			}
			i = seg_size >> 1;//divide by 2
		}
	}
	assert(ptr+i >= pool->alloc_free_sizes);
	//assert(ptr+i == pool->alloc_free_sizes+j);
	//assert(pool->free_lists+j == pool->free_lists+(ptr-pool->alloc_free_sizes+i));
	return pool->free_lists+(ptr-pool->alloc_free_sizes+i);
	//return pool->free_lists+j;
}

/************************* FUNCTION ************************/
/**
 * Return the free list of the given pool which correspond to the size class related the the given
 * size. The size classes are determined by pool->alloc_free_sizes which define the minimal size of
 * bloc contained in the list (greater or equal).
 * @param pool Define the thread pool in which to search the free list.
 * @param size Define the size in which we are interested. We consider the full buffer size.
 * header comprised. For blocs larger than a macro bloc, it will return NULL.
**/
SCTK_STATIC sctk_alloc_free_list_t * sctk_alloc_get_free_list_fast(struct sctk_thread_pool* pool, sctk_size_t size)
{
	//vars
	const sctk_size_t * size_list = pool->alloc_free_sizes;
	int pos;

	//errors
	assert(pool->alloc_free_sizes != NULL);
	assert(pool->reverse_analytic_free_size != NULL);
	assert(pool != NULL);
	assert(size > 0);

	//get position by reverse analytic computation.
	pos = pool->reverse_analytic_free_size(size, size_list);

	//check size of current cell, if too small, take the next one
	if (size_list[pos] < size)
		pos++;

	//check
	assert(pos >= 0 && pos <=pool->nb_free_lists);

	assert(pool->free_lists + pos == sctk_alloc_get_free_list_slow(pool,size));

	//return position
	return pool->free_lists + pos;
}

/************************* FUNCTION ************************/
/**
 * Return the free list of the given pool which correspond to the size class related the the given
 * size. The size classes are determined by pool->alloc_free_sizes which define the minimal size of
 * bloc contained in the list (greater or equal).
 * @param pool Define the thread pool in which to search the free list.
 * @param size Define the size in which we are interested. We consider the full buffer size.
 * header comprised. For blocs larger than a macro bloc, it will return NULL.
**/
SCTK_STATIC sctk_alloc_free_list_t * sctk_alloc_get_free_list(struct sctk_thread_pool* pool, sctk_size_t size)
{
	//errors
	assert(pool->alloc_free_sizes != NULL);
	assert(pool != NULL);
	assert(size > 0);

	if (pool->reverse_analytic_free_size != NULL)
		return sctk_alloc_get_free_list_fast(pool,size);
	else
		return sctk_alloc_get_free_list_slow(pool,size);
}

/************************* FUNCTION ************************/
/**
 * @param list Define the list to consider.
 * @param pool Define the pool which contain the given list.
 * @return Return the size class corresponding to the given list.
**/
SCTK_STATIC sctk_size_t sctk_alloc_get_list_class(struct sctk_thread_pool* pool,sctk_alloc_free_list_t * list)
{
	int id = (int)(list - pool->free_lists);
	#ifndef SCTK_ALLOC_FAST_BUT_LESS_SAFE
	assume_m(id >= 0 && id < SCTK_ALLOC_NB_FREE_LIST,"The given list didn't be a member of the given thread pool.");
	#endif
	return pool->alloc_free_sizes[id];
}

/************************* FUNCTION ************************/
/**
 * Calculate the body size of a given chunk. It simply remove the bloc header size and return
 * the result.
 * @param chunk_size Define the chunk size to consider.
**/
SCTK_STATIC sctk_size_t sctk_alloc_calc_body_size(sctk_size_t chunk_size)
{
	/** @todo need to support small blocs here **/
	return chunk_size - sizeof(struct sctk_alloc_chunk_header_large);
}

/************************* FUNCTION ************************/
/**
 * Insert the given raw chunk as a free chunk in the free related free list.
 * @param pool Define the pool in which to register the chunk.
 * @param ptr Define the base address of the chunk to register.
 * @param size define the total size of the chunk comprising the header size.
 * @param prev Define the address of previous bloc to update pointers in header. If none, use NULL.
**/
SCTK_STATIC void sctk_alloc_free_list_insert_raw(struct sctk_thread_pool * pool,void* ptr, sctk_size_t size,void * prev)
{
	sctk_alloc_vchunk chunk;

	//errors
	assert(ptr != NULL);

	//setup bloc header
	chunk = sctk_alloc_setup_chunk(ptr,size,prev);

	sctk_alloc_free_list_insert(pool,sctk_alloc_get_large(chunk),SCTK_ALLOC_INSERT_AT_START);
}

/************************* FUNCTION ************************/
/**
 * Insert the given raw chunk as a free chunk in the free related free list.
 * @param pool Define the pool in which to register the chunk.
 * @param chunk Define the chunk to insert into the list.
 * @param insert_mode Define if the given bloc must be inserted at the end of the list or not.
**/
SCTK_STATIC void sctk_alloc_free_list_insert(struct sctk_thread_pool * pool,struct sctk_alloc_chunk_header_large * chunk_large,enum sctk_alloc_insert_mode insert_mode)
{
	struct sctk_alloc_free_chunk* flist;
	struct sctk_alloc_free_chunk * fchunk;
	sctk_size_t list_class;
	
	//errors
	assert(pool != NULL);
	assert(chunk_large != NULL);
	assert(chunk_large->size > 0);

	//get the free list
	flist = sctk_alloc_get_free_list(pool,sctk_alloc_get_chunk_header_large_size(chunk_large));
	assert(flist != NULL);
	list_class = sctk_alloc_get_list_class(pool,flist);
	if (flist != pool->free_lists && list_class != -1 && list_class != sctk_alloc_get_chunk_header_large_size(chunk_large))
		flist--;

	//small bloc not supported, need specific list or external declaration of list elements
	/** @todo Implement support for small bloc in free lists **/
	assert(chunk_large->info.type == SCTK_ALLOC_CHUNK_TYPE_LARGE);

	//setup chunk
	fchunk = (struct sctk_alloc_free_chunk*)chunk_large;

	//mark as free
	sctk_alloc_get_chunk_header_large_info(&fchunk->header)->state = SCTK_ALLOC_CHUNK_STATE_FREE;

	//insert in list
	switch (insert_mode)
	{
		case SCTK_ALLOC_INSERT_AT_END:
			//insert at the end of the list
			fchunk->prev = flist->prev;
			fchunk->next = flist;
			flist->prev->next = fchunk;
			flist->prev = fchunk;
			break;
		case SCTK_ALLOC_INSERT_AT_START:
			//insert at the begining of the list
			fchunk->prev = flist;
			fchunk->next = flist->next;
			flist->next->prev = fchunk;
			flist->next = fchunk;
			break;
		default:
			assume_m(false,"Unknown insert mode in free list.");
	}

	//mark non empty
	sctk_alloc_free_list_mark_non_empty(pool,flist);
}

/************************* FUNCTION ************************/
/**
 * Extract the free chunk from the free list. This methode simply update the list pointers.
 * @param fchunk Define the chunk to remove from the free list.
**/
SCTK_STATIC void sctk_alloc_free_list_remove(struct sctk_thread_pool * pool,struct sctk_alloc_free_chunk * fchunk)
{
	//vars
	struct sctk_alloc_free_chunk * prev;
	struct sctk_alloc_free_chunk * next;

	//error
	assert(fchunk != NULL);

	//keep localy to avoid dereferencing the pointer every time
	prev = fchunk->prev;
	next = fchunk->next;

	next->prev = prev;
	prev->next = next;

	//check if this is the last of the list
	//if true, mark the list as empty
	if (next == prev)
		sctk_alloc_free_list_mark_empty(pool,prev);

	fchunk->next = NULL;
	fchunk->prev = NULL;

	sctk_alloc_get_chunk_header_large_info(&fchunk->header)->state = SCTK_ALLOC_CHUNK_STATE_ALLOCATED;
}

/************************* FUNCTION ************************/
SCTK_STATIC struct sctk_alloc_free_chunk * sctk_alloc_find_adapted_free_chunk(sctk_alloc_free_list_t * list,sctk_size_t size)
{
	struct sctk_alloc_free_chunk * fchunk;

	//error
	assert(list!=NULL);

	//first in the list fo oldest one -> FIFO
	fchunk = list->next;
	while (fchunk != NULL && fchunk != list && sctk_alloc_get_chunk_header_large_size(&fchunk->header) < size)
		fchunk = fchunk->next;

	if (fchunk == list)
		return NULL;
	else
		return fchunk;
}

/************************* FUNCTION ************************/
SCTK_STATIC bool sctk_alloc_free_list_is_not_empty_quick(struct sctk_thread_pool * pool,sctk_alloc_free_list_t * list)
{
	//vars
	short int id;

	//error check
	assert(pool != NULL);
	assert(list != NULL);
	assert(list >= pool->free_lists && list < pool->free_lists+SCTK_ALLOC_NB_FREE_LIST);

	//get id
	id = (short int) (list - pool->free_lists);

	//test related bit
	return (pool->free_list_status[id]);
}

/************************* FUNCTION ************************/
SCTK_STATIC void sctk_alloc_free_list_mark_empty(struct sctk_thread_pool * pool,sctk_alloc_free_list_t * list)
{
	//vars
	short int id; 

	//error check
	assert(pool != NULL);
	assert(list != NULL);
	assert(list >= pool->free_lists && list < pool->free_lists+SCTK_ALLOC_NB_FREE_LIST);
	
	//get id
	id = (short int) (list - pool->free_lists);

	//down related bit to 0
	pool->free_list_status[id] = false;
}

/************************* FUNCTION ************************/
SCTK_STATIC void sctk_alloc_free_list_mark_non_empty(struct sctk_thread_pool * pool,sctk_alloc_free_list_t * list)
{
	//vars
	short int id;

	//error check
	assert(pool != NULL);
	assert(list != NULL);
	assert(list >= pool->free_lists && list < pool->free_lists+SCTK_ALLOC_NB_FREE_LIST);

	//get id
	id = (short int) (list - pool->free_lists);

	//setup related bit to 1
	pool->free_list_status[id] = true;
}

/************************* FUNCTION ************************/
SCTK_STATIC sctk_alloc_free_list_t * sctk_alloc_find_first_free_non_empty_list(struct sctk_thread_pool * pool,sctk_alloc_free_list_t * list)
{
	//vars
	short int id;
	short int i;
	short int nb_free_lists = pool->nb_free_lists;
	
	//error
	assert(pool != NULL);
	assert(list != NULL);
	assert(nb_free_lists <= SCTK_ALLOC_NB_FREE_LIST);
	assert(list >= pool->free_lists && list < pool->free_lists+nb_free_lists);
	//get free list id
	id = (short int) (list - pool->free_lists);
	
	assert(id < nb_free_lists);

	for ( i = id ; i < nb_free_lists ; ++i)
		if (pool->free_list_status[i])
			return pool->free_lists+i;

	//we didn't foudn anything
	return NULL;
}

/************************* FUNCTION ************************/
/**
 * Search an available free bloc to store a block of a given size. It may return a larger bloc which
 * may be splited, but this step is the responsibility of the caller. The chunk isn't removed from
 * the free list here (to keep trac of position and replace the current bloc by the small on in case
 * of cut).
 * @param pool Define the thread pool in which to search.
 * @param size Define the size we need to store.
 * @return If their is not available bloc, return NULL.
**/
SCTK_STATIC struct sctk_alloc_free_chunk * sctk_alloc_find_free_chunk(struct sctk_thread_pool * pool,sctk_size_t size)
{
	//vars
	struct sctk_alloc_free_chunk * res = NULL;
	struct sctk_alloc_free_chunk * start_point = NULL;
	sctk_alloc_free_list_t * list;

	//error
	assert(pool != NULL);
	assert(size > 0);

	//add header size
	size = sctk_alloc_calc_chunk_size(size);

	//get the minimum valid size
	list = sctk_alloc_get_free_list(pool,size);
	start_point = list;

	//if empty list, go to next if available
	//otherwite, take the first of the list (oldest one -> FIFO)
	list = sctk_alloc_find_first_free_non_empty_list(pool,list);
	if (list != NULL)
		res = sctk_alloc_find_adapted_free_chunk(list,size);

	//if not found, try our chance in the previous list (we may find some sufficent bloc, but may
	//require more steps of searching as their may be some smaller blos in this one on the contrary
	//of our starting point which guaranty to get sufficent size
	if (res == NULL && start_point != pool->free_lists)
	{
		list = start_point - 1;
		res = sctk_alloc_find_adapted_free_chunk(list,size);
	}

	//if find, remove from list
	if (res != NULL)
		sctk_alloc_free_list_remove(pool,res);

	return res;
}

/************************* FUNCTION ************************/
/**
 * Get the next class size list if available, otherwise, return NULL.
 * @param pool Define the pool in which to search the free list.
 * @param list Define the current list.
 * @return Return the free for the next size class or NULL if not available.
**/
SCTK_STATIC sctk_alloc_free_list_t* sctk_alloc_get_next_list(const struct sctk_thread_pool * pool,sctk_alloc_free_list_t* list)
{
	//error
	assert(pool != NULL);
	assert(pool->nb_free_lists <= SCTK_ALLOC_NB_FREE_LIST);
	assume_m(list - pool->free_lists <= pool->nb_free_lists,"The given list didn't be a member of the given pool");
	
	if (list == NULL)
		return NULL;
	else if (list - pool->free_lists >= pool->nb_free_lists - 1)
		return NULL;
	else
		return list+1;
}

/************************* FUNCTION ************************/
/**
 * Test if the free list is empty or not.
 * @param list Define the free list to test.
**/
SCTK_STATIC bool sctk_alloc_free_list_empty(const sctk_alloc_free_list_t* list)
{
	return (list->next == list && list->prev == list);
}

/************************* FUNCTION ************************/
SCTK_STATIC sctk_size_t sctk_alloc_align_size(sctk_size_t size,sctk_size_t align)
{
	if (size%align == 0)
		return size;
	else
		return size + align - (size % align);
}

/************************* FUNCTION ************************/
SCTK_STATIC sctk_size_t sctk_alloc_align_size_pow_of_2(sctk_size_t size,sctk_size_t align)
{
	assert(sctk_alloc_is_power_of_two(align));
	return ((size + align - 1) & (~(align-1)));
}

/************************* FUNCTION ************************/
/**
 * Split an original chunk into two small ones. The first one will be of requested size, the second
 * one if available will cover the rest of the original size.
 * @param chunk Define the chunk to split. It will be shrink to "size" if possible.
 * @param user_size Define the targeted size (without the header size, so the size requested by the malloc user).
 * @return Return the extra new chunk if available. It must be registred by the caller to a free list.
**/
/** @todo maybe can remove the pointer now if address stay the same. **/
SCTK_STATIC sctk_alloc_vchunk sctk_alloc_split_free_bloc(sctk_alloc_vchunk * chunk,sctk_size_t size)
{
	sctk_alloc_vchunk res;
	sctk_size_t residut_size;
	sctk_addr_t residut_start;
	sctk_alloc_vchunk next;
	
	//convert size to take in accound the header size
	size = sctk_alloc_calc_chunk_size(size);
	size = sctk_alloc_align_size_pow_of_2(size,SCTK_ALLOC_BASIC_ALIGN);
	
	//error
	assert(chunk != NULL);
	#ifndef SCTK_ALLOC_FAST_BUT_LESS_SAFE
	assume_m(sctk_alloc_get_size(*chunk) >= size,"Chunk size if too small to fit with requested size.");
	#endif

	residut_size = sctk_alloc_get_size(*chunk) - size;
	if (residut_size >= SCTK_ALLOC_MIN_SIZE)
	{
		//build residut
		residut_start = sctk_alloc_get_addr(*chunk) + size;
		res = sctk_alloc_setup_chunk((void*)residut_start,residut_size,sctk_alloc_get_ptr(*chunk));
		//update size of current
		/** @todo work only for large blocs **/
		/** @todo optimize **/
		*chunk = sctk_alloc_setup_chunk(sctk_alloc_get_ptr(*chunk),size,(void *)(sctk_alloc_get_addr(*chunk)-sctk_alloc_get_prev_size(*chunk)));
		//update prevSize in next chunk from res
		next = sctk_alloc_get_next_chunk(res);
		/** @todo work only for large blocs **/
		sctk_alloc_set_chunk_header_large_previous_size(sctk_alloc_get_large(next), residut_size);
	} else {
		res = NULL;
	}

	return res;
}

/************************* FUNCTION ************************/
/** @todo To document **/
SCTK_STATIC sctk_alloc_vchunk sctk_alloc_get_prev_chunk(sctk_alloc_vchunk chunk)
{
	sctk_alloc_vchunk res;

	#ifndef SCTK_ALLOC_FAST_BUT_LESS_SAFE
	assume_m(chunk->unused_magik == SCTK_ALLOC_MAGIK_STATUS,"Small block not supported for now.");
	#endif

	if (sctk_alloc_get_prev_size(chunk) == 0)
	{
		res = NULL;
	} else {
		/** @todo optimize **/
		res = (sctk_alloc_vchunk)((sctk_addr_t)chunk - sctk_alloc_get_prev_size(chunk));
	}

	return res;
}

/************************* FUNCTION ************************/
/**
 * Free all elements if not freed from first to last given one (comprised).
 * Elements will be chained by their size so using their addresses to move from one to another.
 * @param first Define the first element to freed.
 * @param last Define the last element de freed.
**/
SCTK_STATIC void sctk_alloc_free_chunk_range(struct sctk_thread_pool * pool,sctk_alloc_vchunk first,sctk_alloc_vchunk last)
{
	//vars
	bool loop = true;
	struct sctk_alloc_free_chunk * fchunk;

	//error
	assert(first <= last);

	//free all elements in the given interval
	while (loop == true)
	{
		if (first->state == SCTK_ALLOC_CHUNK_STATE_FREE)
		{
			/** @todo Caution, it didn t support small blocs like that **/
			fchunk = (struct sctk_alloc_free_chunk *)sctk_alloc_get_ptr(first);
			sctk_alloc_free_list_remove(pool,fchunk);
		}
		//jump to the next one if not at the end
		if (first < last)
			first = sctk_alloc_get_next_chunk(first);
		else
			loop = false;
	}
}

/************************* FUNCTION ************************/
/**
 * Function used to merge memory chunks. It will merge the free blocs on the left and on the right.
 * @param chunk Define the chunk to merge, caution, this chunk must be allocated and not registered
 * in the free list.
 * @param first_page_chunk Define the first chunk of the page. This will be the starting point for the search.
 * @param max_address Define the maximum address to reach for the merging.
**/
SCTK_STATIC sctk_alloc_vchunk sctk_alloc_merge_chunk(struct sctk_thread_pool * pool,sctk_alloc_vchunk chunk,sctk_alloc_vchunk first_page_chunk,sctk_addr_t max_address)
{
	sctk_alloc_vchunk cur = chunk;
	sctk_alloc_vchunk last;
	sctk_size_t size;
	struct sctk_alloc_free_chunk * fchunk;
	
	//error
	assume_m(chunk->state == SCTK_ALLOC_CHUNK_STATE_ALLOCATED,"The central chunk must be allocated to be merged.");

	//search the first free chunk before the central one.
	first_page_chunk = chunk;
	cur = sctk_alloc_get_prev_chunk(chunk);
	/** @todo caution it did not support small blocs **/
	while (cur != NULL && (cur->state == SCTK_ALLOC_CHUNK_STATE_FREE))
	{
		first_page_chunk = cur;
		//can remove current one from free list to be merged at the end of the function
		fchunk = (struct sctk_alloc_free_chunk *)sctk_alloc_get_ptr(cur);
		sctk_alloc_free_list_remove(pool,fchunk);
		//movre to prev one
		cur = sctk_alloc_get_prev_chunk(cur);
	}

	//search the last mergeable after the central one
	//so start from central one and move until find an allocated one
	//last is comprised into the merge
	last = chunk;
	cur = sctk_alloc_get_next_chunk(chunk);
// 	while (cur.bloc.abstract + cur.size < max_address
	while(cur != NULL && (cur->state == SCTK_ALLOC_CHUNK_STATE_FREE))
	{
		last = cur;
		//can remove current one from free list to be merged at the end of the function
		fchunk = (struct sctk_alloc_free_chunk *)sctk_alloc_get_ptr(cur);
		sctk_alloc_free_list_remove(pool,fchunk);
		//move to next one
		cur = sctk_alloc_get_next_chunk(cur);
	} 

	//calc final bloc size
	size = sctk_alloc_get_addr(last) - sctk_alloc_get_addr(first_page_chunk) + sctk_alloc_get_size(last);

	//update prevSize from next bloc if presentl
	/** @todo  to test presence and check bloc type **/
	cur = sctk_alloc_get_next_chunk(last);
	sctk_alloc_set_chunk_header_large_previous_size(sctk_alloc_get_large(cur), size);
	
	//setup bloc and return
	/** @todo Large access, so do not support small blocs **/
	/** @todo replace substraction by get_prev **/
	return sctk_alloc_setup_chunk(sctk_alloc_get_ptr(first_page_chunk),size,(void *)(sctk_alloc_get_addr(first_page_chunk) - sctk_alloc_get_prev_size(first_page_chunk)));
}

/************************* FUNCTION ************************/
/**
 * Base initialization of allocation.
 * @param chain Define the allocation to init.
**/
SCTK_STATIC void sctk_alloc_chain_base_init(struct sctk_alloc_chain * chain,enum sctk_alloc_chain_flags flags)
{
	//init lists
	/** @todo TODO add a call to spin destroy while removing the chain **/
	sctk_alloc_thread_pool_init(&chain->pool,SCTK_ALLOC_FREE_SIZES);

	//init lock
	chain->flags = flags,
	sctk_alloc_spinlock_init(&chain->lock,PTHREAD_PROCESS_PRIVATE);

	//init Remote Free Queue
	sctk_alloc_rfq_init(&chain->rfq);

	//defaults
	chain->name = "Unknwon";

	//destroy system
	chain->destroy_handler = NULL;
	chain->cnt_macro_blocs = 0;

	//init spy and stat module
	SCTK_ALLOC_STATS_HOOK(sctk_alloc_stats_chain_init(&chain->stats));
	SCTK_ALLOC_SPY_HOOK(sctk_alloc_spy_chain_init(chain));
}

/************************* FUNCTION ************************/
/**
 * Create a user allocation chain and provide it's fixed segement to manage.
 * @param buffer Define the segment to be managed by the user allocation chain.
 * @param size Define de size og the segment.
**/
void sctk_alloc_chain_user_init(struct sctk_alloc_chain * chain,void * buffer,sctk_size_t size,enum sctk_alloc_chain_flags flags)
{
	struct sctk_alloc_macro_bloc * macro_bloc;
	sctk_alloc_vchunk vchunk;
	
	//error
	assume_m(buffer != NULL || size == 0, "Can't manage NULL buffer with non NULL size.");
	/** @TODO compute min size cleaner with size of struct. **/
	assume_m(size == 0 || size > 64+16, "Buffer size must null or greater than 80o.");

	//base init
	sctk_alloc_chain_base_init(chain,flags);

	//insert the buffer in the thread pool
	if (buffer == NULL || size == 0)
	{
		chain->base_addr = NULL;
		chain->end_addr = NULL;
	} else {
		macro_bloc = sctk_alloc_setup_macro_bloc(buffer,size);
		vchunk = sctk_alloc_chain_prepare_and_reg_macro_bloc(chain,macro_bloc);
		sctk_alloc_free_list_insert(&chain->pool,sctk_alloc_get_large(vchunk),SCTK_ALLOC_INSERT_AT_START);
		chain->base_addr = buffer;
		chain->end_addr = (char*)buffer + size;
		chain->cnt_macro_blocs++;
	}
	chain->source = NULL;
}

/************************* FUNCTION ************************/
bool sctk_alloc_chain_is_thread_safe(struct sctk_alloc_chain * chain)
{
	//errors
	assert(chain != NULL);
  
	return chain->flags & SCTK_ALLOC_CHAIN_FLAGS_THREAD_SAFE;
}

/************************* FUNCTION ************************/
void sctk_alloc_chain_make_thread_safe(struct sctk_alloc_chain * chain,bool value)
{
	//errors
	assert(chain != NULL);

	if (value)
		chain->flags |= SCTK_ALLOC_CHAIN_FLAGS_THREAD_SAFE;
	else
		chain->flags &= ~SCTK_ALLOC_CHAIN_FLAGS_THREAD_SAFE;
}

/************************* FUNCTION ************************/
void sctk_alloc_chain_mark_for_destroy(struct sctk_alloc_chain * chain,void (*destroy_handler)(struct sctk_alloc_chain * chain))
{
	//errors
	assert(chain != NULL);
	assume_m(destroy_handler != NULL,"Your must provide a destroy handler to destroy an allocation.");

	//mark the chain as thread safe so we didn't use RFQ anymore.
	sctk_alloc_chain_make_thread_safe(chain,true);

	//mark for destroy
	chain->destroy_handler = destroy_handler;

	//flush the RFQ
	sctk_alloc_chain_purge_rfq(chain);
}

/************************* FUNCTION ************************/

/**
 * Permit to destroy a user allocation chain or a default allocation chain.
 * @param chain Define the allocation chain to destroy.
 * @param force Define if the function made some checks before detroying the allocation. You may
 * let this to false, this is more for unit test implementation avoiding crashing the whole test
 * suite on test failed.
**/
void sctk_alloc_chain_destroy(struct sctk_alloc_chain* chain,bool force)
{
	struct sctk_alloc_region * region;
	
	//nothing to do
	if (chain == NULL)
		return;

	//test if can detroy
	if ( ! force )
		assume_m(sctk_alloc_chain_can_destroy(chain),"Can't destroy the given allocation chain.");

	//destroy stat and spy module
	SCTK_ALLOC_STATS_HOOK(sctk_alloc_stats_chain_destroy(&chain->stats));
	SCTK_ALLOC_SPY_HOOK(sctk_alloc_spy_chain_destroy(chain));

	//destroy the rfq
	sctk_alloc_rfq_destroy(&chain->rfq);

	//unregister segment in regions.
	if (! (chain->flags & SCTK_ALLOC_CHAIN_DISABLE_REGION_REGISTER) )
	{
		region = sctk_alloc_region_get(chain->base_addr);
		sctk_alloc_region_del_chain(region,chain);
	}
}

/************************* FUNCTION ************************/
/**
 * Create a standard allocation chain.
**/
void sctk_alloc_chain_default_init(struct sctk_alloc_chain * chain, struct sctk_alloc_mm_source * source,enum sctk_alloc_chain_flags flags)
{
	//base init
	sctk_alloc_chain_base_init(chain,flags);

	chain->base_addr = NULL;
	chain->end_addr = NULL;
	chain->source = source;
}

/************************* FUNCTION ************************/
/**
 * Function used to convert a free chunk to a vchunk.
 * @param chunk Define the address of the free chunk to convert.
**/
SCTK_STATIC sctk_alloc_vchunk sctk_alloc_free_chunk_to_vchunk(struct sctk_alloc_free_chunk * chunk)
{
	return sctk_alloc_get_chunk_header_large_info(&chunk->header);
}

/************************* FUNCTION ************************/
/**
 * Function used to obtain the pointer to return to the user in malloc from a vchunk. It simply
 * return the address next to the header or NULL if chunk has a null size.
**/
SCTK_STATIC void * sctk_alloc_chunk_body(sctk_alloc_vchunk vchunk)
{
	if (vchunk == NULL)
	{
		return NULL;
	} else {
		assert(vchunk->type == SCTK_ALLOC_CHUNK_TYPE_LARGE || vchunk->type == SCTK_ALLOC_CHUNK_TYPE_PADDED);
		return vchunk + 1;
	}
}

/************************* FUNCTION ************************/
SCTK_STATIC sctk_alloc_vchunk sctk_alloc_chain_prepare_and_reg_macro_bloc(struct sctk_alloc_chain * chain,struct sctk_alloc_macro_bloc * macro_bloc)
{
	//vars
	sctk_alloc_vchunk vchunk = NULL;
	sctk_size_t size;

	if (macro_bloc != NULL)
	{
		size = sctk_alloc_get_chunk_header_large_size(&macro_bloc->header);
		vchunk = sctk_alloc_setup_chunk(macro_bloc+1,size - sizeof(struct sctk_alloc_macro_bloc) - sizeof(struct sctk_alloc_chunk_header_large),NULL);
		sctk_alloc_create_stopper((void *)((sctk_addr_t)macro_bloc+size-sizeof(struct sctk_alloc_chunk_header_large)),macro_bloc+1);
		/** @todo TOTO create a set_entry which accept blocs directly this may be cleaner to maintain **/
		SCTK_NO_PDEBUG("Reg macro bloc : %p -> %p",macro_bloc,(sctk_addr_t)macro_bloc + sctk_alloc_get_chunk_header_large_size(&macro_bloc->header));
		if (! (chain->flags & SCTK_ALLOC_CHAIN_DISABLE_REGION_REGISTER ))
			sctk_alloc_region_set_entry(chain,macro_bloc);
	}

	return vchunk;
}

/************************* FUNCTION ************************/
/**
 * Request memory the the memory source of allocation chain if available and setup chunk headers
 * to be ready for use. It will be ready for insertion in free lists of for direct usage for
 * huge allocation.
 * @param chain Define the allocation chain to refill.
 * @param size Define the requested size (will be rounded to multiple of macro bloc
 * size : SCTK_MACRO_BLOC_SIZE).
**/
SCTK_STATIC sctk_alloc_vchunk sctk_alloc_chain_request_mem(struct sctk_alloc_chain* chain,sctk_size_t size)
{
	struct sctk_alloc_macro_bloc * bloc;
	sctk_alloc_vchunk vchunk = SCTK_ALLOC_DEFAULT_CHUNK;

	//error
	assert(chain != NULL);
	SCTK_NO_PDEBUG("Try to refill memory for size = %lu",size);

	//trivial
	if (chain->source == NULL)
		return vchunk;

	//round size
	size += sizeof(struct sctk_alloc_macro_bloc) + sizeof(struct sctk_alloc_chunk_header_large);
	//region didn't support smaller than macro bloc size, but for larger we only need to align on page size
	/*if (size <= SCTK_MACRO_BLOC_SIZE)
	{
		size = SCTK_MACRO_BLOC_SIZE;
	} else {
		if (size % SCTK_PAGE_SIZE)
			size += SCTK_PAGE_SIZE - size % SCTK_PAGE_SIZE;
	}
	assert(size % SCTK_PAGE_SIZE == 0);
	assert(size >= SCTK_MACRO_BLOC_SIZE);*/
	
	if (size % SCTK_MACRO_BLOC_SIZE != 0)
		size += SCTK_MACRO_BLOC_SIZE - size % SCTK_MACRO_BLOC_SIZE;
	assert(size % SCTK_MACRO_BLOC_SIZE == 0);

	//request memory and refill the free list with it
	bloc = chain->source->request_memory(chain->source,size);
	SCTK_ALLOC_SPY_HOOK(sctk_alloc_spy_emit_event_add_macro_bloc(chain,bloc));
	if (bloc != NULL)
		assert(bloc->chain == NULL);

	//insert in free list
	vchunk = sctk_alloc_chain_prepare_and_reg_macro_bloc(chain,bloc);

	//update counter or replace default value
	if (vchunk == NULL)
		vchunk = SCTK_ALLOC_DEFAULT_CHUNK;
	else
		chain->cnt_macro_blocs++;

	//ok we can return
	return vchunk;
}

/************************* FUNCTION ************************/
/**
 * This function is a merge of sctk_alloc_chain_request_mem() and sctk_alloc_chain_free_macro_bloc()
 * which exploit mremap capabilities to avoid a copy of large segment for realloc() implementation.
 * @param chain Define the chain in which to realloc. Caution you must ensure that the original
 * segment was managed by the same chain otherwise it can procuce unpredictable behaviors.
 * @param size Define the size of the new chunk to allocate.
 * @param vchunk Define the old vchunk to remap. Caution it must be a macro bloc.
**/
SCTK_STATIC sctk_alloc_vchunk sctk_alloc_chain_realloc_macro_bloc(struct sctk_alloc_chain * chain,sctk_size_t size,sctk_alloc_vchunk vchunk)
{
	//vars
	struct sctk_alloc_macro_bloc * macro_bloc;
	
	//errors
	assert(chain != NULL);
	assert(chain->source != NULL);
	assert(chain->source->remap != NULL);
	assert(size > 0);
	assert(sctk_alloc_chain_can_remap(chain));
	/** @todo Maybe request the bloc size to memory source insteed of directly use the constant **/
	assert (sctk_alloc_get_size(vchunk) % SCTK_MACRO_BLOC_SIZE == SCTK_MACRO_BLOC_SIZE - sizeof(struct sctk_alloc_macro_bloc) - sizeof(struct sctk_alloc_chunk_header_large));

	//round size
	size += sizeof(struct sctk_alloc_macro_bloc) + sizeof(struct sctk_alloc_chunk_header_large);
	if (size % SCTK_MACRO_BLOC_SIZE != 0)
		size += SCTK_MACRO_BLOC_SIZE - size % SCTK_MACRO_BLOC_SIZE;
	assert(size % SCTK_MACRO_BLOC_SIZE == 0);

	//get original macro bloc
	/** @todo wrap the address calculation due to inheritance **/
	macro_bloc = (struct sctk_alloc_macro_bloc *)(sctk_alloc_get_addr(vchunk) - sizeof(struct sctk_alloc_macro_bloc));

	//unregister from regions
	if (! (chain->flags & SCTK_ALLOC_CHAIN_DISABLE_REGION_REGISTER) )
		sctk_alloc_region_unset_entry(macro_bloc);

	//request a realloc
	//SCTK_ALLOC_SPY_HOOK(sctk_alloc_spy_emit_event_free_macro_bloc(chain,macro_bloc));
	macro_bloc = chain->source->remap(macro_bloc,size);
	assert(macro_bloc->chain == NULL);
	//SCTK_ALLOC_SPY_HOOK(sctk_alloc_spy_emit_event_add_macro_bloc(chain,bloc));

	//setup and return
	vchunk = sctk_alloc_chain_prepare_and_reg_macro_bloc(chain,macro_bloc);
	if (vchunk == NULL)
		vchunk = SCTK_ALLOC_DEFAULT_CHUNK;

	//ok we can return
	return vchunk;
}

/************************* FUNCTION ************************/
/**
 * Refill memory of the given chain when it fall to low level. It will request the new memory
 * to the related memory source if available.
 * @param chain Define the allocation chain to refill.
 * @param size Define the requested size (will be rounded to multiple of macro bloc
 * size : SCTK_MACRO_BLOC_SIZE).
**/
SCTK_STATIC bool sctk_alloc_chain_refill_mem(struct sctk_alloc_chain* chain,sctk_size_t size)
{
	//request a segments
	sctk_alloc_vchunk vchunk = sctk_alloc_chain_request_mem(chain,size);
	
	if (vchunk == NULL)
	{
		return false;
	} else {
		//ok we got it, now register the segment in free lists.
		sctk_alloc_free_list_insert(&chain->pool,sctk_alloc_get_large(vchunk),SCTK_ALLOC_INSERT_AT_END);
		return true;
	}
}

/************************* FUNCTION ************************/
/**
 * Try to allocate a segement of a given size from the given chain.
 * @param chain Define the allocation chain in which to request memory.
 * @param size Define the expected size of the segment (can be larger).
**/
void * sctk_alloc_chain_alloc(struct sctk_alloc_chain * chain,sctk_size_t size)
{
	return sctk_alloc_chain_alloc_align(chain,0,size);
}

/************************* FUNCTION ************************/
/**
 * Try to allocate a segement of a given size from the given chain.
 * @param chain Define the allocation chain in which to request memory.
 * @param boundary Define the memory alignement to force for the bloc base address.
 * @param size Define the expected size of the segment (can be larger).
**/
void * sctk_alloc_chain_alloc_align(struct sctk_alloc_chain * chain,sctk_size_t boundary,sctk_size_t size)
{
	struct sctk_alloc_free_chunk * chunk;
	sctk_alloc_vchunk vchunk;
	sctk_alloc_vchunk residut;
	
	//error
	assume_m(chain != NULL,"Can't work with NULL allocation chain.");

	//trivial
	if (size == 0)
		return NULL;

	//alignement support
	/** @todo Optim : free the left part of the bloc if too largest than a threshold. **/
	if (boundary > 1)
		size += boundary;

	//check if huge allocation of normal
	/** @todo Split in two sub-functions **/
	/** @todo Define a function to detect huge blocs intead of use two time the calculation.**/
	if (SCTK_ALLOC_HUGE_CHUNK_SEGREGATION && chain->source != NULL && sctk_alloc_calc_chunk_size(size) > SCTK_HUGE_BLOC_LIMIT)
	{
		//for huge block, we bypass the thread pool and call directly the memory source.
		//huge bloc are > SCTK_MACRO_BLOC_SIZE / 2
		vchunk = sctk_alloc_chain_request_mem(chain,size);

		//spy
		SCTK_ALLOC_SPY_HOOK(sctk_alloc_spy_emit_event_chain_huge_alloc(chain,size-boundary,sctk_alloc_chunk_body((boundary > 1)?sctk_alloc_setup_chunk_padded(vchunk,boundary):vchunk),sctk_alloc_get_size(vchunk),boundary));
	} else {
		//lock if required
		 if (chain->flags & SCTK_ALLOC_CHAIN_FLAGS_THREAD_SAFE)
			sctk_alloc_spinlock_lock(&chain->lock);

		//try to find a chunk
		chunk = sctk_alloc_find_free_chunk(&chain->pool,size);

		//out of memory, refill mem and retry
		if (chunk == NULL)
			if (sctk_alloc_chain_refill_mem(chain,sctk_alloc_calc_chunk_size(size)))
				chunk = sctk_alloc_find_free_chunk(&chain->pool,size);

		//temporaty check non support of small blocs
		if (chunk != NULL)
			assume_m(sctk_alloc_get_chunk_header_large_size(&chunk->header) >= 32lu,"Small blocs are not supported for now, so it's imposible to get such a small size here.");

		//error
		if (chunk == NULL)
		{
			//SCTK_NO_PDEBUG("Try to allocate %ld",size);
			//SCTK_NO_PDEBUG("Out of memory in user segment.");
			//SCTK_CRASH_DUMP();
			//unlock if required
			if (chain->flags & SCTK_ALLOC_CHAIN_FLAGS_THREAD_SAFE)
				sctk_alloc_spinlock_unlock(&chain->lock);
			return NULL;
		}

		//try to split
		vchunk = sctk_alloc_free_chunk_to_vchunk(chunk);
		residut = sctk_alloc_split_free_bloc(&vchunk,size);
		assume_m(sctk_alloc_get_size(vchunk) >= sctk_alloc_calc_chunk_size(size), "Size error in chunk spliting function.");

		if (residut != NULL)
		{
			sctk_alloc_free_list_insert(&chain->pool,sctk_alloc_get_large(residut),SCTK_ALLOC_INSERT_AT_START);
			SCTK_ALLOC_SPY_HOOK(sctk_alloc_spy_emit_event_chain_split(chain,sctk_alloc_get_ptr(vchunk),sctk_alloc_get_size(vchunk),sctk_alloc_get_size(residut)));
		}

		//spy
		SCTK_ALLOC_SPY_HOOK(sctk_alloc_spy_emit_event_chain_alloc(chain,size,sctk_alloc_chunk_body((boundary > 1)?sctk_alloc_setup_chunk_padded(vchunk,boundary):vchunk),sctk_alloc_get_size(vchunk),boundary));

		//unlock if required
		if (chain->flags & SCTK_ALLOC_CHAIN_FLAGS_THREAD_SAFE)
			sctk_alloc_spinlock_unlock(&chain->lock);
	}

	//align support
	if (boundary > 1)
		vchunk = sctk_alloc_setup_chunk_padded(vchunk,boundary);

	//return
	if (vchunk == NULL)
		return NULL;
	else
		return sctk_alloc_chunk_body(vchunk);
}

/************************* FUNCTION ************************/
/**
 * Free the given macro bloc to the memory source.
 * @param chain Define the allocation chain to use.
 * @param vchunk Define the memory bloc which cover the while macro bloc.
**/
SCTK_STATIC void sctk_alloc_chain_free_macro_bloc(struct sctk_alloc_chain * chain,sctk_alloc_vchunk vchunk)
{
	//vars
	struct sctk_alloc_macro_bloc * macro_bloc;

	//some checks
	/** @todo Maybe request the bloc size to memory source insteed of directly use the constant **/
	assert (sctk_alloc_get_size(vchunk) % SCTK_MACRO_BLOC_SIZE == SCTK_MACRO_BLOC_SIZE - sizeof(struct sctk_alloc_macro_bloc) - sizeof(struct sctk_alloc_chunk_header_large));

	//get the macro bloc
	/** @todo wrap the address calculation due to inheritance **/
	macro_bloc = (struct sctk_alloc_macro_bloc *)(sctk_alloc_get_addr(vchunk) - sizeof(struct sctk_alloc_macro_bloc));

	//unregister from regions
	SCTK_NO_PDEBUG("Send macro bloc to memory source 0x%p -> 0x%p.",macro_bloc,(sctk_addr_t)macro_bloc + sctk_alloc_get_chunk_header_large_size(&macro_bloc->header));
	if (! (chain->flags & SCTK_ALLOC_CHAIN_DISABLE_REGION_REGISTER ))
		sctk_alloc_region_unset_entry(macro_bloc);

	//free it
	SCTK_ALLOC_SPY_HOOK(sctk_alloc_spy_emit_event_free_macro_bloc(chain,macro_bloc));
	chain->source->free_memory(chain->source,macro_bloc);

	//update counter
	chain->cnt_macro_blocs--;
	assert(chain->cnt_macro_blocs >= 0);

	//check if we must destroy the allocation chain
	if (chain->cnt_macro_blocs == 0 && chain->destroy_handler != NULL && chain->flags & SCTK_ALLOC_CHAIN_FLAGS_THREAD_SAFE)
		chain->destroy_handler(chain);
}

/************************* FUNCTION ************************/
/**
 * Free the given pointer assuming it was allcoated by the given chain.
 * @param chain Allocation chain responsible of the management of ptr
 * @param ptr The buffer to freed.
**/
void sctk_alloc_chain_free(struct sctk_alloc_chain * chain,void * ptr)
{
	sctk_alloc_vchunk vchunk;
	sctk_alloc_vchunk vfirst = NULL;
	bool insert_bloc = true;
	#ifdef SCTK_ALLOC_SPY
	sctk_size_t old_size;
	#endif
	
	//error
	assume_m(chain != NULL, "Can't free the memory without an allocation chain.");

	//trivial
	if (ptr == NULL)
		return;

	//get and check the chunk header
	vchunk = sctk_alloc_get_chunk_unpadded((sctk_addr_t)ptr);
	//manage the cas of bad pointer and return withour doing anything
	if (vchunk == NULL)
		return;
	assume_m(vchunk->state == SCTK_ALLOC_CHUNK_STATE_ALLOCATED,"Double free corruption.");

	//check if huge bloc or not
	/** @todo Split in two sub-functions **/
	/** @todo Define a function to detect huge blocs intead of use two time the calculation. **/
	if (SCTK_ALLOC_HUGE_CHUNK_SEGREGATION && chain->source != NULL && sctk_alloc_get_size(vchunk) > SCTK_HUGE_BLOC_LIMIT)
	{
		//spy
		SCTK_ALLOC_SPY_HOOK(sctk_alloc_spy_emit_event_chain_huge_free(chain,ptr,sctk_alloc_get_size(vchunk)));
		
		//for huge blocs, we send the memory directly to the memory source
		sctk_alloc_chain_free_macro_bloc(chain,vchunk);
	} else {
		//lock if required
		if (chain->flags & SCTK_ALLOC_CHAIN_FLAGS_THREAD_SAFE)
			sctk_alloc_spinlock_lock(&chain->lock);

		//spy
		SCTK_ALLOC_SPY_HOOK(sctk_alloc_spy_emit_event_chain_free(chain,ptr,sctk_alloc_get_size(vchunk)));

		//try some merge
		/** @todo Here this is a trick, NEED TO BE FIXED => NOW NEED TO REMOVE THIS OR USE THE MACRO BLOC START POINT IF AVAILABLE. **/
		if (chain->base_addr != NULL)
			vfirst = sctk_alloc_get_chunk((sctk_addr_t)chain->base_addr+sizeof(struct sctk_alloc_chunk_header_large));
		SCTK_ALLOC_SPY_HOOK(old_size = sctk_alloc_get_size(vchunk));
		
		if (! (chain->flags & SCTK_ALLOC_CHAIN_DISABLE_MERGE) )
			vchunk = sctk_alloc_merge_chunk(&chain->pool,vchunk,vfirst,(sctk_addr_t)chain->end_addr);
		SCTK_ALLOC_SPY_COND_HOOK(old_size != sctk_alloc_get_size(vchunk),sctk_alloc_spy_emit_event_chain_merge(chain, sctk_alloc_get_ptr(vchunk),sctk_alloc_get_size(vchunk)));

		//if whe have a source, we may try to check if we can clear the bloc
		/** @todo Maybe request the bloc size to memory source insteed of directly use the constant **/
		if (chain->source != NULL && sctk_alloc_get_size(vchunk) % SCTK_MACRO_BLOC_SIZE == SCTK_MACRO_BLOC_SIZE - sizeof(struct sctk_alloc_macro_bloc) - sizeof(struct sctk_alloc_chunk_header_large))
		{
			insert_bloc = false;
			sctk_alloc_chain_free_macro_bloc(chain,vchunk);
		}

		//insert in free list
		if (insert_bloc)
			sctk_alloc_free_list_insert(&chain->pool,sctk_alloc_get_large(vchunk),SCTK_ALLOC_INSERT_AT_END);

		//lock if required
		if (chain->flags & SCTK_ALLOC_CHAIN_FLAGS_THREAD_SAFE)
			sctk_alloc_spinlock_unlock(&chain->lock);
	}
}

/************************* FUNCTION ************************/
SCTK_STATIC bool sctk_alloc_chain_can_remap(struct sctk_alloc_chain * chain)
{
	#ifdef HAVE_MREMAP
		bool res = false;
		if (chain->source != NULL)
			if (chain->source->remap != NULL)
				res = true;
		return res;
	#else
		return false;
	#endif
}

/************************* FUNCTION ************************/
/**
 * Implementation of posix realloc on the given chain.
 * CAUTION, this function is limited to reallocated a segment in the same chain, using it to reallocate
 * a segment in another chain may lead to bugs.
 * @param chain Define the allocation chain to use for reallocation.
 * @param ptr Define the base pointer of the segment to reallocate.
 * @param size Define the new size of the segment.
**/
void * sctk_alloc_chain_realloc(struct sctk_alloc_chain * chain, void * ptr, sctk_size_t size)
{
	//vars
	void * res = NULL;
	sctk_size_t old_size;
	sctk_size_t copy_size;
	sctk_alloc_vchunk vchunk;

	//errors
	assert(chain != NULL);

	SCTK_NO_PDEBUG("Do realloc %p -> %llu",ptr,size);

	//cases
	if (ptr == NULL && size == 0)
	{
		//nothing to do
		res = NULL;
	} else if (ptr == NULL) {
		//only alloc, no previous segment
		SCTK_NO_PDEBUG("Simple chain alloc instead of realloc %p -> %llu",ptr,size);
		res = sctk_alloc_chain_alloc(chain,size);
	} else if (size == 0) {
		//only free, no new segment
		sctk_alloc_chain_free(chain,ptr);
		res = NULL;
	} else if ( ( vchunk = sctk_alloc_get_chunk((sctk_addr_t)ptr) ) != NULL) {
		//errors
		if (! (chain->flags & SCTK_ALLOC_CHAIN_DISABLE_REGION_REGISTER ))
		{
			assert(sctk_alloc_region_get_macro_bloc(ptr) != NULL);
			assert(chain == sctk_alloc_region_get_macro_bloc(ptr)->chain);
		}

		//get chunk header
		/** @todo Use a function which compute the inner size instead of hacking with sizeof() . **/
		old_size = sctk_alloc_get_size(vchunk) - sizeof(struct sctk_alloc_chunk_header_large);

		if (old_size >= size && old_size - size < old_size / SCTK_REALLOC_THRESHOLD) {
			//simply keep the old segment, nothing to change
			SCTK_NO_PDEBUG("realloc with same address");
			res = ptr;
		} else if (SCTK_ALLOC_HUGE_CHUNK_SEGREGATION && old_size > SCTK_HUGE_BLOC_LIMIT && size > 0 && size > SCTK_HUGE_BLOC_LIMIT && sctk_alloc_chain_can_remap(chain)) {
			//use remap to realloc
			SCTK_NO_PDEBUG("Use mremap %llu -> %llu",old_size,size);
			res = sctk_alloc_chunk_body(sctk_alloc_chain_realloc_macro_bloc(chain,size,vchunk));
		} else {
			SCTK_NO_PDEBUG("Use chain_alloc / chain_free %p -> %llu -> %llu",ptr,old_size,size);
			//need to reallocate a new segment and copy the old data
			res = sctk_alloc_chain_alloc(chain,size);
			//copy the data
			if (res != NULL)
			{
				copy_size = (old_size < size) ? old_size : size;
				memcpy(res,ptr,copy_size);
			}
			//free the old one
			sctk_alloc_chain_free(chain,ptr);
		}
	}

	return res;
}

/************************* FUNCTION ************************/
/**
 * Method used to cleanup the given allocation chain. It only check if all blocks are freed and
 * break the link between the chain and the buffer (not call free on it as it can be static).
 * @param chain Pointer to the allocation chain we want to check.
 * @return True if all internal blocs are free, false otherwise or if chain is NULL.
**/
SCTK_STATIC bool sctk_alloc_chain_can_destroy(struct sctk_alloc_chain* chain)
{
	struct sctk_alloc_chunk_header_large * first;
	
	//error
	assert(chain != NULL);
	
	//nothing to do
	if (chain == NULL)
		return false;

	/** @todo Caution it do not support small block like that **/
	first = (struct sctk_alloc_chunk_header_large *)((sctk_addr_t)chain->base_addr + sizeof(struct sctk_alloc_macro_bloc));

	//check if first segement cover the wall size (all merged) and if free.
	return (sctk_alloc_get_chunk_header_large_size(first) + sizeof(struct sctk_alloc_chunk_header_large) + sizeof(struct sctk_alloc_macro_bloc) == ((sctk_addr_t)chain->end_addr - (sctk_addr_t)chain->base_addr)
		&& sctk_alloc_get_chunk_header_large_info(first)->state == SCTK_ALLOC_CHUNK_STATE_FREE);
}

/************************* FUNCTION ************************/
/**
 * Check if the given allocation chain contain blocs to purge and done a real free on them.
 * @param chain Define the chain we want to purge, if NULL, the function exit immediately.
**/
void sctk_alloc_chain_purge_rfq(struct sctk_alloc_chain * chain)
{
	struct sctk_alloc_rfq_entry * entries;
	struct sctk_alloc_rfq_entry * next;
	//int cnt = 0;
	
	//error
	if (chain == NULL)
		return;

	//SCTK_PDEBUG("Check if need to purge RFQ (%p)",chain);

	//check if RFQ is empty or not, if true, nothing to do
	if (sctk_alloc_rfq_empty(&chain->rfq))
		return;

	//get the list
	entries = sctk_alloc_rfq_extract(&chain->rfq);

	SCTK_ALLOC_SPY_COND_HOOK(entries != NULL,
	                         sctk_alloc_spy_emit_event_flush_rfq(chain,sctk_alloc_rfq_count_entries(entries)));
	//SCTK_PDEBUG("Need to purge RFQ (%p)",chain);

	//run over the list and call free on segments
	/** @todo Check for lock **/
	while (entries != NULL)
	{
		SCTK_ALLOC_SPY_HOOK(sctk_alloc_spy_emit_event_flush_rfq_entry(chain,entries->ptr));
		//cnt++;
		//CAUTION, we store the entry directly in the freed segment, so the content can be erase or
		//unmap by sctk_alloc_chain_free, we must't use anymore the entry pointer after calling this
		//method.
		next = entries->next;
		sctk_alloc_chain_free(chain,entries->ptr);
		entries = next;
	}

	//SCTK_PDEBUG("Has purge %d entries.",cnt);
}

/************************* FUNCTION ************************/
/**
 * Insert a given memory segement into the given memory source. Caution, there is some conditions :
 *   - The related memory region have to be setup previously to this call.
 *   - base and size must be multiple of SCTK_MACRO_BLOC_SIZE.
 * @param source Define the memory source in which to insert the segment.
 * @param base Define the base address of the segement. Must be multiple of SCTK_MACRO_BLOC_SIZE.
 * @param size Define the size of the segement. Must be multiple of SCTK_MACRO_BLOC_SIZE.
**/
SCTK_STATIC void sctk_alloc_mm_source_insert_segment(struct sctk_alloc_mm_source_default* source,void * base,sctk_size_t size)
{
	//vars
	struct sctk_alloc_free_macro_bloc * start_bloc;
	struct sctk_alloc_macro_bloc * end_bloc;
	sctk_alloc_vchunk vchunk;

	//check errors
	assert(source != NULL);
	assume_m((sctk_addr_t)base % SCTK_MACRO_BLOC_SIZE == 0, "Base address must be multiple of SCTK_MACRO_BLOC_SIZE to insert the segment in memory source.");
	assume_m(size % SCTK_MACRO_BLOC_SIZE == 0, "Base address must be multiple of SCTK_MACRO_BLOC_SIZE to insert the segment in memory source.");
	assume_m(sctk_alloc_region_get((void *)base) == sctk_alloc_region_get((void*)((sctk_addr_t)base+size-1)),"Segments can't cover more than one region.");
	//assume_m(base >= (void*)sctk_alloc_region_get(base) + SCTK_REGION_HEADER_SIZE,"Segment mustn't overlap the region header.");

	//remove the size for end bloc
	size -= SCTK_ALLOC_PAGE_SIZE;

	//setup initial bloc header
	start_bloc = sctk_mmap((void*)((sctk_addr_t)base),SCTK_ALLOC_PAGE_SIZE);
	if (start_bloc == MAP_FAILED)
	{
		perror("Error on initial macro bloc allocation.");
		abort();
	}

	//setup final bloc
	end_bloc = sctk_mmap((void*)((sctk_addr_t)base+size),SCTK_ALLOC_PAGE_SIZE);
	if (end_bloc == MAP_FAILED)
	{
		perror("Error on final macro bloc allocation.");
		abort();
	}

	//setup bloc info and link list
	vchunk = sctk_alloc_setup_chunk(start_bloc,size,NULL);
	sctk_alloc_create_stopper(end_bloc,start_bloc);

	//init the pool and register to free list.
	sctk_alloc_thread_pool_init(&source->pool,SCTK_ALLOC_MMS_FREE_SIZES);
	sctk_alloc_free_list_insert(&source->pool,sctk_alloc_get_large(vchunk),SCTK_ALLOC_INSERT_AT_END);
	start_bloc->maping = SCTK_ALLOC_BLOC_UNMAPPED;
}

/************************* FUNCTION ************************/
void sctk_alloc_mm_source_base_init(struct sctk_alloc_mm_source * source)
{
	source->cleanup        = NULL;
	source->free_memory    = NULL;
	source->remap          = NULL;
	source->request_memory = NULL;
}

/************************* FUNCTION ************************/
/**
 * Function used to initialized the default memory source for the common allocation chain.
 * @param source Define the memory source to initialized.
 * @param heap_base Define the base address for the heap managed by this memory source. Must be
 *        multiple of SCTK_MACRO_BLOC_SIZE.
 *        CAUTION, it can't be placed to addresses lower than SCTK_ALLOC_HEAP_BASE.
 * @param heap_size Define the maximum size which can be use by the heap. It will really allocate
 *        only the required part. Must be multiple of SCTK_MACRO_BLOC_SIZE.
 * @todo Remove the lower bound limitation by using a spacial header address for this segment in region management.
**/
void sctk_alloc_mm_source_default_init(struct sctk_alloc_mm_source_default* source,sctk_addr_t heap_base,sctk_size_t heap_size)
{
	void * current = (void*)heap_base;

	//basic setup
	sctk_alloc_mm_source_base_init(&source->source);
	
	//setup functions
	source->source.cleanup = sctk_alloc_mm_source_default_cleanup;
	source->source.free_memory = sctk_alloc_mm_source_default_free_memory;
	source->source.request_memory = sctk_alloc_mm_source_default_request_memory;

	//init spinlock
	sctk_alloc_spinlock_init(&source->spinlock,PTHREAD_PROCESS_PRIVATE);

	//some checks
	assume_m(heap_base % SCTK_MACRO_BLOC_SIZE == 0,"Memory source heap_base address must be multiple of SCTK_MACRO_BLOC_SIZE.");
	assume_m(heap_size % SCTK_MACRO_BLOC_SIZE == 0,"Memory source heap_size must be multiple of SCTK_MACRO_BLOC_SIZE.");

	//get the first region and loop to register all related regions
	while (current < (void*)((sctk_addr_t)heap_base + heap_size))
	{
		assume_m( sctk_alloc_region_setup(current) != NULL , "Can't create region header." );
		current = (void*)((sctk_addr_t)current + SCTK_REGION_SIZE);
	}

	//insert the segment in memory source
	sctk_alloc_mm_source_insert_segment(source,(void*)heap_base,heap_size);

	//mm source info
	source->heap_addr = (void*)heap_base;
	source->heap_size = heap_size;
}

/************************* FUNCTION ************************/
/**
 * Function used to request memory from the memory source. Caution, for now size must be a multiple
 * of SCTK_MACRO_BLOC_SIZE.
 * @param source Define the memory source on which to request memory.
 * @param size Define the size of the request (multiple of SCTK_MACRO_BLOC_SIZE). It must take
 * in accound the header size.
**/
SCTK_STATIC struct sctk_alloc_macro_bloc* sctk_alloc_mm_source_default_request_memory(struct sctk_alloc_mm_source* source, sctk_size_t size)
{
	//vars
	struct sctk_alloc_mm_source_default * source_default = (struct sctk_alloc_mm_source_default *)source;
	struct sctk_alloc_free_macro_bloc * bloc;
	struct sctk_alloc_macro_bloc* macro_bloc;
	enum sctk_alloc_mapping_state mapping;
	void * tmp = NULL;
	sctk_size_t aligned_size = size;
	sctk_alloc_vchunk vchunk;
	sctk_alloc_vchunk residut;
	
	//errors
	/** @todo Can down this restriction to multiple of 4k and round here **/
	assume_m(size % SCTK_MACRO_BLOC_SIZE == 0,"The request size on a memory source must be multiple of SCTK_MACRO_BLOC_SIZE.");

	//lock access to the memory source
	sctk_alloc_spinlock_lock(&source_default->spinlock);

	//find a free bloc
	size -= sizeof(struct sctk_alloc_chunk_header_large);
	bloc = (struct sctk_alloc_free_macro_bloc*)sctk_alloc_find_free_chunk(&source_default->pool,size);
	if (bloc == NULL)
	{
		sctk_alloc_perror("Out of memory, current limitation is %lu MB of virtual memory, request is %lu MB.",source_default->heap_size/1024ul/1024ul,aligned_size/1024ul/1024ul);
		sctk_alloc_spinlock_unlock(&source_default->spinlock);
		return NULL;
	}

	//mmap the starting bloc if not already done
	mapping = bloc->maping;
	if (mapping != SCTK_ALLOC_BLOC_MAPPED)
	{
		//if we will split, we must allocated one more page to store the header of next bloc
		//We also skip the first page as it was already allocated to store the header, hence
		//we avoid to loss it's content.
		SCTK_NO_PDEBUG("Have non mapped macro bloc, call mmap on addr = %p (%lu) PID=%d.",bloc,aligned_size/1024,getpid());
		if (sctk_alloc_get_chunk_header_large_size(&bloc->header.header) == aligned_size)
			tmp = sctk_mmap((void*)((sctk_addr_t)bloc+SCTK_ALLOC_PAGE_SIZE),aligned_size-SCTK_ALLOC_PAGE_SIZE);
		else
			tmp = sctk_mmap((void*)((sctk_addr_t)bloc+SCTK_ALLOC_PAGE_SIZE),aligned_size);
		if (tmp == MAP_FAILED)
		{
			perror("Error on requiresting memory to OS.");
			return NULL;
		}
	} else {
		SCTK_NO_PDEBUG("Have already mapped macro bloc on addr = %p (%lu).",bloc,aligned_size/1024);
	}

	//try to split if required
	vchunk = sctk_alloc_free_chunk_to_vchunk(&bloc->header);
	residut = sctk_alloc_split_free_bloc(&vchunk,size);
	assume_m(sctk_alloc_get_size(vchunk) >= sctk_alloc_calc_chunk_size(size), "Size error in chunk spliting function.");

	//insert residut in free list
	if (residut != NULL)
	{
		bloc = sctk_alloc_get_ptr(residut);
		bloc->maping = mapping;
		SCTK_NO_PDEBUG("Split bloc at addr=%p (%lu), mapping=%d",bloc,sctk_alloc_get_size(residut)/1024,mapping);
		sctk_alloc_free_list_insert(&source_default->pool,sctk_alloc_get_large(residut),SCTK_ALLOC_INSERT_AT_START);
	}

	//unlock access to the memory source
	sctk_alloc_spinlock_unlock(&source_default->spinlock);

	//final macro bloc setup
	macro_bloc = sctk_alloc_get_ptr(vchunk);
	macro_bloc->chain = NULL;

	//return it
	return macro_bloc;
}

/************************* FUNCTION ************************/
/**
 * Function used to free a macro bloc in the given memory source. The bloc is freed and insert into
 * macro blocs free list. If it can be merge it will be in this case, its memory will be returned to
 * the system by calling munmap.
 * @param source Define the memory source managing the given bloc.
 * @param bloc Degine the macro bloc to insert in free lists.
**/
SCTK_STATIC void sctk_alloc_mm_source_default_free_memory(struct sctk_alloc_mm_source* source, struct sctk_alloc_macro_bloc* bloc)
{
	sctk_alloc_vchunk vchunk;
	/** @todo TO REMOVE **/
	sctk_alloc_vchunk vfirst = NULL;
	struct sctk_alloc_free_macro_bloc* fbloc = (struct sctk_alloc_free_macro_bloc* )bloc;
	struct sctk_alloc_mm_source_default * source_default = (struct sctk_alloc_mm_source_default*)source;

	//error
	assume_m(source != NULL, "Can't free the memory without an allocation chain.");

	//trivial
	if (bloc == NULL)
		return;

	//get and check the chunk header
	vchunk = sctk_alloc_get_chunk((sctk_addr_t)bloc+sizeof(struct sctk_alloc_chunk_header_large));
	//manage the case of bad pointer and return withour doing anything
	if (vchunk == NULL)
		return;
	assume_m(vchunk->state == SCTK_ALLOC_CHUNK_STATE_ALLOCATED,"Double free corruption.");

	//lock access to the memory source
	sctk_alloc_spinlock_lock(&source_default->spinlock);

	//try some merge
	vchunk = sctk_alloc_merge_chunk(&source_default->pool,vchunk,vfirst,(sctk_addr_t)source_default->heap_addr+source_default->heap_size);
	fbloc = sctk_alloc_get_ptr(vchunk);

	//update mmap status
// 	if (sctk_alloc_get_size(vchunk) == original_size)
// 	{
// 		fbloc->maping = SCTK_ALLOC_BLOC_MAPPED;
// 		SCTK_PDEBUG("Insert as free mapped bloc : %p (%lu)",sctk_alloc_get_ptr(vchunk),sctk_alloc_get_size(vchunk)/1024/1024);
// 	} else {
		SCTK_NO_PDEBUG("Return memory to system : %p (%lu)",sctk_alloc_get_ptr(vchunk),sctk_alloc_get_size(vchunk)/1024/1024);
		fbloc->maping = SCTK_ALLOC_BLOC_UNMAPPED;
		sctk_munmap((void*)(sctk_alloc_get_addr(vchunk)+SCTK_ALLOC_PAGE_SIZE),sctk_alloc_get_size(vchunk)-SCTK_ALLOC_PAGE_SIZE);
// 	}

	//insert in free list
	sctk_alloc_free_list_insert(&source_default->pool,sctk_alloc_get_large(vchunk),SCTK_ALLOC_INSERT_AT_START);

	//lock access to the memory source
	sctk_alloc_spinlock_unlock(&source_default->spinlock);
}

/************************* FUNCTION ************************/
SCTK_STATIC void sctk_alloc_mm_source_default_cleanup(struct sctk_alloc_mm_source* source)
{
	//vars
	struct sctk_alloc_mm_source_default * source_default = (struct sctk_alloc_mm_source_default *)source;
	void * current;
	struct sctk_alloc_region * region;

	//lock access to the memory source
	assume_m(sctk_alloc_spinlock_trylock(&source_default->spinlock)==0,"Multiple thread access to memory source while calling cleanup is an error.");

	current = source_default->heap_addr;
	while (current < (void*)((sctk_addr_t)source_default->heap_addr + source_default->heap_size))
	{
		region = sctk_alloc_region_get(current);
 		/** @todo  remove internal segments if no always delete the while region. **/
 		/** @todo  Add a counter to avoid deleting a shared region. **/
 		sctk_alloc_region_del_chain(region,NULL);
 		sctk_alloc_region_del(region);
 		current = (void*)((sctk_addr_t)current + SCTK_REGION_SIZE);
 	}
	sctk_munmap(source_default->heap_addr,source_default->heap_size + SCTK_ALLOC_PAGE_SIZE);

	//cleanup spinlocks
	sctk_alloc_spinlock_unlock(&source_default->spinlock);
	sctk_alloc_spinlock_destroy(&source_default->spinlock);
}

/************************* FUNCTION ************************/
SCTK_STATIC struct sctk_alloc_macro_bloc* sctk_alloc_get_macro_bloc(void* ptr)
{
	return (struct sctk_alloc_macro_bloc *)((sctk_addr_t)ptr - ((sctk_addr_t)ptr % SCTK_MACRO_BLOC_SIZE));
}

/************************* FUNCTION ************************/
/**
 * As for sctk_alloc_pdebug, it may help for printing error in a non buffered way.
**/
void sctk_alloc_perror (const char * format,...)
{
	char tmp[4096];
	char tmp2[4096];
	va_list param;
	sctk_alloc_sprintf (tmp,4096,"SCTK_ALLOC_ERROR : %s\n", format);
	va_start (param, format);
	sctk_alloc_vsprintf (tmp2,4096, tmp, param);
	va_end (param);
	sctk_safe_write(STDERR_FILENO,tmp2,strlen(tmp2));
}

/************************* FUNCTION ************************/
/**
 * As for sctk_alloc_pdebug, it may help for printing error in a non buffered way.
**/
void sctk_alloc_pwarning (const char * format,...)
{
	char tmp[4096];
	char tmp2[4096];
	va_list param;
	sctk_alloc_sprintf (tmp,4096, "SCTK_ALLOC_WARNING : %s\n", format);
	va_start (param, format);
	sctk_alloc_vsprintf (tmp2,4096, tmp, param);
	va_end (param);
	sctk_safe_write(STDERR_FILENO,tmp2,(unsigned int)strlen(tmp2));
}

/************************* FUNCTION ************************/
/**
 * Calculate and return the region ID corresponding the the given address.
**/
SCTK_STATIC int sctk_alloc_region_get_id(void * addr)
{
	/** @todo can be optimize if we consider power of 2 **/
	int id = (int)((sctk_addr_t)addr / SCTK_REGION_SIZE);
	assert(id >= 0 && id < SCTK_ALLOC_MAX_REGIONS);
	return id;
}

/************************* FUNCTION ************************/
/**
 * Setup a specific region, it mostly call mmap to prepare memory for the region header.
 * The memory will not be touched except the first page (this may be optimized by moving
 * this region header out of the region, but it may became complexe.)
 * Caution, it will use a mutex to protect access to region list.
 * @param addr Define the address for which to setup a region (don't need to be region aligned).
**/
SCTK_STATIC struct sctk_alloc_region * sctk_alloc_region_setup(void * addr)
{
	struct sctk_alloc_region * region = NULL;
	//get region ID
	int id = sctk_alloc_region_get_id(addr);
	assert(id >= 0);

	//some checks
	assert(sizeof(struct sctk_alloc_region) <= SCTK_REGION_HEADER_SIZE);

	//ensure init and take the lock
	sctk_alloc_region_init();
	sctk_alloc_spinlock_lock(&sctk_alloc_glob_regions_lock);

	//check if already mapped, otherwise, to nothing
	if (sctk_alloc_glob_regions[id] == NULL)
	{
		/** @todo this may be better to hardly control this address choice, maybe use the allocator when a first chain is available. **/
		region = sctk_mmap(NULL,SCTK_REGION_HEADER_SIZE);
		if (region == MAP_FAILED)
		{
			perror("Can't allocate region header.");
			abort();
		}
		/** @todo PARALLEL check for atomic operation **/
		sctk_alloc_glob_regions[id] = region;
	} else {
		region = sctk_alloc_glob_regions[id];
	}

	//unlock
	sctk_alloc_spinlock_unlock(&sctk_alloc_glob_regions_lock);

	//return pointer to the region
	return region;
}

/************************* FUNCTION ************************/
/**
 * Calculate the adress of region header from the given address.
 * Caution, it didn't check the existance of the region, if it didn't, you will get a segfault while
 * trying to access to it.
 * @param addr Define the address for which we request the related region.
**/
SCTK_STATIC struct sctk_alloc_region * sctk_alloc_region_get(void * addr)
{
	int id = sctk_alloc_region_get_id(addr);
	return sctk_alloc_glob_regions[id];
}

/************************* FUNCTION ************************/
/**
 * Calculate the address of region entry for a given macro bloc.
 * Caution, it didn't check the existance of the region, if it didn't, you will get a segfault while
 * trying to access to it.
 * @param addr Define the address for chich we request the related region entry.
**/
struct sctk_alloc_region_entry * sctk_alloc_region_get_entry(void* addr)
{
	//vars
	sctk_addr_t id;
	struct sctk_alloc_region * region = sctk_alloc_region_get(addr);
	if (region == NULL)
		region = sctk_alloc_region_setup(addr);
	id = (((sctk_addr_t)addr)%SCTK_REGION_SIZE) / SCTK_MACRO_BLOC_SIZE;
	
	assert(id < SCTK_REGION_HEADER_ENTRIES);
	assert(id >= 0);
	
	return region->entries+id;
}

/************************* FUNCTION ************************/
SCTK_STATIC void sctk_alloc_region_unset_entry(struct sctk_alloc_macro_bloc * macro_bloc)
{
	//vars
	sctk_addr_t ptr = (sctk_addr_t)macro_bloc;
	struct sctk_alloc_region_entry * dest;

	//errors
	assert(macro_bloc != NULL);

	/** @todo TO OPTIMIZE by avoiding calling sctk_alloc_region_get_entry each time **/
	while (ptr < (sctk_addr_t)macro_bloc + sctk_alloc_get_chunk_header_large_size(&macro_bloc->header))
	{
		dest = sctk_alloc_region_get_entry((void *)ptr);
		dest->macro_bloc = NULL;
		ptr += SCTK_MACRO_BLOC_SIZE;
	}
}

/************************* FUNCTION ************************/
/**
 * Setup a copy af all entries in region header to cover the given segment.
 * @param entry Define the new state of the header entry top copy.
 * @param base_addr Define the base address of the segment for which to setup the region entries.
 * @param size Define the segment size to know how many time we need to copy the entry.
**/
SCTK_STATIC void sctk_alloc_region_set_entry(struct sctk_alloc_chain * chain, struct sctk_alloc_macro_bloc * macro_bloc)
{
	sctk_addr_t ptr = (sctk_addr_t)macro_bloc;
	struct sctk_alloc_region_entry * dest;

	//errors
	assert(macro_bloc != NULL);
	assert(macro_bloc->chain == NULL || macro_bloc->chain == chain);
	assert(! (chain->flags & SCTK_ALLOC_CHAIN_DISABLE_REGION_REGISTER));

	//setup macro bloc chain
	macro_bloc->chain = chain;

	//warn if too small
	if (sctk_alloc_get_chunk_header_large_size(&macro_bloc->header) < SCTK_MACRO_BLOC_SIZE)
		warning("Caution, using macro blocs smaller than SCTK_MACRO_BLOC_SIZE is dangerous, check usage of flag SCTK_ALLOC_CHAIN_DISABLE_REGION_REGISTER.");

	/** @TODO TO OPTIMIZE by avoiding calling sctk_alloc_region_get_entry each time. **/
	/** @TODO Support sub link list for macro_bloc imbrication. **/
	while (ptr < (sctk_addr_t)macro_bloc + sctk_alloc_get_chunk_header_large_size(&macro_bloc->header))
	{
		dest = sctk_alloc_region_get_entry((void *)ptr);
		if (dest != NULL)
			assume_m(dest->macro_bloc == NULL, "For now, don't support multiple usage of region entries");
		dest->macro_bloc = macro_bloc;
		ptr += SCTK_MACRO_BLOC_SIZE;
	}
}

/************************* FUNCTION ************************/
/**
 * Check if the region contains references to some allocation chaine. In other it check if some
 * segments of the region are in use, if not we can safetly delete the region.
 * This may be improved by using a counter to know how-mainy allocation chain use this region.
**/
SCTK_STATIC bool sctk_alloc_region_has_ref(struct sctk_alloc_region * region)
{
	/** @todo Use a counter to know how many chain shares the same region.**/
	unsigned long i = 0;

	for ( i = 0 ; i < SCTK_REGION_HEADER_ENTRIES ; ++i )
		if (region->entries[i].macro_bloc != NULL)
			return true;

	return false;
}

/************************* FUNCTION ************************/
/**
 * Remove all referencies to the given chain in the header of given region.
 * @param chain You can use NULL on chain to remove all chains.
**/
SCTK_STATIC void sctk_alloc_region_del_chain(struct sctk_alloc_region * region,struct sctk_alloc_chain * chain)
{
	int i;
	//nothing to do
	if (region == NULL)
		return;
	
	//ensure init and take the lock
	sctk_alloc_region_init();
	sctk_alloc_spinlock_lock(&sctk_alloc_glob_regions_lock);
	
	for ( i = 0 ; i < SCTK_REGION_HEADER_ENTRIES ; ++i )
	{
		if (chain == NULL)
			region->entries[i].macro_bloc = NULL;
		if (region->entries[i].macro_bloc != NULL)
			if (region->entries[i].macro_bloc->chain == chain)
				region->entries[i].macro_bloc = NULL;
	}

	//unlock
	sctk_alloc_spinlock_unlock(&sctk_alloc_glob_regions_lock);
}

/************************* FUNCTION ************************/
struct sctk_alloc_macro_bloc * sctk_alloc_region_get_macro_bloc(void * ptr)
{
	//vars
	struct sctk_alloc_macro_bloc * macro_bloc = NULL;
	struct sctk_alloc_region_entry * entry = NULL;

	//get the entry
	entry = sctk_alloc_region_get_entry(ptr);
	if (entry == NULL)
	{
		macro_bloc = NULL;
	} else if ((entry->macro_bloc == NULL || (void*)entry->macro_bloc > ptr) && (sctk_addr_t)ptr > SCTK_MACRO_BLOC_SIZE) {
		entry = sctk_alloc_region_get_entry((void*)((sctk_addr_t)ptr - SCTK_MACRO_BLOC_SIZE));
		if (entry == NULL || entry->macro_bloc == NULL)
			macro_bloc = NULL;
		else if (ptr > (void*)((sctk_addr_t)entry->macro_bloc) && ptr < (void*)((sctk_addr_t)entry->macro_bloc +sctk_alloc_get_chunk_header_large_size(&entry->macro_bloc->header)))
			macro_bloc = entry->macro_bloc;
		else
			macro_bloc = NULL;
	} else {
		macro_bloc = entry->macro_bloc;
	}

	//final status
	return macro_bloc;
}

/************************* FUNCTION ************************/
/**
 * Remove a specific region. It simply apply a bug munmap on the while region (not only on the header)
 * so we ensure global liberation.
 * @param region Define the region to remove.
**/
SCTK_STATIC void sctk_alloc_region_del(struct sctk_alloc_region * region)
{
	int id = 0;
	//trivial
	if (region == NULL)
		return;

	//check if it stay something
	if (sctk_alloc_region_has_ref(region) == true)
	{
		warning("Can't delete the given region because it contain refs to valid chains.");
		return;
	}

	//ensure init and take the lock
	sctk_alloc_region_init();
	sctk_alloc_spinlock_lock(&sctk_alloc_glob_regions_lock);

	//search region id
	while (id < SCTK_ALLOC_MAX_REGIONS && sctk_alloc_glob_regions[id] != region)
		id++;

	/** @todo  Need to check multiple usage of the region. **/
	if (id < SCTK_ALLOC_MAX_REGIONS)
	{
		/** @todo  PARALLEL Check for atomic operation **/
		sctk_alloc_glob_regions[id] = NULL;
	}
	
	//unlock
	sctk_alloc_spinlock_unlock(&sctk_alloc_glob_regions_lock);

	//unmap the region
	sctk_munmap(region,SCTK_REGION_HEADER_SIZE);
}

/************************* FUNCTION ************************/
/**
 * Test if a specific region exist by searching in a global list. It permit to avoid a segfault
 * on access if we didn't know if a specific region already exist (mainly on memory source creation
 * if we tolarate sharing regions even if it may not append at the end).
 * @param addr Define a pointer in the wanted region, it could be the base address or an inner
 * element. This pointer didn't need to be aligned as for sctk_alloc_region_get()
**/
SCTK_STATIC bool sctk_alloc_region_exist(void* addr)
{
	return sctk_alloc_region_get(addr) != NULL;
}

/************************* FUNCTION ************************/
/** @todo  Call this in global allocator init instead of everywhere **/
/**
 * Use to init the region list global lock. For now we call it everytime before using the spinlock
 * but it may be preferable to call it one time at allocator initialisation.
**/
SCTK_STATIC void sctk_alloc_region_init(void )
{
	if (!sctk_alloc_glob_regions_init)
	{
		sctk_alloc_spinlock_init(&sctk_alloc_glob_regions_lock,0);
		sctk_alloc_glob_regions_init = true;

		//init pointers to NULL
		memset(sctk_alloc_glob_regions,0,sizeof(sctk_alloc_glob_regions));
	}
}

/************************* FUNCTION ************************/
/**
 * Remove all active region, this is used to make a global cleanup.
**/
SCTK_STATIC void sctk_alloc_region_del_all(void)
{
	int i;
	//the lock is taken by _del, but it may be better to place them here....
	//ensure init and take the lock
	//sctk_alloc_region_init();
	//sctk_alloc_spinlock_lock(&sctk_alloc_glob_regions_lock);

	/** @todo  To optimize, we done multiple loop as region_del also loop on the list. **/
	for (i = 0 ; i < SCTK_ALLOC_MAX_REGIONS ; ++i)
		if (sctk_alloc_glob_regions[i] != NULL)
		{
			sctk_alloc_region_del_chain(sctk_alloc_glob_regions[i],NULL);
			sctk_alloc_region_del(sctk_alloc_glob_regions[i]);
		}

	//unlock
	//sctk_alloc_spinlock_unlock(&sctk_alloc_glob_regions_lock);
}

/************************* FUNCTION ************************/
/**
 * Function used to initialize the content of RFQ struct.
 * @param rfq Define the RFQ struct to initialize.
**/
SCTK_STATIC void sctk_alloc_rfq_init(struct sctk_alloc_rfq * rfq)
{
	if (rfq != NULL)
	{
		rfq->first = NULL;
		rfq->last = NULL;
		sctk_alloc_spinlock_init(&rfq->lock,PTHREAD_PROCESS_PRIVATE);
	}
}

/************************* FUNCTION ************************/
/**
 * Check if the rfq contain elements or not. It try to not disturb parallel usage of the rfq
 * and didn't take the lock. Return true on NULL rfq.
 * Without the lock it cannot stricly guaranty the result.
 * @param rfq Define the RFQ to check.
**/
SCTK_STATIC bool sctk_alloc_rfq_empty(struct sctk_alloc_rfq * rfq)
{
	if (rfq == NULL)
		return true;
	else
		return rfq->first == NULL;
}

/************************* FUNCTION ************************/
/**
 * Register a new pointer to free into the list. It will take the lock befire insterion.
 * @param rfq Define the RFQ into which to insert the free request.
 * @param ptr Define the segment for which to delay the free.
**/
void sctk_alloc_rfq_register(struct sctk_alloc_rfq * rfq,void * ptr)
{
	//vars
	sctk_alloc_vchunk vchunk;
	struct sctk_alloc_rfq_entry * entry;
	
	//nothing to do
	if (ptr == NULL)
		return;

	//error
	assert(rfq != NULL);

	//create the request info
	vchunk = sctk_alloc_get_chunk((sctk_addr_t)ptr);
	switch(vchunk->type)
	{
		case SCTK_ALLOC_CHUNK_TYPE_LARGE://can contain the struct internally
			entry = ptr;
			break;
		default:
			assume_m(false,"Invalid chunk type.");
	}

	//setup entry
	entry->ptr = ptr;
	entry->next = NULL;

	//take the lock
	sctk_alloc_spinlock_lock(&rfq->lock);

	//insert
	if (rfq->last != NULL)
		rfq->last->next = entry;
	rfq->last = entry;
	if (rfq->first == NULL)
		rfq->first = entry;

	//let the lock
	sctk_alloc_spinlock_unlock(&rfq->lock);
}

/************************* FUNCTION ************************/
/**
 * Take the lock, extract the base pointer of the list and reset the rfq. The base element of
 * extracted list is returned. Return NULL if empty.
 * @param rfq Define the remote free queue to use.
**/
SCTK_STATIC struct sctk_alloc_rfq_entry * sctk_alloc_rfq_extract(struct sctk_alloc_rfq * rfq)
{
	struct sctk_alloc_rfq_entry * res;
	
	//error
	if (rfq == NULL)
		return NULL;

	//if empty, no need to take the lock, can exi now
	if (rfq->first == NULL)
		return NULL;

	//take the lock
	sctk_alloc_spinlock_lock(&rfq->lock);

	res = rfq->first;
	rfq->first = NULL;
	rfq->last = NULL;

	//let the lock
	sctk_alloc_spinlock_unlock(&rfq->lock);

	return ( struct sctk_alloc_rfq_entry *)res;
}

/************************* FUNCTION ************************/
/**
 * Return the number of element in the given list of RFQ entries. This is more a debugging helper
 * than a required function.
 * @param entries Define the first element of the list of NULL if empty.
**/
SCTK_STATIC int sctk_alloc_rfq_count_entries(struct sctk_alloc_rfq_entry * entries)
{
	struct sctk_alloc_rfq_entry * cur = entries;
	int cnt = 0;

	while (entries != NULL)
	{
		cnt++;
		cur = cur->next;
	}
	return cnt;
}

/************************* FUNCTION ************************/
/**
 * Function used to cleanup internal values of remote free queue.
 * The lock on RFQ must be free otherwise it will report an error as the structure mustn't be in
 * use in another function to be freed.
 * @param rfq Define the RFQ to cleanup.
**/
SCTK_STATIC void sctk_alloc_rfq_destroy(struct sctk_alloc_rfq * rfq)
{
	if (rfq == NULL)
		return;

	//try to take the lock
	assume_m( ! sctk_alloc_spinlock_trylock(&rfq->lock) ,"Can't take the lock of Remote Free Queue to made the cleanup.");

	//check empty list
	assume_m(rfq->first == NULL && rfq->last == NULL,"Can't cleanup a non empty Remote Free Queue.");

	//cleanup the spinlock
	sctk_alloc_spinlock_destroy(&rfq->lock);
}

/************************* FUNCTION ************************/
/**
 * Migrate an allocation to another NUMA node if numa is supported otherwise it does nothing.
 * @param chain Define the allocation chain to migrate.
 * @param target_numa_node Define the targeted NUMA node, you can use -1 to reset memory binding.
**/
#ifdef HAVE_HWLOC
SCTK_STATIC void sctk_alloc_chain_numa_migrate_content(struct sctk_alloc_chain * chain, int target_numa_node)
{
	//vars
	int i;
	int j;
	struct sctk_alloc_region * region = NULL;

	//errors
	assert(chain != NULL);
	assert(target_numa_node >= -1);
	assume_m(!(chain->flags & SCTK_ALLOC_CHAIN_DISABLE_REGION_REGISTER),"NUMA migration of allocation chain is not supported in conjunction with SCTK_ALLOC_CHAIN_DISABLE_REGION_REGISTER.");

	//lock the region map
	sctk_alloc_spinlock_lock(&sctk_alloc_glob_regions_lock);

	//loop on all region entries
	/** @TODO Caution by reading like this we touch empty pages, maybe add a bitmap to avoid that for j. **/
	for ( i = 0 ; i < SCTK_ALLOC_MAX_REGIONS ; i++)
	{
		region = sctk_alloc_glob_regions[i];
		if (region != NULL)
			for ( j = 0 ; j < SCTK_REGION_HEADER_ENTRIES ; j++)
				if (region->entries[j].macro_bloc != NULL)
					if (region->entries[j].macro_bloc->chain == chain)
						sctk_alloc_migrate_numa_mem(region->entries[j].macro_bloc,sctk_alloc_get_chunk_header_large_size(&region->entries[j].macro_bloc->header),target_numa_node);
	}

	//lock the region map
	sctk_alloc_spinlock_unlock(&sctk_alloc_glob_regions_lock);
}
#endif //HAVE_HWLOC

/************************* FUNCTION ************************/
void sctk_alloc_chain_get_numa_stat(struct sctk_alloc_numa_stat_s * numa_stat,struct sctk_alloc_chain * chain)
{
	//vars
	int i;
	int j;
	struct sctk_alloc_region * region = NULL;

	//errors
	assert(numa_stat != NULL);

	//init stat
	sctk_alloc_numa_stat_init(numa_stat);

	//lock the region map
	sctk_alloc_spinlock_lock(&sctk_alloc_glob_regions_lock);

	//loop on all region entries
	/** @TODO Caution by reading like this we touch empty pages, maybe add a bitmap to avoid that for j. **/
	for ( i = 0 ; i < SCTK_ALLOC_MAX_REGIONS ; i++)
	{
		region = sctk_alloc_glob_regions[i];
		if (region != NULL)
			for ( j = 0 ; j < SCTK_REGION_HEADER_ENTRIES ; j++)
				if (region->entries[j].macro_bloc != NULL)
					if (region->entries[j].macro_bloc->chain == chain)
						sctk_alloc_numa_stat_cumul(numa_stat,region->entries[j].macro_bloc,sctk_alloc_get_chunk_header_large_size(&region->entries[j].macro_bloc->header));
	}

	//lock the region map
	sctk_alloc_spinlock_unlock(&sctk_alloc_glob_regions_lock);
}

/************************* FUNCTION ************************/
void sctk_alloc_chain_get_stat(struct sctk_alloc_chain_stat * chain_stat,struct sctk_alloc_chain * chain)
{
	//vars
	int i;
	int nb_free_lists;
	struct sctk_alloc_free_chunk * free_chunk;
	struct sctk_alloc_free_chunk * free_lists;

	//errors
	assert(chain != NULL);
	assert(chain_stat != NULL);

	//reset stat
	chain_stat->max_free_size = 0;
	chain_stat->min_free_size = -1;
	chain_stat->nb_free_chunks = 0;
	chain_stat->nb_macro_blocs = 0;
	chain_stat->cached_free_memory = 0;

	//lock the chain
	if (chain->flags & SCTK_ALLOC_CHAIN_FLAGS_THREAD_SAFE)
		sctk_alloc_spinlock_lock(&chain->lock);

	//loop on free chunks
	free_lists = chain->pool.free_lists;
	nb_free_lists = chain->pool.nb_free_lists;
	for (i = 0 ; i  < nb_free_lists; ++i) {
		free_chunk = free_lists[i].next;
		while(free_chunk != free_lists+i)
		{
			chain_stat->nb_free_chunks++;
			chain_stat->cached_free_memory += sctk_alloc_get_chunk_header_large_size(&free_chunk->header);
			if (sctk_alloc_get_chunk_header_large_size(&free_chunk->header) < chain_stat->min_free_size)
				chain_stat->min_free_size = sctk_alloc_get_chunk_header_large_size(&free_chunk->header);
			if (sctk_alloc_get_chunk_header_large_size(&free_chunk->header) > chain_stat->max_free_size)
				chain_stat->max_free_size = sctk_alloc_get_chunk_header_large_size(&free_chunk->header);
			free_chunk = free_chunk->next;
		}
	}

	//unlock
	if (chain->flags & SCTK_ALLOC_CHAIN_FLAGS_THREAD_SAFE)
		sctk_alloc_spinlock_lock(&chain->lock);

	if (chain_stat->min_free_size == -1)
		chain_stat->min_free_size = 0;
}

/************************* FUNCTION ************************/
void sctk_alloc_chain_print_stat(struct sctk_alloc_chain * chain)
{
	//vars
	struct sctk_alloc_numa_stat_s numa_stat;
	struct sctk_alloc_chain_stat chain_stat;
	
	//errors
	assert(chain != NULL);

	//read stat
	sctk_alloc_chain_get_numa_stat(&numa_stat,chain);
	sctk_alloc_chain_get_stat(&chain_stat,chain);

	//print it
	printf("====================== ALLOCATION CHAIN STAT ======================\n");
	printf("%-20s : %d\n","Thread ID",getpid());
	printf("%-20s : %s\n","Name",chain->name);
	printf("%-20s : %p\n","Chain",chain);
	printf("%-20s : %p\n","Memory source",chain->source);
	printf("%-20s : %d\n","Source NUMA node",sctk_alloc_chain_get_numa_node(chain));
	#ifdef HAVE_HWLOC
	printf("%-20s : %d\n","Preferred NUMA node",sctk_get_preferred_numa_node());
	#endif //HAVE_HWLOC
	printf("%-20s : %lu\n","Cached free memory",chain_stat.cached_free_memory);
	printf("%-20s : %lu\n","Min free size",chain_stat.min_free_size);
	printf("%-20s : %lu\n","Max free size",chain_stat.max_free_size);
	printf("%-20s : %lu\n","Nb free chunks",chain_stat.nb_free_chunks);
	//TODO
	//printf("%-20s : %lu\n","Nb macro blocs",chain_stat.nb_macro_blocs);
	sctk_alloc_numa_stat_print(&numa_stat,NULL,0);
	printf("===================================================================\n");
}

/************************* FUNCTION ************************/
int sctk_alloc_chain_get_numa_node(struct sctk_alloc_chain * chain)
{
	//vars
	struct sctk_alloc_mm_source_light * light_source;

	//errors
	assert(chain != NULL);

	//convert the source
	light_source = sctk_alloc_get_mm_source_light(chain->source);

	//extract numa node
	if (light_source != NULL)
		return light_source->numa_node;
	else
		return -1;
}

/************************* FUNCTION ************************/
/**
 * Migrate an allocation to another NUMA node if numa is supported otherwise it does nothing.
 * @param chain Define the allocation chain to migrate.
 * @param target_numa_node Define the targeted NUMA node, you can use -1 to reset memory binding.
 * @param migrate_chain_struct Enable of disable migration of the pages of the allocation chain itself.
 * @param migrate_content Enable of disable migration of the current pages returned by the allocation chain.
 * @param new_mm_source Define the new memory source to link to this allocation chain you can use
 * SCTK_ALLOC_KEEP_OLD_MM_SOURCE.
**/
void sctk_alloc_chain_numa_migrate(struct sctk_alloc_chain * chain, int target_numa_node,bool migrate_chain_struct,bool migrate_content,struct sctk_alloc_mm_source * new_mm_source)
{
	//errors
	assert(chain != NULL);
	assert(target_numa_node >= -1);

	SCTK_PDEBUG("Call migration to numa node %d on chain",target_numa_node,chain);

	#ifdef HAVE_HWLOC
	//remap the struct itself
	/** @todo Get error on cassard ??? **/
	if (migrate_chain_struct)
		sctk_alloc_migrate_numa_mem(chain,sizeof(struct sctk_alloc_chain),target_numa_node);

	//remap the content
	if (migrate_chain_struct)
		sctk_alloc_chain_numa_migrate_content(chain,target_numa_node);
	#endif //HAVE_HWLOC
	
	//update the mm source
	if (new_mm_source != SCTK_ALLOC_KEEP_OLD_MM_SOURCE)
		chain->source = new_mm_source;
}
