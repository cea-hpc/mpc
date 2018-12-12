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

sctk_ptl_rail_info_t* grail;

void sctk_ptl_offcoll_init(sctk_ptl_rail_info_t* srail)
{
	sctk_assert(__sctk_ptl_offcoll_enabled(srail));
	md_up = sctk_ptl_md_create_with_cnt(srail, NULL, 0, SCTK_PTL_MD_PUT_NOEV_FLAGS);
	md_down = sctk_ptl_md_create_with_cnt(srail, NULL, 0, SCTK_PTL_MD_PUT_NOEV_FLAGS);
	sctk_ptl_md_register(srail, md_up);
	sctk_ptl_md_register(srail, md_down);

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
	sctk_ptl_local_data_t* me_up = NULL, *me_down = NULL;
	sctk_assert(srail);
	sctk_assert(pte);
	sctk_assert(__sctk_ptl_offcoll_enabled(srail));
	
	me_up = sctk_ptl_me_create_with_cnt(srail, NULL, 0, SCTK_PTL_ANY_PROCESS, SCTK_PTL_MATCH_OFFCOLL_BARRIER_UP, SCTK_PTL_MATCH_INIT, SCTK_PTL_ME_PUT_NOEV_FLAGS);
	me_up->msg = NULL;
	me_up->list = SCTK_PTL_PRIORITY_LIST;
	/// TODO: Wrong setting to check (but not event generated...)
	me_up->type = 0;
	me_up->prot = 0;
	me_down = sctk_ptl_me_create_with_cnt(srail, NULL, 0, SCTK_PTL_ANY_PROCESS, SCTK_PTL_MATCH_OFFCOLL_BARRIER_DOWN, SCTK_PTL_MATCH_INIT, SCTK_PTL_ME_PUT_NOEV_FLAGS);
	me_down->msg = NULL;
	me_down->list = SCTK_PTL_PRIORITY_LIST;
	/// TODO: Wrong setting to check (but not event generated...)
	me_down->type = 0;
	me_down->prot = 0;
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

static inline sctk_ptl_offcoll_tree_node_t __sctk_ptl_offcoll_connect_peers(sctk_ptl_rail_info_t* srail, sctk_ptl_pte_t* pte, int local_rank, int root, int size)
{
        int l_child = -1, h_child = -1, parent_rank, child_rank; 
        size_t i, nb_children;
        sctk_assert(srail);
        sctk_rail_info_t* rail = sctk_ptl_promote_to_rail(srail);
	sctk_ptl_offcoll_tree_node_t* node = pte->node + SCTK_PTL_OFFCOLL_BARRIER;
        sctk_assert(rail);

        if(node->leaf == -1 || node->root != root)
        {
                sctk_spinlock_lock(&node->lock);
                if(node->leaf == -1 || node->root != root)
                {
                        /* get children range for the current process 
                         * h_child is the child with the highest rank 
                         * m_child is the child with the lowest_rank
                         */
                        h_child = (local_rank + 1) * COLL_ARITY;
                        l_child = h_child - (COLL_ARITY -1);
                        node->leaf = (l_child >= size);
			node->root = root;

                        /* save the infos (parent could be negative */
                        parent_rank = (int)((local_rank + (COLL_ARITY - 1)) / COLL_ARITY) - 1;
			/* this allows us to shift the tree root accordingly */
			parent_rank = (parent_rank + root) % size;
                        if(parent_rank >= 0) /* non-root node */
                        {
                                /* TODO: convert local to global */
                                parent_rank = sctk_get_comm_world_rank(pte->idx - SCTK_PTL_PTE_HIDDEN, parent_rank);
				sctk_nodebug("parent = %d", parent_rank);
                                node->parent = sctk_ptl_map_id(rail, parent_rank); /* TODO: dirty PMI call */
                        }

                        if(!node->leaf) /* if not a leaf */
                        {
				/* the highest child should be bound in case of an irregular tree (no a multiple of ARITY) */
				h_child = (h_child >= size) ? size -1 : h_child;
                        	nb_children = h_child - l_child + 1;
				sctk_nodebug("nb = %llu", nb_children);
				
                                node->children = sctk_malloc(sizeof(int) * nb_children);
                                for (i = 0; i < nb_children; ++i) 
                                {
                                        child_rank = sctk_get_comm_world_rank(pte->idx - SCTK_PTL_PTE_HIDDEN, l_child + i);
					/* this allows to shift the root tree */
					child_rank = (child_rank + root) % size;
                                        node->children[i] = sctk_ptl_map_id(rail, child_rank); /* TODO: awful PMI call */
					sctk_nodebug("child[%d] = %d", i, child_rank);
                                }
				node->nb_children = nb_children;
                        }
			else
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

	//sctk_ptl_chk(PtlCTGet(*me_cnt_up, &dummy));
	//sctk_ptl_chk(PtlCTGet(*me_cnt_down, &dummy2));
	//static int cpt = 0;
	//cpt++;
	//sctk_warning("Barrier Start over idx = %d UP = %d, DOWN = %d CHILD = %llu, CPT=%d", pte->idx, dummy.success, dummy2.success, nb_children, cpt);

        /* If a leaf, just notify the parent.
         * Maybe a bit of latency here could be interesting to allow
         * other processes to set up their environment before the barrier starts
         */
        if(bnode->leaf)
        {
                sctk_ptl_emit_put(md_up, 0, bnode->parent, pte, SCTK_PTL_MATCH_OFFCOLL_BARRIER_UP, 0, 0, 0, NULL);
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
			sctk_ptl_emit_trig_put(md_up, 0, bnode->parent, pte, SCTK_PTL_MATCH_OFFCOLL_BARRIER_UP, 0, 0, 0, NULL, *me_cnt_up, (size_t)((cnt_prev_ops + 1) * nb_children));
                }
                
		/* emitting to all the children :
                 * (1) directly straight forward for the root (just after the trig_cnt_incr)
                 * (2) only after receiving the put() from the parent
                 */
                for(i = 0; i < nb_children; i++)
                {
                        sctk_ptl_emit_trig_put(md_down, 0, bnode->children[i], pte, SCTK_PTL_MATCH_OFFCOLL_BARRIER_DOWN, 0, 0, 0, NULL, *me_cnt_down, (size_t)(cnt_prev_ops + 1));
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


static inline void __sctk_ptl_offcoll_bcast_run(sctk_ptl_rail_info_t* srail, sctk_ptl_pte_t* pte, void* buf, size_t bytes, int root)
{
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
	
	__sctk_ptl_offcoll_connect_peers(grail, pte, rank, 0, size);
        __sctk_ptl_offcoll_barrier_run(grail, pte);
        return 0;
}

/* contiguous data */
int ptl_offcoll_bcast(int comm_idx, int rank, int size, void* buf, size_t bytes, int root)
{
	sctk_ptl_pte_t* pte = SCTK_PTL_PTE_ENTRY(grail->pt_table, comm_idx);
	sctk_assert(pte);

	__sctk_ptl_offcoll_connect_peers(grail, pte, rank, root, size);
	__sctk_ptl_offcoll_bcast_run(grail, pte, buf, bytes, root);
	return 0;
}

int ptl_offcoll_reduce()
{
	return 0;
}

#endif
