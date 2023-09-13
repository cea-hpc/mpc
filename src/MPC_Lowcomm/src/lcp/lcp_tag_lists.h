#ifndef LCP_TAG_LISTS_H
#define LCP_TAG_LISTS_H

#include <mpc_common.h>
#include <mpc_mempool.h>

/*******************************************************
 * Data structures - Matching lists
 ******************************************************/
// PRQ
typedef struct lcp_mtch_prq_elem_s
{
	int tag;
	int tmask;
	uint64_t src;
	uint64_t smask;
	struct lcp_mtch_prq_elem_s *prev, *next;
	void *value;
} lcp_mtch_prq_elem_t;

typedef struct  
{
	mpc_common_spinlock_t lock;
	mpc_mempool elem_pool;
	lcp_mtch_prq_elem_t *list;
	int size;
} lcp_mtch_prq_list_t;

int lcp_prq_cancel(lcp_mtch_prq_list_t *list, void *req);
int lcp_prq_get_size(lcp_mtch_prq_list_t *list);
void *lcp_prq_find_dequeue(lcp_mtch_prq_list_t *list, int tag, uint64_t peer);
void lcp_prq_append(lcp_mtch_prq_list_t *list, void *payload, int tag, uint64_t source);
lcp_mtch_prq_list_t *lcp_prq_init();
void lcp_mtch_prq_destroy(lcp_mtch_prq_list_t *list);

// UMQ
typedef struct lcp_mtch_umq_elem_s
{
	int tag;
	uint64_t src;
	struct lcp_mtch_umq_elem_s *prev, *next;
	void *value;
} lcp_mtch_umq_elem_t;

typedef struct  
{
	mpc_common_spinlock_t lock;
	mpc_mempool elem_pool;
	lcp_mtch_umq_elem_t *list;
	int size;
} lcp_mtch_umq_list_t;

int lcp_umq_cancel(lcp_mtch_umq_list_t *list, void *req);
int lcp_umq_get_size(lcp_mtch_umq_list_t *list);
void *lcp_umq_find(lcp_mtch_umq_list_t *list, int tag, uint64_t peer);
void *lcp_umq_find_dequeue(lcp_mtch_umq_list_t *list, int tag, uint64_t peer);
void lcp_umq_append(lcp_mtch_umq_list_t *list, void *payload, int tag, uint64_t source);
lcp_mtch_umq_list_t *lcp_umq_init();
void lcp_mtch_umq_destroy(lcp_mtch_umq_list_t *list);

#endif
