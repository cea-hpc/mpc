#ifndef MPC_LOWCOMM_COMMUNICATOR_H
#define MPC_LOWCOMM_COMMUNICATOR_H

#ifdef __cplusplus
extern "C" {
#endif

#include <mpc_lowcomm_group.h>
#include <mpc_common_types.h>

/*********************************
* PUBLIC TYPE FOR COMMUNICATORS *
*********************************/

struct mpc_lowcomm_internal_communicator_s;
typedef struct mpc_lowcomm_internal_communicator_s *mpc_lowcomm_communicator_t;

/************************
* COMMON COMMUNICATORS *
************************/

/** define the ID of a NULL communicator */
#define MPC_LOWCOMM_COMM_NULL_ID     0
/** This is the id of COMM world */
#define MPC_LOWCOMM_COMM_WORLD_ID    1
/** This is the ID of COMM self */
#define MPC_LOWCOMM_COMM_SELF_ID     2
/** This is the wildcard for COMMs to be used as ANY source/tag */
#define MPC_ANY_COMM                 3

/**
 * @brief Get a pointer to COMM_WORLD
 *
 * @return mpc_lowcomm_communicator_t pointer to world
 */
mpc_lowcomm_communicator_t mpc_lowcomm_communicator_world();

/**
 * @brief Get a pointer to COMM_SELF
 *
 * @return mpc_lowcomm_communicator_t pointer to comm_self
 */
mpc_lowcomm_communicator_t mpc_lowcomm_communicator_self();

/** define the MPI_COMM_WORLD internal communicator number **/
#define MPC_COMM_WORLD    (mpc_lowcomm_communicator_world() )
/** define the MPI_COMM_SELF internal communicator number **/
#define MPC_COMM_SELF     (mpc_lowcomm_communicator_self() )
/** Define the NULL communicator number */
#define MPC_COMM_NULL     (NULL)

/**************
* ID FACTORY *
**************/

/**
 * @brief Get a communicator from its integer identifier
 *
 * @param id iteger identifier of the comm
 * @return mpc_lowcomm_communicator_t pointer to the comm NULL is not existing
 */
mpc_lowcomm_communicator_t mpc_lowcomm_get_communicator_from_id(uint32_t id);

/**
 * @brief Get the numerical identifier of a communicator
 *
 * @param comm communicator to get the ID from
 * @return uint32_t Communicator ID (MPC_LOWCOMM_COMM_NULL_ID if error)
 */
uint32_t mpc_lowcomm_communicator_id(mpc_lowcomm_communicator_t comm);

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
 * @brief Create a communicator from a group object
 *
 * @param comm Source communicator
 * @param group Group of the new communicator
 * @return mpc_lowcomm_communicator_t new communicator (if member or MPI_COMM_NULL if not)
 */
mpc_lowcomm_communicator_t mpc_lowcomm_communicator_from_group(mpc_lowcomm_communicator_t comm,
                                                               mpc_lowcomm_group_t *group);

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
 * @return int SCTK_SUCCESS if no error
 */
int mpc_lowcomm_communicator_free(mpc_lowcomm_communicator_t *pcomm);

/* Query */

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
                                        const int lookup_cw_rank);

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
int mpc_lowcomm_communicator_in_left_group_rank(const mpc_lowcomm_communicator_t communicator, int comm_world_rank);

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
int mpc_lowcomm_communicator_in_right_group_rank(const mpc_lowcomm_communicator_t communicator, int comm_world_rank);

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
 * @param lookup_cw_rank the rank to check for membership in the comm
 * @return mpc_lowcomm_communicator_t the local comm for lookup_cw_rank
 */
mpc_lowcomm_communicator_t mpc_lowcomm_communicator_get_local_as(mpc_lowcomm_communicator_t comm,
                                                                 int lookup_cw_rank);
                                                            
/**
 * @brief Get the remote communicator (on intracomm returns intracomm)
 * 
 * @param comm the communicator to querry
 * @return mpc_lowcomm_communicator_t the remote comm for current rank
 */
mpc_lowcomm_communicator_t mpc_lowcomm_communicator_get_remote(mpc_lowcomm_communicator_t comm);

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

/***********************************
* COMMUNICATOR COLLECTIVE CONTEXT *
***********************************/

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

#ifdef __cplusplus
}
#endif

#endif /* MPC_LOWCOMM_COMMUNICATOR_H */
