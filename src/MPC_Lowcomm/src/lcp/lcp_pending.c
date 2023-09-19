/* ############################# MPC License ############################## */
/* # Wed Nov 19 15:19:19 CET 2008                                         # */
/* # Copyright or (C) or Copr. Commissariat a l'Energie Atomique          # */
/* # Copyright or (C) or Copr. 2010-2012 Université de Versailles         # */
/* # St-Quentin-en-Yvelines                                               # */
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
/* #   - MOREAU Gilles gilles.moreau@cea.fr                               # */
/* #                                                                      # */
/* ######################################################################## */

#include <mpc_common_debug.h>
#include "msg_cpy.h"
#include <sctk_alloc.h> 


#include "lcp_pending.h"
#include "lcp_context.h"
#include "lcp_request.h"
#include "uthash.h"


/**
 * @brief Create a pending request.
 * 
 * @param table table to register the request into
 * @param req request to register
 * @param msg_key key of message
 * @return lcp_pending_entry_t* resulting pending entry
 */
lcp_pending_entry_t *lcp_pending_create(lcp_pending_table_t *table,
                                        lcp_request_t *req,
                                        uint64_t msg_key)
{
        lcp_pending_entry_t *entry;

        mpc_common_spinlock_lock(&(table->table_lock));

        HASH_FIND(hh, table->table, &msg_key, sizeof(uint64_t), entry);

        if (entry == NULL) {
                entry = mpc_mempool_alloc(&table->pending_pool);
                if (entry == NULL) {
                        mpc_common_debug_error("LCP: could not allocate pending request.");	
                        return entry;
                }

                entry->msg_key = msg_key;
                entry->req = req;
                HASH_ADD(hh, table->table, msg_key, sizeof(uint64_t), entry);	
                mpc_common_debug("LCP: add pending req=%p, msg_id=%llu.", req, msg_key);
        } else {
                mpc_common_debug_warning("LCP: req=%p is already in pending list.");
        }

        mpc_common_spinlock_unlock(&(table->table_lock));

        return entry;
}

//TODO: add return call
/**
 * @brief Delete a pending request.
 * 
 * @param table table to delete the request from
 * @param msg_key key of the deleted message
 */
void lcp_pending_delete(lcp_pending_table_t *table, 
                        uint64_t msg_key)
{
        lcp_pending_entry_t *entry;

        mpc_common_spinlock_lock(&(table->table_lock));
        HASH_FIND(hh, table->table, &msg_key, sizeof(uint64_t), entry);
        if (entry == NULL) {
                //FIXME: should it be fatal ?
                mpc_common_debug_fatal("LCP: pending request=%llu did not exist",
                                       msg_key);
        }

        HASH_DELETE(hh, table->table, entry);
        mpc_mempool_free(NULL, entry);
        mpc_common_debug_info("LCP: del pending request msg_id=%llu", 
                              msg_key);
        mpc_common_spinlock_unlock(&(table->table_lock));
}

lcp_request_t *lcp_pending_get_request(lcp_pending_table_t *table,
                                       uint64_t msg_key)
{
        lcp_pending_entry_t *entry;

        mpc_common_spinlock_lock(&(table->table_lock));
        HASH_FIND(hh, table->table, &msg_key, sizeof(uint64_t), entry);
        if (entry == NULL) {
                mpc_common_spinlock_unlock(&(table->table_lock));
                return NULL;
        }
        mpc_common_spinlock_unlock(&(table->table_lock));
        return entry->req;
}

/*******************************************************
 * Data manager initiator 
 ******************************************************/

/**
 * @brief Init pending list.
 * 
 */
lcp_pending_table_t * lcp_pending_init()
{
        lcp_pending_table_t * ret = sctk_malloc(sizeof(lcp_pending_table_t));
        assume(ret != NULL);
        /* And rdv list */
        ret->table = NULL;
        mpc_mempool_init(&ret->pending_pool, 10, 512, sizeof(lcp_pending_entry_t), sctk_malloc, sctk_free);
        mpc_common_spinlock_init(&ret->table_lock, 0);

        return ret;
}

/**
 * @brief Terminate pending list.
 * 
 * @param ctx lcp context
 */
void lcp_pending_fini(lcp_pending_table_t *table)
{
        /* Matching list */
        lcp_pending_entry_t *entry, *e_tmp;
        /* Delete control send table */
        if (HASH_COUNT(table->table) > 0) {
                HASH_ITER(hh, table->table, entry, e_tmp) {
                        HASH_DELETE(hh, table->table, entry);
                        mpc_mempool_free(NULL, entry);
                }
        }
}
