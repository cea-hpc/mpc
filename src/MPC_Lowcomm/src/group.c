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

#include "peer.h"

/*******************************
* COMMUNICATORS AND PROCESSES *
*******************************/

struct __rank_to_process_map
{
	int           size;
	unsigned int *process_id;
};

struct __rank_to_process_map __process_map = { 0 };


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

void __rank_to_process_map_init(void)
{
	assume(__process_map.process_id == 0);

	__process_map.size       = mpc_lowcomm_get_size();
	__process_map.process_id = sctk_malloc(__process_map.size * sizeof(unsigned int) );

	assume(__process_map.process_id != NULL);

	int i;

	for(i = 0; i < __process_map.size; i++)
	{
		__process_map.process_id[i] = ___process_rank_from_world_rank(i);
	}
}

void __rank_to_process_map_release(void)
{
	__process_map.size = 0;
	sctk_free(__process_map.process_id);
	__process_map.process_id = NULL;
}

int mpc_lowcomm_group_process_rank_from_world(int comm_world_rank)
{
	assert(__process_map.size != 0);
	assert(comm_world_rank < mpc_lowcomm_get_size() );

	return __process_map.process_id[comm_world_rank];
}

static inline void __fill_process_info(mpc_lowcomm_group_t *g)
{
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
	assert(__process_map.size != 0);

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
	assert(__process_map.size != 0);

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
	assert(__process_map.size != 0);

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

int mpc_lowcomm_group_includes(mpc_lowcomm_group_t *g, int cw_rank)
{
	if(cw_rank == MPC_PROC_NULL)
	{
		return 0;
	}

	return mpc_lowcomm_group_rank_for(g, cw_rank) != MPC_PROC_NULL;
}

/****************************************
* _MPC_LOWCOMM_GROUP_RANK_DESCRIPTOR_S *
****************************************/

int _mpc_lowcomm_group_rank_descriptor_equal(_mpc_lowcomm_group_rank_descriptor_t *a,
                                             _mpc_lowcomm_group_rank_descriptor_t *b)
{
	return memcmp( (void *)a, (void *)b, sizeof(_mpc_lowcomm_group_rank_descriptor_t) ) == 0;
}

int _mpc_lowcomm_group_rank_descriptor_set_in_grp(_mpc_lowcomm_group_rank_descriptor_t *rd,
											      mpc_lowcomm_set_uid_t set,
											      int cw_rank)
{
	if(!rd)
	{
		return SCTK_ERROR;
	}

	rd->comm_world_rank = cw_rank;
	rd->uid = mpc_lowcomm_monitor_get_uid_of(set, cw_rank);


	return SCTK_SUCCESS;
}

int _mpc_lowcomm_group_rank_descriptor_set(_mpc_lowcomm_group_rank_descriptor_t *rd, int cw_rank)
{
	return _mpc_lowcomm_group_rank_descriptor_set_in_grp(rd, mpc_lowcomm_monitor_get_gid(), cw_rank);
}




uint32_t _mpc_lowcomm_group_rank_descriptor_hash(_mpc_lowcomm_group_rank_descriptor_t *rd)
{
	return (uint32_t)rd->comm_world_rank;
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

	if(g1->size == g2->size)
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
	else
	{
		return MPC_GROUP_UNEQUAL;
	}

	if(!do_similar)
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

int mpc_lowcomm_group_rank_for(mpc_lowcomm_group_t *g, int rank)
{
	assert( (0 <= rank) && (rank < mpc_lowcomm_get_size() ) );

	if(rank == MPC_PROC_NULL)
	{
		return rank;
	}


	if(g->global_to_local)
	{
		//mpc_common_debug_error("RANK %d MAPS to %d", rank, g->global_to_local[rank]);
		return g->global_to_local[rank];
	}



	assume(g->ranks != NULL);

	unsigned int i;

	for(i = 0; i < g->size; i++)
	{
		//mpc_common_debug_error("[%d] %d == %d ?",i,  rank, g->ranks[i].comm_world_rank);

		if(g->ranks[i].comm_world_rank == rank)
		{
			return i;
		}
	}

	return MPC_PROC_NULL;
}

int mpc_lowcomm_group_rank(mpc_lowcomm_group_t *g)
{
	return mpc_lowcomm_group_rank_for(g, mpc_lowcomm_get_rank() );
}

int mpc_lowcomm_group_translate_ranks(mpc_lowcomm_group_t *g1, int n, const int ranks1[], mpc_lowcomm_group_t *g2, int ranks2[])
{
	int i;

	for(i = 0; i < n; i++)
	{
		unsigned int j;

		if(ranks1[n] == MPC_PROC_NULL)
		{
			ranks2[n] = MPC_PROC_NULL;
			continue;
		}

		if(g1->size <= (unsigned int)ranks1[n])
		{
			return SCTK_ERROR;
		}

		_mpc_lowcomm_group_rank_descriptor_t *rd = &g1->ranks[n];

		ranks2[n] = MPC_UNDEFINED;

		for(j = 0; j < g2->size; j++)
		{
			if(_mpc_lowcomm_group_rank_descriptor_equal(rd, &g2->ranks[j]) )
			{
				ranks2[n] = j;
				break;
			}
		}
	}

	return SCTK_SUCCESS;
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

mpc_lowcomm_group_t *_mpc_lowcomm_group_create(unsigned int size, _mpc_lowcomm_group_rank_descriptor_t *ranks)
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

#if 1
	__fill_global_to_local(ret);
#else
	ret->global_to_local = NULL;
#endif

	/* As we create we set refcounter to 1 */
	OPA_store_int(&ret->refcount, 1);

	/* Force compute of local leader */
	ret->local_leader = mpc_lowcomm_group_get_local_leader(ret);

	return _mpc_lowcomm_group_list_register(ret);
}

mpc_lowcomm_group_t *mpc_lowcomm_group_create(unsigned int size, int *comm_world_ranks)
{
	_mpc_lowcomm_group_rank_descriptor_t *cw_desc = sctk_malloc(sizeof(_mpc_lowcomm_group_rank_descriptor_t) * size);

	assume(cw_desc != NULL);

	unsigned int i;

	for(i = 0; i < size; i++)
	{
		if(_mpc_lowcomm_group_rank_descriptor_set(&cw_desc[i], comm_world_ranks[i]) != SCTK_SUCCESS)
		{
			mpc_common_debug_fatal("An error occured when creating descriptor for RANK %d in COMM_WORLD", i);
		}
	}

	return _mpc_lowcomm_group_create(size, cw_desc);
}

mpc_lowcomm_group_t *mpc_lowcomm_group_dup(mpc_lowcomm_group_t *g)
{
	if(g)
	{
		_mpc_lowcomm_group_acquire(g);
	}

	return g;
}

/*****************************
*  mpc_lowcomm_group_list_t *
*****************************/

typedef struct _mpc_lowcomm_group_list_s
{
	struct mpc_common_hashtable ht;
	mpc_common_spinlock_t       lock;
}_mpc_lowcomm_group_list_t;


static _mpc_lowcomm_group_list_t __group_list = { 0 };


static inline void __group_list_init(void)
{
	mpc_common_spinlock_init(&__group_list.lock, 0);
	mpc_common_hashtable_init(&__group_list.ht, 1024);
}

static inline void __group_list_release(void)
{
	mpc_common_hashtable_release(&__group_list.ht);
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
		int      off = (i*4 % 32);
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
		}
	}

	mpc_common_spinlock_unlock(&__group_list.lock);

	/* 0 if freed something 1 otherwise */
	return ret;
}

/*******************************
* CREATION OF STANDARD GROUPS *
*******************************/

static mpc_lowcomm_group_t *__world_group = NULL;

mpc_lowcomm_group_t *_mpc_lowcomm_group_world(void)
{
	assume(__world_group != NULL);
	return __world_group;
}

void _mpc_lowcomm_group_create_world(void)
{
	/* This creates the group list */
	__group_list_init();

	/* This map holds world rank to process translations */
	__rank_to_process_map_init();


	/* Comm world */
	int i;
	int size = mpc_lowcomm_get_size();

	_mpc_lowcomm_group_rank_descriptor_t *cw_desc = sctk_malloc(sizeof(_mpc_lowcomm_group_rank_descriptor_t) * size);

	assume(cw_desc != NULL);

	for(i = 0; i < size; i++)
	{
		if(_mpc_lowcomm_group_rank_descriptor_set(&cw_desc[i], i) != SCTK_SUCCESS)
		{
			mpc_common_debug_fatal("An error occured when creating descriptor for RANK %d in COMM_WORLD", i);
		}
	}

	assume(__world_group == NULL);
	__world_group = _mpc_lowcomm_group_create(size, cw_desc);
	__world_group->do_not_free = 1;
}

void _mpc_lowcomm_group_release_world(void)
{
	mpc_lowcomm_group_free(&__world_group);
	__group_list_release();
	__rank_to_process_map_release();
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
		if(_mpc_lowcomm_group_rank_descriptor_set(&desc[i], world_ranks[i]) != SCTK_SUCCESS)
		{
			mpc_common_debug_fatal("An error occured when creating descriptor for RANK %d", i);
		}
	}

	return _mpc_lowcomm_group_create(size, desc);
}
