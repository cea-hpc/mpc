#ifndef MPC_LOWCOMM_COMMUNICATOR_H
#define MPC_LOWCOMM_COMMUNICATOR_H

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

#define MPC_LOWCOMM_COMM_NULL_ID     0
#define MPC_LOWCOMM_COMM_WORLD_ID    1
#define MPC_LOWCOMM_COMM_SELF_ID     2
#define MPC_ANY_COMM                 3

mpc_lowcomm_communicator_t mpc_lowcomm_communicator_world();
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

mpc_lowcomm_communicator_t mpc_lowcomm_get_communicator_from_id(uint32_t id);


/**************
* INTRACOMMS *
**************/

/* Constructors & Desctructors */


mpc_lowcomm_communicator_t mpc_lowcomm_communicator_create(const mpc_lowcomm_communicator_t comm,
                                                           const int size,
                                                           const int *members);

mpc_lowcomm_communicator_t mpc_lowcomm_communicator_from_group(mpc_lowcomm_communicator_t comm,
                                                               mpc_lowcomm_group_t *group);

mpc_lowcomm_communicator_t mpc_lowcomm_communicator_dup(mpc_lowcomm_communicator_t comm);

mpc_lowcomm_communicator_t mpc_lowcomm_communicator_split(mpc_lowcomm_communicator_t comm, int color, int key);

int mpc_lowcomm_communicator_free(mpc_lowcomm_communicator_t *pcomm);

/* Query */

int mpc_lowcomm_communicator_exists(mpc_lowcomm_communicator_t comm);
uint32_t mpc_lowcomm_communicator_id(mpc_lowcomm_communicator_t comm);
int mpc_lowcomm_communicator_local_lead(mpc_lowcomm_communicator_t comm);
void mpc_lowcomm_communicator_print(mpc_lowcomm_communicator_t comm,  int root_only);

/* Accessors */

int mpc_lowcomm_communicator_rank_of(const mpc_lowcomm_communicator_t comm, const int comm_world_rank);

int mpc_lowcomm_communicator_rank_of_as(const mpc_lowcomm_communicator_t comm,
										const int comm_world_rank,
										const int lookup_cw_rank);

int mpc_lowcomm_communicator_world_rank(const mpc_lowcomm_communicator_t comm, int rank);

int mpc_lowcomm_communicator_rank(const mpc_lowcomm_communicator_t comm);

int mpc_lowcomm_communicator_size(const mpc_lowcomm_communicator_t comm);

int mpc_lowcomm_communicator_local_task_count(mpc_lowcomm_communicator_t comm);

int mpc_lowcomm_communicator_get_process_count(const mpc_lowcomm_communicator_t comm);

int *mpc_lowcomm_communicator_get_process_list(const mpc_lowcomm_communicator_t comm);

/*********************
* INTERCOMM SUPPORT *
*********************/

/* Constructor */

mpc_lowcomm_communicator_t mpc_lowcomm_communicator_intercomm_create(const mpc_lowcomm_communicator_t local_comm,
                                                                     const int local_leader,
                                                                     const mpc_lowcomm_communicator_t peer_comm,
                                                                     const int remote_leader,
                                                                     const int tag);

mpc_lowcomm_communicator_t mpc_lowcomm_intercommunicator_merge(mpc_lowcomm_communicator_t intercomm, int high);


/* Query */

int mpc_lowcomm_communicator_is_intercomm(mpc_lowcomm_communicator_t comm);


int mpc_lowcomm_communicator_in_master_group(const mpc_lowcomm_communicator_t communicator);


int mpc_lowcomm_communicator_in_left_group(const mpc_lowcomm_communicator_t communicator);
int mpc_lowcomm_communicator_in_left_group_rank(const mpc_lowcomm_communicator_t communicator, int comm_world_rank);

int mpc_lowcomm_communicator_in_right_group(const mpc_lowcomm_communicator_t communicator);
int mpc_lowcomm_communicator_in_right_group_rank(const mpc_lowcomm_communicator_t communicator, int comm_world_rank);


/* Accessors */

mpc_lowcomm_communicator_t mpc_lowcomm_communicator_get_local(mpc_lowcomm_communicator_t comm);
mpc_lowcomm_communicator_t mpc_lowcomm_communicator_get_local_as(mpc_lowcomm_communicator_t comm,
																 int lookup_cw_rank);
mpc_lowcomm_communicator_t mpc_lowcomm_communicator_get_remote(mpc_lowcomm_communicator_t comm);

int mpc_lowcomm_communicator_remote_size(mpc_lowcomm_communicator_t comm);
int mpc_lowcomm_communicator_remote_world_rank(const mpc_lowcomm_communicator_t communicator, const int rank);

/***********************************
* COMMUNICATOR COLLECTIVE CONTEXT *
***********************************/

int mpc_lowcomm_communicator_is_shared_mem(const mpc_lowcomm_communicator_t comm);
int mpc_lowcomm_communicator_is_shared_node(const mpc_lowcomm_communicator_t comm);

struct sctk_comm_coll;
struct sctk_comm_coll *mpc_communicator_shm_coll_get(const mpc_lowcomm_communicator_t comm);

#endif /* MPC_LOWCOMM_COMMUNICATOR_H */
