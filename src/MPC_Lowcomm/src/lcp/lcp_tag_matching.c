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

#include "queue.h"
#include "lcp_tag_matching.h"
#include "lcp_request.h"
#include "mpc_common_datastructure.h"
#include "mpc_common_debug.h"


int lcp_prq_match_table_init(lcp_prq_match_table_t *prq)
{
        int rc = LCP_SUCCESS;
        prq->prq_table = sctk_malloc(4096 * sizeof(lcp_mtch_prq_list_t *));
        if (prq->prq_table == NULL) {
                mpc_common_debug_error("MATCH: could not allocated prq table.");
                rc = LCP_ERROR;
                goto err;
        }
        memset(prq->prq_table, 0, 4096 * sizeof(lcp_mtch_prq_list_t *));

err:
	return rc;
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
        for (int i = 0; i < 4096; i++) {
                if (prq->prq_table[i] != NULL) {
                        lcp_mtch_prq_destroy(prq->prq_table[i]);
                }
	}
        sctk_free(prq->prq_table);

        for (int i = 0; i < 4096; i++) {
                if (umq->umq_table[i] != NULL) {
                        lcp_mtch_umq_destroy(umq->umq_table[i]);
                }
	}
        sctk_free(umq->umq_table);

}

lcp_mtch_prq_list_t *_prq_match_create_entry(lcp_prq_match_table_t *prq, uint16_t comm_id)
{
        lcp_mtch_prq_list_t *list;
        /* Check if prq list already created */
        if ((list = prq->prq_table[comm_id]) != NULL) {
                return list;
        }
        assert(comm_id < 4096 && comm_id >= 0);
        //FIXME: no memory check
	return prq->prq_table[comm_id] = lcp_prq_init();
}


/**
 * @brief Get a posted receive matching queue
 * 
 * @param table hash table containing queues
 * @param comm communicator linked to the prq
 * @return lcp_prq_match_entry_t* 
 */
lcp_mtch_prq_list_t *lcp_get_prq_entry(
		 lcp_prq_match_table_t *table,
		 uint16_t comm_id)
{
        return table->prq_table[comm_id];
}


void* _umq_match_create_entry(lcp_umq_match_table_t *umq, uint16_t comm_id)
{
        lcp_mtch_umq_list_t *list;
        /* Check if prq list already created */
        if ((list = umq->umq_table[comm_id]) != NULL) {
                return list;
        }
        assert(comm_id < 4096 && comm_id >= 0);
        //FIXME: no memory check
	return umq->umq_table[comm_id] = lcp_umq_init();
}


/**
 * @brief Get an unexpected message queue
 * 
 * @param table hash table containing queues
 * @param comm communicator linked to the umq
 * @return lcp_prq_match_entry_t* 
 */
lcp_mtch_umq_list_t *lcp_get_umq_entry(
		 lcp_umq_match_table_t *table,
		 uint16_t comm_id)
{
        return table->umq_table[comm_id];
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
		    uint16_t comm_id, int tag, uint64_t src)
{

	lcp_mtch_prq_list_t *prq_list;

	prq_list = lcp_get_prq_entry(prq, comm_id);
        if (prq_list == NULL) {
                prq_list = _prq_match_create_entry(prq, comm_id);
        }

	lcp_prq_append(prq_list, req, tag, src);
	mpc_common_debug("LCP: prq added req=%p, msg_n=%lu, "
			 "comm_id=%llu, tag=%d, src=%llu.", 
			 req, req->seqn, comm_id, tag, src);
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
			     uint16_t comm_id, int tag, 
			     uint64_t src)
{
	lcp_mtch_prq_list_t *prq_list;
	lcp_request_t *found = NULL;

	prq_list = lcp_get_prq_entry(prq, comm_id);
        if (prq_list == NULL) {
                prq_list = _prq_match_create_entry(prq, comm_id);
        }
	found =	lcp_prq_find_dequeue(prq_list, tag, src);
	mpc_common_debug("LCP: prq tag matching FOUND=%p, "
			 "comm_id=%lu, tag=%d, src=%llu.", 
			 found, comm_id, tag, src);

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
		    uint16_t comm_id, int tag, uint64_t src)
{
	lcp_mtch_umq_list_t *umq_list;

	umq_list = lcp_get_umq_entry(umq, comm_id);
        if (umq_list == NULL) {
                umq_list = _umq_match_create_entry(umq, comm_id);
        }
	lcp_umq_append(umq_list, req, tag, src);
	mpc_common_debug("LCP: umq added req=%p, comm_id=%lu, tag=%d, "
			 "src=%llu.", req, comm_id, tag, src);
}



int lcp_umq_match_table_init(lcp_umq_match_table_t *umq)
{
        int rc = LCP_SUCCESS;
        umq->umq_table = sctk_malloc(4096 * sizeof(lcp_mtch_umq_list_t));
        if (umq->umq_table == NULL) {
                mpc_common_debug_error("MATCH: could not allocated prq table.");
                rc = LCP_ERROR;
                goto err;
        }
        memset(umq->umq_table, 0, 4096 * sizeof(lcp_mtch_umq_list_t));

err:
	return rc;
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
		    uint16_t comm_id, int tag, 
		    uint64_t src)
{
	lcp_mtch_umq_list_t *umq_list;
	void *found = NULL;

	umq_list = lcp_get_umq_entry(umq, comm_id);
        if (umq_list == NULL) {
                umq_list = _umq_match_create_entry(umq, comm_id);
        }
	found =	lcp_umq_find_dequeue(umq_list, tag, src);
	mpc_common_debug("LCP: umq tag matching FOUND=%p, comm_id=%llu, tag=%d, "
			 "src=%llu.", found, comm_id, tag, src);

	return found;
}

void *lcp_search_umq(lcp_umq_match_table_t *umq,
                     uint16_t comm_id, int tag,
                     uint64_t src)
{
	lcp_mtch_umq_list_t *umq_list;
	void *found = NULL;

	umq_list = lcp_get_umq_entry(umq, comm_id);
        if (umq_list == NULL) {
                umq_list = _umq_match_create_entry(umq, comm_id);
        }
	found =	lcp_umq_find(umq_list, tag, src);
	mpc_common_debug("LCP: umq tag matching FOUND=%p, comm_id=%llu, tag=%d, "
			 "src=%llu.", found, comm_id, tag, src);

	return found;
}
