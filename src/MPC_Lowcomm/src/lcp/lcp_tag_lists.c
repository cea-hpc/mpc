/* ############################# MPC License ############################## */
/* # Thu May  6 10:26:16 CEST 2021                                        # */
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
/* # Maintainers:                                                         # */
/* # - CARRIBAULT Patrick patrick.carribault@cea.fr                       # */
/* # - JAEGER Julien julien.jaeger@cea.fr                                 # */
/* # - PERACHE Marc marc.perache@cea.fr                                   # */
/* # - ROUSSEL Adrien adrien.roussel@cea.fr                               # */
/* # - TABOADA Hugo hugo.taboada@cea.fr                                   # */
/* #                                                                      # */
/* # Authors:                                                             # */
/* # - CANAT Paul pcanat@paratools.fr                                     # */
/* # - BESNARD Jean-Baptiste jbbesnard@paratools.com                      # */
/* # - MOREAU Gilles gilles.moreau@cea.fr                                 # */
/* #                                                                      # */
/* ######################################################################## */

#include <comm.h>
#include <utlist.h>
#include <sctk_alloc.h>

#include "lcp_tag_lists.h"


/*******************************************************
 * PRQ Posted Receive Queue
 ******************************************************/
 /**
  * @brief Cancel and delete a request from the PRQ.
  * 
  * @param list PRQ
  * @param req request to delete
  * @return int 
  */
int lcp_prq_cancel(lcp_mtch_prq_list_t *list, void *req)
{
	lcp_mtch_prq_elem_t *elem, *tmp;

	DL_FOREACH_SAFE(list->list, elem, tmp){
		if (req == elem->value) {
			DL_DELETE(list->list, elem);
			sctk_free(elem);
			list->size--;
			return 1;
		}
	}
	return 0;
}

/**
 * @brief Find a request by tag in the PRQ.
 * 
 * @param list PRQ
 * @param tag tag of the target request
 * @param peer source to be matched using request mask
 * @return void* 
 */
void *lcp_prq_find(lcp_mtch_prq_list_t *list, int tag, uint64_t peer)
{
	int result;
	lcp_mtch_prq_elem_t *elem, *tmp;

	DL_FOREACH_SAFE(list->list, elem, tmp){
		result = ((elem->tag & tag) || (!elem->tmask && tag >= 0)) &&
			((elem->src & elem->smask) == (peer & elem->smask));
		if (result) {
			return elem->value;
		}
	}
	return 0;
}

/**
 * @brief Find a request by tag and delete it from the PRQ.
 * 
 * @param list PRQ
 * @param tag tag of the request to delete
 * @param peer source to be matched using request mask
 * @return void* 
 */
void *lcp_prq_find_dequeue(lcp_mtch_prq_list_t *list, int tag, uint64_t peer)
{
	int result;
	lcp_mtch_prq_elem_t *elem, *tmp;

	DL_FOREACH_SAFE(list->list, elem, tmp){
		result = ((elem->tag == tag) || (!elem->tmask && tag >= 0)) &&
			((elem->src & elem->smask) == (peer & elem->smask));
		if (result) {
			DL_DELETE(list->list, elem);	
			list->size--;
			void * ret = elem->value;
			sctk_free(elem);
			return ret;
		}
	}
	return 0;
}

/**
 * @brief Add a request to the PRQ.
 * 
 * @param list PRQ
 * @param payload content of the request
 * @param tag tag of the request (can be MPC_ANY_TAG)
 * @param source request source 
 */
void lcp_prq_append(lcp_mtch_prq_list_t *list, void *payload, int tag, uint64_t source)
{
	int mask_tag;
	uint64_t mask_src;

	if((int)source == MPC_ANY_SOURCE) {
		mask_src = 0;
	} else {
		mask_src = ~0;
	}

	if(tag == MPC_ANY_TAG) {
		mask_tag = 0;
	} else {
		mask_tag = ~0;
	}
	
	lcp_mtch_prq_elem_t *elem = sctk_malloc(sizeof(lcp_mtch_prq_elem_t));

	elem->tag = tag;
	elem->tmask = mask_tag;
	elem->src = source;
	elem->smask = mask_src;
	elem->value = payload;	
	list->size++;

	DL_APPEND(list->list, elem);
}

/**
 * @brief Allocate and initialize a PRQ.
 * 
 * @return lcp_mtch_prq_list_t* initialized request
 */
lcp_mtch_prq_list_t *lcp_prq_init()
{
	lcp_mtch_prq_list_t *list = sctk_malloc(
			sizeof(lcp_mtch_prq_list_t));
	memset(list, 0, sizeof(lcp_mtch_prq_list_t));

	return list;
}
/**
 * @brief Destroy a PRQ.
 * 
 * @param list PRQ to destroy
 */
void lcp_mtch_prq_destroy(lcp_mtch_prq_list_t *list)
{
	sctk_free(list->list);
}

/*******************************************************
 * UMQ Unexpected Message Queue
 ******************************************************/
/**
 * @brief Cancel and delete a request from an UMQ
 * 
 * @param list UMQ
 * @param req reqeuest to delete
 * @return int LCP_SUCCESS in case of success
 */
int lcp_umq_cancel(lcp_mtch_umq_list_t *list, void *req)
{
	lcp_mtch_umq_elem_t *elem, *tmp;

	DL_FOREACH_SAFE(list->list, elem, tmp){
		if (req == elem->value) {
			DL_DELETE(list->list, elem);
			sctk_free(elem);
			list->size--;
			return 1;
		}
	}
	return 0;
}

/**
 * @brief Find a request by tag in an UMQ.
 * 
 * @param list UMQ
 * @param tag tag of the target request
 * @param peer source to be matched using request mask
 * @return void* 
 */
void *lcp_umq_find(lcp_mtch_umq_list_t *list, int tag, uint64_t peer)
{
	int result;
	lcp_mtch_umq_elem_t *elem, *tmp;

	int tmask = ~0;
	uint64_t smask = ~0;
	if (tag == MPC_ANY_TAG) {
		tmask = 0;
	}

	if ((int)peer == MPC_ANY_SOURCE) {
		smask = 0;
	}

	DL_FOREACH_SAFE(list->list, elem, tmp){
		result = ((elem->tag == tag) || (!tmask && elem->tag >= 0)) &&
			((elem->src & smask) == (peer & smask));
		if (result) {
			void * ret = elem->value;
			return ret;
		}
	}
	return 0;
}

/**
 * @brief Find a request by tag in an UMQ and delete it.
 * 
 * @param list UMQ
 * @param tag tag of the request to delete
 * @param peer source to be matched using request mask
 * @return void* 
 */
void *lcp_umq_find_dequeue(lcp_mtch_umq_list_t *list, int tag, uint64_t peer)
{
	int result;
	lcp_mtch_umq_elem_t *elem, *tmp;

	int tmask = ~0;
	uint64_t smask = ~0;
	if (tag == MPC_ANY_TAG) {
		tmask = 0;
	}

	if ((int)peer == MPC_ANY_SOURCE) {
		smask = 0;
	}

	DL_FOREACH_SAFE(list->list, elem, tmp){
                result = ((elem->tag == tag) || (!tmask && elem->tag >= 0)) &&
                        ((elem->src & smask) == (peer & smask));
		if (result) {
			DL_DELETE(list->list, elem);	
			list->size--;
			void * ret = elem->value;
			sctk_free(elem);
			return ret;
		}
	}
	return 0;
}

/**
 * @brief Add a request in an UMQ.
 * 
 * @param list UMQ
 * @param payload content of the request
 * @param tag tag of the request (can be MPC_ANY_TAG)
 * @param source request source 
 */
void lcp_umq_append(lcp_mtch_umq_list_t *list, void *payload, int tag, uint64_t source)
{
	lcp_mtch_umq_elem_t *elem = sctk_malloc(sizeof(lcp_mtch_umq_elem_t));

	elem->tag = tag;
	elem->src = source;
	elem->value = payload;	
	list->size++;

	DL_APPEND(list->list, elem);
}

/**
 * @brief Allocate and initialize an UMQ.
 * 
 * @return lcp_mtch_prq_list_t* initialized request
 */
lcp_mtch_umq_list_t *lcp_umq_init()
{
	lcp_mtch_umq_list_t *list = sctk_malloc(
			sizeof(lcp_mtch_umq_list_t));
	memset(list, 0, sizeof(lcp_mtch_umq_list_t));

	list->size = 0;

	return list;
}

/**
 * @brief Destroy an UMQ
 * 
 * @param list UMQ to destroy
 */
void lcp_mtch_umq_destroy(lcp_mtch_umq_list_t *list)
{
	sctk_free(list->list);
}
