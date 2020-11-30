#ifndef MPC_LOWCOMM_GROUP_H
#define MPC_LOWCOMM_GROUP_H

#include <mpc_lowcomm_group.h>
#include <mpc_lowcomm_types.h>
#include <mpc_common_asm.h>
#include <mpc_common_spinlock.h>
#include <mpc_common_debug.h>

/****************************************
* _MPC_LOWCOMM_GROUP_RANK_DESCRIPTOR_S *
****************************************/

typedef struct _mpc_lowcomm_group_rank_descriptor_s
{
	int comm_world_rank;
}_mpc_lowcomm_group_rank_descriptor_t;

int _mpc_lowcomm_group_rank_descriptor_equal(_mpc_lowcomm_group_rank_descriptor_t *a,
                                             _mpc_lowcomm_group_rank_descriptor_t *b);

int _mpc_lowcomm_group_rank_descriptor_set(_mpc_lowcomm_group_rank_descriptor_t *rd, int comm_world_rank);

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

mpc_lowcomm_group_t * _mpc_lowcomm_group_create_from_world_ranks(int size, int * world_ranks);

#endif /* MPC_LOWCOMM_GROUP_H */
