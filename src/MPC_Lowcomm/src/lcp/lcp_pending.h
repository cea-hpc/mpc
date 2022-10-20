#ifndef LCP_PENDING_H 
#define LCP_PENDING_H 

#include "lcp_request.h"

#include <uthash.h>
#include <utlist.h>
#include <mpc_common_spinlock.h>
#include <mpc_lowcomm_monitor.h>
#include <comm.h>

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
void lcp_pending_fini(lcp_pending_table_t *send,
                      lcp_pending_table_t *recv);

/*******************************************************
 * Accessors and mutators stripes 
 ******************************************************/
void lcp_frag_copy_frag(lcp_request_t *req, void *data, 
			intptr_t offset, size_t size);

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
