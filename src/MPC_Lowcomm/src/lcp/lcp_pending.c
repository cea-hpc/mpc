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
#include "sctk_net_tools.h"
#include <sctk_alloc.h> 

#include "lcp_pending.h"

#include "lcp.h"
#include "lcp_context.h"

/*******************************************************
 * Fragment and message building 
 ******************************************************/
void lcp_frag_copy_frag(lcp_request_t *req, void *data, intptr_t offset,
			size_t size)
{
	void *buf = req->recv.buffer + offset;
	memcpy(buf + offset, data, size);
}

/*******************************************************
 * Mutators and accessors control messages
 ******************************************************/
lcp_pending_entry_t *lcp_pending_create(lcp_pending_table_t *table,
					      lcp_request_t *req,
					      uint64_t msg_key)
{
	lcp_pending_entry_t *entry;

	mpc_common_spinlock_lock(&(table->table_lock));

	HASH_FIND(hh, table->table, &msg_key, sizeof(uint64_t), entry);

	if (entry == NULL) 
	{
		entry = sctk_malloc(sizeof(lcp_pending_entry_t));
                if (entry == NULL) {
                        mpc_common_debug_error("LCP: could not allocate pending request.");	
                        return entry;
                }
		memset(entry, 0, sizeof(lcp_pending_entry_t));

		entry->msg_key = msg_key;
		entry->req = req;

		HASH_ADD(hh, table->table, msg_key, sizeof(uint64_t), entry);	
		mpc_common_debug_info("LCP: add pending msg_id=%llu.", msg_key);
	}
	mpc_common_spinlock_unlock(&(table->table_lock));

	return entry;
}

//TODO: add return call
void lcp_pending_delete(lcp_pending_table_t *table, 
                        uint64_t msg_key)
{
	lcp_pending_entry_t *entry;

	HASH_FIND(hh, table->table, &msg_key, sizeof(uint64_t), entry);
	if (entry == NULL) {
		mpc_common_debug_fatal("LCP: pending request=%llu did not exist",
				       msg_key);
	}

	HASH_DELETE(hh, table->table, entry);
	sctk_free(entry);
	mpc_common_debug_info("LCP: del pending request msg_id=%llu", 
			 msg_key);
	mpc_common_spinlock_unlock(&(table->table_lock));
}

lcp_request_t *lcp_pending_get_request(lcp_pending_table_t *table,
					   uint64_t msg_key)
{
	lcp_pending_entry_t *entry;

	HASH_FIND(hh, table->table, &msg_key, sizeof(uint64_t), entry);
	if (entry == NULL) 
		return NULL;

	return entry->req;
}

/*******************************************************
 * Data manager initiator 
 ******************************************************/
void lcp_pending_init()
{
	/* And rdv list */
}

void lcp_pending_fini(lcp_pending_table_t *send, 
                      lcp_pending_table_t *recv)
{
	/* Control table */
	lcp_pending_entry_t *entry, *e_tmp;
	/* Matching list */

	/* Delete control send table */
        if (HASH_COUNT(send->table) > 0) {
                HASH_ITER(hh, send->table, entry, e_tmp) {
                        HASH_DELETE(hh, send->table, entry);
                        sctk_free(entry);
                }
        }

	/* Delete control recv table */
        if (HASH_COUNT(recv->table) > 0) {
                HASH_ITER(hh, recv->table, entry, e_tmp) {
                        HASH_DELETE(hh, recv->table, entry);
                        sctk_free(entry);
                }
        }
}
