#include <comm.h>
#include <utlist.h>
#include <sctk_alloc.h>

#include "lcp_tag_lists.h"


/*******************************************************
 * PRQ 
 ******************************************************/
int lcp_prq_cancel(lcp_mtch_prq_list_t *list, void *req)
{
	lcp_mtch_prq_elem_t *elem, *tmp;

	DL_FOREACH_SAFE(list->list, elem, tmp){
		if (req == elem->value) {
			DL_DELETE(list->list, elem);
			sctk_free(elem);
			list->size--;
			return 1;
		}
	}
	return 0;
}

void *lcp_prq_find(lcp_mtch_prq_list_t *list, int tag, uint64_t peer)
{
	int result;
	lcp_mtch_prq_elem_t *elem, *tmp;

	DL_FOREACH_SAFE(list->list, elem, tmp){
		result = ((elem->tag & elem->tmask) == (tag & elem->tmask)) &&
			((elem->src & elem->smask) == (peer & elem->smask));
		if (result) {
			return elem->value;
		}
	}
	return 0;
}

void *lcp_prq_find_dequeue(lcp_mtch_prq_list_t *list, int tag, uint64_t peer)
{
	int result;
	lcp_mtch_prq_elem_t *elem, *tmp;

	DL_FOREACH_SAFE(list->list, elem, tmp){
		result = ((elem->tag & elem->tmask) == (tag & elem->tmask)) &&
			((elem->src & elem->smask) == (peer & elem->smask));
		if (result) {
			DL_DELETE(list->list, elem);	
			list->size--;
			return elem->value;
		}
	}
	return 0;
}

//FIXME: add malloc check
void lcp_prq_append(lcp_mtch_prq_list_t *list, void *payload, int tag, uint64_t source)
{
	int mask_tag;
	uint64_t mask_src;

	if((int)source == MPC_ANY_SOURCE) {
		mask_src = 0;
	} else {
		mask_src = ~0;
	}

	if(tag == MPC_ANY_TAG) {
		mask_tag = 0;
	} else {
		mask_tag = ~0;
	}
	
	lcp_mtch_prq_elem_t *elem = sctk_malloc(
			sizeof(lcp_mtch_prq_elem_t));

	elem->tag = tag;
	elem->tmask = mask_tag;
	elem->src = source;
	elem->smask = mask_src;
	elem->value = payload;	
	list->size++;

	DL_APPEND(list->list, elem);
}

//FIXME: add malloc check
lcp_mtch_prq_list_t *lcp_prq_init()
{
	lcp_mtch_prq_list_t *list = sctk_malloc(
			sizeof(lcp_mtch_prq_list_t));
	memset(list, 0, sizeof(lcp_mtch_prq_list_t));

	list->size = 0;

	mpc_common_spinlock_init(&list->lock, 0);
	return list;
}

void lcp_mtch_prq_destroy(lcp_mtch_prq_list_t *list)
{
	lcp_mtch_prq_elem_t *elem, *tmp;

	DL_FOREACH_SAFE(list->list, elem, tmp){
		DL_DELETE(list->list, elem);
		sctk_free(elem);
	}
	sctk_free(list->list);
}

/*******************************************************
 * PRQ 
 ******************************************************/
int lcp_umq_cancel(lcp_mtch_umq_list_t *list, void *req)
{
	lcp_mtch_umq_elem_t *elem, *tmp;

	DL_FOREACH_SAFE(list->list, elem, tmp){
		if (req == elem->value) {
			DL_DELETE(list->list, elem);
			sctk_free(elem);
			list->size--;
			return 1;
		}
	}
	return 0;
}

void *lcp_umq_find(lcp_mtch_umq_list_t *list, int tag, uint64_t peer)
{
	int result;
	lcp_mtch_umq_elem_t *elem, *tmp;

	int tmask = ~0;
	uint64_t smask = ~0;
	if (tag == MPC_ANY_TAG) {
		tmask = 0;
	}

	if ((int)peer == MPC_ANY_SOURCE) {
		smask = 0;
	}

	tag = tag & tmask;
	peer = peer & smask;

	DL_FOREACH_SAFE(list->list, elem, tmp){
		result = ((elem->tag & tmask) == tag) &&
			((elem->src & smask) == peer);
		if (result) {
			return elem->value;
		}
	}
	return 0;
}

void *lcp_umq_find_dequeue(lcp_mtch_umq_list_t *list, int tag, uint64_t peer)
{
	int result;
	lcp_mtch_umq_elem_t *elem, *tmp;

	int tmask = ~0;
	uint64_t smask = ~0;
	if (tag == MPC_ANY_TAG) {
		tmask = 0;
	}

	if ((int)peer == MPC_ANY_SOURCE) {
		smask = 0;
	}

	tag = tag & tmask;
	peer = peer & smask;

	DL_FOREACH_SAFE(list->list, elem, tmp){
		result = ((elem->tag & tmask) == tag) &&
			((elem->src & smask) == peer);
		if (result) {
			DL_DELETE(list->list, elem);	
			list->size--;
			return elem->value;
		}
	}
	return 0;
}

void lcp_umq_append(lcp_mtch_umq_list_t *list, void *payload, int tag, uint64_t source)
{
	lcp_mtch_umq_elem_t *elem = sctk_malloc(
			sizeof(lcp_mtch_umq_elem_t));

	elem->tag = tag;
	elem->src = source;
	elem->value = payload;	
	list->size++;

	DL_APPEND(list->list, elem);
}

lcp_mtch_umq_list_t *lcp_umq_init()
{
	lcp_mtch_umq_list_t *list = sctk_malloc(
			sizeof(lcp_mtch_umq_list_t));
	memset(list, 0, sizeof(lcp_mtch_umq_list_t));

	list->size = 0;
	mpc_common_spinlock_init(&list->lock, 0);
	return list;
}

void lcp_mtch_umq_destroy(lcp_mtch_umq_list_t *list)
{
	lcp_mtch_umq_elem_t *elem = NULL, *tmp = NULL;

	DL_FOREACH_SAFE(list->list, elem, tmp){
		DL_DELETE(list->list, elem);
		sctk_free(elem);
	}
	sctk_free(list->list);
}
