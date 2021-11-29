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

#include "pset.h"

#include <sctk_alloc.h>
#include <mpc_common_debug.h>
#include <mpc_common_rank.h>
#include <mpc_lowcomm_types.h>
#include "group.h"
#include <string.h>
#include <mpc_launch_pmi.h>
#include <mpc_topology.h>

#include <mpc_lowcomm.h>

struct _mpc_lowcomm_pset_list_entry
{
	int                                  is_comm_self;
	struct mpc_lowcomm_proces_set_s      pset;
	struct _mpc_lowcomm_pset_list_entry *next;
};


struct _mpc_lowcomm_pset_list_entry *__new_entry(char *name,
                                                 mpc_lowcomm_group_t *group,
                                                 int is_comm_self)
{
	struct _mpc_lowcomm_pset_list_entry *new = sctk_malloc(sizeof(struct _mpc_lowcomm_pset_list_entry) );

	assume(new != NULL);

	snprintf(new->pset.name, MPC_LOWCOMM_GROUP_MAX_PSET_NAME_LEN, "%s", name);

	if(group)
	{
		new->pset.group = mpc_lowcomm_group_dup(group);
	}
	else
	{
		assume(is_comm_self == 1);
	}

	new->is_comm_self = is_comm_self;

	return new;
}

int __free_entry(struct _mpc_lowcomm_pset_list_entry *entry)
{
	mpc_lowcomm_group_free(&entry->pset.group);
	sctk_free(entry);

	return MPC_LOWCOMM_SUCCESS;
}

struct _mpc_lowcomm_pset_list
{
	mpc_common_spinlock_t                lock;
	struct _mpc_lowcomm_pset_list_entry *head;
	uint64_t                             count;
};

/**
 * @brief One list per local task
 *
 */

static mpc_common_spinlock_t __pset_init_lock = MPC_COMMON_SPINLOCK_INITIALIZER;
static struct _mpc_lowcomm_pset_list * __pset_lists = NULL;

int _mpc_lowcomm_init_psets(void)
{
	mpc_common_spinlock_lock(&__pset_init_lock);

	if(!__pset_lists)
	{

		int loc_count = mpc_common_get_local_task_count();
		__pset_lists = sctk_malloc(sizeof(struct _mpc_lowcomm_pset_list) * loc_count);

		assume(__pset_lists != NULL);

		int i;

		for(i=0 ; i < loc_count ; i++)
		{
			mpc_common_spinlock_init(&__pset_lists[i].lock, 0);
			__pset_lists[i].head  = NULL;
			__pset_lists[i].count = 0;
		}
	}

	mpc_common_spinlock_unlock(&__pset_init_lock);

	return 0;
}



static inline struct _mpc_lowcomm_pset_list * __get_my_list(void)
{
	int local_rank = mpc_common_get_local_task_rank();
	assume(0 <= local_rank);
	return &__pset_lists[local_rank];
}


int _mpc_lowcomm_release_psets(void)
{
	struct _mpc_lowcomm_pset_list * my_list = __get_my_list();

	struct _mpc_lowcomm_pset_list_entry *cur = my_list->head;

	while(cur)
	{
		struct _mpc_lowcomm_pset_list_entry *to_free = cur;
		cur = cur->next;
		my_list->count--;
		__free_entry(to_free);
	}

	mpc_common_spinlock_lock(&__pset_init_lock);

	static int free_counter = 0;

	free_counter++;

	if(free_counter == mpc_common_get_local_task_count())
	{
		sctk_free(__pset_lists);
		__pset_lists = NULL;
	}

	mpc_common_spinlock_unlock(&__pset_init_lock);

	return 0;
}

static inline struct _mpc_lowcomm_pset_list_entry *__pset_get_by_name_no_lock(const char *name)
{
	struct _mpc_lowcomm_pset_list_entry *cur = __get_my_list()->head;

	struct _mpc_lowcomm_pset_list_entry *ret = NULL;

	while(cur)
	{
		//mpc_common_debug_error("%s == %s ?", name, cur->pset.name);
		if(!strcmp(name, cur->pset.name) )
		{
			ret = cur;
			break;
		}
		cur = cur->next;
	}

	return ret;
}

int _mpc_lowcomm_pset_push(char *name, mpc_lowcomm_group_t *group, int is_comm_self)
{
	struct _mpc_lowcomm_pset_list * my_list = __get_my_list();

	mpc_common_spinlock_lock(&my_list->lock);

	/* Ensure we do not have a set with same name */
	if(__pset_get_by_name_no_lock(name) )
	{
		mpc_common_spinlock_unlock(&my_list->lock);
		mpc_common_debug_error("Pset %s is already registered", name);
		return MPC_LOWCOMM_ERROR;
	}

	struct _mpc_lowcomm_pset_list_entry *new = __new_entry(name, group, is_comm_self);

	new->next        = my_list->head;
	my_list->head = new;
	my_list->count++;

	mpc_common_spinlock_unlock(&my_list->lock);

	return MPC_LOWCOMM_SUCCESS;
}

int mpc_lowcomm_group_pset_count(void)
{
	struct _mpc_lowcomm_pset_list * my_list = __get_my_list();

	int ret = 0;

	mpc_common_spinlock_lock(&my_list->lock);
	ret = my_list->count;
	mpc_common_spinlock_unlock(&my_list->lock);

	return ret;
}

static inline mpc_lowcomm_process_set_t *__rebuild_and_dup(struct _mpc_lowcomm_pset_list_entry *entry)
{
	if(!entry)
	{
		return NULL;
	}

	mpc_lowcomm_process_set_t *ret = sctk_malloc(sizeof(mpc_lowcomm_process_set_t) );

	assume(ret);

	memcpy(ret, &entry->pset, sizeof(mpc_lowcomm_process_set_t) );

	if(entry->is_comm_self)
	{
		/* Build a self group on the fly */
		int my_rank = mpc_common_get_task_rank();
		ret->group = mpc_lowcomm_group_create(1, &my_rank);
	}
	else
	{
		/* Dup the group */
		ret->group = mpc_lowcomm_group_dup(entry->pset.group);
	}

	return ret;
}

int mpc_lowcomm_group_pset_free(mpc_lowcomm_process_set_t *pset)
{
	if(!pset)
	{
		return MPC_LOWCOMM_SUCCESS;
	}

	mpc_lowcomm_group_free(&pset->group);
	sctk_free(pset);

	return MPC_LOWCOMM_SUCCESS;
}

mpc_lowcomm_process_set_t *mpc_lowcomm_group_pset_get_nth(int n)
{
	if(mpc_lowcomm_group_pset_count() <= n)
	{
		return NULL;
	}


	struct _mpc_lowcomm_pset_list * my_list = __get_my_list();

	struct _mpc_lowcomm_pset_list_entry *ret = NULL;

	mpc_common_spinlock_lock(&my_list->lock);

	struct _mpc_lowcomm_pset_list_entry *cur = my_list->head;

	int cnt = 0;

	while(cur)
	{
		if(cnt == n)
		{
			ret = cur;
			break;
		}
		cnt++;
		cur = cur->next;
	}

	mpc_common_spinlock_unlock(&my_list->lock);

	return __rebuild_and_dup(ret);
}

mpc_lowcomm_process_set_t *mpc_lowcomm_group_pset_get_by_name(const char *name)
{
	struct _mpc_lowcomm_pset_list * my_list = __get_my_list();

	struct _mpc_lowcomm_pset_list_entry *ret = NULL;

	mpc_common_spinlock_lock(&my_list->lock);
	ret = __pset_get_by_name_no_lock(name);
	mpc_common_spinlock_unlock(&my_list->lock);
	return __rebuild_and_dup(ret);
}

static inline int __register_base_comms(void)
{
	/* SELF */
	_mpc_lowcomm_pset_push("mpi://SELF", NULL, 1);

	/* WORLD */
	_mpc_lowcomm_pset_push("mpi://WORLD", _mpc_lowcomm_group_world(), 0);

	return 0;
}

static inline int __split_for(mpc_lowcomm_communicator_t src_comm, const char *desc, int color)
{
	mpc_lowcomm_communicator_t comm = mpc_lowcomm_communicator_split(src_comm,
	                                                                 color,
	                                                                 mpc_common_get_task_rank() );

	if(comm == MPC_COMM_NULL)
	{
		return MPC_LOWCOMM_ERROR;
	}

	//mpc_common_debug_error("APP %d LPR %d", color, mpc_common_get_local_process_rank());

	mpc_lowcomm_group_t *grp = mpc_lowcomm_communicator_group(comm);

	/* Store only processes you belong to */
	if(color != MPC_UNDEFINED)
	{
		char sappid[200];

		snprintf(sappid, 200, "%sSELF", desc);
		_mpc_lowcomm_pset_push(sappid, grp, 0);

		snprintf(sappid, 200, "%s%d", desc, color);
		_mpc_lowcomm_pset_push(sappid, grp, 0);
	}

	mpc_lowcomm_group_free(&grp);

	mpc_lowcomm_communicator_free(&comm);

	return 0;
}

int _mpc_lowcomm_pset_bootstrap(void)
{
	_mpc_lowcomm_init_psets();

	mpc_lowcomm_barrier(MPC_COMM_WORLD);

	__register_base_comms();


	/* Application PSETS */
	int my_app_id;

	mpc_launch_pmi_get_app_rank(&my_app_id);

	__split_for(MPC_COMM_WORLD, "app://", my_app_id);

	/* Node PSETS */
	__split_for(MPC_COMM_WORLD, "node://", mpc_common_get_node_rank() );

	/* Process PSETS */
	__split_for(MPC_COMM_WORLD, "unix://", mpc_common_get_process_rank() );


	/* Topological PSETS */
	mpc_lowcomm_communicator_t node_comm = mpc_lowcomm_communicator_split(MPC_COMM_WORLD,
	                                                                      mpc_common_get_node_rank(),
	                                                                      mpc_common_get_task_rank() );

	if(node_comm == MPC_COMM_NULL)
	{
		return MPC_LOWCOMM_ERROR;
	}

	/* One task on the whole node nothing interesting */
	if(mpc_lowcomm_communicator_size(node_comm) == 1)
	{
		mpc_lowcomm_communicator_free(&node_comm);
		return MPC_LOWCOMM_SUCCESS;
	}

	int i;

	for(i = 0; i < MPC_LOWCOMM_HW_TYPE_COUNT; i++)
	{
		int color = mpc_topology_guided_compute_color( (char *)mpc_topology_split_hardware_type_name[i]);

		if(color < 0)
		{
			color = MPC_UNDEFINED;
		}

		/* If we are here all ranks have a color */
		char name[64];
		snprintf(name, 64, "hwloc://%s/", mpc_topology_split_hardware_type_name[i]);
		__split_for(node_comm, name, color);
	}

	mpc_lowcomm_communicator_free(&node_comm);

	return MPC_LOWCOMM_SUCCESS;
}
