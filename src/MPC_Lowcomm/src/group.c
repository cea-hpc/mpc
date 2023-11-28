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
/* #   - BESNARD Jean-Baptiste jbbesnard@paratools.fr                     # */
/* #                                                                      # */
/* ######################################################################## */
#include "group.h"

#include <mpc_lowcomm.h>
#include <string.h>

#include <sctk_alloc.h>
#include <mpc_common_debug.h>
#include <mpc_common_rank.h>
#include <mpc_common_datastructure.h>

#include "mpc_lowcomm_types.h"
#include "peer.h"

/*******************************
* COMMUNICATORS AND PROCESSES *
*******************************/



static inline int ___process_rank_from_world_rank(int world_rank)
{
	if(world_rank < 0)
	{
		return -1;
	}

#ifdef MPC_IN_PROCESS_MODE
	return world_rank;
#endif

	if(mpc_common_get_process_count() == 1)
	{
		return 0;
	}

	/* Compute the task number */
	int proc_rank;

	int world_size = mpc_lowcomm_get_size();

	int local_tasks  = world_size / mpc_common_get_process_count();
	int remain_tasks = world_size % mpc_common_get_process_count();

	if(world_rank < (local_tasks + 1) * remain_tasks)
	{
		proc_rank = world_rank / (local_tasks + 1);
	}
	else
	{
		proc_rank =
			remain_tasks +
			( (world_rank - (local_tasks + 1) * remain_tasks) / local_tasks);
	}

	return proc_rank;
}

mpc_lowcomm_peer_uid_t mpc_lowcomm_group_process_uid_for_rank(mpc_lowcomm_group_t *g, int rank)
{
	_mpc_lowcomm_group_rank_descriptor_t *desc = mpc_lowcomm_group_descriptor(g, rank);

	assume(desc != NULL);
	return desc->host_process_uid;
}

int *mpc_lowcomm_group_world_ranks(mpc_lowcomm_group_t *g)
{
	if(!_mpc_lowcomm_group_rank_descriptor_all_from_local_set(g->size, g->ranks) )
	{
		return NULL;
	}

	int *ret = sctk_malloc(sizeof(int) * g->size);

	assume(ret != NULL);

	unsigned int i;

	for(i = 0; i < g->size; i++)
	{
		ret[i] = g->ranks->comm_world_rank;
	}

	return ret;
}

int mpc_lowcomm_group_process_rank_from_world(int comm_world_rank)
{
	mpc_lowcomm_group_t *gworld = _mpc_lowcomm_group_world_no_assert();

	if(!gworld)
	{
		/* World is not ready yet use computation */
		return mpc_lowcomm_peer_get_rank(___process_rank_from_world_rank(comm_world_rank) );
	}
        
        mpc_lowcomm_peer_uid_t uid = mpc_lowcomm_group_process_uid_for_rank(gworld, comm_world_rank);

        return mpc_lowcomm_peer_get_rank(uid);
}

static inline int __compar_uid(const void *pa, const void *pb)
{
	mpc_lowcomm_peer_uid_t *a = (mpc_lowcomm_peer_uid_t *)pa;
	mpc_lowcomm_peer_uid_t *b = (mpc_lowcomm_peer_uid_t *)pb;

	return *a < *b;
}

static inline void __fill_process_info_remote(mpc_lowcomm_group_t *g)
{
	mpc_lowcomm_peer_uid_t *all_proc_uids = sctk_malloc(g->size * sizeof(mpc_lowcomm_peer_uid_t) );

	assume(all_proc_uids != NULL);

	unsigned int i;

	for(i = 0; i < g->size; i++)
	{
		all_proc_uids[i] = g->ranks[i].host_process_uid;
	}

	qsort(all_proc_uids, g->size, sizeof(mpc_lowcomm_peer_uid_t), __compar_uid);

	/* Now count number of ids */
	mpc_lowcomm_peer_uid_t last_seen_uid = 0;
	int uid_count = 0;

	mpc_lowcomm_peer_uid_t my_uid = mpc_lowcomm_monitor_get_uid();
	int local_task_count          = 0;

	for(i = 0; i < g->size; i++)
	{
		if(!last_seen_uid || (last_seen_uid != all_proc_uids[i]) )
		{
			last_seen_uid = all_proc_uids[i];
			uid_count++;
		}
		if(my_uid == all_proc_uids[i])
		{
			local_task_count++;
		}
	}

	g->tasks_count_in_process = 0;
	g->process_count          = uid_count;
	g->tasks_count_in_process = local_task_count;

	sctk_free(all_proc_uids);
}

static inline void __fill_process_info(mpc_lowcomm_group_t *g)
{
	/* It is a remote group ? */
	if(!_mpc_lowcomm_group_rank_descriptor_all_from_local_set(g->size, g->ranks) )
	{
		__fill_process_info_remote(g);
		return;
	}

	/* Here is the response for a local group */

	unsigned int  process_count = (unsigned int)mpc_common_get_process_count();
	unsigned int *counter       = sctk_malloc(process_count * sizeof(unsigned int) );

	assume(counter != NULL);

	memset(counter, 0, process_count * sizeof(unsigned int) );

	unsigned int i;

	for(i = 0; i < g->size; i++)
	{
		int cw_rank = mpc_lowcomm_group_world_rank(g, i);
		int p_rank  = mpc_lowcomm_group_process_rank_from_world(cw_rank);

		assert(0 <= p_rank);
		assert(p_rank < (int)process_count);

		counter[p_rank]++;
	}

	/* This is the process count */

	int group_process_count = 0;

	for(i = 0; i < process_count; i++)
	{
		if(counter[i] != 0)
		{
			group_process_count++;
		}
	}

	g->process_count = group_process_count;

	/* Now fill process list */
	g->process_list = sctk_malloc(sizeof(int) * group_process_count);
	assume(g->process_list != NULL);

	group_process_count = 0;
	for(i = 0; i < process_count; i++)
	{
		if(counter[i] != 0)
		{
			g->process_list[group_process_count] = i;
			group_process_count++;
		}
	}

	g->tasks_count_in_process = counter[mpc_lowcomm_get_process_rank()];

	sctk_free(counter);
}

int _mpc_lowcomm_group_local_process_count(mpc_lowcomm_group_t *g)
{
	mpc_common_spinlock_lock(&g->process_lock);
	/* Already computed */
	if(0 < g->tasks_count_in_process)
	{
		mpc_common_spinlock_unlock(&g->process_lock);
		return g->tasks_count_in_process;
	}

	__fill_process_info(g);

	assume(0 <= g->tasks_count_in_process);

	mpc_common_spinlock_unlock(&g->process_lock);

	return g->tasks_count_in_process;
}

int _mpc_lowcomm_group_process_count(mpc_lowcomm_group_t *g)
{
	mpc_common_spinlock_lock(&g->process_lock);
	/* Already computed */
	if(0 < g->process_count)
	{
		mpc_common_spinlock_unlock(&g->process_lock);
		return g->process_count;
	}

	__fill_process_info(g);

	assume(0 < g->process_count);

	mpc_common_spinlock_unlock(&g->process_lock);

	return g->process_count;
}

int *_mpc_lowcomm_group_process_list(mpc_lowcomm_group_t *g)
{
	mpc_common_spinlock_lock(&g->process_lock);
	/* Already computed */
	if(g->process_list != NULL)
	{
		mpc_common_spinlock_unlock(&g->process_lock);
		return g->process_list;
	}

	__fill_process_info(g);

	mpc_common_spinlock_unlock(&g->process_lock);

	return g->process_list;
}

int mpc_lowcomm_group_get_local_leader(mpc_lowcomm_group_t *g)
{
	if(g->local_leader != MPC_PROC_NULL)
	{
		return g->local_leader;
	}

	g->local_leader = MPC_PROC_NULL;

	/* We need to compute the leader */
	unsigned int i;
	int          my_proc_rank = mpc_common_get_process_rank();

	for(i = 0; i < g->size; i++)
	{
		if(my_proc_rank == mpc_lowcomm_group_process_rank_from_world(g->ranks[i].comm_world_rank) )
		{
			g->local_leader = i;
			break;
		}
	}

	return g->local_leader;
}

int mpc_lowcomm_group_includes(mpc_lowcomm_group_t *g, int cw_rank, mpc_lowcomm_peer_uid_t uid)
{
	if(cw_rank == MPC_PROC_NULL)
	{
		return 0;
	}

	int rank = mpc_lowcomm_group_rank_for(g, cw_rank, uid);

	return rank != MPC_PROC_NULL;
}

/****************************************
* _MPC_LOWCOMM_GROUP_RANK_DESCRIPTOR_S *
****************************************/

int _mpc_lowcomm_group_rank_descriptor_equal(_mpc_lowcomm_group_rank_descriptor_t *a,
                                             _mpc_lowcomm_group_rank_descriptor_t *b)
{
	//mpc_common_debug_error("\t %d %lld == %d %lld", a->comm_world_rank, a->host_process_uid, b->comm_world_rank, b->host_process_uid);

	/* No memcmp as there is a hole in the struct */
	if( (a->comm_world_rank == b->comm_world_rank) && (a->host_process_uid == b->host_process_uid) )
	{
		return 1;
	}

	return 0;
}

int _mpc_lowcomm_group_rank_descriptor_all_from_local_set(unsigned int size, _mpc_lowcomm_group_rank_descriptor_t *ranks)
{
	unsigned int i;

	mpc_lowcomm_set_uid_t my_id = mpc_lowcomm_monitor_get_gid();


	for(i = 0; i < size; i++)
	{
		if(mpc_lowcomm_peer_get_set(ranks[i].host_process_uid) != my_id)
		{
			return 0;
		}
	}

	return 1;
}

int _mpc_lowcomm_group_rank_descriptor_set_in_grp(_mpc_lowcomm_group_rank_descriptor_t *rd,
                                                  mpc_lowcomm_set_uid_t set,
                                                  int cw_rank)
{
	if(!rd)
	{
		return MPC_LOWCOMM_ERROR;
	}

	rd->comm_world_rank  = cw_rank;
	rd->host_process_uid = mpc_lowcomm_monitor_uid_of(set, ___process_rank_from_world_rank(cw_rank) );


	return MPC_LOWCOMM_SUCCESS;
}

int _mpc_lowcomm_group_rank_descriptor_set_uid(_mpc_lowcomm_group_rank_descriptor_t *rd, mpc_lowcomm_peer_uid_t uid)
{
	if(!rd)
	{
		return MPC_LOWCOMM_ERROR;
	}

	rd->comm_world_rank  = mpc_lowcomm_peer_get_rank(uid);
	rd->host_process_uid = uid;

	return MPC_LOWCOMM_SUCCESS;
}

int _mpc_lowcomm_group_rank_descriptor_set(_mpc_lowcomm_group_rank_descriptor_t *rd, int cw_rank)
{
	return _mpc_lowcomm_group_rank_descriptor_set_in_grp(rd, mpc_lowcomm_monitor_get_gid(), cw_rank);
}

uint32_t _mpc_lowcomm_group_rank_descriptor_hash(_mpc_lowcomm_group_rank_descriptor_t *rd)
{
	return (uint32_t)rd->comm_world_rank ^ rd->host_process_uid ^ rd->host_process_uid >> 32;
}

/************************
* _MPC_LOWCOMM_GROUP_T *
************************/

static inline void __group_free(mpc_lowcomm_group_t *g)
{
	sctk_free(g->ranks);
	g->ranks = NULL;
	sctk_free(g->global_to_local);
	g->global_to_local = NULL;
	g->size            = 0;
	g->process_count   = 0;
	sctk_free(g->process_list);
	g->process_list           = NULL;
	g->local_leader           = MPC_PROC_NULL;
	g->tasks_count_in_process = MPC_PROC_NULL;
}

static inline int __internal_group_free(mpc_lowcomm_group_t **group, int check_list)
{
	if(!group)
	{
		return 1;
	}

	mpc_lowcomm_group_t *g = *group;

	if(!g)
	{
		return 0;
	}

	if(g->do_not_free)
	{
		return 0;
	}

	int val = OPA_fetch_and_decr_int(&(g->refcount) );

	assume(val >= 0);

	if(val == 1)
	{
		if(check_list)
		{
			/* First pop the group */
			int ret = _mpc_lowcomm_group_list_pop(g);

			if(ret == 0)
			{
				/* Was not present or had not pending ref do free */
				__group_free(g);
			}
		}
		else
		{
			__group_free(g);
		}
	}

	*group = NULL;

	return 0;
}

int mpc_lowcomm_group_free(mpc_lowcomm_group_t **group)
{
	return __internal_group_free(group, 1);
}

mpc_lowcomm_group_eq_e __group_compare(mpc_lowcomm_group_t *g1, mpc_lowcomm_group_t *g2, int do_similar)
{
	unsigned int           i;
	mpc_lowcomm_group_eq_e ret = MPC_GROUP_UNEQUAL;

	if(g1 == g2)
	{
		return MPC_GROUP_IDENT;
	}
	else if(g1->size == g2->size)
	{
		ret = MPC_GROUP_IDENT;

		/* Check for eq */
		for(i = 0; i < g1->size; i++)
		{
			if(!_mpc_lowcomm_group_rank_descriptor_equal(&g1->ranks[i], &g2->ranks[i]) )
			{
				/* Mismatch back to unequal */
				ret = MPC_GROUP_UNEQUAL;
				break;
			}
		}

		/* Did we pass ? */
		if(ret == MPC_GROUP_IDENT)
		{
			return ret;
		}
	}

	if(!do_similar || (g1->size != g2->size) )
	{
		return MPC_GROUP_UNEQUAL;
	}

	/* If we are here check for SIMILAR */
	ret = MPC_GROUP_SIMILAR;

	unsigned int j;

	for(i = 0; i < g1->size; i++)
	{
		_mpc_lowcomm_group_rank_descriptor_t *rd    = &g1->ranks[i];
		_mpc_lowcomm_group_rank_descriptor_t *found = NULL;

		for(j = 0; j < g2->size; j++)
		{
			if(_mpc_lowcomm_group_rank_descriptor_equal(rd, &g2->ranks[j]) )
			{
				found = &g2->ranks[j];
			}
		}

		if(!found)
		{
			ret = MPC_GROUP_UNEQUAL;
		}
	}

	return ret;
}

mpc_lowcomm_group_eq_e mpc_lowcomm_group_compare(mpc_lowcomm_group_t *g1, mpc_lowcomm_group_t *g2)
{
	return __group_compare(g1, g2, 1);
}

unsigned int mpc_lowcomm_group_size(mpc_lowcomm_group_t *g)
{
	assert(g != NULL);
	return g->size;
}

_mpc_lowcomm_group_rank_descriptor_t *mpc_lowcomm_group_descriptor(mpc_lowcomm_group_t *g, int rank)
{
	//mpc_common_debug_warning("GET %d / %d", rank, g->size);
	assert(g != NULL);
	assert(0 <= rank);
	assert(rank < (int)g->size);
	return &g->ranks[rank];
}

int mpc_lowcomm_group_world_rank(mpc_lowcomm_group_t *g, int rank)
{
	if(rank == MPC_PROC_NULL)
	{
		return MPC_PROC_NULL;
	}

	_mpc_lowcomm_group_rank_descriptor_t *desc = mpc_lowcomm_group_descriptor(g, rank);
	return desc->comm_world_rank;
}

int mpc_lowcomm_group_rank_for(mpc_lowcomm_group_t *g, int rank, mpc_lowcomm_peer_uid_t uid)
{
	assert(0 <= rank);

	if(g->size == 0)
	{
		/* Nobody in the empty group */
		return MPC_PROC_NULL;
	}

	if(rank == MPC_PROC_NULL)
	{
		return rank;
	}

	if(g->global_to_local)
	{
		int trank = g->global_to_local[rank];

		if(trank == MPC_PROC_NULL)
		{
			return MPC_PROC_NULL;
		}

		if(g->ranks[trank].host_process_uid == uid)
		{
			return trank;
		}
	}

	assume(g->ranks != NULL);

	unsigned int i;

	for(i = 0; i < g->size; i++)
	{
		//mpc_common_debug_error("[%d] %d == %d ?",i,  rank, g->ranks[i].comm_world_rank);

		if( (g->ranks[i].comm_world_rank == rank) && (g->ranks[i].host_process_uid == uid) )
		{
			return i;
		}
	}

	return MPC_PROC_NULL;
}

int mpc_lowcomm_group_rank(mpc_lowcomm_group_t *g)
{
	return mpc_lowcomm_group_rank_for(g, mpc_lowcomm_get_rank(), mpc_lowcomm_monitor_get_uid() );
}

int mpc_lowcomm_group_translate_ranks(mpc_lowcomm_group_t *g1, int n, const int ranks1[], mpc_lowcomm_group_t *g2, int ranks2[])
{
	int i;

	for(i = 0; i < n; i++)
	{
		unsigned int j;

		if(ranks1[i] == MPC_PROC_NULL)
		{
			ranks2[i] = MPC_PROC_NULL;
			continue;
		}

		//mpc_common_debug_error("%d / %d", ranks1[i], g1->size);

		if(g1->size <= (unsigned int)ranks1[i])
		{
			return MPC_LOWCOMM_ERROR;
		}

		_mpc_lowcomm_group_rank_descriptor_t *rd = &g1->ranks[ranks1[i]];

		ranks2[i] = MPC_UNDEFINED;

		for(j = 0; j < g2->size; j++)
		{
			if(_mpc_lowcomm_group_rank_descriptor_equal(rd, &g2->ranks[j]) )
			{
				ranks2[i] = j;
				break;
			}
		}
	}


	return MPC_LOWCOMM_SUCCESS;
}

static inline int __rank_array_invalid(mpc_lowcomm_group_t *grp, const int ranks[], int n)
{
	int i;

	/* Check size memberhship */
	for(i = 0; i < n; i++)
	{
		if((int)grp->size < ranks[i] || (ranks[i] < 0) )
		{
			mpc_common_debug_error("%d is not part of group of size %d", ranks[i], grp->size);
			return 1;
		}
	}

	return 0;
}

static inline int __ranks_array_has_duplicate(const int ranks[], int n)
{
	/* Check for unicity */
	int i, j;

	for(i = 0; i < n; i++)
	{
		for(j = 0; j < n; j++)
		{
			if(j == i)
			{
				continue;
			}

			if(ranks[i] == ranks[j])
			{
				return 1;
			}
		}
	}

	return 0;
}

static inline int __rank_in_ranks_array(const int ranks[], int n, int candidate)
{
	/* Check for unicity */
	int i;

	for(i = 0; i < n; i++)
	{
		if(candidate == ranks[i])
		{
			return 1;
		}
	}

	return 0;
}

mpc_lowcomm_group_t *mpc_lowcomm_group_excl(mpc_lowcomm_group_t *grp, int n, const int ranks[])
{
	unsigned int i;

	if(__rank_array_invalid(grp, ranks, n) )
	{
		return NULL;
	}

	if(__ranks_array_has_duplicate(ranks, n) )
	{
		return NULL;
	}

	size_t new_size = grp->size - n;

	_mpc_lowcomm_group_rank_descriptor_t *descs = sctk_malloc(new_size * sizeof(_mpc_lowcomm_group_rank_descriptor_t) );

	assume(descs != NULL);

	int cnt = 0;

	for(i = 0; i < grp->size; i++)
	{
		if(__rank_in_ranks_array(ranks, n, i) )
		{
			continue;
		}

		memcpy(&descs[cnt], mpc_lowcomm_group_descriptor(grp, i), sizeof(_mpc_lowcomm_group_rank_descriptor_t) );
		cnt++;
	}

	return _mpc_lowcomm_group_create(new_size, descs, 1);
}

mpc_lowcomm_group_t *mpc_lowcomm_group_incl(mpc_lowcomm_group_t *grp, int n, const int ranks[])
{
	unsigned int i;

	if(__rank_array_invalid(grp, ranks, n) )
	{
		return NULL;
	}

	if(__ranks_array_has_duplicate(ranks, n) )
	{
		return NULL;
	}

	size_t new_size = n;

	_mpc_lowcomm_group_rank_descriptor_t *descs = sctk_malloc(new_size * sizeof(_mpc_lowcomm_group_rank_descriptor_t) );

	assume(descs != NULL);

	for(i = 0; i < (unsigned int)n; i++)
	{
		memcpy(&descs[i], mpc_lowcomm_group_descriptor(grp, ranks[i]), sizeof(_mpc_lowcomm_group_rank_descriptor_t) );
	}

	return _mpc_lowcomm_group_create(new_size, descs, 1);
}

mpc_lowcomm_group_t *mpc_lowcomm_group_difference(mpc_lowcomm_group_t *grp, mpc_lowcomm_group_t *grp_to_sub)
{
	/* We know that at most all members of grp remain */
	_mpc_lowcomm_group_rank_descriptor_t *descs = sctk_malloc(grp->size * sizeof(_mpc_lowcomm_group_rank_descriptor_t) );

	assume(descs != NULL);

	unsigned int i, j;

	unsigned int new_size = 0;

	for(i = 0; i < grp->size; i++)
	{
		int is_in_grp_to_sub = 0;

		for(j = 0; j < grp_to_sub->size; j++)
		{
			if(_mpc_lowcomm_group_rank_descriptor_equal(&grp->ranks[i], &grp_to_sub->ranks[j]) )
			{
				is_in_grp_to_sub = 1;
				break;
			}
		}

		if(is_in_grp_to_sub)
		{
			/* Skip */
			continue;
		}

		memcpy(&descs[new_size], &grp->ranks[i], sizeof(_mpc_lowcomm_group_rank_descriptor_t) );
		new_size++;
	}

	/* Realloc */
	if(new_size)
	{
		descs = sctk_realloc(descs, new_size * sizeof(_mpc_lowcomm_group_rank_descriptor_t) );
		assume(descs != NULL);
	}

	return _mpc_lowcomm_group_create(new_size, descs, 1);
}

mpc_lowcomm_group_t *mpc_lowcomm_group_instersection(mpc_lowcomm_group_t *grp, mpc_lowcomm_group_t *grp2)
{
	/* We know that at most all members of grp remain */
	_mpc_lowcomm_group_rank_descriptor_t *descs = sctk_malloc(grp->size * sizeof(_mpc_lowcomm_group_rank_descriptor_t) );

	assume(descs != NULL);

	unsigned int i, j;

	unsigned int new_size = 0;

	for(i = 0; i < grp->size; i++)
	{
		int is_in_grp2 = 0;

		for(j = 0; j < grp2->size; j++)
		{
			if(_mpc_lowcomm_group_rank_descriptor_equal(&grp->ranks[i], &grp2->ranks[j]) )
			{
				is_in_grp2 = 1;
				break;
			}
		}

		if(!is_in_grp2)
		{
			/* Skip as not in second group*/
			continue;
		}

		memcpy(&descs[new_size], &grp->ranks[i], sizeof(_mpc_lowcomm_group_rank_descriptor_t) );
		new_size++;
	}

	if(new_size)
	{
		/* Realloc */
		descs = sctk_realloc(descs, new_size * sizeof(_mpc_lowcomm_group_rank_descriptor_t) );
		assume(descs != NULL);
	}

	return _mpc_lowcomm_group_create(new_size, descs, 1);
}

mpc_lowcomm_group_t *mpc_lowcomm_group_union(mpc_lowcomm_group_t *grp, mpc_lowcomm_group_t *grp2)
{
	/* Max size if sum of both */
	unsigned int max_size = (grp->size + grp2->size);
	_mpc_lowcomm_group_rank_descriptor_t *descs = sctk_malloc(max_size * sizeof(_mpc_lowcomm_group_rank_descriptor_t) );

	assume(descs != NULL);

	unsigned int i, j;

	unsigned int new_size = 0;

	mpc_lowcomm_group_t *ptrs[2];

	ptrs[0] = grp;
	ptrs[1] = grp2;

	int g;

	for(g = 0; g < 2; g++)
	{
		mpc_lowcomm_group_t *refgroup = ptrs[g];

		for(i = 0; i < refgroup->size; i++)
		{
			int already_present = 0;

			for(j = 0; j < new_size; j++)
			{
				if(_mpc_lowcomm_group_rank_descriptor_equal(&refgroup->ranks[i], &descs[j]) )
				{
					already_present = 1;
					break;
				}
			}

			if(already_present)
			{
				continue;
			}

			memcpy(&descs[new_size], &refgroup->ranks[i], sizeof(_mpc_lowcomm_group_rank_descriptor_t) );
			new_size++;

			assume(new_size <= max_size);
		}
	}

	if(new_size)
	{
		/* Realloc */
		descs = sctk_realloc(descs, new_size * sizeof(_mpc_lowcomm_group_rank_descriptor_t) );
		assume(descs != NULL);
	}

	return _mpc_lowcomm_group_create(new_size, descs, 1);
}

static inline void __fill_global_to_local(mpc_lowcomm_group_t *g)
{
	if(g->ranks)
	{
		/* Create the global to local list to speedup rank translation */
		int cw_size = mpc_lowcomm_get_size();


		g->global_to_local = sctk_malloc(sizeof(int) * cw_size);
		assume(g->global_to_local != NULL);

		int j;
		for(j = 0; j < cw_size; j++)
		{
			g->global_to_local[j] = MPC_PROC_NULL;
		}

		unsigned int i;
		for(i = 0; i < g->size; i++)
		{
			int trank = g->ranks[i].comm_world_rank;
			assume(trank < cw_size);
			assume(0 <= trank);
			g->global_to_local[trank] = i;
		}
	}
}

mpc_lowcomm_group_t *_mpc_lowcomm_group_create(unsigned int size, _mpc_lowcomm_group_rank_descriptor_t *ranks, int deduplicate)
{
	mpc_lowcomm_group_t *ret = sctk_malloc(sizeof(mpc_lowcomm_group_t) );

	assume(ret != NULL);

	memset(ret, 0, sizeof(mpc_lowcomm_group_t) );

	ret->ranks       = ranks;
	ret->size        = size;
	ret->do_not_free = 0;

	/* We do lazy evaluation here */
	mpc_common_spinlock_init(&ret->process_lock, 0);
	ret->process_count          = MPC_PROC_NULL;
	ret->process_list           = NULL;
	ret->tasks_count_in_process = MPC_PROC_NULL;
	ret->local_leader           = MPC_PROC_NULL;
	ret->extra_ctx_ptr          = MPC_LOWCOMM_HANDLE_CTX_NULL;
	ret->is_a_copy              = 0;

	if(_mpc_lowcomm_group_rank_descriptor_all_from_local_set(size, ranks) )
	{
		__fill_global_to_local(ret);
	}
	else
	{
		ret->global_to_local = NULL;
	}

	/* As we create we set refcounter to 1 */
	OPA_store_int(&ret->refcount, 1);

	/* Force compute of local leader */
	ret->local_leader = mpc_lowcomm_group_get_local_leader(ret);

	if(deduplicate)
	{
		return _mpc_lowcomm_group_list_register(ret);
	}

	return ret;
}

mpc_lowcomm_group_t * mpc_lowcomm_group_create(unsigned int size, int *comm_world_ranks)
{
	_mpc_lowcomm_group_rank_descriptor_t *cw_desc = sctk_malloc(sizeof(_mpc_lowcomm_group_rank_descriptor_t) * size);

	assume(cw_desc != NULL);

	unsigned int i;

	for(i = 0; i < size; i++)
	{
		if(_mpc_lowcomm_group_rank_descriptor_set(&cw_desc[i], comm_world_ranks[i]) != MPC_LOWCOMM_SUCCESS)
		{
			mpc_common_debug_fatal("An error occured when creating descriptor for RANK %d in COMM_WORLD", i);
		}
	}

	return _mpc_lowcomm_group_create(size, cw_desc, 1);
}

mpc_lowcomm_group_t *mpc_lowcomm_group_dup(mpc_lowcomm_group_t *g)
{
	if(g)
	{
		_mpc_lowcomm_group_acquire(g);
	}

	return g;
}

mpc_lowcomm_group_t *mpc_lowcomm_group_copy(mpc_lowcomm_group_t *g)
{
	mpc_lowcomm_group_t * ret = NULL;

	if(!g)
	{
		return g;
	}

	_mpc_lowcomm_group_rank_descriptor_t * rd = NULL;

	if(g->size)
	{
		rd = sctk_malloc(sizeof(_mpc_lowcomm_group_rank_descriptor_t) * g->size);
		assume(rd != NULL);
		memcpy(rd, g->ranks, sizeof(_mpc_lowcomm_group_rank_descriptor_t) * g->size);
	}

	ret = _mpc_lowcomm_group_create( g->size, rd, 0 /* No deduplication */);

	/* Flag as an independent copy */
	ret->is_a_copy = 1;

	return ret;
}

/*****************************
*  mpc_lowcomm_group_list_t *
*****************************/

typedef struct _mpc_lowcomm_group_list_s
{
	OPA_int_t                   group_id;
	struct mpc_common_hashtable id_to_grp;
	struct mpc_common_hashtable grp_to_id;
	struct mpc_common_hashtable ht;
	mpc_common_spinlock_t       lock;
}_mpc_lowcomm_group_list_t;


static _mpc_lowcomm_group_list_t __group_list = { 0 };


static inline void __group_list_init(void)
{
	mpc_common_spinlock_init(&__group_list.lock, 0);
	mpc_common_hashtable_init(&__group_list.ht, 1024);

	/* For Fortran IDs */
	mpc_common_hashtable_init(&__group_list.id_to_grp, 1024);
	OPA_store_int(&__group_list.group_id, 1);
}

static inline void __group_list_release(void)
{
	mpc_common_hashtable_release(&__group_list.ht);
	mpc_common_hashtable_release(&__group_list.id_to_grp);
}

mpc_lowcomm_group_t *mpc_lowcomm_group_from_id(int linear_id)
{
	uint64_t lid = linear_id;

	return (mpc_lowcomm_group_t *)mpc_common_hashtable_get(&__group_list.id_to_grp, lid);
}

int mpc_lowcomm_group_linear_id(mpc_lowcomm_group_t *group)
{
	return group->id;
}

static inline uint64_t __hash_group(mpc_lowcomm_group_t *group)
{
	uint64_t ret = 1337;

	if(!group)
	{
		return ret;
	}

	ret ^= group->size;

	unsigned int i;

	for(i = 0; i < group->size; i++)
	{
		int      off = (i * 4 % 32);
		uint32_t rdh = _mpc_lowcomm_group_rank_descriptor_hash(&group->ranks[i]);
		ret = ret ^ (rdh << off);
	}

	return ret;
}

static inline mpc_lowcomm_group_t *__group_get_and_compare_locked(mpc_lowcomm_group_t *group, uint64_t group_hash)
{
	mpc_lowcomm_group_t *ret = NULL;

	mpc_lowcomm_group_t *existing = (mpc_lowcomm_group_t *)mpc_common_hashtable_get(&__group_list.ht, group_hash);

	if(existing)
	{
		if(__group_compare(existing, group, 0) == MPC_GROUP_IDENT)
		{
			ret = existing;
		}
	}

	return ret;
}

mpc_lowcomm_group_t *_mpc_lowcomm_group_list_register(mpc_lowcomm_group_t *group)
{
	uint64_t group_hash = __hash_group(group);

	mpc_common_spinlock_lock_yield(&__group_list.lock);

	mpc_lowcomm_group_t *existing = __group_get_and_compare_locked(group, group_hash);

	if(!existing)
	{
		/* Insert */
		mpc_common_hashtable_set(&__group_list.ht, group_hash, (void *)group);

		/* Gen New ID and store in resolution HT */
		group->id = OPA_fetch_and_incr_int(&__group_list.group_id);
		uint64_t lid = group->id;
		mpc_common_hashtable_set(&__group_list.id_to_grp, lid, (void *)group);
	}
	else
	{
		/* Get ref to existing */
		_mpc_lowcomm_group_acquire(existing);
	}

	mpc_common_spinlock_unlock(&__group_list.lock);

	if(existing)
	{
		/* Relax the handle to previous group
		 * and hand back new handle */
		__internal_group_free(&group, 0);
	}
	else
	{
		existing = group;
	}

	return existing;
}

int _mpc_lowcomm_group_list_pop(mpc_lowcomm_group_t *group)
{
	int ret = 1;

	uint64_t group_hash = __hash_group(group);

	mpc_common_spinlock_lock_yield(&__group_list.lock);

	mpc_lowcomm_group_t *existing = __group_get_and_compare_locked(group, group_hash);

	if(existing)
	{
		/* Group is present and equal it has not been taken back yet */
		if(OPA_load_int(&existing->refcount) == 0)
		{
			mpc_common_hashtable_delete(&__group_list.ht, group_hash);
			mpc_common_hashtable_delete(&__group_list.id_to_grp, group->id);
			ret = 0;
		}
	}

	mpc_common_spinlock_unlock(&__group_list.lock);

	/* 0 if freed something 1 otherwise */
	return ret;
}

/****************************
 * CONTEXT POINTER HANDLING *
 ****************************/

mpc_lowcomm_handle_ctx_t mpc_lowcomm_group_get_context_pointer(mpc_lowcomm_group_t * g)
{
	if(!g)
	{
		return MPC_LOWCOMM_HANDLE_CTX_NULL;
	}

	return g->extra_ctx_ptr;
}


int mpc_lowcomm_group_set_context_pointer(mpc_lowcomm_group_t * g, mpc_lowcomm_handle_ctx_t ctxptr)
{
	if(!g)
	{
		return 1;
	}

	if(!g->is_a_copy)
	{
		mpc_common_debug_error("Cannot set a context on a non-copied group");
		return 1;
	}

	g->extra_ctx_ptr = ctxptr;

	return 0;
}

/*******************************
* CREATION OF STANDARD GROUPS *
*******************************/

static mpc_lowcomm_group_t *__world_group = NULL;
static mpc_lowcomm_group_t *__empty_group = NULL;


mpc_lowcomm_group_t *_mpc_lowcomm_group_world_no_assert(void)
{
	return __world_group;
}

mpc_lowcomm_group_t *_mpc_lowcomm_group_world(void)
{
	assume(__world_group != NULL);
	return _mpc_lowcomm_group_world_no_assert();
}

mpc_lowcomm_group_t *mpc_lowcomm_group_empty(void)
{
	assume(__empty_group != NULL);
	return __empty_group;
}

void _mpc_lowcomm_group_create_world(void)
{
	/* This creates the group list */
	__group_list_init();

	/* Create the empty group */
	__empty_group = _mpc_lowcomm_group_create(0, NULL, 1);
	__empty_group->do_not_free = 1;

	/* Comm world */
	int i;
	int size = mpc_lowcomm_get_size();

	_mpc_lowcomm_group_rank_descriptor_t *cw_desc = sctk_malloc(sizeof(_mpc_lowcomm_group_rank_descriptor_t) * size);

	assume(cw_desc != NULL);

	for(i = 0; i < size; i++)
	{
		if(_mpc_lowcomm_group_rank_descriptor_set(&cw_desc[i], i) != MPC_LOWCOMM_SUCCESS)
		{
			mpc_common_debug_fatal("An error occurred when creating descriptor for RANK %d in COMM_WORLD", i);
		}
	}

	assume(__world_group == NULL);
	__world_group = _mpc_lowcomm_group_create(size, cw_desc, 1);
	__world_group->do_not_free = 1;
}

void _mpc_lowcomm_group_release_world(void)
{
	mpc_lowcomm_group_free(&__world_group);
	__group_list_release();
}

int _mpc_lowcomm_communicator_world_first_local_task()
{
	assume(__world_group != NULL);
	return __world_group->local_leader;
}

int _mpc_lowcomm_communicator_world_last_local_task()
{
	assume(__world_group != NULL);
	return __world_group->local_leader + mpc_common_get_local_task_count() - 1;
}

static inline int __duplicate_ranks(int size, int *world_ranks)
{
	int i, j;

	for(i = 0; i < size; i++)
	{
		for(j = 0; j < size; j++)
		{
			if(j == i)
			{
				continue;
			}

			if(world_ranks[i] == world_ranks[j])
			{
				if(world_ranks[j] != MPC_PROC_NULL)
				{
					mpc_common_debug_fatal("Error: rank %d and %d both point to comm world rank %d", i, j, world_ranks[j]);
					return 1;
				}
			}
		}
	}

	return 0;
}

mpc_lowcomm_group_t *_mpc_lowcomm_group_create_from_world_ranks(int size, int *world_ranks)
{
	/* Sanity checks for no duplicate ranks */
	assert(!__duplicate_ranks(size, world_ranks) );

	_mpc_lowcomm_group_rank_descriptor_t *desc = sctk_malloc(sizeof(_mpc_lowcomm_group_rank_descriptor_t) * size);

	assume(desc != NULL);

	int i;

	for(i = 0; i < size; i++)
	{
		if(_mpc_lowcomm_group_rank_descriptor_set(&desc[i], world_ranks[i]) != MPC_LOWCOMM_SUCCESS)
		{
			mpc_common_debug_fatal("An error occured when creating descriptor for RANK %d", i);
		}
	}

	return _mpc_lowcomm_group_create(size, desc, 1);
}
