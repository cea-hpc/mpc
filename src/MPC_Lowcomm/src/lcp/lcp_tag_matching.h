#ifndef LCP_TAG_MATCHING_H
#define LCP_TAG_MATCHING_H

#include <uthash.h>
#include <mpc_common_spinlock.h>

#include "lcp.h"

#include "lcp_tag_lists.h"

typedef struct lcp_prq_match_entry_s
{
	uint64_t             comm_key;
	lcp_mtch_prq_list_t *pr_queue;
	UT_hash_handle       hh;
} lcp_prq_match_entry_t;

typedef struct lcp_prq_match_table_s
{
	mpc_common_spinlock_t  lock;
	lcp_prq_match_entry_t *table;
} lcp_prq_match_table_t;

typedef struct lcp_umq_match_entry_s
{
	uint64_t             comm_key;
	lcp_mtch_umq_list_t *um_queue;
	UT_hash_handle       hh;
} lcp_umq_match_entry_t;

typedef struct lcp_umq_match_table_s
{
	mpc_common_spinlock_t  lock;
	lcp_umq_match_entry_t *table;
} lcp_umq_match_table_t;

lcp_umq_match_entry_t *lcp_get_umq_entry(
		 lcp_umq_match_table_t *table,
		 uint64_t comm);

void lcp_append_prq(lcp_prq_match_table_t *prq, lcp_request_t *req,
		    uint64_t comm_id, int tag, uint64_t src);
lcp_request_t *lcp_match_prq(lcp_prq_match_table_t *prq, 
		    uint64_t comm_id, int tag, uint64_t src);

void lcp_append_umq(lcp_umq_match_table_t *umq, void *req,
		    uint64_t comm_id, int tag, uint64_t src);
void *lcp_match_umq(lcp_umq_match_table_t *umq,
		    uint64_t comm_id, int tag, uint64_t src);
void *lcp_search_umq(lcp_umq_match_table_t *umq,
                     uint64_t comm_id, int tag, uint64_t src);

void lcp_fini_matching_engine(lcp_umq_match_table_t *umq, 
			      lcp_prq_match_table_t *prq);
#endif
