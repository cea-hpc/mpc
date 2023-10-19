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

#ifndef LCP_PENDING_H 
#define LCP_PENDING_H 

#include <uthash.h>
#include <mpc_mempool.h>
#include <mpc_common_spinlock.h>

#include "lcp_def.h"

/*******************************************************
 * Data structures - Control message
 ******************************************************/
typedef struct 
{
	uint64_t               msg_key;
	lcp_request_t         *req;
	UT_hash_handle         hh;
} lcp_pending_entry_t;

typedef struct 
{
	mpc_common_spinlock_t  table_lock;
	mpc_mempool_t pending_pool;
	lcp_pending_entry_t   *table;
} lcp_pending_table_t;


/*******************************************************
 * Data manager initiator and destructor
 ******************************************************/

lcp_pending_table_t * lcp_pending_init();
void lcp_pending_fini(lcp_pending_table_t *table);

/*******************************************************
 * Accessors and mutators pending requests
 ******************************************************/
lcp_pending_entry_t *lcp_pending_create(lcp_pending_table_t *table,
                                        lcp_request_t *req,
                                        uint64_t msg_key);

void lcp_pending_delete(lcp_pending_table_t *table,
                        uint64_t msg_key);

lcp_request_t *lcp_pending_get_request(lcp_pending_table_t *table,
                                       uint64_t msg_key);
#endif 
