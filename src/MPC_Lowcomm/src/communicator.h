#ifndef COMMUNICATOR_H
#define COMMUNICATOR_H

#include <mpc_lowcomm_communicator.h>

#include <mpc_common_asm.h>
#include <mpc_common_types.h>
#include <mpc_common_rank.h>

#include "coll.h"
#include "shm_coll.h"

typedef struct mpc_lowcomm_internal_communicator_s
{
	unsigned int               id;
	/* This is the group supporting the comm */
	mpc_lowcomm_group_t *      group;
	OPA_int_t                  refcount;

	unsigned int               process_span;
	int *                      process_array;

	int                        is_comm_self;

	/* Collective comm */
	struct mpc_lowcomm_coll_s *coll;
	struct sctk_comm_coll *    shm_coll;

	/* Intercomms */

	/* These are the internall intracomm for
	 * intercomms. If the group is NULL
	 * it means the communicator is an intercomm
	 * and then functions will refer to this functions */
	mpc_lowcomm_communicator_t left_comm;
	mpc_lowcomm_communicator_t right_comm;
	int is_intercomm_lead;
}mpc_lowcomm_internal_communicator_t;

/*********************************
* COMMUNICATOR INIT AND RELEASE *
*********************************/

void _mpc_lowcomm_communicator_init(void);
void _mpc_lowcomm_communicator_acquire(mpc_lowcomm_internal_communicator_t *comm);
int _mpc_lowcomm_communicator_relax(mpc_lowcomm_internal_communicator_t *comm);

/**************
* ID FACTORY *
**************/

mpc_lowcomm_communicator_t _mpc_lowcomm_get_communicator_from_id(uint32_t id);

/***************************
* LOCAL PLACEMENT GETTERS *
***************************/

int _mpc_lowcomm_communicator_world_first_local_task();
int _mpc_lowcomm_communicator_world_last_local_task();

/**********************
* COLLECTIVE GETTERS *
**********************/


/**
 * This method get the collectives related to the communicator.
 * @param communicator Identification number of the local communicator.
 * @return the structure containing the collectives.
 **/
static inline struct mpc_lowcomm_coll_s *_mpc_comm_get_internal_coll(const mpc_lowcomm_communicator_t comm)
{
	return comm->coll;
}

/**
 * This method set the collectives related to the communicator.
 * @param id Identification number of the local communicator.
 * @param coll structure containing the collectives.
 **/
static inline void _mpc_comm_set_internal_coll(mpc_lowcomm_communicator_t comm, struct mpc_lowcomm_coll_s *coll)
{
	comm->coll = coll;
}

static inline uint32_t _mpc_lowcomm_communicator_id(mpc_lowcomm_internal_communicator_t *comm)
{
	if(comm != MPC_COMM_NULL)
	{
		return comm->id;
	}

	return MPC_LOWCOMM_COMM_NULL_ID;
}

#endif /* COMMUNICATOR_H */
