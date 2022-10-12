#include <sctk_alloc.h>
#include <uthash.h>

#include "lcp_tag_matching.h"
#include "lcp_request.h"

void lcp_fini_matching_engine(lcp_umq_match_table_t *umq, 
			      lcp_prq_match_table_t *prq)
{
	lcp_prq_match_entry_t *prq_entry = NULL, *prq_tmp = NULL;
	lcp_umq_match_entry_t *umq_entry = NULL, *umq_tmp = NULL;

	HASH_ITER(hh, prq->table, prq_entry, prq_tmp) {
		lcp_mtch_prq_destroy(prq_entry->pr_queue);
		HASH_DELETE(hh, prq->table, prq_entry);
		sctk_free(prq_entry->pr_queue);
		sctk_free(prq_entry);
	}

	HASH_ITER(hh, umq->table, umq_entry, umq_tmp) {
		lcp_mtch_umq_destroy(umq_entry->um_queue);
		HASH_DELETE(hh, umq->table, umq_entry);
		sctk_free(umq_entry->um_queue);
		sctk_free(umq_entry);
	}
}

lcp_prq_match_entry_t *lcp_get_prq_entry(
		 lcp_prq_match_table_t *table,
		 uint64_t comm)
{
	lcp_prq_match_entry_t *item;
	mpc_common_spinlock_lock(&table->lock);
	HASH_FIND(hh, table->table, &comm, sizeof(uint64_t), item);

	if (item == NULL)
	{
		item = sctk_malloc(sizeof(lcp_prq_match_entry_t));
		assume(item != NULL);
		memset(item, 0, sizeof(lcp_prq_match_entry_t));

		item->comm_key = comm;
		item->pr_queue = lcp_prq_init();
		/* Add the entry */
		HASH_ADD(hh, table->table, comm_key, sizeof(uint64_t), item);
		mpc_common_debug("LCP: prq communicator comm=%llu.", comm);
	}
	mpc_common_spinlock_unlock(&table->lock);

	return item;
}

lcp_umq_match_entry_t *lcp_get_umq_entry(
		 lcp_umq_match_table_t *table,
		 uint64_t comm)
{
	lcp_umq_match_entry_t *item;
	mpc_common_spinlock_lock(&table->lock);
	HASH_FIND(hh, table->table, &comm, sizeof(uint64_t), item);

	if (item == NULL)
	{
		item = sctk_malloc(sizeof(lcp_umq_match_entry_t));
		assume(item != NULL);
		memset(item, 0, sizeof(lcp_umq_match_entry_t));

		item->comm_key = comm;
		item->um_queue = lcp_umq_init();
		/* Add the entry */
		HASH_ADD(hh, table->table, comm_key, sizeof(uint64_t), item);
		mpc_common_debug("LCP: umq communicator comm=%llu", comm);
	}
	mpc_common_spinlock_unlock(&table->lock);

	return item;
}

void lcp_append_prq(lcp_prq_match_table_t *prq, lcp_request_t *req,
		    uint64_t comm, int tag, uint64_t src)
{

	lcp_prq_match_entry_t *entry;

	entry = lcp_get_prq_entry(prq, comm);
	mpc_common_spinlock_lock(&(entry->pr_queue->lock));
	lcp_prq_append(entry->pr_queue, req, tag, src);
	mpc_common_debug("LCP: prq added req=%p, msg_n=%lu, "
			 "comm_id=%llu, tag=%d, src=%llu, msg=%p.", 
			 req, req->msg_number, comm, tag, src, 
			 req->request);
	mpc_common_spinlock_unlock(&(entry->pr_queue->lock));
}

//TODO: add return value because we have heap allocation
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

void *lcp_match_umq(lcp_umq_match_table_t *umq, 
		    uint64_t comm_id, int tag, 
		    uint64_t src)
{
	lcp_umq_match_entry_t *entry;
	void *found = NULL;

	entry = lcp_get_umq_entry(umq, comm_id);
	mpc_common_spinlock_lock(&(entry->um_queue->lock));
	found =	lcp_umq_find_dequeue(entry->um_queue, tag, src);
	mpc_common_debug("LCP: umq tag matching FOUND=%p, comm_id=%llu, tag=%d, "
			 "src=%llu.", found, comm_id, tag, src);
	mpc_common_spinlock_unlock(&(entry->um_queue->lock));

	return found;
}
