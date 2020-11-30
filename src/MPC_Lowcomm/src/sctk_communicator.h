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
/* #   - PERACHE Marc marc.perache@cea.fr                                 # */
/* #                                                                      # */
/* ######################################################################## */
#ifndef __SCTK_COMMUNICATOR_H_
#define __SCTK_COMMUNICATOR_H_
#if 0

#include <mpc_lowcomm_types.h>
#include <mpc_common_asm.h>
#include <mpc_common_spinlock.h>

#include <mpc_common_rank.h>

#include "mpc_runtime_config.h"



/************************** MACROS *************************/
/** define the max number of communicators in the common table **/
#define SCTK_MAX_COMMUNICATOR_TAB 512

/******************************** STRUCTURE *********************************/

struct mpc_lowcomm_coll_s;

/**
 * Here we define the structure responsible for the communicators.
 * It works for both intracommunications and intercommunications.
**/
typedef struct sctk_internal_communicator_s
{
	/** communicator identification number **/
	mpc_lowcomm_communicator_t id;

	/** A flag indicating if the communicator is being freed */
	OPA_int_t freed;

	/** structure for collectives communications **/
	struct mpc_lowcomm_coll_s *collectives;

	/** number of tasks in the communicator **/
	int nb_task;

	/** minimum rank of a local task **/
	int last_local;
	/** maximum rank of a local task **/
	int first_local;
	/** number of tasks involved with the communicator for the current process **/
	int local_tasks;
	/** gives MPI_COMM_WORLD ranks used in this communicator **/
	int *local_to_global;
	/** gives task ranks corresponding to MPI_COMM_WORLD ranks**/
	int *global_to_local;
	/** gives identification of process which schedules the task **/
	int *task_to_process;
	/** processes involved with the communicator **/
	int *process;
	/** number of processes for the communicator **/
	int process_nb;

	mpc_common_rwlock_t lock;
	/** spinlock for the creation of the comunicator **/
	mpc_common_spinlock_t creation_lock;

	volatile int has_zero;

	OPA_int_t nb_to_delete;
	/** hash table for communicators **/
	UT_hash_handle hh;

	struct sctk_internal_communicator_s *new_comm;
	struct sctk_internal_communicator_s *remote_comm;
	/** determine if intercommunicator **/
	int is_inter_comm;
	/** rank of remote leader **/
	int remote_leader;
	/** group rank of local leader **/
	int local_leader;
	/** Tells if we have to handle this comm as COMM_SELF */
	int is_comm_self;
	/** Tells if all the ranks of the communicator are in shared-mem */
	int is_shared_mem;
	/** Tells if all the ranks of the communicator are in shared-node */
	int is_shared_node;
	/** peer communication (only for intercommunicator) **/
	mpc_lowcomm_communicator_t peer_comm;
	/** local id (only for intercommunicators)**/
	mpc_lowcomm_communicator_t local_id;
	mpc_lowcomm_communicator_t remote_id;

	/** Collective context */
	struct sctk_comm_coll coll;

} sctk_internal_communicator_t;

/********************************* FUNCTION *********************************/
void _mpc_comm_set_internal_coll ( const mpc_lowcomm_communicator_t id, struct mpc_lowcomm_coll_s *tmp );

int _mpc_comm_exists(const mpc_lowcomm_communicator_t communicator);

void sctk_communicator_world_init ( int task_nb );
void sctk_communicator_self_init();
void sctk_communicator_delete();

int mpc_lowcomm_communicator_remote_world_rank ( const mpc_lowcomm_communicator_t communicator, const int rank );
int _mpc_lowcomm_communicator_world_first_local_task ( const mpc_lowcomm_communicator_t communicator );

int _mpc_lowcomm_communicator_world_rank ( const mpc_lowcomm_communicator_t communicator, const int rank );

static inline int mpc_lowcomm_communicator_world_rank ( const mpc_lowcomm_communicator_t communicator, const int rank )
{
	if ( communicator == MPC_COMM_WORLD )
	{
		return rank;
	}
	else
	{
		return _mpc_lowcomm_communicator_world_rank( communicator, rank );
	}
}

int _mpc_lowcomm_communicator_world_last_local_task ( const mpc_lowcomm_communicator_t communicator );
int mpc_lowcomm_communicator_remote_size ( const mpc_lowcomm_communicator_t communicator );
int mpc_lowcomm_communicator_local_task_count ( const mpc_lowcomm_communicator_t communicator );
int mpc_lowcomm_communicator_size ( const mpc_lowcomm_communicator_t communicator );
int mpc_lowcomm_communicator_in_left_group ( const mpc_lowcomm_communicator_t id );
int sctk_get_remote_leader ( const mpc_lowcomm_communicator_t );
int sctk_get_local_leader ( const mpc_lowcomm_communicator_t );

/************************* FUNCTION ************************/
/**
 * This method check if the communicator is an intercommunicator.
 * @param communicator given communicator.
 * @return 1 if it is, 0 if it is not.
**/

int __mpc_lowcomm_communicator_is_intercomm( const mpc_lowcomm_communicator_t );

static inline int mpc_lowcomm_communicator_is_intercomm( const mpc_lowcomm_communicator_t communicator )
{
	if ( communicator == MPC_COMM_WORLD )
	{
		return 0;
	}

	static __thread int last_comm = -2;
	static __thread int last_val = -1;

	if ( last_comm == communicator )
	{
		return last_val;
	}

	last_comm = communicator;
	last_val = __mpc_lowcomm_communicator_is_intercomm( communicator );
	return last_val;
}

/************************* FUNCTION ************************/
/**
 * This method check if the communicator is limited to a shared-memory space.
 * @param communicator given communicator.
 * @return 1 if it is, 0 if it is not.
**/

int __mpc_lowcomm_communicator_is_shared_mem( const mpc_lowcomm_communicator_t communicator );

static inline int mpc_lowcomm_communicator_is_shared_mem( const mpc_lowcomm_communicator_t communicator )
{
	static __thread int last_comm = -2;
	static __thread int last_val = -1;

	if ( last_comm == communicator )
	{
		return last_val;
	}

	last_comm = communicator;
	last_val = __mpc_lowcomm_communicator_is_shared_mem( communicator );
	// mpc_common_debug_error("%d == %d", communicator, tmp->is_shared_mem );
	return last_val;
}


int __mpc_lowcomm_communicator_is_shared_node( const mpc_lowcomm_communicator_t communicator );

static inline int mpc_lowcomm_communicator_is_shared_node( const mpc_lowcomm_communicator_t communicator )
{
	static __thread int last_comm = -2;
	static __thread int last_val = -1;

	if ( last_comm == communicator )
	{
		return last_val;
	}

	last_comm = communicator;
	last_val = __mpc_lowcomm_communicator_is_shared_node( communicator );
	return last_val;
}



int sctk_is_in_group ( const mpc_lowcomm_communicator_t communicator );
int mpc_lowcomm_communicator_rank_of ( const mpc_lowcomm_communicator_t communicator, const int comm_world_rank );
int sctk_get_node_rank_from_task_rank ( const int rank );

int _mpc_lowcomm_group_process_rank_from_world ( int rank );


static inline int mpc_lowcomm_group_process_rank_from_world( int rank )
{
#ifdef MPC_IN_PROCESS_MODE
	return rank;
#endif

	if ( mpc_common_get_process_count() == 1 )
	{
		return 0;
	}

	return _mpc_lowcomm_group_process_rank_from_world( rank );
}


int sctk_get_comm_number();

int mpc_lowcomm_communicator_get_process_count ( const mpc_lowcomm_communicator_t communicator );
int *mpc_lowcomm_communicator_get_process_list ( const mpc_lowcomm_communicator_t communicator );

mpc_lowcomm_communicator_t sctk_get_peer_comm ( const mpc_lowcomm_communicator_t communicator );
mpc_lowcomm_communicator_t mpc_lowcomm_communicator_create ( const mpc_lowcomm_communicator_t origin_communicator, const int nb_task_involved, const int *task_list );
mpc_lowcomm_communicator_t mpc_lowcomm_communicator_intercomm_create ( const mpc_lowcomm_communicator_t local_comm, const int local_leader, const mpc_lowcomm_communicator_t peer_comm, const int remote_leader,
        const int tag, const int first );
mpc_lowcomm_communicator_t sctk_duplicate_communicator ( const mpc_lowcomm_communicator_t origin_communicator, int is_inter_comm, int rank );
mpc_lowcomm_communicator_t mpc_lowcomm_communicator_get_local ( const mpc_lowcomm_communicator_t communicator );
mpc_lowcomm_communicator_t sctk_delete_communicator ( const mpc_lowcomm_communicator_t );
mpc_lowcomm_communicator_t mpc_lowcomm_communicator_create_from_intercomm ( const mpc_lowcomm_communicator_t origin_communicator, const int nb_task_involved, const int *task_list );
mpc_lowcomm_communicator_t mpc_lowcomm_communicator_intercomm_create_from_intercommunicator ( const mpc_lowcomm_communicator_t origin_communicator, int remote_leader, int local_com );
struct mpc_lowcomm_coll_s *_mpc_comm_get_internal_coll ( const mpc_lowcomm_communicator_t communicator );

/**
 * @brief Check if a communicator is valid
 * 
 * @param communicator comm to check for existency
 * @return int 1 if exists 0 if not
 */
int mpc_lowcomm_communicator_exists(const mpc_lowcomm_communicator_t communicator);

#endif

#endif
