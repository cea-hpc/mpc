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
#ifndef MPC_LOWCOMM_GROUP_H
#define MPC_LOWCOMM_GROUP_H

#include <mpc_lowcomm_group.h>
#include <mpc_lowcomm_types.h>
#include <mpc_common_asm.h>
#include <mpc_common_spinlock.h>
#include <mpc_common_debug.h>

#include <mpc_lowcomm_monitor.h>

/****************************************
* _MPC_LOWCOMM_GROUP_RANK_DESCRIPTOR_S *
****************************************/

typedef struct _mpc_lowcomm_group_rank_descriptor_s
{
	int comm_world_rank;
	mpc_lowcomm_peer_uid_t host_process_uid;
}_mpc_lowcomm_group_rank_descriptor_t;

int _mpc_lowcomm_group_rank_descriptor_equal(_mpc_lowcomm_group_rank_descriptor_t *a,
                                             _mpc_lowcomm_group_rank_descriptor_t *b);

int _mpc_lowcomm_group_rank_descriptor_set(_mpc_lowcomm_group_rank_descriptor_t *rd, int comm_world_rank);

int _mpc_lowcomm_group_rank_descriptor_set_in_grp(_mpc_lowcomm_group_rank_descriptor_t *rd,
											      mpc_lowcomm_set_uid_t set,
											      int group_rank);


/************************
* _MPC_LOWCOMM_GROUP_T *
************************/

struct _mpc_lowcomm_group_s
{
	OPA_int_t                             refcount;
	int                                   do_not_free;

	unsigned int                          size;
	_mpc_lowcomm_group_rank_descriptor_t *ranks;

    int                                  *global_to_local;

	mpc_common_spinlock_t                 process_lock;
	int                                   process_count;
	int *                                 process_list;
	int                                   tasks_count_in_process;

	int                                   local_leader;
};

static inline void _mpc_lowcomm_group_acquire(mpc_lowcomm_group_t *g)
{
	OPA_incr_int(&g->refcount);
}

static inline void _mpc_lowcomm_group_relax(mpc_lowcomm_group_t *g)
{
	OPA_decr_int(&g->refcount);
}

mpc_lowcomm_group_t *_mpc_lowcomm_group_create(unsigned int size, _mpc_lowcomm_group_rank_descriptor_t *ranks);

_mpc_lowcomm_group_rank_descriptor_t * mpc_lowcomm_group_descriptor(mpc_lowcomm_group_t *g, int rank);

int _mpc_lowcomm_group_process_count(mpc_lowcomm_group_t * g);
int * _mpc_lowcomm_group_process_list(mpc_lowcomm_group_t * g);
int _mpc_lowcomm_group_local_process_count(mpc_lowcomm_group_t * g);

/*******************
 * LIST MANAGEMENT *
 *******************/

mpc_lowcomm_group_t *_mpc_lowcomm_group_list_register(mpc_lowcomm_group_t *group);
int _mpc_lowcomm_group_list_pop(mpc_lowcomm_group_t *group);


/*************
 * UTILITIES *
 *************/

void _mpc_lowcomm_group_create_world(void);
void _mpc_lowcomm_group_release_world(void);

mpc_lowcomm_group_t * _mpc_lowcomm_group_world(void);
mpc_lowcomm_group_t * _mpc_lowcomm_group_world_no_assert(void);

mpc_lowcomm_group_t * _mpc_lowcomm_group_create_from_world_ranks(int size, int * world_ranks);

#endif /* MPC_LOWCOMM_GROUP_H */
