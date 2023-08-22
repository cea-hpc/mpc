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
#ifndef COMMUNICATOR_H
#define COMMUNICATOR_H

#include <mpc_lowcomm_communicator.h>

#include <mpc_common_asm.h>
#include <mpc_common_types.h>
#include <mpc_common_rank.h>

#include "coll.h"
#include "shm_coll.h"

#define MPC_INITIAL_TOPO_COMMS_SIZE 10
/**
 * @brief This is the struct for the informations needed for topological collectives
 */
typedef struct mpc_lowcomm_topo_comms
{
  int size;                               /**< Size of the arrays. */

  int *roots;                             /**< Array of roots used in collectives. */
  mpc_hardware_split_info_t **hw_infos;   /**< Array of the structure containing topological communicators. */
} mpc_lowcomm_topo_comms;


/**
 * @brief This is the struct defining a lowcomm communicator
 *
 */
typedef struct MPI_ABI_Comm
{
	mpc_lowcomm_communicator_id_t               id;				/**< Integer unique identifier of the comm */
	int                        linear_comm_id;  /** Linear communicator id on int32 used for FORTRAN */
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

	/* Topological comm */
	mpc_lowcomm_topo_comms *topo_comms;  /**< Topological communicators. */
  
	/* Extra context (sessions) */
	mpc_lowcomm_handle_ctx_t extra_ctx_ptr;
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
 * @brief Increment refcounting on a comm
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
    mpc_lowcomm_communicator_t local_comm = __mpc_lowcomm_communicator_from_predefined(comm);
	return local_comm->coll;
}

/**
 * This method set the collectives related to the communicator.
 * @param id Identification number of the local communicator.
 * @param coll structure containing the collectives.
 **/
static inline void _mpc_comm_set_internal_coll(mpc_lowcomm_communicator_t comm, struct mpc_lowcomm_coll_s *coll)
{
    comm = __mpc_lowcomm_communicator_from_predefined(comm);
	comm->coll = coll;
}

/**
 * @brief Inline version of the communicator ID getter
 * 
 * @param comm the communicator
 * @return mpc_lowcomm_communicator_id_t the corresponding ID of the given comm
 */
static inline mpc_lowcomm_communicator_id_t _mpc_lowcomm_communicator_id(mpc_lowcomm_internal_communicator_t *comm)
{
	if(comm != MPC_COMM_NULL)
	{
        comm = __mpc_lowcomm_communicator_from_predefined(comm);
		return comm->id;
	}

	return MPC_LOWCOMM_COMM_NULL_ID;
}

#endif /* COMMUNICATOR_H */
