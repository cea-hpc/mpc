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

#include <sctk_alloc.h>
#include <uthash.h>

#include "lcp_tag_matching.h"
#include "lcp_request.h"
#include "mpc_common_datastructure.h"
#include "mpc_common_debug.h"
#include "mpc_common_spinlock.h"


lcp_prq_match_table_t * lcp_prq_match_table_init()
{
	lcp_prq_match_table_t * ret = sctk_malloc(sizeof(lcp_prq_match_table_t));
	assume(ret != NULL);

	mpc_common_spinlock_init(&ret->lock, 0);
	mpc_common_hashtable_init(&ret->ht, 2048);

	return ret;
}

/**
 * @brief Finalize the matching engine by deleting the posted/receive and unexpected queues.
 * 
 * @param umq unexpected messages queue (to destroy)
 * @param prq posted/receive queue (to destroy)
 */
void lcp_fini_matching_engine(lcp_umq_match_table_t *umq, 
			      lcp_prq_match_table_t *prq)
{
	lcp_prq_match_entry_t *prq_entry = NULL;
	lcp_umq_match_entry_t *umq_entry = NULL;

	MPC_HT_ITER( &prq->ht, prq_entry ) {
		lcp_mtch_prq_destroy(prq_entry->pr_queue);
		sctk_free(prq_entry->pr_queue);
		sctk_free(prq_entry);
	}
	MPC_HT_ITER_END(&prq->ht)

	MPC_HT_ITER( &umq->ht, umq_entry ) {
		lcp_mtch_umq_destroy(umq_entry->um_queue);
		sctk_free(umq_entry->um_queue);
		sctk_free(umq_entry);
	}
	MPC_HT_ITER_END(&umq->ht)

}

void* _prq_match_create_entry( uint64_t key )
{
	lcp_prq_match_entry_t *item = sctk_malloc(sizeof(lcp_prq_match_entry_t));
	assume(item != NULL);

	item->comm_key = key;
	item->pr_queue = lcp_prq_init();

	return item;
}


/**
 * @brief Get a posted receive matching queue
 * 
 * @param table hash table containing queues
 * @param comm communicator linked to the prq
 * @return lcp_prq_match_entry_t* 
 */
lcp_prq_match_entry_t *lcp_get_prq_entry(
		 lcp_prq_match_table_t *table,
		 uint64_t comm)
{
	int did_create = 0;
	return mpc_common_hashtable_get_or_create( &table->ht, comm, _prq_match_create_entry, &did_create);;
}


void* _umq_match_create_entry( uint64_t key )
{
	lcp_umq_match_entry_t *item = sctk_malloc(sizeof(lcp_umq_match_entry_t));
	assume(item != NULL);

	item->comm_key = key;
	item->um_queue = lcp_umq_init();

	return item;
}


/**
 * @brief Get an unexpected message queue
 * 
 * @param table hash table containing queues
 * @param comm communicator linked to the umq
 * @return lcp_prq_match_entry_t* 
 */
lcp_umq_match_entry_t *lcp_get_umq_entry(
		 lcp_umq_match_table_t *table,
		 uint64_t comm)
{
	int did_create = 0;
	return mpc_common_hashtable_get_or_create( &table->ht, comm, _umq_match_create_entry, &did_create);;
}

/**
 * @brief Add a request to a posted receive queue
 * 
 * @param prq hash table containing PRQs
 * @param req request to add
 * @param comm comm corresponding to the target prq
 * @param tag tag of the request
 * @param src request source
 */
void lcp_append_prq(lcp_prq_match_table_t *prq, lcp_request_t *req,
		    uint64_t comm, int tag, uint64_t src)
{

	lcp_prq_match_entry_t *entry;

	entry = lcp_get_prq_entry(prq, comm);
	mpc_common_spinlock_lock(&(entry->pr_queue->lock));
	lcp_prq_append(entry->pr_queue, req, tag, src);
	mpc_common_debug("LCP: prq added req=%p, msg_n=%lu, "
			 "comm_id=%llu, tag=%d, src=%llu.", 
			 req, req->seqn, comm, tag, src);
	mpc_common_spinlock_unlock(&(entry->pr_queue->lock));
}


/**
 * @brief Dequeue a posted receive request from a prq list matched with a tag
 * 
 * @param prq prq list
 * @param comm_id comm corresponding to the request to be dequeued
 * @param tag tag to be dequeued
 * @param src source of the request to be dequeued
 * @return lcp_request_t* dequeued request
 */
lcp_request_t *lcp_match_prq(lcp_prq_match_table_t *prq, 
			     uint64_t comm_id, int tag, 
			     uint64_t src)
{
	lcp_prq_match_entry_t *entry;
	lcp_request_t *found = NULL;

	entry = lcp_get_prq_entry(prq, comm_id);
	mpc_common_spinlock_lock(&(entry->pr_queue->lock));
	found =	lcp_prq_find_dequeue(entry->pr_queue, tag, src);
	mpc_common_debug("LCP: prq tag matching FOUND=%p, "
			 "comm_id=%llu, tag=%d, src=%llu.", 
			 found, comm_id, tag, src);
	mpc_common_spinlock_unlock(&(entry->pr_queue->lock));

	return found;
}


//TODO: add return value because we have heap allocation
/**
 * @brief Add a request to an unexpected message queue
 * 
 * @param umq hash table containing UMQs
 * @param req request to add
 * @param comm comm corresponding to the target umq
 * @param tag tag of the request
 * @param src request source
 */
void lcp_append_umq(lcp_umq_match_table_t *umq, void *req,
		    uint64_t comm, int tag, uint64_t src)
{
	lcp_umq_match_entry_t *entry;

	entry = lcp_get_umq_entry(umq, comm);
	mpc_common_spinlock_lock(&(entry->um_queue->lock));
	lcp_umq_append(entry->um_queue, req, tag, src);
	mpc_common_debug("LCP: umq added req=%p, comm_id=%llu, tag=%d, "
			 "src=%llu.", req, comm, tag, src);
	mpc_common_spinlock_unlock(&(entry->um_queue->lock));
}



lcp_umq_match_table_t * lcp_umq_match_table_init()
{
	lcp_umq_match_table_t * ret = sctk_malloc(sizeof(lcp_umq_match_table_t));
	assume(ret != NULL);

	mpc_common_spinlock_init(&ret->lock, 0);
	mpc_common_hashtable_init(&ret->ht, 128);

	return ret;
}

/**
 * @brief Dequeue an unexpected message request from a umq list matched with a tag
 * 
 * @param umq umq list
 * @param comm_id comm corresponding to the request to be dequeued
 * @param tag tag to be dequeued
 * @param src source of the request to be dequeued
 * @return lcp_request_t* dequeued request
 */
void *lcp_match_umq(lcp_umq_match_table_t *umq, 
		    uint64_t comm_id, int tag, 
		    uint64_t src)
{
	lcp_umq_match_entry_t *entry;
	void *found = NULL;

	entry = lcp_get_umq_entry(umq, comm_id);
	mpc_common_spinlock_lock(&(entry->um_queue->lock));
	found =	lcp_umq_find_dequeue(entry->um_queue, tag, src);
	mpc_common_spinlock_unlock(&(entry->um_queue->lock));

	return found;
}

void *lcp_search_umq(lcp_umq_match_table_t *umq,
                     uint64_t comm_id, int tag,
                     uint64_t src)
{
	lcp_umq_match_entry_t *entry;
	void *found = NULL;

	entry = lcp_get_umq_entry(umq, comm_id);
	mpc_common_spinlock_lock(&(entry->um_queue->lock));
	found =	lcp_umq_find(entry->um_queue, tag, src);
	mpc_common_debug("LCP: umq tag matching FOUND=%p, comm_id=%llu, tag=%d, "
			 "src=%llu.", found, comm_id, tag, src);
	mpc_common_spinlock_unlock(&(entry->um_queue->lock));

	return found;
}
