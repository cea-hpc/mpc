#include "group.h"

#include <mpc_lowcomm.h>
#include <string.h>

#include <sctk_alloc.h>
#include <mpc_common_debug.h>

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
	return g->size;
}

int mpc_lowcomm_group_rank_for(mpc_lowcomm_group_t *g, int rank)
{
    assert(rank < (int)g->size);

    if(rank == MPC_PROC_NULL)
    {
        return rank;
    }

    if(g->global_to_local)
    {
        return g->global_to_local[rank];
    }



    assume(g->ranks != NULL);

	unsigned int i;

	for(i = 0; i < g->size; i++)
	{
		if(g->ranks[i].comm_world_rank == rank)
		{
			return i;
		}
	}

	return MPC_GROUP_UNDEFINED;
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

		ranks2[n] = MPC_GROUP_UNDEFINED;

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

	return _mpc_lowcomm_group_list_register(ret);
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

void _mpc_lowcomm_group_create_world(void)
{
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

    mpc_common_debug_warning("HELLO W GROUP");
}