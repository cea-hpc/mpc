#ifndef MPC_LOWCOMM_GROUP_H
#define MPC_LOWCOMM_GROUP_H

#include <mpc_lowcomm_group.h>
#include <mpc_lowcomm_types.h>
#include <mpc_common_asm.h>
#include <mpc_common_spinlock.h>

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

	unsigned int                          size;
	_mpc_lowcomm_group_rank_descriptor_t *ranks;
    int                                  *global_to_local;

	int                                   is_comm_world;
	int                                   is_comm_self;
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


/*****************************
*  mpc_lowcomm_group_list_t *
*****************************/

struct _mpc_lowcomm_group_entry_s
{
	struct _mpc_lowcomm_group_s *      group;
	struct _mpc_lowcomm_group_entry_s *next;
};

typedef struct _mpc_lowcomm_group_list_s
{
	struct _mpc_lowcomm_group_entry_s *entries;
	mpc_common_spinlock_t              lock;
}_mpc_lowcomm_group_list_t;

mpc_lowcomm_group_t *_mpc_lowcomm_group_list_register(mpc_lowcomm_group_t *group);
int _mpc_lowcomm_group_list_pop(mpc_lowcomm_group_t *group);

/*************
 * UTILITIES *
 *************/

void _mpc_lowcomm_group_create_world(void);

#endif /* MPC_LOWCOMM_GROUP_H */
