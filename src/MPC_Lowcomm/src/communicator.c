#include "communicator.h"
#include "group.h"

#include "coll.h"

#include <mpc_config.h>

#include <sctk_alloc.h>
#include <mpc_common_debug.h>
#include <mpc_common_rank.h>

#include <mpc_common_datastructure.h>

/*************************
 * COMMUNICATOR PRINTING *
 *************************/
void mpc_lowcomm_communicator_print(mpc_lowcomm_communicator_t comm, int root_only)
{
	if(!comm)
	{
		mpc_common_debug_error("COMM Is NULL");
		return;
	}

	int is_lead = (mpc_lowcomm_communicator_rank(comm) == 0);

	if(!is_lead && root_only)
	{
		return;
	}

	mpc_common_debug_error("========COMM %u @ %p=======", comm->id, comm);
	
	mpc_common_debug_error("TYPE: %s", (comm->group || comm->is_comm_self)?"intracomm":"intercomm");
	
	if(mpc_lowcomm_communicator_is_intercomm(comm))
	{
		mpc_common_debug_error("LEFT");
		mpc_lowcomm_communicator_print(comm->left_comm, 0);

		mpc_common_debug_error("RIGHT");
		mpc_lowcomm_communicator_print(comm->right_comm, 0);
	}
	else
	{
		int size = mpc_lowcomm_communicator_size(comm);
		mpc_common_debug_error("SIZE: %u", size);

		mpc_common_debug_error("--- RANK LIST ---");

		int i;
		for(i = 0 ; i < size; i++)
		{
			mpc_common_debug_error("COMM %d is CW %d", i, mpc_lowcomm_communicator_world_rank(comm, i));
		}
		mpc_common_debug_error("-----------------");

	}


	mpc_common_debug_error("==========================");
	
}


/****************************
* COMMUNICATOR REFCOUNTING *
****************************/

void _mpc_lowcomm_communicator_acquire(mpc_lowcomm_internal_communicator_t *comm)
{
	assume(comm != NULL);
	OPA_incr_int(&comm->refcount);
}

int _mpc_lowcomm_communicator_relax(mpc_lowcomm_internal_communicator_t *comm)
{
	assume(comm != NULL);
	return OPA_fetch_and_decr_int(&comm->refcount);
}

/*********************************
* COLL GETTERS FOR COMMUNICATOR *
*********************************/


int mpc_lowcomm_communicator_is_shared_mem(const mpc_lowcomm_communicator_t comm)
{
	return comm->shm_coll != NULL;
}

struct sctk_comm_coll *mpc_communicator_shm_coll_get(const mpc_lowcomm_communicator_t comm)
{
	return comm->shm_coll;
}

int mpc_lowcomm_communicator_is_shared_node(const mpc_lowcomm_communicator_t comm)
{
	return 0;
}

/***************************
* BASIC COMMUNICATOR INIT *
***************************/

static inline void __communicator_id_register(mpc_lowcomm_communicator_t comm);

static inline mpc_lowcomm_internal_communicator_t *__init_communicator_with_id(unsigned int comm_id,
                                                                               mpc_lowcomm_group_t *group)
{
	mpc_lowcomm_internal_communicator_t *ret = sctk_malloc(sizeof(mpc_lowcomm_internal_communicator_t) );

	assume(ret != NULL);

	ret->id           = comm_id;
	ret->group        = group;
	ret->is_comm_self = 0;
	ret->shm_coll     = NULL;

	/* Intercomm ctx */
	ret->left_comm  = MPC_COMM_NULL;
	ret->right_comm = MPC_COMM_NULL;

	if(comm_id == MPC_LOWCOMM_COMM_SELF_ID)
	{
		/* Comm self is practically
		 * a virtual communicator without internal
		 * group to handle as it is different
		 * for each MPI Process */
		assume(group == NULL);
		ret->is_comm_self = 1;
	}
	else if(group != NULL)
	{
		/* This is an intracomm */
		assume(group != NULL);

		/* Increase group refcount by 1 as the comm
		 * now holds a reference to it */
		_mpc_lowcomm_group_acquire(group);

		ret->process_span = _mpc_lowcomm_group_process_count(group);

		/* SET SHM Collective context if needed */
		if(ret->process_span == 1)
		{
			ret->shm_coll = NULL;//sctk_comm_coll_init(group->size);
		}
	}

	/* The comm is initialized with one reference to it */
	OPA_store_int(&ret->refcount, 1);

	__communicator_id_register(ret);

	/* Initialize coll for this comm */
	mpc_lowcomm_coll_init_hook(ret);

	return ret;
}

/**************************
* COMMUNICATOR NUMBERING *
**************************/
static inline int __is_comm_global_lead(mpc_lowcomm_communicator_t comm, int lead_rank)
{
	assert(lead_rank < mpc_lowcomm_communicator_size(comm) );
	int my_rank = mpc_lowcomm_communicator_rank(comm);
	return my_rank == lead_rank;
}

static inline int __get_comm_world_lead(mpc_lowcomm_communicator_t comm)
{
	int group_local_lead = mpc_lowcomm_group_get_local_leader(comm->group);

	return mpc_lowcomm_communicator_world_rank(comm, group_local_lead);
}

struct __communicator_id_factory
{
	unsigned int                first_local;
	unsigned int                local_count;
	unsigned int                local_used;
	mpc_common_spinlock_t       lock;
	struct mpc_common_hashtable id_table;
	struct mpc_common_hashtable comm_table;
};

struct __communicator_id_factory __id_factory = { 0 };

#define COMM_ID_TO_SKIP    5

static inline void __communicator_id_factory_init(void)
{
	int process_count = mpc_common_get_process_count();

	/* We have the whole range except two */
	unsigned int global_dynamic = ( ( (unsigned int )-1) - COMM_ID_TO_SKIP);

	/* Ensure it can be divided evenly */
	if(global_dynamic % process_count)
	{
		global_dynamic -= (global_dynamic % process_count);
	}

	__id_factory.local_count = global_dynamic / process_count;

	int process_rank = mpc_common_get_process_rank();
	__id_factory.first_local = COMM_ID_TO_SKIP + process_rank * __id_factory.local_count;
	__id_factory.local_used  = 0;

	mpc_common_spinlock_init(&__id_factory.lock, 0);

	mpc_common_hashtable_init(&__id_factory.id_table, 4096);
	mpc_common_hashtable_init(&__id_factory.comm_table, 4096);
}

static inline void __communicator_id_register(mpc_lowcomm_communicator_t comm)
{
	uint64_t key = comm->id;

	mpc_common_hashtable_set(&__id_factory.id_table, key, (void *)comm);
	mpc_common_hashtable_set(&__id_factory.comm_table, (uint64_t)comm, (void *)comm);
	mpc_common_nodebug("New reg comm %d", key);
}

static inline void __communicator_id_release(mpc_lowcomm_communicator_t comm)
{
	uint64_t key = comm->id;

	mpc_common_hashtable_delete(&__id_factory.id_table, key);
	mpc_common_hashtable_delete(&__id_factory.comm_table, (uint64_t)comm);
}

mpc_lowcomm_communicator_t _mpc_lowcomm_get_communicator_from_id(uint32_t id)
{
	mpc_lowcomm_communicator_t ret = NULL;

	ret = (mpc_lowcomm_communicator_t)mpc_common_hashtable_get(&__id_factory.id_table, id);
	mpc_common_nodebug("GET comm %d == %p", id, ret);
	return ret;
}

int mpc_lowcomm_communicator_exists(mpc_lowcomm_communicator_t comm)
{
	return mpc_common_hashtable_get(&__id_factory.comm_table, (uint64_t)comm) != NULL;
}

static inline unsigned int  __communicator_id_new(void)
{
	unsigned int ret = 0;

	mpc_common_spinlock_lock_yield(&__id_factory.lock);

	if(__id_factory.local_used == __id_factory.local_count)
	{
		mpc_common_debug_fatal("You exhausted communicator id dynamic (up to %d) communicators on ranl %d", __id_factory.local_count, mpc_common_get_task_rank() );
	}

	ret = __id_factory.first_local + __id_factory.local_used;
	__id_factory.local_used++;

	mpc_common_spinlock_unlock(&__id_factory.lock);

	return ret;
}

static inline uint32_t __communicator_id_new_for_comm_intercomm(mpc_lowcomm_communicator_t intercomm)
{
	uint32_t new_id = 0;

	int rank = mpc_lowcomm_communicator_rank(intercomm);

	if( rank == 0)
	{
		/* Leaders pre exchange the ID */
		if(mpc_lowcomm_communicator_in_master_group(intercomm))
		{
			/* In the master group we set the id */
			new_id =  __communicator_id_new();
			mpc_lowcomm_send(0, &new_id, sizeof(uint32_t), 123,  intercomm);

		}
		else
		{
			mpc_lowcomm_recv(0, &new_id, sizeof(uint32_t), 123,  intercomm);
		}
		assume(new_id != 0);
	}

	/* Then all do bcast local */
	mpc_lowcomm_communicator_t local_comm = mpc_lowcomm_communicator_get_local(intercomm);
	mpc_lowcomm_bcast(&new_id, sizeof(uint32_t), 0, local_comm);

	return new_id;
}



static inline uint32_t  __communicator_id_new_for_comm(mpc_lowcomm_communicator_t comm)
{
	if(mpc_lowcomm_communicator_is_intercomm(comm))
	{
		return __communicator_id_new_for_comm_intercomm(comm);
	}

	/* Here we pick a random leader to spread
	 * communicator numbering pressure */
	int lead = comm->id % mpc_lowcomm_communicator_size(comm);

	uint32_t comm_id = 0;

	if(__is_comm_global_lead(comm, lead) )
	{
		comm_id = __communicator_id_new();
	}

	mpc_lowcomm_bcast( (void *)&comm_id, sizeof(uint32_t), lead, comm);

	assume(comm_id != 0);

	return comm_id;
}

uint32_t mpc_lowcomm_communicator_id(mpc_lowcomm_communicator_t comm)
{
	if(comm != MPC_COMM_NULL)
	{
		return comm->id;
	}

	return MPC_LOWCOMM_COMM_NULL_ID;
}


/*****************************
* COMMUNICATOR CONSTRUCTORS *
*****************************/


int mpc_lowcomm_communicator_free(mpc_lowcomm_communicator_t *pcomm)
{
	if(!pcomm)
	{
		/* Already freed */
		return SCTK_SUCCESS;
	}

	mpc_lowcomm_communicator_t comm = *pcomm;

	mpc_lowcomm_non_shm_barrier(comm);

	int refcount = _mpc_lowcomm_communicator_relax(comm);

	assume(0 <= refcount);

	if(refcount == 1)
	{
		/* It is now time to free this comm */
		__communicator_id_release(comm);
	}

	*pcomm = MPC_COMM_NULL;

	return SCTK_SUCCESS;
}

#define LOCAL_COMM_ID_CELLS    1024

static inline mpc_lowcomm_communicator_t __new_communicator(mpc_lowcomm_communicator_t comm,
                                                            mpc_lowcomm_group_t *group,
                                                            mpc_lowcomm_communicator_t left_comm,
                                                            mpc_lowcomm_communicator_t right_comm,
                                                            int is_comm_self,
                                                            int check_if_current_rank_belongs,
                                                            uint32_t forced_id)
{
	/* Only the group leader should allocate and register the others should wait */
	static mpc_common_spinlock_t lock = { 0 };
	static mpc_lowcomm_internal_communicator_t *new_comm[LOCAL_COMM_ID_CELLS] = { 0 };

	assume(mpc_common_get_local_task_count() <= LOCAL_COMM_ID_CELLS);

	mpc_lowcomm_internal_communicator_t *ret = NULL;

	unsigned int new_id = forced_id;

	mpc_common_debug_warning("New ID generation asked for on %u", comm->id);

	/* We need a new context ID */
	if(new_id == MPC_LOWCOMM_COMM_NULL_ID)
	{
		new_id = __communicator_id_new_for_comm(comm);
	}

	int comm_local_lead    = mpc_lowcomm_communicator_local_lead(comm);
	int comm_local_mailbox = mpc_lowcomm_communicator_world_rank(comm, comm_local_lead) % LOCAL_COMM_ID_CELLS;
	int my_rank            = mpc_lowcomm_communicator_rank(comm);

	if(group == NULL)
	{
        if(!is_comm_self)
        {
		    /* Intercomm or comm self */
		    assume(left_comm && right_comm);
            assume(left_comm != right_comm);
        }
	}
	else
	{
		/* Intracomm */
		assume(!left_comm && !right_comm);
	}

	if(comm_local_lead == my_rank)
	{
		mpc_common_spinlock_lock_yield(&lock);
		/* We can then directly create a new group using the current group */

		new_comm[comm_local_mailbox] = __init_communicator_with_id(new_id, group);

		ret = new_comm[comm_local_mailbox];
		/* Make sure that dups of comm self behave as comm self */
		ret->is_comm_self = is_comm_self;

		/* Intercommm ctx we acquire a ref to inner comms */

		ret->left_comm = left_comm;

		if(ret->left_comm != MPC_COMM_NULL)
		{
			_mpc_lowcomm_communicator_acquire(ret->left_comm);
		}

		ret->right_comm = right_comm;

		if(ret->right_comm != MPC_COMM_NULL)
		{
			_mpc_lowcomm_communicator_acquire(ret->right_comm);
		}

		mpc_common_spinlock_unlock(&lock);
	}

	/* Do a barrier when done to ensure new comm is posted */
	mpc_lowcomm_barrier(comm);

	if(comm_local_lead != my_rank)
	{
		assume(new_comm[comm_local_mailbox] != NULL);
		assume(new_comm[comm_local_mailbox]->id == new_id);
		ret = new_comm[comm_local_mailbox];
		/* Acquire the comm handle */
		_mpc_lowcomm_communicator_acquire(ret);
	}

	/* Do a barrier when done to ensure dup do not interleave */
	mpc_lowcomm_barrier(comm);

	if(ret->group && check_if_current_rank_belongs)
	{
		/* If the process is not in the group we do return MPI_COMM_NULL */
		if(!mpc_lowcomm_group_includes(ret->group, mpc_lowcomm_get_rank() ) )
		{
			ret = MPC_COMM_NULL;
		}
	}

	return ret;
}

mpc_lowcomm_communicator_t mpc_lowcomm_communicator_from_group(mpc_lowcomm_communicator_t comm,
                                                               mpc_lowcomm_group_t *group)
{
	assume(!mpc_lowcomm_communicator_is_intercomm(comm));
	/* This is necessarily an intracomm */
	return __new_communicator(comm, group, NULL, NULL, comm->is_comm_self, 1, MPC_LOWCOMM_COMM_NULL_ID);
}

mpc_lowcomm_communicator_t mpc_lowcomm_communicator_dup(mpc_lowcomm_communicator_t comm)
{
	return __new_communicator(comm, comm->group, comm->left_comm, comm->right_comm, comm->is_comm_self, 0, MPC_LOWCOMM_COMM_NULL_ID);
}

mpc_lowcomm_communicator_t mpc_lowcomm_communicator_create(const mpc_lowcomm_communicator_t comm,
                                                           const int size,
                                                           const int *members)
{
	/* Time to create a new group */
	_mpc_lowcomm_group_rank_descriptor_t *rank_desc = sctk_malloc(size * sizeof(_mpc_lowcomm_group_rank_descriptor_t) );

	int i;

	if(!comm->is_comm_self)
	{
		for(i = 0; i < size; i++)
		{
			mpc_lowcomm_communicator_t local = mpc_lowcomm_communicator_get_local(comm);
			assume(mpc_lowcomm_group_includes(local->group, members[i]) );
			_mpc_lowcomm_group_rank_descriptor_set(&rank_desc[i], members[i] );
		}
	}
	else
	{
		assume(members[i] == mpc_lowcomm_get_rank());
		_mpc_lowcomm_group_rank_descriptor_set(&rank_desc[i], mpc_lowcomm_get_rank() );
	}

	mpc_lowcomm_group_t *new = _mpc_lowcomm_group_create(size, rank_desc);
	return mpc_lowcomm_communicator_from_group(comm, new);
}

/************************
* COMMON COMMUNICATORS *
************************/

static struct mpc_lowcomm_internal_communicator_s *__comm_world = NULL;
static struct mpc_lowcomm_internal_communicator_s *__comm_self  = NULL;

mpc_lowcomm_communicator_t mpc_lowcomm_communicator_world()
{
	assume(__comm_world != NULL);
	return __comm_world;
}

mpc_lowcomm_communicator_t mpc_lowcomm_communicator_self()
{
	assume(__comm_self != NULL);
	return __comm_self;
}

void _mpc_lowcomm_communicator_init(void)
{
	__communicator_id_factory_init();
	/* This creates the world group */
	_mpc_lowcomm_group_create_world();
	mpc_lowcomm_group_t *cw_group = _mpc_lowcomm_group_world();

	assume(__comm_world == NULL);
	__comm_world = __init_communicator_with_id(MPC_LOWCOMM_COMM_WORLD_ID, cw_group);

	assume(__comm_self == NULL);
	__comm_self = __init_communicator_with_id(MPC_LOWCOMM_COMM_SELF_ID, NULL);
}

/*************
* ACCESSORS *
*************/

int mpc_lowcomm_communicator_local_task_count(mpc_lowcomm_communicator_t comm)
{
	assert(comm != NULL);


	if(comm->is_comm_self)
	{
		return 1;
	}

	if(comm->process_span == 1)
	{
		return mpc_lowcomm_communicator_size(comm);
	}

	return _mpc_lowcomm_group_local_process_count(comm->group);
}


int mpc_lowcomm_communicator_local_lead(mpc_lowcomm_communicator_t comm)
{
	assert(comm != NULL);
	mpc_lowcomm_communicator_t tcomm = mpc_lowcomm_communicator_get_local(comm);
	assert(tcomm != NULL);

	if(tcomm->is_comm_self)
	{
		return 0;
	}

	assert(tcomm->group != NULL);

	return mpc_lowcomm_group_get_local_leader(tcomm->group);
}


int mpc_lowcomm_communicator_rank_of(const mpc_lowcomm_communicator_t comm, const int comm_world_rank)
{
	assert(comm != NULL);
	mpc_lowcomm_communicator_t tcomm = mpc_lowcomm_communicator_get_local(comm);
	assert(tcomm != NULL);

	if(tcomm->is_comm_self)
	{
		return 0;
	}

	//mpc_common_debug_error("GET RANK from %p targets %p", comm, tcomm);

	return mpc_lowcomm_group_rank_for(tcomm->group, comm_world_rank);
}

int mpc_lowcomm_communicator_rank(const mpc_lowcomm_communicator_t comm)
{
	return mpc_lowcomm_communicator_rank_of(comm, mpc_lowcomm_get_rank() );
}

int mpc_lowcomm_communicator_size(const mpc_lowcomm_communicator_t comm)
{
	assert(comm != NULL);
	mpc_lowcomm_communicator_t tcomm = mpc_lowcomm_communicator_get_local(comm);
	assert(tcomm != NULL);

	if(tcomm->is_comm_self)
	{
		return 1;
	}

	return mpc_lowcomm_group_size(tcomm->group);
}

int mpc_lowcomm_communicator_world_rank(const mpc_lowcomm_communicator_t comm, int rank)
{
	assert(comm != NULL);
	mpc_lowcomm_communicator_t tcomm = mpc_lowcomm_communicator_get_local(comm);
	assert(tcomm != NULL);

	if(tcomm->is_comm_self)
	{
		/* For self rank is always the process itself */
		return mpc_common_get_task_rank();
	}

	return mpc_lowcomm_group_world_rank(tcomm->group, rank);
}

int mpc_lowcomm_communicator_get_process_count(const mpc_lowcomm_communicator_t comm)
{
	assert(comm != NULL);
	mpc_lowcomm_communicator_t tcomm = mpc_lowcomm_communicator_get_local(comm);
	assert(tcomm != NULL);

	if(tcomm->is_comm_self)
	{
		return 1;
	}

	return _mpc_lowcomm_group_process_count(tcomm->group);
}

int *mpc_lowcomm_communicator_get_process_list(const mpc_lowcomm_communicator_t comm)
{
	assert(comm != NULL);
	mpc_lowcomm_communicator_t tcomm = mpc_lowcomm_communicator_get_local(comm);
	assert(tcomm != NULL);

	if(tcomm->is_comm_self)
	{
		/* Here we always have current process */
		return &__process_rank;
	}

	return _mpc_lowcomm_group_process_list(tcomm->group);
}

typedef struct
{
	int color;
	int key;
	int rank;
} mpc_comm_split_t;

mpc_lowcomm_communicator_t mpc_lowcomm_communicator_split(mpc_lowcomm_communicator_t comm, int color, int key)
{
	int i, j, k;

	mpc_lowcomm_communicator_t comm_out = MPC_COMM_NULL;

	int size = mpc_lowcomm_communicator_size(comm);
	int rank = mpc_lowcomm_communicator_rank(comm);

	mpc_comm_split_t *tab = ( mpc_comm_split_t * )sctk_malloc(size * sizeof(mpc_comm_split_t) );
	assume(tab != NULL);

	mpc_comm_split_t tab_this;

	tab_this.rank  = rank;
	tab_this.color = color;
	tab_this.key   = key;

	mpc_lowcomm_allgather(&tab_this, tab, sizeof(mpc_comm_split_t), comm);

	assert(tab[rank].rank == tab_this.rank);
	assert(tab[rank].color == tab_this.color);
	assert(tab[rank].key == tab_this.key);

	mpc_comm_split_t tab_tmp;
	/*Sort the new tab */
	for(i = 0; i < size; i++)
	{
		for(j = 0; j < size - 1; j++)
		{
			if(tab[j].color > tab[j + 1].color)
			{
				tab_tmp    = tab[j + 1];
				tab[j + 1] = tab[j];
				tab[j]     = tab_tmp;
			}
			else
			{
				if( (tab[j].color == tab[j + 1].color) &&
				    (tab[j].key > tab[j + 1].key) )
				{
					tab_tmp    = tab[j + 1];
					tab[j + 1] = tab[j];
					tab[j]     = tab_tmp;
				}
			}
		}
	}

	int *color_tab = ( int * )sctk_malloc(size * sizeof(int) );
	assume(color_tab != NULL);

	int color_number = 0;

	for(i = 0; i < size; i++)
	{
		/* For each cell  check if color is known */
		for(j = 0; j < color_number; j++)
		{
			if(color_tab[j] == tab[i].color)
			{
				break;
			}
		}

		/* If we reached the end of the loop new color */
		if(j == color_number)
		{
			color_tab[j] = tab[i].color;
			color_number++;
		}

		//mpc_common_debug_error("COL rank %d color %d", i, tab[i].rank, tab[i].color);
	}

	//mpc_common_debug_error("%d colors", color_number);

	/*We need on comm_create per color */
	for(k = 0; k < color_number; k++)
	{
		int group_size = 0;
		int tmp_color  = color_tab[k];

		if(tmp_color != MPC_UNDEFINED)
		{
			group_size = 0;

			for(i = 0; i < size; i++)
			{
				if(tab[i].color == tmp_color)
				{
					group_size++;
				}
			}


			int *comm_world_ranks = ( int * )sctk_malloc(group_size * sizeof(int) );
			assume(comm_world_ranks != NULL);

			j = 0;

			for(i = 0; i < size; i++)
			{
				if(tab[i].color == tmp_color)
				{
					comm_world_ranks[j] = mpc_lowcomm_communicator_world_rank(comm, tab[i].rank);
					//mpc_common_debug_error("Thread %d color (%d) size %d on %d rank %d", tmp_color,
					//                   k, j, group_size, tab[i].rank);
					j++;
				}
			}

			mpc_lowcomm_group_t *comm_group = mpc_lowcomm_group_create(group_size, comm_world_ranks);

			mpc_lowcomm_communicator_t new_comm = mpc_lowcomm_communicator_from_group(comm, comm_group);

			if(tmp_color == color)
			{
				comm_out = new_comm;
			}

			/* We free here as the comm now holds a ref */
			mpc_lowcomm_group_free(&comm_group);
			sctk_free(comm_world_ranks);
			//mpc_common_debug_error("Split color %d done", tmp_color);
		}
	}

	sctk_free(color_tab);
	sctk_free(tab);
	//mpc_common_debug_error("Split done");

	return comm_out;
}

/**********************
* INTERCOMMUNICATORS *
**********************/
mpc_lowcomm_communicator_t mpc_lowcomm_intercommunicator_merge(mpc_lowcomm_communicator_t intercomm, int high)
{
	assume(mpc_lowcomm_communicator_is_intercomm(intercomm) );

	int local_size  = mpc_lowcomm_communicator_size(intercomm);
	int remote_size = mpc_lowcomm_communicator_remote_size(intercomm);

	int total_size = local_size + remote_size;

	_mpc_lowcomm_group_rank_descriptor_t *remote_descriptors = sctk_malloc(total_size * sizeof(_mpc_lowcomm_group_rank_descriptor_t) );
	assume(remote_descriptors != NULL);

	/* Determine order */
	int local_rank = mpc_lowcomm_communicator_rank(intercomm);

	mpc_lowcomm_communicator_t remote_comm = mpc_lowcomm_communicator_get_remote(intercomm);
	mpc_lowcomm_communicator_t local_comm  = mpc_lowcomm_communicator_get_local(intercomm);

	assume(remote_comm != NULL);
	assume(local_comm != NULL);

	uint32_t intracomm_id = MPC_LOWCOMM_COMM_NULL_ID;

	/* Roots build their list according to high */
	if(local_rank == 0)
	{
		/* ########################################
		   Build the process list according to high 
        ########################################### */
		int remote_high = 0;
		mpc_lowcomm_sendrecv(&high, sizeof(int), 0, 0, &remote_high, 0, intercomm);

		int local_root_rank = mpc_lowcomm_communicator_world_rank(intercomm, 0);
		int remote_root_rank = mpc_lowcomm_communicator_remote_world_rank(intercomm, 0);

		mpc_lowcomm_group_t *first  = NULL;
		mpc_lowcomm_group_t *second = NULL;

		/* By default we order according to lead ranks */
		if(local_root_rank < remote_root_rank)
		{
			first = local_comm->group;
			second = remote_comm->group;
		}
		else
		{
			first = remote_comm->group;
			second = local_comm->group;
		}

		if(remote_high != high)
		{
			if(remote_high < high)
			{
				/* I am willing to have the upper part
				note that equal is ignored here */
				if(second != local_comm->group)
				{
					first = remote_comm->group;
					second = local_comm->group;
				}

			}
			else
			{
				/* The other is willing to have the high part */
				if(second == local_comm->group)
				{
					first = local_comm->group;
					second = remote_comm->group;
				}
			}
		}

		int          cnt = 0;
		unsigned int i;
		for(i = 0; i < first->size; i++)
		{
			memcpy(&remote_descriptors[cnt],
			       mpc_lowcomm_group_descriptor(first, i),
			       sizeof(_mpc_lowcomm_group_rank_descriptor_t) );
			cnt++;
		}

		for(i = 0; i < second->size; i++)
		{
			memcpy(&remote_descriptors[cnt],
			       mpc_lowcomm_group_descriptor(second, i),
			       sizeof(_mpc_lowcomm_group_rank_descriptor_t) );
			cnt++;
		}

		for( i = 0 ; i < cnt; i++)
		{
			mpc_common_debug_error("MERGING %d is %d", i, remote_descriptors[i].comm_world_rank );
		}

		/* ########################################
		   Elect who is going to choose the ID 
        ########################################### */

		int my_world_rank = mpc_lowcomm_communicator_world_rank(intercomm, 0);
		int remote_world_rank = mpc_lowcomm_communicator_remote_world_rank(intercomm, 0);

		if(my_world_rank < remote_world_rank)
		{
			/* I do receive the ID */
			mpc_lowcomm_recv(0, &intracomm_id, sizeof(uint32_t), 128, intercomm);
		}
		else
		{
			/* I do create and send the ID */
			intracomm_id = __communicator_id_new();
			mpc_lowcomm_send(0, &intracomm_id, sizeof(uint32_t), 128, intercomm);
		}
	}

	/* Root broadcasts its finding to local comm */

	/* First all the descriptors */
	mpc_lowcomm_bcast(remote_descriptors,
	                  total_size * sizeof(_mpc_lowcomm_group_rank_descriptor_t),
	                  0,
	                  local_comm);

	/* And second the id of the new intracomm */
	mpc_lowcomm_bcast(&intracomm_id,
	                  sizeof(uint32_t),
	                  0,
	                  local_comm);	

	mpc_lowcomm_group_t *intracomm_group = _mpc_lowcomm_group_create(total_size, remote_descriptors);

	mpc_lowcomm_communicator_t intracomm = __new_communicator(intercomm,
															  intracomm_group,
															  NULL,
															  NULL,
															  0,
															  1,
															  intracomm_id);
	mpc_lowcomm_group_free(&intracomm_group);

	return intracomm;
}

static inline uint32_t __intercomm_root_id_exchange(const mpc_lowcomm_communicator_t local_comm,
                                                    const mpc_lowcomm_communicator_t peer_comm,
                                                    const int tag,
                                                    const int local_comm_rank,
                                                    const int local_leader,
                                                    const int remote_leader)
{
	uint32_t new_id = MPC_LOWCOMM_COMM_NULL_ID;

	int cw_rank = mpc_lowcomm_get_rank();

	if(local_comm_rank == local_leader)
	{
		/* We are local leaders */

		/* Get remote world rank */
		int remote_cw_rank = mpc_lowcomm_communicator_world_rank(peer_comm, remote_leader);

		assume(cw_rank != remote_cw_rank);

		if(cw_rank < remote_cw_rank)
		{
			/* I do receive the id */
			mpc_lowcomm_recv(remote_leader, &new_id, sizeof(uint32_t), tag, peer_comm);
		}
		else
		{
			new_id = __communicator_id_new();
			mpc_lowcomm_send(remote_leader, &new_id, sizeof(uint32_t), tag, peer_comm);
		}
	}

	mpc_lowcomm_bcast(&new_id, sizeof(int), local_leader, local_comm);

	return new_id;
}

mpc_lowcomm_communicator_t mpc_lowcomm_communicator_intercomm_create(const mpc_lowcomm_communicator_t local_comm,
                                                                     const int local_leader,
                                                                     const mpc_lowcomm_communicator_t peer_comm,
                                                                     const int remote_leader,
                                                                     const int tag)
{
	if(0 && local_leader == remote_leader)
	{
		return MPC_COMM_NULL;
	}

	if( (peer_comm == MPC_COMM_NULL) || (local_comm == MPC_COMM_NULL) )
	{
		return MPC_COMM_NULL;
	}


	/* #####################################
	 * Create left comm as a dup of local comm
	 #################################### */

	/* First step is to duplicate the local_comm which is left */
	mpc_lowcomm_communicator_t left_comm = mpc_lowcomm_communicator_dup(local_comm);

	if(left_comm == MPC_COMM_NULL)
	{
		return left_comm;
	}

	/* Now we need to create right comm locally by exchanging comm info */

	int local_comm_rank = mpc_lowcomm_communicator_rank(left_comm);
	int local_comm_size = mpc_lowcomm_communicator_size(left_comm);

	/* ######################################
	 *   First of all get this new comm an id
	 ##################################### */

	uint32_t intercomm_id = __intercomm_root_id_exchange(left_comm, peer_comm, tag, local_comm_rank, local_leader, remote_leader);

	/* ###################################
	*   First attemps exchange comm ids
	################################### */
	uint32_t left_comm_id  = mpc_lowcomm_communicator_id(left_comm);
	uint32_t right_comm_id = MPC_LOWCOMM_COMM_NULL_ID;

	/* Roots exchange ids */
	if(local_comm_rank == local_leader)
	{
		mpc_lowcomm_sendrecv(&left_comm_id, sizeof(uint32_t), remote_leader, tag, &right_comm_id, remote_leader, peer_comm);
	}

	/* Ids are bcast on local comm */
	mpc_lowcomm_bcast(&right_comm_id, sizeof(uint32_t), local_leader, left_comm);

	/* One possibility is that the right comm is locally known thanks to global ids */
	mpc_lowcomm_communicator_t right_comm = _mpc_lowcomm_get_communicator_from_id(right_comm_id);

	/* We now want to make sure the comm is known on the whole local comm*/
	int right_comm_missing            = (right_comm == MPC_COMM_NULL);
	int one_local_does_not_know_right = right_comm_missing;

	int i;
	for(i = 0; i < local_comm_size; i++)
	{
		/* Set my flag as I'm root */
		if(i == local_comm_rank)
		{
			one_local_does_not_know_right = right_comm_missing;
		}

		mpc_lowcomm_bcast(&one_local_does_not_know_right, sizeof(int), i, left_comm);

		if(one_local_does_not_know_right)
		{
			break;
		}
	}

	int someone_right_does_not_know_knows_my_local = 0;

	/* Roots exchange remote knowledge */
	if(local_comm_rank == local_leader)
	{
		mpc_lowcomm_sendrecv(&one_local_does_not_know_right,
		                     sizeof(int),
		                     remote_leader,
		                     tag,
		                     &someone_right_does_not_know_knows_my_local,
		                     remote_leader,
		                     peer_comm);
	}

	mpc_lowcomm_bcast(&someone_right_does_not_know_knows_my_local, sizeof(int), local_leader, left_comm);


	mpc_lowcomm_communicator_t ret = MPC_COMM_NULL;

	if(!someone_right_does_not_know_knows_my_local && !one_local_does_not_know_right)
	{
		/* The comm ID is globally known on both sides no need for exchanges */
		/* === DONE */
		/* First create a communicator to become the intercomm */
		ret = __new_communicator(local_comm,
									NULL,
									left_comm,
									right_comm,
									0,
									0,
									intercomm_id);
	}
	else
	{

		/* ########################################
		*  Serialize communicators for right build
		######################################## */

		/* If we are here someone does not know the comm either left of right
		* we will therefore send our descriptors to the right in order to
		* allow it to build my local locally "serializing the comm" to do so
		* we first send our size and then our comm_data allowing the group
		* to be constructed and eventually the final comm */

		int right_comm_size = 0;

		/* Roots exchange their local sizes */
		if(local_comm_rank == local_leader)
		{
			mpc_lowcomm_sendrecv(&local_comm_size, sizeof(uint32_t), remote_leader, tag, &right_comm_size, remote_leader, peer_comm);
		}

		/* Right comm size is bcast on local comm */
		mpc_lowcomm_bcast(&right_comm_size, sizeof(uint32_t), local_leader, left_comm);
		assume(right_comm_size != 0);

		_mpc_lowcomm_group_rank_descriptor_t *remote_descriptors = sctk_malloc(right_comm_size * sizeof(_mpc_lowcomm_group_rank_descriptor_t) );
		assume(remote_descriptors != NULL);

		/* Roots exchange their descriptors */
		if(local_comm_rank == local_leader)
		{
			mpc_lowcomm_request_t reqs[2];

			mpc_lowcomm_isend(remote_leader,
							left_comm->group->ranks,
							local_comm_size * sizeof(_mpc_lowcomm_group_rank_descriptor_t),
							tag,
							peer_comm,
							&reqs[0]);

			mpc_lowcomm_irecv(remote_leader,
							remote_descriptors,
							right_comm_size * sizeof(_mpc_lowcomm_group_rank_descriptor_t),
							tag,
							peer_comm,
							&reqs[1]);

			mpc_lowcomm_waitall(2, reqs, SCTK_STATUS_NULL);
		}

		/* Right descriptors are bcast on local comm */
		mpc_lowcomm_bcast(remote_descriptors, right_comm_size * sizeof(_mpc_lowcomm_group_rank_descriptor_t), local_leader, left_comm);

		/* Now we create a group from descriptors */
		mpc_lowcomm_group_t *right_group = _mpc_lowcomm_group_create(right_comm_size, remote_descriptors);

		/* And eventually our comm of interest */
		right_comm = __new_communicator(left_comm,
										right_group,
										NULL,
										NULL,
										0,
										0,
										right_comm_id);


		mpc_lowcomm_group_free(&right_group);

		/* First create a communicator to become the intercomm */
		ret = __new_communicator(local_comm,
								NULL,
								left_comm,
								right_comm,
								0,
								0,
								intercomm_id);
	}

	mpc_lowcomm_barrier(ret);

	return ret;
}


int mpc_lowcomm_communicator_in_left_group_rank(const mpc_lowcomm_communicator_t communicator, int comm_world_rank)
{
	assert(communicator != NULL);
	assert(communicator->left_comm);
	return mpc_lowcomm_group_includes(communicator->left_comm->group, comm_world_rank);
}

int mpc_lowcomm_communicator_in_left_group(const mpc_lowcomm_communicator_t communicator)
{
	int rank = mpc_lowcomm_get_rank();

	return mpc_lowcomm_communicator_in_left_group_rank(communicator, rank);
}

int mpc_lowcomm_communicator_in_right_group_rank(const mpc_lowcomm_communicator_t communicator, int comm_world_rank)
{
	assert(communicator != NULL);
	assume(communicator->right_comm != NULL);
	return mpc_lowcomm_group_includes(communicator->right_comm->group, comm_world_rank);
}

int mpc_lowcomm_communicator_in_right_group(const mpc_lowcomm_communicator_t communicator)
{
	int rank = mpc_lowcomm_get_rank();

	return mpc_lowcomm_communicator_in_right_group_rank(communicator, rank);
}

int mpc_lowcomm_communicator_in_master_group(const mpc_lowcomm_communicator_t communicator)
{
	assume(mpc_lowcomm_communicator_is_intercomm(communicator));
	int remote_master_rank = mpc_lowcomm_communicator_remote_world_rank(communicator, 0);
	int local_master_rank = mpc_lowcomm_communicator_world_rank(communicator, 0);

	if(local_master_rank < remote_master_rank)
	{
		return 0;
	}
	else
	{
		return 1;
	}

	not_reachable();
	return 0;
}

mpc_lowcomm_communicator_t mpc_lowcomm_communicator_get_local(mpc_lowcomm_communicator_t comm)
{
	assert(comm != NULL);

	if(mpc_lowcomm_communicator_is_intercomm(comm) )
	{
		/* Intercomm we need to see where we are in left and right */
		int rank = mpc_lowcomm_get_rank();

		if(mpc_lowcomm_communicator_in_left_group_rank(comm, rank) )
		{
			return comm->left_comm;
		}
		else if(mpc_lowcomm_communicator_in_right_group_rank(comm, rank) )
		{
			return comm->right_comm;
		}
	}
	else
	{
		/* Intracomm local comm is myself */
		return comm;
	}

	not_reachable();
	return comm;
}

mpc_lowcomm_communicator_t mpc_lowcomm_communicator_get_remote(mpc_lowcomm_communicator_t comm)
{
	/* Only meaningfull for intercomms */
	if(!mpc_lowcomm_communicator_is_intercomm(comm) )
	{
		return MPC_COMM_NULL;
	}

	mpc_lowcomm_communicator_t local = mpc_lowcomm_communicator_get_local(comm);

	/* The remote is the non-local one */
	if(local == comm->left_comm)
	{
		return comm->right_comm;
	}
	else
	{
		return comm->left_comm;
	}

	not_reachable();
	return comm;
}

int mpc_lowcomm_communicator_remote_size(const mpc_lowcomm_communicator_t comm)
{
	mpc_lowcomm_communicator_t remote_comm = mpc_lowcomm_communicator_get_remote(comm);

	return mpc_lowcomm_communicator_size(remote_comm);
}

int mpc_lowcomm_communicator_remote_world_rank(const mpc_lowcomm_communicator_t comm, const int rank)
{
	mpc_lowcomm_communicator_t remote_comm = mpc_lowcomm_communicator_get_remote(comm);
	return mpc_lowcomm_communicator_world_rank(remote_comm, rank);
}

int mpc_lowcomm_communicator_is_intercomm(mpc_lowcomm_communicator_t comm)
{
	assert(comm != NULL);

	/* Comm self also has a null group
	 * so we need to check it first */
	if(comm->is_comm_self)
	{
		return 0;
	}

	int ret = (comm->group == NULL);

	if(ret)
	{
		assert(comm->left_comm != MPC_COMM_NULL);
		assert(comm->right_comm != MPC_COMM_NULL);
	}

	return ret;
}
