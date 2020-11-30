#ifndef COMMUNICATOR_H
#define COMMUNICATOR_H

#include <mpc_lowcomm_communicator.h>

#include <mpc_common_asm.h>
#include <mpc_common_types.h>
#include <mpc_common_rank.h>

#include "coll.h"
#include "shm_coll.h"


/**
 * @brief This is the struct defining a lowcomm communicator
 * 
 */
typedef struct mpc_lowcomm_internal_communicator_s
{
	unsigned int               id;				/**< Integer unique identifier of the comm */	
	mpc_lowcomm_group_t *      group;			/**< Group supporting the comm */
	OPA_int_t                  refcount;		/**< Number of ref to the comm freed when 0 */

	OPA_int_t                  free_count;      /**< Local synchronization for free */

	unsigned int               process_span;	/**< Number of UNIX processes in the group */
	int *                      process_array;	/**< Array of the processes in the group */

	int                        is_comm_self;	/**< 1 if the group is comm_self */

	/* Collective comm */
	struct mpc_lowcomm_coll_s *coll;			/**< This holds the collectives for this comm */
	struct sctk_comm_coll *    shm_coll;		/**< This holds the SHM collectives for this comm */

	/* Intercomms */

	/* These are the internall intracomm for
	 * intercomms. If the group is NULL
	 * it means the communicator is an intercomm
	 * and then functions will refer to this functions */
	mpc_lowcomm_communicator_t left_comm;		/**< The left comm for intercomms */
	mpc_lowcomm_communicator_t right_comm;		/**< The right comm for intercomms */
}mpc_lowcomm_internal_communicator_t;

/*********************************
* COMMUNICATOR INIT AND RELEASE *
*********************************/

/**
 * @brief Initialize base communicators (WORLD and SELF)
 * 
 */
void _mpc_lowcomm_communicator_init(void);

/**
 * @brief Release the base communicators
 * 
 */
void _mpc_lowcomm_communicator_release(void);

/**
 * @brief Icrement refcounting on a comm
 * 
 * @param comm the comm to acquire
 */
void _mpc_lowcomm_communicator_acquire(mpc_lowcomm_internal_communicator_t *comm);

/**
 * @brief Decrement refcounting on a comm
 * 
 * @param comm the comm to relax
 * @return int the value before decrementing
 */
int _mpc_lowcomm_communicator_relax(mpc_lowcomm_internal_communicator_t *comm);

/***************************
* LOCAL PLACEMENT GETTERS *
***************************/

/**
 * @brief Get the first local task in comm_world for this process
 * 
 * @return int id of the first task in the process
 */
int _mpc_lowcomm_communicator_world_first_local_task();

/**
 * @brief Get the last local task in comm_world for this process
 * 
 * @return int id of the last task in the process
 */
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

/**
 * @brief Inline version of the communicator ID getter
 * 
 * @param comm the communicator
 * @return uint32_t the corresponding ID of the given comm
 */
static inline uint32_t _mpc_lowcomm_communicator_id(mpc_lowcomm_internal_communicator_t *comm)
{
	if(comm != MPC_COMM_NULL)
	{
		return comm->id;
	}

	return MPC_LOWCOMM_COMM_NULL_ID;
}

#endif /* COMMUNICATOR_H */
