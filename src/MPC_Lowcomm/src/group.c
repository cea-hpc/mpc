#include "group.h"

#include <mpc_lowcomm.h>
#include <string.h>

#include <sctk_alloc.h>
#include <mpc_common_debug.h>
#include <mpc_common_rank.h>

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

    for(i = 0; i  < __process_map.size; i++)
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
    assert(0 <= comm_world_rank);
    assert(comm_world_rank < mpc_lowcomm_get_size());

    return __process_map.process_id[comm_world_rank];
}

static inline void __fill_process_info(mpc_lowcomm_group_t * g)
{
 	unsigned int process_count = (unsigned int)mpc_common_get_process_count();
    unsigned int *counter = sctk_malloc(process_count * sizeof(unsigned int));
    assume(counter != NULL); 

    memset(counter, 0, process_count * sizeof(unsigned int));

    unsigned int i;

    for(i = 0 ; i < g->size; i++)
    {
        int cw_rank = mpc_lowcomm_group_world_rank(g, i);
        int p_rank = mpc_lowcomm_group_process_rank_from_world(cw_rank);

        assert(0 <= p_rank);
        assert(p_rank < (int)process_count);

        counter[p_rank]++;
    }

	/* This is the process count */

    int group_process_count = 0;

    for(i = 0 ; i < process_count; i++)
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
	for(i = 0 ; i < process_count; i++)
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

int _mpc_lowcomm_group_local_process_count(mpc_lowcomm_group_t * g)
{
    assert(__process_map.size != 0);

	mpc_common_spinlock_lock(&g->process_lock);
	/* Already computed */
	if(0 < g->tasks_count_in_process )
	{
		mpc_common_spinlock_unlock(&g->process_lock);
		return g->tasks_count_in_process;
	}

	__fill_process_info(g);

	assume(0 < g->tasks_count_in_process);

	mpc_common_spinlock_unlock(&g->process_lock);

    return g->tasks_count_in_process;
}

int _mpc_lowcomm_group_process_count(mpc_lowcomm_group_t * g)
{
    assert(__process_map.size != 0);

	mpc_common_spinlock_lock(&g->process_lock);
	/* Already computed */
	if(0 < g->process_count )
	{
		mpc_common_spinlock_unlock(&g->process_lock);
		return g->process_count;
	}

	__fill_process_info(g);

	assume(0 < g->process_count);

	mpc_common_spinlock_unlock(&g->process_lock);

    return g->process_count;
}

int * _mpc_lowcomm_group_process_list(mpc_lowcomm_group_t * g)
{
    assert(__process_map.size != 0);

	mpc_common_spinlock_lock(&g->process_lock);
	/* Already computed */
	if( g->process_list != NULL )
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
	if( g->local_leader != MPC_PROC_NULL)
	{
		return g->local_leader;
	}

	g->local_leader = MPC_PROC_NULL;

	/* We need to compute the leader */
	unsigned int i;
	int my_proc_rank = mpc_common_get_process_rank();
	
	for(i = 0 ; i < g->size; i++)
	{
		if(my_proc_rank == mpc_lowcomm_group_process_rank_from_world(g->ranks[i].comm_world_rank))
		{
			g->local_leader = i;
			break;
		}
	}

	return g->local_leader;
}


int mpc_lowcomm_group_includes(mpc_lowcomm_group_t *g, int cw_rank)
{
	if( cw_rank == MPC_PROC_NULL)
	{
		return 0;
	}

	return (mpc_lowcomm_group_rank_for(g, cw_rank) != MPC_PROC_NULL);
}

/****************************************
* _MPC_LOWCOMM_GROUP_RANK_DESCRIPTOR_S *
****************************************/

int _mpc_lowcomm_group_rank_descriptor_equal(_mpc_lowcomm_group_rank_descriptor_t *a,
                                             _mpc_lowcomm_group_rank_descriptor_t *b)
{
	return memcmp( (void *)a, (void *)b, sizeof(_mpc_lowcomm_group_rank_descriptor_t) ) == 0;
}

int _mpc_lowcomm_group_rank_descriptor_set(_mpc_lowcomm_group_rank_descriptor_t *rd, int comm_world_rank)
{
    if(!rd)
    {
        return SCTK_ERROR;
    }

    rd->comm_world_rank = comm_world_rank;

    return SCTK_SUCCESS;
}

/************************
* _MPC_LOWCOMM_GROUP_T *
************************/

int mpc_lowcomm_group_free(mpc_lowcomm_group_t **group)
{
	if(!group)
	{
		return 1;
	}

	mpc_lowcomm_group_t *g = *group;

	int val = OPA_fetch_and_decr_int(&(g->refcount) );

	if(val == 0)
	{
		/* First pop the group */
		int ret = _mpc_lowcomm_group_list_pop(g);

		if(ret == 0)
		{
			/* Was not present or had not pending ref do free */
			sctk_free(g->ranks);
			g->ranks = NULL;
            sctk_free(g->global_to_local);
            g->global_to_local = NULL;
			g->size  = 0;
			g->process_count = 0;
			sctk_free(g->process_list);
			g->process_list = NULL;
			g->local_leader = MPC_PROC_NULL;
			g->tasks_count_in_process = MPC_PROC_NULL;
		}
	}

	*group = NULL;

	return 0;
}

mpc_lowcomm_group_eq_e mpc_lowcomm_group_compare(mpc_lowcomm_group_t *g1, mpc_lowcomm_group_t *g2)
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
	}

	/* Did we pass ? */
	if(ret != MPC_GROUP_IDENT)
	{
		return ret;
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
			if(_mpc_lowcomm_group_rank_descriptor_equal(rd, &g2->ranks[i]) )
			{
				found = &g2->ranks[i];
			}
		}

		if(!found)
		{
			ret = MPC_GROUP_UNEQUAL;
		}
	}

	return ret;
}

unsigned int mpc_lowcomm_group_size(mpc_lowcomm_group_t *g)
{
	assert(g != NULL);
	return g->size;
}

_mpc_lowcomm_group_rank_descriptor_t * mpc_lowcomm_group_descriptor(mpc_lowcomm_group_t *g, int rank)
{
	assert(g != NULL);
	assert(0 <= rank);
	assert(rank < (int)g->size);
	return &g->ranks[rank];
}

int mpc_lowcomm_group_world_rank(mpc_lowcomm_group_t *g, int rank)
{
	_mpc_lowcomm_group_rank_descriptor_t * desc = mpc_lowcomm_group_descriptor(g, rank);
	return desc->comm_world_rank;
}


int mpc_lowcomm_group_rank_for(mpc_lowcomm_group_t *g, int rank)
{
    assert( (0 <= rank) && (rank < mpc_lowcomm_get_size()) );

    if(rank == MPC_PROC_NULL)
    {
        return rank;
    }

    if(0 && g->global_to_local)
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
    return mpc_lowcomm_group_rank_for(g, mpc_lowcomm_get_rank());
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

mpc_lowcomm_group_t *_mpc_lowcomm_group_create(unsigned int size, _mpc_lowcomm_group_rank_descriptor_t *ranks)
{
	mpc_lowcomm_group_t *ret = sctk_malloc(sizeof(mpc_lowcomm_group_t) );

	assume(ret != NULL);

	memset(ret, 0, sizeof(mpc_lowcomm_group_t) );

	ret->ranks = ranks;
	ret->size  = size;

	/* We do lazy evaluation here */
	mpc_common_spinlock_init(&ret->process_lock, 0);
	ret->process_count = MPC_PROC_NULL;
	ret->process_list = NULL;
	ret->tasks_count_in_process = MPC_PROC_NULL;
	ret->local_leader = MPC_PROC_NULL;

#if 1
    if(ret->ranks)
    {
        /* Create the global to local list to speedup rank translation */
        int cw_size = mpc_lowcomm_get_size();

        ret->global_to_local = sctk_malloc(sizeof(int) * cw_size);
        assume(ret->global_to_local != NULL);

        int j;
        for(j = 0 ; j < cw_size; j++)
        {
            ret->global_to_local[j] = MPC_PROC_NULL;
        }

        unsigned int i;
        for( i = 0 ; i < size; i++)
        {
            int trank = ret->ranks[i].comm_world_rank;
            assume(trank < cw_size);
            assume(0 <= trank);
            ret->global_to_local[trank] = i; 
        }
    }
#else
    ret->global_to_local = NULL;
#endif

	/* As we create we set refcounter to 1 */
	OPA_store_int(&ret->refcount, 1);

	/* Force compute of local leader */
	ret->local_leader = mpc_lowcomm_group_get_local_leader(ret);

	return _mpc_lowcomm_group_list_register(ret);
}

mpc_lowcomm_group_t * mpc_lowcomm_group_create(unsigned int size, int *comm_world_ranks)
{
    _mpc_lowcomm_group_rank_descriptor_t * cw_desc = sctk_malloc(sizeof(_mpc_lowcomm_group_rank_descriptor_t) * size);

    assume(cw_desc != NULL);

	int i;

    for(i = 0 ; i < size; i++)
    {
        if(_mpc_lowcomm_group_rank_descriptor_set(&cw_desc[i], comm_world_ranks[i]) != SCTK_SUCCESS)
        {
            mpc_common_debug_fatal("An error occured when creating descriptor for RANK %d in COMM_WORLD", i);
        }
    }

	return _mpc_lowcomm_group_create(size, cw_desc);
}


mpc_lowcomm_group_t * mpc_lowcomm_group_dup(mpc_lowcomm_group_t * g)
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


static _mpc_lowcomm_group_list_t __group_list = { 0 };


static inline struct _mpc_lowcomm_group_s *__group_is_in_list_no_lock(struct _mpc_lowcomm_group_s *group)
{
	struct _mpc_lowcomm_group_s *ret = NULL;

	struct _mpc_lowcomm_group_entry_s *cur = __group_list.entries;

	while(cur)
	{
		if(mpc_lowcomm_group_compare(cur->group, group) == MPC_GROUP_IDENT)
		{
			/* Matching group is present */
			ret = cur->group;
			break;
		}

		cur = cur->next;
	}

	return ret;
}

mpc_lowcomm_group_t *_mpc_lowcomm_group_list_register(mpc_lowcomm_group_t *group)
{
	mpc_lowcomm_group_t *ret = group;

	mpc_common_spinlock_lock(&__group_list.lock);

	struct _mpc_lowcomm_group_s *exists = __group_is_in_list_no_lock(group);

	if(exists)
	{
		/* Relax the handle to previous group
		 * and hand back new handle */
		mpc_lowcomm_group_free(&group);
		ret = exists;
		_mpc_lowcomm_group_acquire(ret);
	}
	else
	{
		struct _mpc_lowcomm_group_entry_s *new = sctk_malloc(sizeof(struct _mpc_lowcomm_group_entry_s) );
		assume(new != NULL);

		new->next            = __group_list.entries;
		new->group           = group;
		__group_list.entries = new;
	}

	mpc_common_spinlock_unlock(&__group_list.lock);

	return ret;
}

int _mpc_lowcomm_group_list_pop(mpc_lowcomm_group_t *group)
{
	int ret = 0;

	mpc_common_spinlock_lock(&__group_list.lock);

	struct _mpc_lowcomm_group_entry_s *cur = __group_list.entries;

	/* Handle head */
	if(cur)
	{
		if(cur->group == group)
		{
			/* Only pop if not refcounted */
			if(OPA_load_int(&cur->group->refcount) == 0)
			{
				__group_list.entries = cur->next;
				sctk_free(cur);
			}
			else
			{
				mpc_common_debug_warning("Tried to remove a communicator with pending references");
				ret = 1;
			}
		}
		else
		{
			/* Walk the list one ahead */
			while(cur->next)
			{
				if(cur->next->group == group)
				{
					if(OPA_load_int(&cur->group->refcount) == 0)
					{
						cur->next = cur->next->next;
						free(cur->next);
					}
					else
					{
						mpc_common_debug_warning("Tried to remove a communicator with pending references");
						ret = 1;
					}
					break;
				}

				cur = cur->next;
			}
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

mpc_lowcomm_group_t * _mpc_lowcomm_group_world(void)
{
	assume(__world_group != NULL);
	return __world_group;
}

void _mpc_lowcomm_group_create_world(void)
{
    /* This map holds world rank to process translations */
    __rank_to_process_map_init();


    /* Comm world */
    int i;
    int size = mpc_lowcomm_get_size();

    _mpc_lowcomm_group_rank_descriptor_t * cw_desc = sctk_malloc(sizeof(_mpc_lowcomm_group_rank_descriptor_t) * size);

    assume(cw_desc != NULL);

    for(i = 0 ; i < size; i++)
    {
        if(_mpc_lowcomm_group_rank_descriptor_set(&cw_desc[i], i) != SCTK_SUCCESS)
        {
            mpc_common_debug_fatal("An error occured when creating descriptor for RANK %d in COMM_WORLD", i);
        }
    }

    assume(__world_group == NULL);
    __world_group = _mpc_lowcomm_group_create(size, cw_desc);
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

static inline int __duplicate_ranks(int size, int * world_ranks)
{
	int i, j;
	for(i = 0 ; i < size; i++)
	{
		for(j = 0 ; j < size; j++)
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

mpc_lowcomm_group_t * _mpc_lowcomm_group_create_from_world_ranks(int size, int * world_ranks)
{
	/* Sanity checks for no duplicate ranks */
	assert(!__duplicate_ranks(size, world_ranks));

	_mpc_lowcomm_group_rank_descriptor_t * desc = sctk_malloc(sizeof(_mpc_lowcomm_group_rank_descriptor_t) * size);

    assume(desc != NULL);

	int i;

    for(i = 0 ; i < size; i++)
    {
        if(_mpc_lowcomm_group_rank_descriptor_set(&desc[i], world_ranks[i]) != SCTK_SUCCESS)
        {
            mpc_common_debug_fatal("An error occured when creating descriptor for RANK %d", i);
        }
    }	

	return  _mpc_lowcomm_group_create(size, desc);
}