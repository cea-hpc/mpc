#ifndef LCP_TAG_MATCHING_H
#define LCP_TAG_MATCHING_H

#include <mpc_common_spinlock.h>

#include <mpc_common_datastructure.h>

#include "lcp_def.h"

#include "lcp_tag_lists.h"

typedef struct lcp_prq_match_entry_s
{
	uint64_t             comm_key;
	lcp_mtch_prq_list_t *pr_queue;
} lcp_prq_match_entry_t;

typedef struct lcp_prq_match_table_s
{
	mpc_common_spinlock_t  lock;
	struct mpc_common_hashtable ht;
} lcp_prq_match_table_t;

lcp_prq_match_table_t * lcp_prq_match_table_init();

typedef struct lcp_umq_match_entry_s
{
	uint64_t             comm_key;
	lcp_mtch_umq_list_t *um_queue;
} lcp_umq_match_entry_t;

typedef struct lcp_umq_match_table_s
{
	mpc_common_spinlock_t  lock;
	struct mpc_common_hashtable ht;
} lcp_umq_match_table_t;

lcp_umq_match_table_t * lcp_umq_match_table_init();


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
