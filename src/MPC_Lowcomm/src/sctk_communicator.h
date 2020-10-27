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

#include <mpc_lowcomm_types.h>
#include <mpc_common_asm.h>
#include <mpc_common_spinlock.h>

#include <mpc_common_rank.h>

#include "mpc_runtime_config.h"

/******************************** STRUCTURE *********************************/
struct mpc_lowcomm_coll_s;

struct shared_mem_barrier_sig
{
	OPA_ptr_t *sig_points;
	volatile int *tollgate;
	OPA_int_t fare;
	OPA_int_t counter;
};

int sctk_shared_mem_barrier_sig_init( struct shared_mem_barrier_sig *shmb,
                                      int nb_task );
int sctk_shared_mem_barrier_sig_release( struct shared_mem_barrier_sig *shmb );

struct shared_mem_barrier
{
	OPA_int_t counter;
	OPA_int_t phase;
};

int sctk_shared_mem_barrier_init( struct shared_mem_barrier *shmb, int nb_task );
int sctk_shared_mem_barrier_release( struct shared_mem_barrier *shmb );

#define SHM_COLL_BUFF_MAX_SIZE 1024

union shared_mem_buffer
{
	float f[SHM_COLL_BUFF_MAX_SIZE];
	int i[SHM_COLL_BUFF_MAX_SIZE];
	double d[SHM_COLL_BUFF_MAX_SIZE];
	char c[SHM_COLL_BUFF_MAX_SIZE];
};

struct shared_mem_reduce
{
	volatile int *tollgate;
	OPA_int_t fare;
	mpc_common_spinlock_t *buff_lock;
	OPA_int_t owner;
	OPA_int_t left_to_push;
	volatile void *target_buff;
	union shared_mem_buffer *buffer;
	int pipelined_blocks;
};

int sctk_shared_mem_reduce_init( struct shared_mem_reduce *shmr, int nb_task );
int sctk_shared_mem_reduce_release( struct shared_mem_reduce *shmr );

struct shared_mem_bcast
{
	OPA_int_t owner;
	OPA_int_t left_to_pop;

	volatile int *tollgate;
	OPA_int_t fare;

	union shared_mem_buffer buffer;

	OPA_ptr_t to_free;
	void *target_buff;
	int scount;
	size_t stype_size;
	int root_in_buff;
};

int sctk_shared_mem_bcast_init( struct shared_mem_bcast *shmb, int nb_task );
int sctk_shared_mem_bcast_release( struct shared_mem_bcast *shmb );

struct shared_mem_gatherv
{
	OPA_int_t owner;
	OPA_int_t left_to_push;

	volatile int *tollgate;
	OPA_int_t fare;

	/* Leaf infos */
	OPA_ptr_t *src_buffs;

	/* Root infos */
	void *target_buff;
	const int *counts;
	int *send_count;
	mpc_lowcomm_datatype_t recv_type;
	mpc_lowcomm_datatype_t *send_types;
	size_t *send_type_size;
	const int *disps;
	size_t rtype_size;
	int rcount;
	int let_me_unpack;
};

int sctk_shared_mem_gatherv_init( struct shared_mem_gatherv *shmgv, int nb_task );
int sctk_shared_mem_gatherv_release( struct shared_mem_gatherv *shmgv );

struct shared_mem_scatterv
{
	OPA_int_t owner;
	OPA_int_t left_to_pop;

	volatile int *tollgate;
	OPA_int_t fare;

	/* Root infos */
	OPA_ptr_t *src_buffs;
	int was_packed;
	size_t stype_size;
	int *counts;
	int *disps;
};

int sctk_shared_mem_scatterv_init( struct shared_mem_scatterv *shmgv,
                                   int nb_task );
int sctk_shared_mem_scatterv_release( struct shared_mem_scatterv *shmgv );

struct sctk_shared_mem_a2a_infos
{
	size_t stype_size;
	const void *source_buff;
	void **packed_buff;
	const int *disps;
	const int *counts;
};

struct shared_mem_a2a
{
	struct sctk_shared_mem_a2a_infos **infos;
	volatile int has_in_place;
};

int sctk_shared_mem_a2a_init( struct shared_mem_a2a *shmaa, int nb_task );
int sctk_shared_mem_a2a_release( struct shared_mem_a2a *shmaa );


/**
 *  \brief This structure describes the pool allocated context for node local coll
 */

struct sctk_per_node_comm_context
{
	struct shared_mem_barrier shm_barrier;
};


struct sctk_comm_coll
{
	int init_done;
	volatile int *coll_id;
	struct shared_mem_barrier shm_barrier;
	struct shared_mem_barrier_sig shm_barrier_sig;
	int reduce_interleave;
	struct shared_mem_reduce *shm_reduce;
	int bcast_interleave;
	struct shared_mem_bcast *shm_bcast;
	struct shared_mem_gatherv shm_gatherv;
	struct shared_mem_scatterv shm_scatterv;
	struct shared_mem_a2a shm_a2a;
	int comm_size;
	struct sctk_per_node_comm_context *node_ctx;
};

int sctk_comm_coll_init( struct sctk_comm_coll *coll, int nb_task );
int sctk_comm_coll_release( struct sctk_comm_coll *coll );

struct sctk_comm_coll *
__sctk_communicator_get_coll( const mpc_lowcomm_communicator_t communicator );

static inline struct sctk_comm_coll *
sctk_communicator_get_coll( const mpc_lowcomm_communicator_t communicator )
{
	static __thread mpc_lowcomm_communicator_t saved_comm = -2;
	static __thread struct sctk_comm_coll *saved_coll = NULL;

	if ( saved_comm == communicator )
	{
		return saved_coll;
	}

	saved_coll = __sctk_communicator_get_coll( communicator );
	saved_comm = communicator;
	return saved_coll;
}

static inline int __sctk_comm_coll_get_id( struct sctk_comm_coll *coll,
        int rank )
{
	return coll->coll_id[rank]++;
}

static inline int sctk_comm_coll_get_id_red( struct sctk_comm_coll *coll,
        int rank )
{
	return __sctk_comm_coll_get_id( coll, rank ) & ( coll->reduce_interleave - 1 );
}

static inline struct shared_mem_reduce *sctk_comm_coll_get_red( struct sctk_comm_coll *coll, __UNUSED__ int rank )
{
	int xid = sctk_comm_coll_get_id_red(coll, rank);
	return &coll->shm_reduce[xid];
}

static inline int sctk_comm_coll_get_id_bcast( struct sctk_comm_coll *coll,
        int rank )
{
	return __sctk_comm_coll_get_id( coll, rank ) & ( coll->bcast_interleave - 1 );
}

static inline struct shared_mem_bcast *sctk_comm_coll_get_bcast( struct sctk_comm_coll *coll, __UNUSED__ int rank )
{
	int xid = sctk_comm_coll_get_id_bcast(coll, rank);
	return &coll->shm_bcast[xid];
}



int sctk_per_node_comm_context_init( struct sctk_per_node_comm_context *ctx,
                                     mpc_lowcomm_communicator_t comm, int nb_task );

int sctk_per_node_comm_context_release( struct sctk_per_node_comm_context *ctx );







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

int sctk_get_remote_comm_world_rank ( const mpc_lowcomm_communicator_t communicator, const int rank );
int sctk_get_first_task_local ( const mpc_lowcomm_communicator_t communicator );

int _sctk_get_comm_world_rank ( const mpc_lowcomm_communicator_t communicator, const int rank );

static inline int sctk_get_comm_world_rank ( const mpc_lowcomm_communicator_t communicator, const int rank )
{
	if ( communicator == MPC_COMM_WORLD )
	{
		return rank;
	}
	else
	{
		return _sctk_get_comm_world_rank( communicator, rank );
	}
}

int sctk_get_last_task_local ( const mpc_lowcomm_communicator_t communicator );
int mpc_lowcomm_communicator_remote_size ( const mpc_lowcomm_communicator_t communicator );
int sctk_get_nb_task_local ( const mpc_lowcomm_communicator_t communicator );
int mpc_lowcomm_communicator_size ( const mpc_lowcomm_communicator_t communicator );
int sctk_is_in_local_group ( const mpc_lowcomm_communicator_t id );
int sctk_get_remote_leader ( const mpc_lowcomm_communicator_t );
int sctk_get_local_leader ( const mpc_lowcomm_communicator_t );

/************************* FUNCTION ************************/
/**
 * This method check if the communicator is an intercommunicator.
 * @param communicator given communicator.
 * @return 1 if it is, 0 if it is not.
**/

int __sctk_is_inter_comm( const mpc_lowcomm_communicator_t );

static inline int sctk_is_inter_comm( const mpc_lowcomm_communicator_t communicator )
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
	last_val = __sctk_is_inter_comm( communicator );
	return last_val;
}

/************************* FUNCTION ************************/
/**
 * This method check if the communicator is limited to a shared-memory space.
 * @param communicator given communicator.
 * @return 1 if it is, 0 if it is not.
**/

int __sctk_is_shared_mem( const mpc_lowcomm_communicator_t communicator );

static inline int sctk_is_shared_mem( const mpc_lowcomm_communicator_t communicator )
{
	static __thread int last_comm = -2;
	static __thread int last_val = -1;

	if ( last_comm == communicator )
	{
		return last_val;
	}

	last_comm = communicator;
	last_val = __sctk_is_shared_mem( communicator );
	// mpc_common_debug_error("%d == %d", communicator, tmp->is_shared_mem );
	return last_val;
}


int __sctk_is_shared_node( const mpc_lowcomm_communicator_t communicator );

static inline int sctk_is_shared_node( const mpc_lowcomm_communicator_t communicator )
{
	static __thread int last_comm = -2;
	static __thread int last_val = -1;

	if ( last_comm == communicator )
	{
		return last_val;
	}

	last_comm = communicator;
	last_val = __sctk_is_shared_node( communicator );
	return last_val;
}



int sctk_is_in_group ( const mpc_lowcomm_communicator_t communicator );
int mpc_lowcomm_communicator_rank ( const mpc_lowcomm_communicator_t communicator, const int comm_world_rank );
int sctk_get_node_rank_from_task_rank ( const int rank );

int _sctk_get_process_rank_from_task_rank ( int rank );


static inline int sctk_get_process_rank_from_task_rank( int rank )
{
#ifdef MPC_IN_PROCESS_MODE
	return rank;
#endif

	if ( mpc_common_get_process_count() == 1 )
	{
		return 0;
	}

	return _sctk_get_process_rank_from_task_rank( rank );
}


int sctk_get_comm_number();

int sctk_get_process_nb_in_array ( const mpc_lowcomm_communicator_t communicator );
int *sctk_get_process_array ( const mpc_lowcomm_communicator_t communicator );

mpc_lowcomm_communicator_t sctk_get_peer_comm ( const mpc_lowcomm_communicator_t communicator );
mpc_lowcomm_communicator_t sctk_create_communicator ( const mpc_lowcomm_communicator_t origin_communicator, const int nb_task_involved, const int *task_list );
mpc_lowcomm_communicator_t sctk_create_intercommunicator ( const mpc_lowcomm_communicator_t local_comm, const int local_leader, const mpc_lowcomm_communicator_t peer_comm, const int remote_leader,
        const int tag, const int first );
mpc_lowcomm_communicator_t sctk_duplicate_communicator ( const mpc_lowcomm_communicator_t origin_communicator, int is_inter_comm, int rank );
mpc_lowcomm_communicator_t sctk_get_local_comm_id ( const mpc_lowcomm_communicator_t communicator );
mpc_lowcomm_communicator_t sctk_delete_communicator ( const mpc_lowcomm_communicator_t );
mpc_lowcomm_communicator_t sctk_create_communicator_from_intercomm ( const mpc_lowcomm_communicator_t origin_communicator, const int nb_task_involved, const int *task_list );
mpc_lowcomm_communicator_t sctk_create_intercommunicator_from_intercommunicator ( const mpc_lowcomm_communicator_t origin_communicator, int remote_leader, int local_com );
struct mpc_lowcomm_coll_s *_mpc_comm_get_internal_coll ( const mpc_lowcomm_communicator_t communicator );

/**
 * @brief Check if a communicator is valid
 * 
 * @param communicator comm to check for existency
 * @return int 1 if exists 0 if not
 */
int _mpc_lowcomm_comm_exists(const mpc_lowcomm_communicator_t communicator);

#endif
