#ifndef LCP_PENDING_H 
#define LCP_PENDING_H 

#include "lcp_def.h"

#include <uthash.h>
#include <mpc_common_spinlock.h>

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
	lcp_pending_entry_t   *table;
} lcp_pending_table_t;

/*******************************************************
 * Data manager initiator and destructor
 ******************************************************/
void lcp_pending_init();
void lcp_pending_fini(lcp_context_h ctx);

/*******************************************************
 * Accessors and mutators pending requests
 ******************************************************/
lcp_pending_entry_t *lcp_pending_create(lcp_context_h ctx,
                                        lcp_request_t *req,
                                        uint64_t msg_key);

void lcp_pending_delete(lcp_context_h ctx,
                        uint64_t msg_key);

lcp_request_t *lcp_pending_get_request(lcp_context_h ctx,
                                       uint64_t msg_key);
#endif 
