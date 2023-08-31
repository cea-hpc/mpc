/* ############################# MPC License ############################## */
/* # Thu May  6 10:26:16 CEST 2021                                        # */
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
/* # Maintainers:                                                         # */
/* # - CARRIBAULT Patrick patrick.carribault@cea.fr                       # */
/* # - JAEGER Julien julien.jaeger@cea.fr                                 # */
/* # - PERACHE Marc marc.perache@cea.fr                                   # */
/* # - ROUSSEL Adrien adrien.roussel@cea.fr                               # */
/* # - TABOADA Hugo hugo.taboada@cea.fr                                   # */
/* #                                                                      # */
/* # Authors:                                                             # */
/* # - BESNARD Jean-Baptiste jbbesnard@paratools.fr                       # */
/* #                                                                      # */
/* ######################################################################## */
#ifndef MPC_LOWCOMM_COMMUNICATOR_H
#define MPC_LOWCOMM_COMMUNICATOR_H

#ifdef __cplusplus
extern "C" {
#endif

#include <mpc_lowcomm_group.h>
#include <mpc_common_types.h>
#include <mpc_lowcomm_monitor.h>

/*********************************
* PUBLIC TYPE FOR COMMUNICATORS *
*********************************/

typedef struct MPI_ABI_Comm * mpc_lowcomm_communicator_t;
typedef uint64_t              mpc_lowcomm_communicator_id_t;


/************************
* COMMON COMMUNICATORS *
************************/

/** This is the ID of COMM self */
#define MPC_LOWCOMM_COMM_SELF_NUMERIC_ID     1
/** This is the id of COMM world */
#define MPC_LOWCOMM_COMM_WORLD_NUMERIC_ID    2

/* Expose communicator init needed for unit test */
void _mpc_lowcomm_communicator_init(void);
void _mpc_lowcomm_communicator_release(void);

/**
 * @brief Get COMM_WORLD identifier
 *
 * @return mpc_lowcomm_communicator_id_t comm_world communicator ID
 */
mpc_lowcomm_communicator_id_t mpc_lowcomm_get_comm_world_id(void);

/**
 * @brief Get COMM_SELF id
 *
 * @return mpc_lowcomm_communicator_id_t comm_self communicator ID
 */
mpc_lowcomm_communicator_id_t mpc_lowcomm_get_comm_self_id(void);

/**
 * @brief Book a local communicator ID
 *
 * @return mpc_lowcomm_communicator_id_t local communicator ID
 */
mpc_lowcomm_communicator_id_t mpc_lowcomm_communicator_gen_local_id(void);

/** define the ID of a NULL communicator */
#define MPC_LOWCOMM_COMM_NULL_ID     0
/** This is the id of COMM world */
#define MPC_LOWCOMM_COMM_WORLD_ID    (mpc_lowcomm_get_comm_world_id() )
/** This is the ID of COMM self */
#define MPC_LOWCOMM_COMM_SELF_ID     (mpc_lowcomm_get_comm_self_id() )
/** This is the wildcard for COMMs to be used as ANY source/tag */
#define MPC_ANY_COMM                 3

/**
 * @brief Get a pointer to COMM_WORLD
 *
 * @return mpc_lowcomm_communicator_t pointer to world
 */
mpc_lowcomm_communicator_t mpc_lowcomm_communicator_world();

/**
 * @brief Get the set id corresponding to a given comm ID
 *
 * @param comm_id the comm to querry
 * @return The set ID holding this comm
 */
mpc_lowcomm_set_uid_t mpc_lowcomm_get_comm_gid(mpc_lowcomm_communicator_id_t comm_id);


/**
 * @brief Compute the world ID for a remote set of processes
 *
 * @param gid the target group id
 * @return mpc_lowcomm_communicator_id_t the corresponding comm_world
 */
mpc_lowcomm_communicator_id_t mpc_lowcomm_get_comm_world_id_gid(mpc_lowcomm_set_uid_t gid);

/**
 * @brief Get a pointer to COMM_SELF
 *
 * @return mpc_lowcomm_communicator_t pointer to comm_self
 */
mpc_lowcomm_communicator_t mpc_lowcomm_communicator_self();

/** Define the NULL communicator number */
#define MPC_COMM_NULL     ( (mpc_lowcomm_communicator_t)0)
/** define the MPI_COMM_SELF internal communicator number **/
#define MPC_COMM_SELF     ( (mpc_lowcomm_communicator_t)1)
/** define the MPI_COMM_WORLD internal communicator number **/
#define MPC_COMM_WORLD    ( (mpc_lowcomm_communicator_t)2)

/** @brief Converts a generic comm handle in its predefined equivalent
 *
 *  @param comm    The handle to convert
 *  @return        The predefined value of the handle or the handle itself otherwise
 */
static inline mpc_lowcomm_communicator_t __mpc_lowcomm_communicator_to_predefined(const mpc_lowcomm_communicator_t comm)
{
	// Already predefined
	if(comm < (mpc_lowcomm_communicator_t)4096)
	{
		return comm;
	}
	if(comm == mpc_lowcomm_communicator_world() )
	{
		return MPC_COMM_WORLD;
	}
	if(comm == mpc_lowcomm_communicator_self() )
	{
		return MPC_COMM_SELF;
	}

	// Default return the entry
	return comm;
}

/** @brief Converts a predefined comm handle into its generic address
 *
 *  @param comm    The handle to convert
 *  @return        A pointer on the communicator
 */
static inline mpc_lowcomm_communicator_t __mpc_lowcomm_communicator_from_predefined(const mpc_lowcomm_communicator_t comm)
{
	// Already an non predefined handle
	if(comm >= (mpc_lowcomm_communicator_t)4096)
	{
		return comm;
	}
	if(comm == MPC_COMM_WORLD)
	{
		return mpc_lowcomm_communicator_world();
	}
	if(comm == MPC_COMM_SELF)
	{
		return mpc_lowcomm_communicator_self();
	}

	// Default return the entry
	return comm;
}

/**************
* ID FACTORY *
**************/

/**
 * @brief Get a communicator from its integer identifier
 *
 * @param id iteger identifier of the comm
 * @return mpc_lowcomm_communicator_t pointer to the comm NULL is not existing
 */
mpc_lowcomm_communicator_t mpc_lowcomm_get_communicator_from_id(mpc_lowcomm_communicator_id_t id);

/**
 * @brief Get the numerical identifier of a communicator
 *
 * @param comm communicator to get the ID from
 * @return uint32_t Communicator ID (MPC_LOWCOMM_COMM_NULL_ID if error)
 */
mpc_lowcomm_communicator_id_t mpc_lowcomm_communicator_id(mpc_lowcomm_communicator_t comm);


/**
 * @brief Retrieve a local communicator from its linear id (useful for FORTRAN)
 *
 * @param id the linear id to use
 * @return mpc_lowcomm_communicator_t pointer to the comm NULL is not existing
 */
mpc_lowcomm_communicator_t mpc_lowcomm_get_communicator_from_linear_id(int linear_id);

/**
 * @brief Return a  linear integer ID for comm (only valid locally)
 *        mostly useful for FORTRAN
 *
 * @param comm communicator to translate
 * @return int linear local id
 */
int mpc_lowcomm_communicator_linear_id(mpc_lowcomm_communicator_t comm);


int mpc_lowcomm_communicator_scan(void (*callback)(mpc_lowcomm_communicator_t comm, void *arg), void *arg);

/**************
* INTRACOMMS *
**************/

/* Constructors & Desctructors */

/**
 * @brief Create a communicator from another communicator and comm_world ranks
 *
 * @param comm comm to build from
 * @param size size of the new comm (list)
 * @param members list of members in the new comm (in comm_world)
 * @return mpc_lowcomm_communicator_t new communicator (if member or MPI_COMM_NULL if not)
 */
mpc_lowcomm_communicator_t mpc_lowcomm_communicator_create(const mpc_lowcomm_communicator_t comm,
                                                           const int size,
                                                           const int *members);

/**
 * @brief Create a communicator from a comm subgroup
 *
 * @param comm the communicator to use as source
 * @param group the subgroup to create on
 * @param tag the tag to use for ID exchange
 * @param newcomm the new communicator
 * @return int MPC_LOWCOMM_SUCCESS if all OK
 */
int mpc_lowcomm_communicator_create_group(mpc_lowcomm_communicator_t comm,
                                          mpc_lowcomm_group_t *group,
                                          int tag,
                                          mpc_lowcomm_communicator_t *newcomm);

/**
 * @brief Create a communicator from a group object
 *
 * @param comm Source communicator
 * @param group Group of the new communicator
 * @return mpc_lowcomm_communicator_t new communicator (if member or MPI_COMM_NULL if not)
 */
mpc_lowcomm_communicator_t mpc_lowcomm_communicator_from_group(mpc_lowcomm_communicator_t comm,
                                                               mpc_lowcomm_group_t *group);

/**
 * @brief Creare a communicator with a group using a pre-exchanged ID (see @ref mpc_lowcomm_communicator_gen_local_id)
 *
 * @param comm the comm to rely on to build the new comm
 * @param group the group to use for exchange
 * @param forced_id the ID to rely on (same as mpc_lowcomm_communicator_from_group if MPC_LOWCOMM_COMM_NULL_ID)
 * @return mpc_lowcomm_communicator_t  new communicator (if member or MPI_COMM_NULL if not)
 */
mpc_lowcomm_communicator_t mpc_lowcomm_communicator_from_group_forced_id(mpc_lowcomm_group_t *group,
                                                                         mpc_lowcomm_communicator_id_t forced_id);


/**
 * @brief Get the group associated with the communicator (must be intracomm)
 *
 * @param comm communicator to get the group of
 * @return mpc_lowcomm_group_t* the group of this communicator (to free)
 */
mpc_lowcomm_group_t * mpc_lowcomm_communicator_group(mpc_lowcomm_communicator_t comm);

/**
 * @brief Duplicate a communicator
 *
 * @param comm Source communicator
 * @return mpc_lowcomm_communicator_t new duplicated communicator
 */
mpc_lowcomm_communicator_t mpc_lowcomm_communicator_dup(mpc_lowcomm_communicator_t comm);

/**
 * @brief Split a communicator using color and key
 *
 * @param comm Communicator to be split
 * @param color Color determining the group to be member of (can be MPC_UNDEFINED)
 * @param key Value determining the ordering of the new group
 * @return mpc_lowcomm_communicator_t Split communicator or MPC_COMM_NULL if no comm
 */
mpc_lowcomm_communicator_t mpc_lowcomm_communicator_split(mpc_lowcomm_communicator_t comm, int color, int key);

/**
 * @brief Free a communicator
 *
 * @param pcomm pointer to a communicator (set to MPC_COMM_NULL)
 * @return int MPC_LOWCOMM_SUCCESS if no error
 */
int mpc_lowcomm_communicator_free(mpc_lowcomm_communicator_t *pcomm);

/* Query */

/**
 * @brief Check if a given communicator contains a given UID
 *
 * @param comm the comm to check
 * @param uid the UID to check
 * @return int true if UID is part of the comm
 */
int mpc_lowcomm_communicator_contains(const mpc_lowcomm_communicator_t comm,
                                      const mpc_lowcomm_peer_uid_t uid);

/**
 * @brief Get rank in communicator for current process
 *
 * @param comm communicator to query
 * @return int rank in given communicator
 */
int mpc_lowcomm_communicator_rank(const mpc_lowcomm_communicator_t comm);

/**
 * @brief Get size of a given communicator
 *
 * @param comm communicator to get the size of
 * @return int size of the target communicator
 */
int mpc_lowcomm_communicator_size(const mpc_lowcomm_communicator_t comm);

/**
 * @brief Check is a communicator does exist using its pointer
 *
 * @param comm Communicator to be checked for existency
 * @return int 1 if it does exists 0 otherwise
 */
int mpc_lowcomm_communicator_exists(mpc_lowcomm_communicator_t comm);

/**
 * @brief Return the rank (in comm) of the process leading this group in this UNIX process
 *
 * @param comm communicator to query
 * @return int rank of the first MPI process in this UNIX process
 */
int mpc_lowcomm_communicator_local_lead(mpc_lowcomm_communicator_t comm);

/**
 * @brief Print a communicator for debug
 *
 * @param comm communicator to print
 * @param root_only If true print only on the root
 */
void mpc_lowcomm_communicator_print(mpc_lowcomm_communicator_t comm, int root_only);

/* Accessors */

/**
 * @brief Get a rank in comm for a given comm_world rank
 *
 * @param comm communicator to get the rank from
 * @param comm_world_rank world rank to be resolved
 * @return int rank in comm for comm_world_rank (MPC_PROC_NULL if not in comm)
 */
int mpc_lowcomm_communicator_rank_of(const mpc_lowcomm_communicator_t comm, const int comm_world_rank);

/**
 * @brief Get a rank in group from another process point of view (useful for intercomms)
 *
 * @param comm communicator to be queried
 * @param comm_world_rank world rank to be resolved in group
 * @param lookup_cw_rank which rank to adop the point of view of
 * @return int
 */
int mpc_lowcomm_communicator_rank_of_as(const mpc_lowcomm_communicator_t comm,
                                        const int comm_world_rank,
                                        const int lookup_cw_rank,
                                        mpc_lowcomm_peer_uid_t lookup_uid);

/**
 * @brief Get comm world rank for a given communicator rank
 *
 * @param comm communicator to resolve in
 * @param rank rank in communicator to resolve
 * @return int rank in comm world for 'rank'
 */
int mpc_lowcomm_communicator_world_rank_of(const mpc_lowcomm_communicator_t comm, int rank);

/**
 * @brief Get the number of tasks in current UNIX process
 *
 * @param comm target communicator
 * @return int number of tasks in current UNIX process
 */
int mpc_lowcomm_communicator_local_task_count(mpc_lowcomm_communicator_t comm);

/**
 * @brief Get the number of processes involved in the communicator
 *
 * @param comm target communicator
 * @return int number of processes involved
 */
int mpc_lowcomm_communicator_get_process_count(const mpc_lowcomm_communicator_t comm);

/**
 * @brief Get the list of process involved in the communicator
 *
 * @param comm target communicator
 * @return int* list of processes
 */
int *mpc_lowcomm_communicator_get_process_list(const mpc_lowcomm_communicator_t comm);

/**
 * @brief Get the process UID for a given rank in local comm
 *
 * @param comm the communicator to look into
 * @param rank the task rank to target (in comm)
 * @return mpc_lowcomm_peer_uid_t the corresponding process UID
 */
mpc_lowcomm_peer_uid_t mpc_lowcomm_communicator_uid(const mpc_lowcomm_communicator_t comm, int rank);

/**
 * @brief Get the process UID for a given rank in remote comm
 *
 * @param comm the communicator to look into
 * @param rank the task rank to target
 * @return mpc_lowcomm_peer_uid_t the corresponding process UID
 */
mpc_lowcomm_peer_uid_t mpc_lowcomm_communicator_remote_uid(const mpc_lowcomm_communicator_t comm, int rank);

/*********************
* INTERCOMM SUPPORT *
*********************/

/* Constructor */

/**
 * @brief Create an intercommunicator
 *
 * @param local_comm Comm which current process is part of
 * @param local_leader Leader in local_coll (rank in local_comm)
 * @param peer_comm Communicator used by leaders to exchange data
 * @param remote_leader Leader in remote comm (rank in peer_comm)
 * @param tag Tag to be used between leaders on peer_comm
 * @return mpc_lowcomm_communicator_t New intercomm
 */
mpc_lowcomm_communicator_t mpc_lowcomm_communicator_intercomm_create(const mpc_lowcomm_communicator_t local_comm,
                                                                     const int local_leader,
                                                                     const mpc_lowcomm_communicator_t peer_comm,
                                                                     const int remote_leader,
                                                                     const int tag);

/**
 * @brief Merge an intercommunicator into an intra-comm
 *
 * @param intercomm The intercomm to be merged
 * @param high If one of the group sets it to high its ranks will be higher in the groups
 * @return mpc_lowcomm_communicator_t the corresponding intracomm
 */
mpc_lowcomm_communicator_t mpc_lowcomm_intercommunicator_merge(mpc_lowcomm_communicator_t intercomm, int high);


/* Query */

/**
 * @brief Check if a communicator is an intercomm
 *
 * @param comm the corresponding communicator
 * @return int one if this is an intercomm
 */
int mpc_lowcomm_communicator_is_intercomm(mpc_lowcomm_communicator_t comm);

/**
 * @brief Return one group of the two coherently in the two groups (master)
 *
 * @param communicator the inter-communicator to querry
 * @return int one in one of the two groups (coherently)
 */
int mpc_lowcomm_communicator_in_master_group(const mpc_lowcomm_communicator_t communicator);

/**
 * @brief Check if the rank is in the left group
 *
 * @param communicator intercomm to be checked
 * @return int true if in left group
 */
int mpc_lowcomm_communicator_in_left_group(const mpc_lowcomm_communicator_t communicator);

/**
 * @brief Check if a comm world rank is in left group
 *
 * @param communicator the intercomm to be checked
 * @param comm_world_rank the rank to be checked
 * @return int true if the rank is in left group
 */
int mpc_lowcomm_communicator_in_left_group_rank(const mpc_lowcomm_communicator_t communicator,
                                                int comm_world_rank,
                                                mpc_lowcomm_peer_uid_t uid);

/**
 * @brief Check if the rank is in the right group
 *
 * @param communicator intercomm to be checked
 * @return int true if in right group
 */
int mpc_lowcomm_communicator_in_right_group(const mpc_lowcomm_communicator_t communicator);

/**
 * @brief Check if a comm world rank is in right group
 *
 * @param communicator the intercomm to be checked
 * @param comm_world_rank the rank to be checked
 * @return int true if the rank is in right group
 */
int mpc_lowcomm_communicator_in_right_group_rank(const mpc_lowcomm_communicator_t communicator,
                                                 int comm_world_rank,
                                                 mpc_lowcomm_peer_uid_t uid);

/* Accessors */

/**
 * @brief Get the local communicator (on intracomm returns intracomm)
 *
 * @param comm the communicator to querry
 * @return mpc_lowcomm_communicator_t the local comm for current rank
 */
mpc_lowcomm_communicator_t mpc_lowcomm_communicator_get_local(mpc_lowcomm_communicator_t comm);

/**
 * @brief Get the local communicator (on intracomm returns intracomm) for a given rank
 *
 * @param comm the communicator to querry
 * @param lookup_cw_rank the rank to check for membership in the comm (in comm_world)
 * @return mpc_lowcomm_communicator_t the local comm for lookup_cw_rank
 */
mpc_lowcomm_communicator_t mpc_lowcomm_communicator_get_local_as(mpc_lowcomm_communicator_t comm,
                                                                 int lookup_cw_rank,
                                                                 mpc_lowcomm_peer_uid_t uid);

/**
 * @brief Get the remote communicator (on intracomm returns intracomm)
 *
 * @param comm the communicator to querry
 * @return mpc_lowcomm_communicator_t the remote comm for current rank
 */
mpc_lowcomm_communicator_t mpc_lowcomm_communicator_get_remote(mpc_lowcomm_communicator_t comm);

/**
 * @brief Get the remote communicator (on intracomm returns intracomm) from a given cw rank
 *
 * @param comm the communicator to querry
 * @param lookup_cw_rank the rank to check for membership in the comm (in comm_world)
 * @return mpc_lowcomm_communicator_t the remote comm for current rank
 */
mpc_lowcomm_communicator_t mpc_lowcomm_communicator_get_remote_as(mpc_lowcomm_communicator_t comm,
                                                                  int lookup_cw_rank,
                                                                  mpc_lowcomm_peer_uid_t uid);

/**
 * @brief Get the size for the remote comm
 *
 * @param comm intercomm to querry
 * @return int the size of the remote comm
 */
int mpc_lowcomm_communicator_remote_size(mpc_lowcomm_communicator_t comm);

/**
 * @brief Get world rank for a process in the remote comm
 *
 * @param communicator the intercomm to query
 * @param rank the remote rank
 * @return int the comm world rank for the remote rank
 */
int mpc_lowcomm_communicator_remote_world_rank(const mpc_lowcomm_communicator_t communicator, const int rank);

/*****************
* UNIVERSE COMM *
*****************/

/**
 * @brief Build a given communicator ID as seen by a remote peer
 *
 * @param remote remote peer aware of the communicator with given ID
 * @param id the ID of the communicator to build
 * @param outcomm the output communicator
 * @return int MPC_LOWCOMM_SUCCESS if all OK
 */
int mpc_lowcomm_communicator_build_remote(mpc_lowcomm_peer_uid_t remote,
                                          const mpc_lowcomm_communicator_id_t id,
                                          mpc_lowcomm_communicator_t *outcomm);

/**
 * @brief Build the remote comm_world as seen by the remote group
 *
 * @param gid the identifier of the group to be targetted
 * @param comm output communicator
 * @return int MPC_LOWCOMM_SUCCESS if all OK
 */
int mpc_lowcomm_communicator_build_remote_world(const mpc_lowcomm_set_uid_t gid,
                                                mpc_lowcomm_communicator_t *comm);

/***********************************
* COMMUNICATOR CONNECT AND ACCEPT *
***********************************/

int mpc_lowcomm_communicator_connect(const char *port_name,
                                     int root,
                                     mpc_lowcomm_communicator_t comm,
                                     mpc_lowcomm_communicator_t *new_comm);

int mpc_lowcomm_communicator_accept(const char *port_name,
                                    int root,
                                    mpc_lowcomm_communicator_t comm,
                                    mpc_lowcomm_communicator_t *new_comm);

/********************
* CONTEXT POINTERS *
********************/

/**
 * @brief Attach an extra context pointer to a communicator group
 *
 * @param comm the target communicator
 * @param ctxptr the context pointer to attach
 * @return int 0 on success
 */
int mpc_lowcomm_communicator_set_context_pointer(mpc_lowcomm_communicator_t comm, mpc_lowcomm_handle_ctx_t ctxptr);

/**
 * @brief Get the extra context pointer from the communicator object (it is inherited from parents)
 *
 * @param comm the communicator to querry
 * @return mpc_lowcomm_handle_ctx_t the context pointer (NULL if none)
 */
mpc_lowcomm_handle_ctx_t mpc_lowcomm_communicator_get_context_pointer(mpc_lowcomm_communicator_t comm);


/******************************
* HANDLE CONTEXT UNIFICATION *
******************************/

/**
 * @brief Initially all sessions have their own context this calls unify them with the same ID
 *        called in @ref PMPI_Comm_create_from_group
 *
 *  @note This call is defined in the handle_ctx.c file but depends on communicators
 *
 * @param comm the communicator to rely on to build unified context
 * @param hctx the context to unify
 * @return int 0 on success
 */
int mpc_lowcomm_communicator_handle_ctx_unify(mpc_lowcomm_communicator_t comm, mpc_lowcomm_handle_ctx_t hctx);

/**
 * @brief Get the identifier from the Handle CTX
 *
 *  @note This call is defined in the handle_ctx.c file but depends on communicators
 *
 * @param hctx the handle CTX to query
 * @return mpc_lowcomm_communicator_id_t the corresponding ID (MPC_LOWCOMM_COMM_NULL_ID if none or error)
 */
mpc_lowcomm_communicator_id_t mpc_lowcomm_communicator_handle_ctx_id(mpc_lowcomm_handle_ctx_t hctx);


/***********************************
* COMMUNICATOR COLLECTIVE CONTEXT *
***********************************/

/**
 * @brief Retrieve all infos about a comm in one call (fastpath for collectives)
 *
 * @param comm the communicator of interest
 * @param is_intercomm set to true if it is an intercomm
 * @param is_shm set to true if the comm resides in shared memory
 * @param is_shared_node set to true if the comm is on a single node
 * @return int MPC_LOWCOMM_SUCCESS if all OK
 */
int mpc_lowcomm_communicator_attributes(const mpc_lowcomm_communicator_t comm,
                                        int *is_intercomm,
                                        int *is_shm,
                                        int *is_shared_node);

/**
 * @brief Check if the communicator is in shared memory
 *
 * @param comm the communicator
 * @return int true if the comm is shared memory
 */
int mpc_lowcomm_communicator_is_shared_mem(const mpc_lowcomm_communicator_t comm);

/**
 * @brief Check if the communicator is on a single node
 *
 * @param comm the communicator
 * @return int true if it is in shared-node
 */
int mpc_lowcomm_communicator_is_shared_node(const mpc_lowcomm_communicator_t comm);

struct sctk_comm_coll;

/** Get the communicator collective context */
struct sctk_comm_coll *mpc_communicator_shm_coll_get(const mpc_lowcomm_communicator_t comm);


/*****************************
* TOPOLOGICAL COMMUNICATORS *
*****************************/

/* used to manage communicators in topological algorithm */
typedef struct mpc_hardware_split_info_s
{
	int                         highest_local_hardware_level;
	int                         deepest_hardware_level;
	mpc_lowcomm_communicator_t *hwcomm;            /* communicator of hardware splited topological level */
	mpc_lowcomm_communicator_t *rootcomm;          /* communicator of master node topological level */

	int **                      childs_data_count; /* For each topological level, an array containing the number of ranks under each rank of the same hwcomm of this level in the topological tree. */
	int *                       send_data_count;   /* For each topological level, the sum of the child_data_count_array. */

	int                         topo_rank;
	int *                       swap_array;         /* Reordering array used to link mpi ranks with the topology. */
	int *                       reverse_swap_array; /* Reverse reordering array used to link mpi ranks with the topology. */
}mpc_hardware_split_info_t;

/**
 * @brief Get the topological communicators associated with the root parameter.
 *
 * @param comm Target communicator.
 * @param root Target root.
 *
 * @return The topological communicators or NULL if not found.
 */
mpc_hardware_split_info_t * mpc_lowcomm_topo_comm_get(mpc_lowcomm_communicator_t comm, int root);

/**
 * @brief Set the topological communicators associated with the root parameter.
 *
 * @param comm Target communicator.
 * @param root Target root.
 */
void mpc_lowcomm_topo_comm_set(mpc_lowcomm_communicator_t comm, int root, mpc_hardware_split_info_t *hw_info);

#ifdef __cplusplus
}
#endif

#endif /* MPC_LOWCOMM_COMMUNICATOR_H */
