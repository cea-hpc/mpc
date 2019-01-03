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

#ifndef SCTK_ALLOC_INLINED_H
#define SCTK_ALLOC_INLINED_H

#ifdef __cplusplus
extern "C"
{
#endif

/************************** HEADERS ************************/
#include <stdlib.h>
#include "sctk_alloc_debug.h"
#include "sctk_alloc_chunk.h"
#include "sctk_alloc_mmsrc.h"

/************************** MACROS *************************/
#define SCTK_ALLOC_PADDED_VCHUNK_SIZE 1ul

/************************** CONSTS *************************/
extern const sctk_alloc_vchunk SCTK_ALLOC_DEFAULT_CHUNK;

/************************* FUNCTION ************************/
static __inline__ bool sctk_alloc_is_power_of_two(sctk_size_t size)
{
	return ((size != 0) && !(size & (size-1)));
}

/**
 * Some functions to access attributes structure 
 */
/************************* FUNCTION ************************/
static __inline__ sctk_ssize_t sctk_alloc_get_chunk_header_large_size(struct sctk_alloc_chunk_header_large * chunk)
{
	#ifdef _WIN32
		//Windows doesn't support bitfield value of more than 32bits, but we have 56, so re-implement by and with masks and shift.
		//Here, take the first 56bits were the size if stored.
		return ( (*(sctk_ssize_t * ) chunk->size) & ((1ULL << 56 ) - 1 ) );
	#else
		//On Unix, we can use the more safe way with bitfield.
		return chunk->size;
	#endif
}

/************************* FUNCTION ************************/
static __inline__ unsigned char sctk_alloc_get_chunk_header_large_addr(struct sctk_alloc_chunk_header_large * chunk)
{
	return chunk->addr;
}

/************************* FUNCTION ************************/
static __inline__ sctk_size_t sctk_alloc_get_chunk_header_large_previous_size(struct sctk_alloc_chunk_header_large * chunk)
{
	#ifdef _WIN32
		//Windows doesn't support bitfield value of more than 32bits, but we have 56, so re-implement by and with masks and shift.
		//Here, take the first 56bits were the size if stored.
		return ( (*(sctk_size_t * ) chunk->prevSize) & ((1ULL << 56 ) - 1 ) );
	#else
		//On Unix, we can use the more safe way with bitfield.
		return chunk->prevSize;
	#endif
}

/************************* FUNCTION ************************/
static __inline__ struct sctk_alloc_chunk_info * sctk_alloc_get_chunk_header_large_info(struct sctk_alloc_chunk_header_large * chunk)
{
	return &chunk->info;
}

/************************* FUNCTION ************************/
static __inline__ sctk_size_t sctk_alloc_get_chunk_header_padded_padding(struct sctk_alloc_chunk_header_padded * chunk)
{
	#ifdef _WIN32
		//Windows doesn't support bitfield value of more than 32bits, but we have 56, so re-implement by and with masks and shift.
		//Here, take the first 56bits were the size if stored.
		return ( (*(sctk_size_t * ) chunk->padding) & ((1ULL << 56 ) - 1 ) );
	#else
		//On Unix, we can use the more safe way with bitfield.
		return chunk->padding;
	#endif
}

/************************* FUNCTION ************************/
static __inline__ struct sctk_alloc_chunk_info * sctk_alloc_get_chunk_header_padded_info(struct sctk_alloc_chunk_header_padded * chunk)
{
	return &chunk->info;
}

/**
 * Some functions to set attributes structures
 */
/************************* FUNCTION ************************/
static __inline__ void sctk_alloc_set_chunk_header_large_size(struct sctk_alloc_chunk_header_large * chunk, sctk_size_t size)
{
	#ifdef _WIN32
		//Windows doesn't support bitfield value of more than 32bits, but we have 56, so re-implement by and with masks and shift.
		//Here, remplace the first 56bit with the onces from "size" and complete with the old value of addr, then erase the whole 64bit word
		//need to copy locally to be sure of typing.
		sctk_size_t addr = (sctk_size_t)chunk->addr;
		*(sctk_size_t *)chunk->size = ((size & ((1ULL<<56)-1)) | ((addr) << 56));
	#else
		chunk->size = size;
	#endif
}
/************************* FUNCTION ************************/
static __inline__ void sctk_alloc_set_chunk_header_large_addr(struct sctk_alloc_chunk_header_large * chunk, unsigned char addr)
{
	chunk->addr = addr;
}

/************************* FUNCTION ************************/
static __inline__ void sctk_alloc_set_chunk_header_large_previous_size(struct sctk_alloc_chunk_header_large * chunk, sctk_size_t prevSize)
{
	#ifdef _WIN32
		//Windows doesn't support bitfield value of more than 32bits, but we have 56, so re-implement by and with masks and shift.
		//Here, remplace the first 56bit with the onces from "size" and complete with the old value of addr, then erase the whole 64bit word
		//need to copy locally to be sure of typing.
		sctk_size_t info = (sctk_size_t)(*((unsigned char*)(&chunk->info)));
		(*(sctk_size_t *)(chunk->prevSize)) = ((prevSize & ((1ULL<<56)-1)) | ((info) << 56));
	#else
		chunk->prevSize = prevSize;
	#endif
}

/************************* FUNCTION ************************/
static __inline__ void sctk_alloc_set_chunk_header_padded_padding(struct sctk_alloc_chunk_header_padded * chunk, sctk_size_t padding)
{
	#ifdef _WIN32
		//Windows doesn't support bitfield value of more than 32bits, but we have 56, so re-implement by and with masks and shift.
		//Here, remplace the first 56bit with the onces from "size" and complete with the old value of addr, then erase the whole 64bit word
		//need to copy locally to be sure of typing.
		sctk_size_t info = (sctk_size_t)(*((unsigned char*)(&chunk->info)));
		*(sctk_size_t *)chunk->padding = ((padding & ((1ULL<<56)-1)) | ((info) << 56));
	#else
		chunk->padding = padding;
	#endif
}

/************************* FUNCTION ************************/
/**
 * Get the vchunk description from a raw pointer. This is mostly to be used in free/realloc
 * to get the chunk header from the void pointer given by the user.
 * This method automatically detect the type of chunk depending on their size class.
 * @param ptr Define the pointer used by the final user (next byte after the chunk header).
**/

static __inline__ sctk_alloc_vchunk sctk_alloc_get_chunk(sctk_addr_t ptr)
{
	sctk_alloc_vchunk vchunk = SCTK_ALLOC_DEFAULT_CHUNK;


	//if null, return default
	if (ptr == 0)
		return SCTK_ALLOC_DEFAULT_CHUNK;

	//calc header info position
	vchunk = ((struct sctk_alloc_chunk_info *)ptr) - 1;

	//check type and magik number
	if (vchunk->unused_magik != SCTK_ALLOC_MAGIC_STATUS)
	{

	CRASH();
		SCTK_PDEBUG("Bad address is %p.",ptr);
		warning("Header content error while trying to find chunk header.");
                warning("Did you free a system malloc chunk with MPC ?");
                return SCTK_ALLOC_DEFAULT_CHUNK;
	}

        // check
        /** @todo maybe can be replaced by assert **/
        assume_m(vchunk->type == SCTK_ALLOC_CHUNK_TYPE_LARGE ||
                     vchunk->type == SCTK_ALLOC_CHUNK_TYPE_PADDED,
                 "Invalid chunk type.");

        return vchunk;
}

/************************* FUNCTION ************************/
static __inline__ struct sctk_alloc_chunk_header_large * sctk_alloc_get_large(sctk_alloc_vchunk vchunk)
{
	assert(vchunk!=NULL);
	assert(vchunk->type == SCTK_ALLOC_CHUNK_TYPE_LARGE);
	return (struct sctk_alloc_chunk_header_large *)(vchunk+1) - 1;
}

/************************* FUNCTION ************************/
static __inline__ struct sctk_alloc_chunk_header_padded * sctk_alloc_get_padded(sctk_alloc_vchunk vchunk)
{
	assert(vchunk!=NULL);
	assert(vchunk->type == SCTK_ALLOC_CHUNK_TYPE_PADDED);
	return (struct sctk_alloc_chunk_header_padded *)(vchunk+1) - 1;
}

/************************* FUNCTION ************************/
static __inline__ void * sctk_alloc_get_ptr(sctk_alloc_vchunk vchunk)
{
	assert(vchunk!=NULL);
	switch(vchunk->type)
	{
		case SCTK_ALLOC_CHUNK_TYPE_LARGE:
			return sctk_alloc_get_large(vchunk);
		case SCTK_ALLOC_CHUNK_TYPE_PADDED:
			return sctk_alloc_get_padded(vchunk);
		default:
			assume_m(false,"Invalid type of vchunk.");
			return NULL;
	}
}

/************************* FUNCTION ************************/
static __inline__ sctk_addr_t sctk_alloc_get_addr(sctk_alloc_vchunk vchunk)
{
	return (sctk_addr_t)sctk_alloc_get_ptr(vchunk);
}

/************************* FUNCTION ************************/
static __inline__ sctk_size_t sctk_alloc_get_size(sctk_alloc_vchunk vchunk)
{
	return sctk_alloc_get_chunk_header_large_size(sctk_alloc_get_large(vchunk));
}

/************************* FUNCTION ************************/
static __inline__ sctk_size_t sctk_alloc_get_usable_size(sctk_alloc_vchunk vchunk)
{
	return sctk_alloc_get_chunk_header_large_size(sctk_alloc_get_large(vchunk)) - sizeof(struct sctk_alloc_chunk_header_large);
}

/************************* FUNCTION ************************/
static __inline__ sctk_size_t sctk_alloc_get_prev_size(sctk_alloc_vchunk vchunk)
{
	return sctk_alloc_get_chunk_header_large_previous_size(sctk_alloc_get_large(vchunk));
}

/************************* FUNCTION ************************/
static __inline__ sctk_alloc_vchunk sctk_alloc_large_to_vchunk(struct sctk_alloc_chunk_header_large * chunk_large)
{
	assert(chunk_large != NULL);
	return sctk_alloc_get_chunk_header_large_info(chunk_large);
}

/************************* FUNCTION ************************/
static __inline__ sctk_alloc_vchunk sctk_alloc_padded_to_vchunk(struct sctk_alloc_chunk_header_padded * chunk_padded)
{
	assert(chunk_padded != NULL);
	return sctk_alloc_get_chunk_header_padded_info(chunk_padded);
}

/************************* FUNCTION ************************/
/**
 * Remove the padding described in header of the given vchunk and return the parent vchunk.
 * @param vchunk Define the vchunk from which to remove padding.
**/
static __inline__ sctk_alloc_vchunk sctk_alloc_unpadd_vchunk(struct sctk_alloc_chunk_header_padded * chunk_padded)
{
	//error
	assert(chunk_padded != NULL);
	assert(chunk_padded->info.type == SCTK_ALLOC_CHUNK_TYPE_PADDED);

	//return previous header by removing padding space.
	return sctk_alloc_get_chunk((sctk_addr_t)chunk_padded - sctk_alloc_get_chunk_header_padded_padding(chunk_padded));
}

/************************* FUNCTION ************************/
/**
 * Same than sctk_alloc_get_chunk, but automatically remove padding if get a padded header.
 * Caution, the current implementation didn't support encapsulation of multiple level of padding,
 * it will not be called recursivly.
 * @param ptr Define the pointer used by the final user (next byte after the chunk header).
**/
static __inline__ sctk_alloc_vchunk sctk_alloc_unpad_vchunk(sctk_alloc_vchunk vchunk)
{
	if (vchunk == NULL)
		return vchunk;
	else if (vchunk->type == SCTK_ALLOC_CHUNK_TYPE_PADDED)
		return sctk_alloc_unpadd_vchunk(sctk_alloc_get_padded(vchunk));
	else
		return vchunk;
}

/************************* FUNCTION ************************/
/**
 * Same than sctk_alloc_get_chunk, but automatically remove padding if get a padded header.
 * Caution, the current implementation didn't support encapsulation of multiple level of padding,
 * it will not be called recursivly.
 * @param ptr Define the pointer used by the final user (next byte after the chunk header).
**/
static __inline__ sctk_alloc_vchunk sctk_alloc_get_chunk_unpadded(sctk_addr_t ptr)
{
	return sctk_alloc_unpad_vchunk(sctk_alloc_get_chunk(ptr));
}

/************************* FUNCTION ************************/
static __inline__ sctk_size_t sctk_alloc_get_unpadded_size(sctk_alloc_vchunk vchunk)
{
	return sctk_alloc_get_chunk_header_large_size(sctk_alloc_get_large(sctk_alloc_unpad_vchunk(vchunk)));
}

/************************* FUNCTION ************************/
/**
 * Return the bloc next to the current one by using the size to calculate the next address.
 * Caution for now, this method only support large headers, we may need to generate another one
 * for the small one. Merging the two one may be less safer due to current header structure.
 * @param chunk Define the current chunk from which to jump to the next one.
 * @return Return the next chunk.
**/
static __inline__ sctk_alloc_vchunk sctk_alloc_get_next_chunk(sctk_alloc_vchunk chunk)
{
	sctk_alloc_vchunk res;

	#ifndef SCTK_ALLOC_FAST_BUT_LESS_SAFE
	assume_m(chunk->unused_magik == SCTK_ALLOC_MAGIC_STATUS,"Small block not supported for now.");
	#endif

	res = (sctk_alloc_vchunk)((sctk_addr_t)chunk + sctk_alloc_get_chunk_header_large_size(sctk_alloc_get_large(chunk)));

	return res;
}

/************************* FUNCTION ************************/
/**
 * Methode used to setup the chunk header. It select automatically the good one depending
 * on the given size.
 * @param ptr Define the base address of the chunk to setup.
 * @param size Define the size of the chunk (with the header size which will be removed
 * internaly).
 * @param prev Define address of previous chunk. It was used only in large bloc headers. Use NULL
 * or same address than ptr if none.
**/
static __inline__ struct sctk_alloc_chunk_header_large * sctk_alloc_setup_large_header(void * ptr, sctk_size_t size, void * prev)
{
	struct sctk_alloc_chunk_header_large * chunk_large;
	assert(ptr >= prev);

	//large blocs
	chunk_large = (struct sctk_alloc_chunk_header_large *)ptr;
	sctk_alloc_set_chunk_header_large_size(chunk_large, size);
	/** @todo  Need to cleanup this **/
	//for now we used a short for addr, by we got 5 more bytes which could be used to
	//store more checking bits
	sctk_alloc_set_chunk_header_large_addr(chunk_large, (unsigned char)((sctk_addr_t)ptr));
	sctk_alloc_get_chunk_header_large_info(chunk_large)->state = SCTK_ALLOC_CHUNK_STATE_ALLOCATED;
	sctk_alloc_get_chunk_header_large_info(chunk_large)->type = SCTK_ALLOC_CHUNK_TYPE_LARGE;
	sctk_alloc_get_chunk_header_large_info(chunk_large)->unused_magik = SCTK_ALLOC_MAGIC_STATUS;
	if (prev == NULL || prev == ptr)
		sctk_alloc_set_chunk_header_large_previous_size(chunk_large, 0);
	else
		sctk_alloc_set_chunk_header_large_previous_size(chunk_large, ((sctk_addr_t)ptr - (sctk_addr_t)prev));

	return chunk_large;
}

/************************* FUNCTION ************************/
static __inline__ struct sctk_alloc_macro_bloc * sctk_alloc_setup_macro_bloc(void * ptr,size_t size)
{
	struct sctk_alloc_macro_bloc * macro_bloc = (struct sctk_alloc_macro_bloc *)ptr;
	sctk_alloc_setup_large_header(&macro_bloc->header,size,NULL);
	macro_bloc->chain = NULL;
	SCTK_ALLOC_MMCHECK_NOACCESS(ptr,size);
	return macro_bloc;
}

/************************* FUNCTION ************************/
/**
 * Methode used to setup the chunk header. It select automatically the good one depending
 * on the given size.
 * @param ptr Define the base address of the chunk to setup.
 * @param size Define the size of the chunk (with the header size which will be removed
 * internaly).
 * @param prev Define address of previous chunk. It was used only in large bloc headers. Use NULL
 * or same address than ptr if none.
**/
static __inline__ sctk_alloc_vchunk sctk_alloc_setup_chunk(void * ptr, sctk_size_t size, void * prev)
{
	return sctk_alloc_large_to_vchunk(sctk_alloc_setup_large_header(ptr,size,prev));
}

#ifdef __cplusplus
}
#endif

#endif //SCTK_ALLOC_INLINED_H
