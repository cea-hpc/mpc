#ifndef MPC_COMMON_RANK_H_
#define MPC_COMMON_RANK_H_

#include <mpc_config.h>

#include <sys/types.h>
#include <unistd.h>
#include <mpc_common_flags.h>

/**
 * @addtogroup MPC_Common
 * @{
 */

/****************
 * FILE CONTENT *
 ****************/

/**
 * @defgroup rank_management Rank Management and Query Interface
 * This module is in charge of providing ranks and count for the whole MPC
 */

        /**
         * @defgroup rank_getters Rank Getters
         * This is the interface to query ranks and count
         */

                /**
                 * @defgroup generic_rank_getters Generic Task-rank Getters
                 * These are the generic task rank and count getters when MPC_Threads is not available
                 */

        /**
         * @defgroup rank_setters
         * Initialization modules use these functions to set ranks
         */

/***************************************
 * RANK MANAGEMENT AND QUERY INTERFACE *
 ***************************************/

/**
 * @addtogroup rank_management
 * @{
 */

/****************
 * RANK GETTERS *
 ****************/

/**
 * @addtogroup rank_getters
 * @{
 */

/* Process */

/** Global variables storing process count */
extern int __process_count;

/** Global variable storing process rank */
extern int __process_rank;

/** Global variable storing app rank*/
extern int __process_app_rank;


/* NOLINTBEGIN(clang-diagnostic-unused-function): False positives */
/**
 * @brief Get the number of UNIX processes
 *
 * @return int Number of UNIX processes
 */
static inline int mpc_common_get_process_count( void )
{
	return __process_count;
}

/**
 * @brief Get the rank for current UNIX process
 *
 * @return int rank of current UNIX process
 */
static inline int mpc_common_get_process_rank( void )
{
	return __process_rank;
}

/* Local Process */


/** Storage for the number of local UNIX processes */
extern int __local_process_count;

/** Storage for the rank of local UNUX process */
extern int __local_process_rank;

/**
 * @brief Get the number of local UNIX processes (on same NONE)
 *
 * @return int number of local processes
 */
static inline int mpc_common_get_local_process_count( void )
{
	return __local_process_count;
}

/**
 * @brief Get the rank in local UNIX processses for current process
 *
 * @return int rank for current process in local UNIX processes
 */
static inline int mpc_common_get_local_process_rank( void )
{
	return __local_process_rank;
}

/* Node */

/** Storage for node count */
extern int __node_count;
/** Storage for node rank */
extern int __node_rank;

/**
 * @brief Get node count
 *
 * @return int node count
 */
static inline int mpc_common_get_node_count( void )
{
	return __node_count;
}

/**
 * @brief Get node rank for current process
 *
 * @return int node rank
 */
static inline int mpc_common_get_node_rank( void )
{
	return __node_rank;
}


#ifndef MPC_Threads

/**
 * @addtogroup generic_rank_getters
 * @{
 */

/* Tasks */


static inline int mpc_common_get_app_rank(){
	return __process_app_rank;
}

static inline void mpc_common_set_app_rank(int app_rank){
	__process_app_rank = app_rank;
}

static inline int mpc_common_get_app_size(){
	return mpc_common_get_flags()->appsize;
}

static inline int mpc_common_get_app_num(){
	return mpc_common_get_flags()->appnum;
}

static inline int mpc_common_get_app_count(){
	return mpc_common_get_flags()->appcount;
}

/**
 * @brief Get task count in current UNIX process
 *
 * @return int task count
 */
static inline int mpc_common_get_task_count( void )
{
	return mpc_common_get_process_count();
}

/**
 * @brief Get the task rank for current thread
 *
 * @return int task rank
 */
static inline int mpc_common_get_task_rank( void )
{
	return mpc_common_get_process_rank();
}

/**
 * @brief Get the number of tasks in current process
 *
 * @return int number of tasks on node
 */
static inline int mpc_common_get_local_task_count( void )
{
	return 1;
}

/**
 * @brief Get the task rank of given thread in curent process
 *
 * @return int task rank on node
 */
static inline int mpc_common_get_local_task_rank( void )
{
	return 0;
}

/**
 * @brief Return the thread id for current thread (only when in MPC threads)
 *        returns the PID otherwise
 * @return int thread id when in MPC's scheduler (PID otherwise)
 */
static inline int mpc_common_get_thread_id( void )
{
	return (int)getpid();
}
/* NOLINTEND(clang-diagnostic-unused-function) */

/* End Generic task-rank getters */
/**
 * @}
 */

#else
#include "mpc_thread_accessor.h"
#endif /* MPC_Thread */

/* End rank getters */
/**
 * @}
 */

/****************
 * RANK SETTERS *
 ****************/

/**
 * @addtogroup rank_setters
 * @{
 */

/* Process Count */

/**
 * @brief Set the number of UNIX processes
 *
 * @param process_count Number of UNIX processes to set
 */
void mpc_common_set_process_count( int process_count );

/**
 * @brief Set the UNIX process rank
 *
 * @param process_rank the unix process rank to set
 */
void mpc_common_set_process_rank( int process_rank );

/* Local Process Count */

/**
 * @brief Set the number of local UNIX process (on same NODE)
 *
 * @param local_process_count number of UNIX processes on node
 */
void mpc_common_set_local_process_count( int local_process_count );

/**
 * @brief Set the local rank for UNIX process (on same NONE)
 *
 * @param local_process_rank on node rank for UNIX process
 */
void mpc_common_set_local_process_rank( int local_process_rank );

/* Node Count */

/**
 * @brief Set the number of nodes
 *
 * @param node_count number of nodes
 */
void mpc_common_set_node_count( int node_count );

/**
 * @brief The the node rank for current UNIX process
 *
 * @param node_rank node rank
 */
void mpc_common_set_node_rank( int node_rank );

/* End Setters */
/**
 * @}
 */


/* End Rank Management */
/**
 * @}
 */


/* End MPC Common */
/**
 * @}
 */



#endif /* MPC_COMMON_RANK_H_ */
