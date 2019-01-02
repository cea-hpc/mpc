/* ############################# MPC License ############################## */
/* # Wed Nov 19 15:19:19 CET 2008                                         # */
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
/* # Authors:                                                             # */
/* #   - PERACHE Marc marc.perache@cea.fr                                 # */
/* #   - ADAM Julien julien.adam@paratools.com                            # */
/* #   - BESNARD Jean-Baptiste jean-baptiste.besnard@paratools.com        # */
/* #                                                                      # */
/* ######################################################################## */

#ifdef MPC_USE_PORTALS

#include "sctk_alloc.h"
#include "sctk_ptl_iface.h"
#include "sctk_ptl_toolkit.h"
#include "sctk_ptl_offcoll.h"
//#include "sctk_inter_thread_comm.h"

#define COLL_ARITY 2

extern sctk_ptl_id_t* ranks_id_map;

static inline int __sctk_ptl_offcoll_enabled(sctk_ptl_rail_info_t* srail)
{
	static int cache = -1;
	if(cache == -1)
		cache = SCTK_PTL_IS_OFFLOAD_COLL(srail->offload_support); 

	return cache;
}

int sctk_ptl_offcoll_enabled(sctk_ptl_rail_info_t* srail)
{
	return __sctk_ptl_offcoll_enabled(srail);
}
 
/** TODO: REMOVE all these global variables (should be per rail) */
sctk_ptl_local_data_t* md_up;
sctk_ptl_local_data_t* md_down;
sctk_ptl_local_data_t* dummy_md;

sctk_ptl_rail_info_t* grail;

void sctk_ptl_offcoll_init(sctk_ptl_rail_info_t* srail)
{
	sctk_assert(__sctk_ptl_offcoll_enabled(srail));
	md_up = sctk_ptl_md_create_with_cnt(srail, NULL, 0, (SCTK_PTL_MD_PUT_NOEV_FLAGS | PTL_MD_EVENT_CT_SEND));
	md_down = sctk_ptl_md_create_with_cnt(srail, NULL, 0, (SCTK_PTL_MD_PUT_NOEV_FLAGS | PTL_MD_EVENT_CT_SEND));
	dummy_md = sctk_ptl_md_create_with_cnt(srail, NULL, 0, (SCTK_PTL_MD_PUT_NOEV_FLAGS | PTL_MD_EVENT_CT_SEND));
	md_up->prot = SCTK_PTL_PROT_EAGER;
	md_up->type = SCTK_PTL_TYPE_OFFCOLL;
	md_down->prot = SCTK_PTL_PROT_EAGER;
	md_down->type = SCTK_PTL_TYPE_OFFCOLL;
	dummy_md->prot = SCTK_PTL_PROT_RDV;
	dummy_md->type = SCTK_PTL_TYPE_OFFCOLL;
	sctk_ptl_md_register(srail, md_up);
	sctk_ptl_md_register(srail, md_down);
	sctk_ptl_md_register(srail, dummy_md);

        grail = srail;
}

void sctk_ptl_offcoll_fini(sctk_ptl_rail_info_t* srail)
{
	sctk_assert(__sctk_ptl_offcoll_enabled(srail));
	sctk_ptl_md_release(md_up);   md_up = NULL;
	sctk_ptl_md_release(md_down); md_down = NULL;
}

void sctk_ptl_offcoll_pte_init(sctk_ptl_rail_info_t* srail, sctk_ptl_pte_t* pte)
{
	sctk_ptl_local_data_t* me_up = NULL, *me_down = NULL, *tmp = NULL;
	sctk_assert(srail);
	sctk_assert(pte);
	sctk_assert(__sctk_ptl_offcoll_enabled(srail));
	
	me_up = sctk_ptl_me_create_with_cnt(srail, NULL, 0, SCTK_PTL_ANY_PROCESS, SCTK_PTL_MATCH_OFFCOLL_BARRIER_UP, SCTK_PTL_MATCH_INIT, SCTK_PTL_ME_PUT_NOEV_FLAGS);
	me_up->msg = NULL;
	me_up->list = SCTK_PTL_PRIORITY_LIST;
	me_up->type = SCTK_PTL_TYPE_OFFCOLL;
	me_up->prot = SCTK_PTL_PROT_EAGER;
	me_down = sctk_ptl_me_create_with_cnt(srail, NULL, 0, SCTK_PTL_ANY_PROCESS, SCTK_PTL_MATCH_OFFCOLL_BARRIER_DOWN, SCTK_PTL_MATCH_INIT, SCTK_PTL_ME_PUT_NOEV_FLAGS);
	me_down->msg = NULL;
	me_down->list = SCTK_PTL_PRIORITY_LIST;
	me_down->type = SCTK_PTL_TYPE_OFFCOLL;
	me_down->prot = SCTK_PTL_PROT_EAGER;
	sctk_assert(me_up);
	sctk_assert(me_down);
	sctk_ptl_me_register(srail, me_up, pte);
	sctk_ptl_me_register(srail, me_down, pte);

	int i;
	sctk_ptl_offcoll_tree_node_t* cur;
	for (i = 0; i < SCTK_PTL_OFFCOLL_NB; ++i)
	{
		cur = pte->node+i;
		cur->leaf = -1; /* condition to make a first init */
		cur->parent = SCTK_PTL_ANY_PROCESS;
		cur->children = NULL;
		cur->nb_children = -1;
		cur->root = -1;
		sctk_atomics_store_int(&cur->iter, 0);
        	sctk_spinlock_init((&cur->lock), 0);
		switch(i)
		{
			case SCTK_PTL_OFFCOLL_BARRIER:
				cur->spec.barrier.cnt_hb_up = &me_up->slot.me.ct_handle;
				cur->spec.barrier.cnt_hb_down = &me_down->slot.me.ct_handle;
				break;
			case SCTK_PTL_OFFCOLL_BCAST:
				tmp = sctk_ptl_me_create_with_cnt(
					srail,
					NULL,
					0,
					SCTK_PTL_ANY_PROCESS,
					SCTK_PTL_MATCH_OFFCOLL_BCAST_LARGE,
					SCTK_PTL_MATCH_INIT,
					SCTK_PTL_ME_PUT_NOEV_FLAGS
				);
				sctk_ptl_me_register(srail, tmp, pte);
				sctk_assert(tmp);
				tmp->msg = NULL; /* no need for it (for noww */
				tmp->list = SCTK_PTL_PRIORITY_LIST;
				tmp->type = SCTK_PTL_TYPE_OFFCOLL; /* Standard MPI message */
				tmp->prot = SCTK_PTL_PROT_RDV; /* handled by offload protocols */
				cur->spec.bcast.large_puts = tmp; 
				break;
			case SCTK_PTL_OFFCOLL_REDUCE:
				break;
			default:
				not_reachable();
				break;
		}
	}
}

void sctk_ptl_offcoll_pte_fini(sctk_ptl_rail_info_t* srail, sctk_ptl_pte_t* pte)
{
	sctk_assert(__sctk_ptl_offcoll_enabled(srail));
}


/**
 * Adjust ranks to know its position in the tree.
 * This will exchange the tree root with the actual root rank. Any other ranks in left unchanged
 * Useful for collectives where the root can change between calls (Bcast,Reduce...)
 */
static inline int __sctk_ptl_offcoll_rotate_ranks(int rank, int root)
{
	if(!rank)
		return root;
	if(rank == root)
		return 0;
	return rank;
}

static inline void __sctk_ptl_offcoll_build_tree(sctk_ptl_rail_info_t* srail, sctk_ptl_pte_t* pte, int local_rank, int root, int size, int collective)
{
        int l_child = -1, h_child = -1, parent_rank, child_rank; 
        size_t i, nb_children;
        sctk_assert(srail);
        sctk_rail_info_t* rail = sctk_ptl_promote_to_rail(srail);
	sctk_ptl_offcoll_tree_node_t* node = pte->node + collective;
        sctk_assert(rail);

	/* if no tree has been built yet OR if the currently saved tree is not the one we want => built it */
        if(node->leaf == -1 || node->root != root)
        {
                sctk_spinlock_lock(&node->lock);
                if(node->leaf == -1 || node->root != root)
                {
                        /* get children range for the current process 
                         * h_child is the child with the highest rank 
                         * m_child is the child with the lowest_rank
                         */
			local_rank = __sctk_ptl_offcoll_rotate_ranks(local_rank, root);
                        h_child = (local_rank + 1) * COLL_ARITY;
                        l_child = h_child - (COLL_ARITY -1);
                        node->leaf = (l_child >= size);
			node->root = root;

			/* as ranks are "rotated" the rank 0 is now the tree root (whatever its 
			 * initial MPI rank was).
			 * If not the root, compute the parent rank.
			 */
                        if(local_rank != 0)
                        {
	                        parent_rank = (int)((local_rank + (COLL_ARITY - 1)) / COLL_ARITY) - 1;
                                parent_rank = sctk_get_comm_world_rank(pte->idx - SCTK_PTL_PTE_HIDDEN, parent_rank);
                                node->parent = sctk_ptl_map_id(
					rail,
					__sctk_ptl_offcoll_rotate_ranks(parent_rank, root)
				);
                        }

			/* if not a leaf, compute children ranks */
                        if(!node->leaf)
                        {
				/* the highest child should be bound in case of an irregular tree (no a multiple of ARITY) */
				h_child = (h_child >= size) ? size -1 : h_child;
                        	nb_children = h_child - l_child + 1;
				
                                node->children = sctk_malloc(sizeof(int) * nb_children);
                                for (i = 0; i < nb_children; ++i) 
                                {
                                        child_rank = sctk_get_comm_world_rank(pte->idx - SCTK_PTL_PTE_HIDDEN, l_child + i);
                                        node->children[i] = sctk_ptl_map_id(
						rail, 
						__sctk_ptl_offcoll_rotate_ranks(child_rank, root)
					);
                                }
				node->nb_children = nb_children;
                        }
			else /* the node is a leaf => no child */
			{
				node->children = NULL;
				node->nb_children = 0;
			}
                }
                sctk_spinlock_unlock(&node->lock);
        }
}

/**
 * Barrier offloading.
 *
 * deadlock should be prevented by the fact that the same halfbarrier (UP or DOWN) cannot be
 * performed concurrently. For example, if some processes already released, start a new barrier
 * (for example the root 0 has a great chance to be released the first), they will be blocked by
 * the fact the downstream processes has to complete the previous half barrier (DOWN) to start
 * the UP one. This way, it should not be possible to overwrite the same counter (and only two ME
 * could be used per communicator to handle all the barrier collectives)
 */
static inline void __sctk_ptl_offcoll_barrier_run(sctk_ptl_rail_info_t* srail, sctk_ptl_pte_t* pte)
{
        sctk_assert(srail);
        sctk_assert(__sctk_ptl_offcoll_enabled(srail));

        size_t i, nb_children; 
	int cnt_prev_ops;
	sctk_ptl_cnth_t *me_cnt_up, *me_cnt_down;
        sctk_ptl_cnt_t dummy, dummy2; 
	sctk_ptl_offcoll_tree_node_t* bnode; 

	bnode        = pte->node + SCTK_PTL_OFFCOLL_BARRIER;
        nb_children  = bnode->nb_children;
	cnt_prev_ops = sctk_atomics_fetch_and_incr_int(&bnode->iter);
	me_cnt_up    = bnode->spec.barrier.cnt_hb_up;
	me_cnt_down  = bnode->spec.barrier.cnt_hb_down;

        /* If a leaf, just notify the parent.
         * Maybe a bit of latency here could be interesting to allow
         * other processes to set up their environment before the barrier starts
         */
        if(bnode->leaf)
        {
                sctk_ptl_emit_put(md_up, 0, bnode->parent, pte, SCTK_PTL_MATCH_OFFCOLL_BARRIER_UP, 0, 0, 0, md_up);
        }
        else
        {
		/* if the root : */
                if(SCTK_PTL_IS_ANY_PROCESS(bnode->parent))
                {
                        /* wait the completion of all its children */
			sctk_ptl_emit_trig_cnt_incr(*me_cnt_down, 1, *me_cnt_up, (size_t)((cnt_prev_ops + 1) * nb_children));
                }
                else /* otherwise, it is an intermediate node */
                {
                        /* forward put() to my parent after receiving a direct put() from all my children (meaning there are ready)
                         */
			sctk_ptl_emit_trig_put(md_up, 0, bnode->parent, pte, SCTK_PTL_MATCH_OFFCOLL_BARRIER_UP, 0, 0, 0, md_up, *me_cnt_up, (size_t)((cnt_prev_ops + 1) * nb_children));
                }
                
		/* emitting to all the children :
                 * (1) directly straight forward for the root (just after the trig_cnt_incr)
                 * (2) only after receiving the put() from the parent
                 */
                for(i = 0; i < nb_children; i++)
                {
                        sctk_ptl_emit_trig_put(md_down, 0, bnode->children[i], pte, SCTK_PTL_MATCH_OFFCOLL_BARRIER_DOWN, 0, 0, 0, md_down, *me_cnt_down, (size_t)(cnt_prev_ops + 1));
                }
        }

        /* wait for the parent to notify the current process the half-barrier returned. This is required to hold
         * the execution flow (within the software) as all calls are directly set up into the NIC
         */
	sctk_ptl_ct_wait_thrs(*me_cnt_down, (size_t)(cnt_prev_ops + 1), &dummy);

        /* do not forget to reset the second ME to its initial value
         */
	/*sctk_ptl_emit_cnt_incr(*me_cnt_down, (size_t)((-1-nb_children))); //TODO: Issue here w/ the standard if trig_op (p. 89 & 101)*/

	/*sctk_warning("Barrier Done over idx = %d CPT=%d", pte->idx, cpt);*/
}

static inline void __sctk_ptl_offcoll_bcast_eager_run(sctk_ptl_rail_info_t* srail, sctk_ptl_pte_t* pte, void* buf, size_t bytes, int is_root)
{
	sctk_ptl_offcoll_tree_node_t* bnode; 
	size_t nb_children, i;
	sctk_ptl_cnt_t dummy;
	sctk_ptl_local_data_t* recv_me = NULL, *tmp_md = NULL;

	sctk_assert(buf);
	sctk_assert(srail);

	/* first, retrieve the tree related to this broadcast */
	sctk_assert(pte);
	bnode        = pte->node + SCTK_PTL_OFFCOLL_BCAST;
	sctk_assert(bnode);
        nb_children  = bnode->nb_children;

	/* if only one node in the tree, don't */
	if(bnode->leaf && is_root)
		return;

	/* If current node is not a leaf, children exist 
	 * and data will be propagated ==> need an MD to do it
	 */
	if(!bnode->leaf)
	{
		tmp_md = sctk_ptl_md_create_with_cnt(srail, buf, bytes, (SCTK_PTL_MD_PUT_NOEV_FLAGS | PTL_MD_EVENT_CT_SEND));
		tmp_md->prot = SCTK_PTL_PROT_EAGER;
		tmp_md->type = SCTK_PTL_TYPE_OFFCOLL;
		sctk_assert(tmp_md);
		sctk_ptl_md_register(srail, tmp_md);
	}

	/* If current node is not the tree root, a parent exist
	 * and data will have to be received before anything else
	 * ==> need an ME to do so
	 */
	if(!is_root)
	{
		recv_me = sctk_ptl_me_create_with_cnt(
				srail,
				buf,
				bytes,
				bnode->parent,
				SCTK_PTL_MATCH_OFFCOLL_BCAST,
				SCTK_PTL_MATCH_INIT,
				(SCTK_PTL_ME_PUT_FLAGS | SCTK_PTL_ONCE)
			);
		recv_me->msg = NULL; /* no need for it (for noww */
		recv_me->list = SCTK_PTL_PRIORITY_LIST;
		recv_me->type = SCTK_PTL_TYPE_OFFCOLL; /* Standard MPI message */
		recv_me->prot = SCTK_PTL_PROT_EAGER; /* handled by offload protocols */
		sctk_ptl_me_register(srail, recv_me, pte);
		sctk_assert(recv_me);
		
		/* register a trigger to send data to children once data have been received.
		 * The ct_event is set to SCTK_PTL_ACTIVE_UNLOCK_THRS from a software manner
		 * (eq_poll_me) as the payload can reach an ME in the PRIORITY_LIST (when the ME is submitted
		 * BEFORE receiving a PUT) or in the OVERFLOW_LIST (when the ME is submitted AFTER receiving
		 * a PUT). In that second case, the implementation cannot guess where data have been copied and
		 * a full event is used to retrieve the address. When polling such an event, the counter is incremented
		 * and the data propagation to children is done by the hardware
		 */
		for (i = 0; i < nb_children; ++i)
		{
			sctk_ptl_emit_trig_put(
				tmp_md,
				bytes,
				bnode->children[i],
				pte,
				SCTK_PTL_MATCH_OFFCOLL_BCAST,
				0, 0, 0, tmp_md,
				recv_me->slot.me.ct_handle,
				SCTK_PTL_ACTIVE_UNLOCK_THRS
			);
		}
	}
	else /* The current node is the root one */
	{
		for (i = 0; i < nb_children; ++i)
		{
			/* Directly emit the right PUT to children */
			sctk_ptl_emit_put(
				tmp_md,
				bytes,
				bnode->children[i],
				pte,
				SCTK_PTL_MATCH_OFFCOLL_BCAST,
				0, 0, 0, tmp_md
			);
		}
	}

	/* Now, wait until completion of the bcast for the current process */
	if(bnode->leaf)
	{
		/* if the node is a leaf, just wait to receive the data in the user buffer (data written into the ME) */
		sctk_ptl_ct_wait_thrs(recv_me->slot.me.ct_handle, SCTK_PTL_ACTIVE_UNLOCK_THRS, &dummy);
	}
	else
	{
		/* Otherwise, Wait for all the messages to children to be sent before releasing the node
		 * because the actual memory is used to send the message and we don't want the user
		 * to alter the data before any 'send' to come is processed
		 */
		sctk_ptl_ct_wait_thrs(tmp_md->slot.md.ct_handle, (size_t)(nb_children), &dummy);
		sctk_ptl_ct_free(tmp_md->slot.md.ct_handle);
		sctk_ptl_md_release(tmp_md); /* don't forget to free, to avoid deadlock because of starvation in the NIC */
	}

	/* And just free allocated ME_handle (no interaction w/ the NIC here, it is just a free */
	if(!is_root)
	{
		/* zero means to not try to free the actual buffer pointed by start (here, the user buffer
		 * not a temp one)
		 */
		sctk_ptl_ct_free(recv_me->slot.me.ct_handle); /* Don't forget to free, to avoid starvation in the NIC */
		sctk_ptl_me_free(recv_me, 0);
	}
}

static inline void __sctk_ptl_offcoll_bcast_large_run(sctk_ptl_rail_info_t* srail, sctk_ptl_pte_t* pte, void* buf, size_t bytes, int is_root)
{
}

static inline void __sctk_ptl_offcoll_bcast_run(sctk_ptl_rail_info_t* srail, sctk_ptl_pte_t* pte, void* buf, size_t bytes, int is_root)
{
	if(bytes <= srail->eager_limit)
	{
		__sctk_ptl_offcoll_bcast_eager_run(srail, pte, buf, bytes, is_root);
	}
	else
	{
		__sctk_ptl_offcoll_bcast_large_run(srail, pte, buf, bytes, is_root);
	}
}

void sctk_ptl_offcoll_event_me(sctk_rail_info_t* rail, sctk_ptl_event_t ev)
{
	sctk_ptl_local_data_t* user_ptr = (sctk_ptl_local_data_t*) ev.user_ptr;
	/*sctk_warning("PORTALS: EQS EVENT '%s' idx=%d, type=%x, prot=%x, match=%s from %s, sz=%llu, user=%p", sctk_ptl_event_decode(ev), ev.pt_index, user_ptr->type, user_ptr->prot, __sctk_ptl_match_str(malloc(32), 32, ev.match_bits), SCTK_PTL_STR_LIST(((sctk_ptl_local_data_t*)ev.user_ptr)->list), ev.mlength, ev.user_ptr);*/

	switch(ev.type)
	{
		case PTL_EVENT_PUT_OVERFLOW:         /* a previous received PUT matched a just appended ME */
			sctk_assert(ev.mlength <= user_ptr->slot.me.length);
			if(user_ptr->prot == SCTK_PTL_PROT_EAGER && ev.mlength > 0)
				memcpy(user_ptr->slot.me.start, ev.start, ev.mlength);
		case PTL_EVENT_PUT:                  /* a Put() reached the local process */
			if(user_ptr->prot == SCTK_PTL_PROT_EAGER && user_ptr->match.offload.type == SCTK_BROADCAST_OFFLOAD_MESSAGE)
				PtlCTSet(
				user_ptr->slot.me.ct_handle,
				(sctk_ptl_cnt_t){.success = SCTK_PTL_ACTIVE_UNLOCK_THRS, .failure = 0});
			break;

		case PTL_EVENT_GET:                  /* a remote process get the data back */
			break; /* will increment a counter automagically, nothing to do */
		case PTL_EVENT_GET_OVERFLOW:          /* a previous received GET matched a just appended ME */
			//not_reachable(); /* a GET op is always set with an ME BEFORE sending the ready msg */
			break;
		case PTL_EVENT_ATOMIC:                /* an Atomic() reached the local process */
		case PTL_EVENT_ATOMIC_OVERFLOW:       /* a previously received ATOMIC matched a just appended one */
		case PTL_EVENT_FETCH_ATOMIC:          /* a FetchAtomic() reached the local process */
		case PTL_EVENT_FETCH_ATOMIC_OVERFLOW: /* a previously received FETCH-ATOMIC matched a just appended one */
		case PTL_EVENT_PT_DISABLED:           /* ERROR: The local PTE is disabeld (FLOW_CTRL) */
		case PTL_EVENT_SEARCH:                /* a PtlMESearch completed */
		case PTL_EVENT_LINK:                  /* MISC: A new ME has been linked, (maybe not useful) */
		case PTL_EVENT_AUTO_UNLINK:           /* an USE_ONCE ME has been automatically unlinked */
		case PTL_EVENT_AUTO_FREE:             /* an USE_ONCE ME can be now reused */
			not_reachable();              /* have been disabled */
			break;
		default:
			sctk_fatal("Portals ME event not recognized: %d", ev.type);
			break;
	}
}

void sctk_ptl_offcoll_event_md(sctk_rail_info_t* rail, sctk_ptl_event_t ev)
{
	sctk_ptl_local_data_t* user_ptr = (sctk_ptl_local_data_t*) ev.user_ptr;
	/*sctk_warning("PORTALS: MDS EVENT '%s' from %s, type=%d, prot=%d",sctk_ptl_event_decode(ev), SCTK_PTL_STR_LIST(ev.ptl_list), user_ptr->type, user_ptr->prot);*/

	switch(ev.type)
	{
		case PTL_EVENT_ACK:   /* a PUT reached a remote process */
			break;

		case PTL_EVENT_REPLY: /* a GET operation completed */
			break;

		case PTL_EVENT_SEND: /* a Put() left the local process */
			break;
		default:
			sctk_fatal("Unrecognized MD event: %d", ev.type);
			break;
	}
}



/**************************************************/
/**************************************************/
/*********** MPI Interface                        */
/**************************************************/
/**************************************************/

int ptl_offcoll_barrier(int comm_idx, int rank, int size)
{
        sctk_ptl_pte_t* pte = SCTK_PTL_PTE_ENTRY(grail->pt_table, comm_idx);
        sctk_assert(pte);
	
	__sctk_ptl_offcoll_build_tree(grail, pte, rank, 0, size, SCTK_PTL_OFFCOLL_BARRIER);
        __sctk_ptl_offcoll_barrier_run(grail, pte);
        return 0;
}

/* contiguous data */
int ptl_offcoll_bcast(int comm_idx, int rank, int size, void* buf, size_t bytes, int root)
{
	sctk_ptl_pte_t* pte = SCTK_PTL_PTE_ENTRY(grail->pt_table, comm_idx);
	sctk_ptl_offcoll_tree_node_t* node = NULL;
	sctk_assert(pte);


	__sctk_ptl_offcoll_build_tree(grail, pte, rank, root, size, SCTK_PTL_OFFCOLL_BCAST);
	__sctk_ptl_offcoll_bcast_run(grail, pte, buf, bytes, (root == rank));
	return 0;
}

int ptl_offcoll_reduce()
{
	return 0;
}

#endif
