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
/* #   - PULVERAIL Sebastien sebastien.pulverail@sogeti.com               # */
/* #   - COTTRET Maxime maxime.cottret@sogeti.com                         # */
/* #                                                                      # */
/* ######################################################################## */
#ifndef MPC_LAUNCH_INCLUDE_MPC_LAUNCH_PMI_H_
#define MPC_LAUNCH_INCLUDE_MPC_LAUNCH_PMI_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <uthash.h>
#include <mpc_config.h>
#include <mpc_common_types.h>

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * @addtogroup MPC_Launch
 * @{
 */

/****************
 * FILE CONTENT *
 ****************/

/**
 * @defgroup pmi Process Management Interface
 * This implemented getters and setters for the PMI interface in MPC
 */

        /**
         * @defgroup libmode_hooks MPC Library mode hooks
        * Libmode allows MPC to run alongside another MPI instance
        * to do so simply compile mpc with ./installmpc --lib-mode.
        * Note that a TCP rail is required to bootstrap the comm ring
        * to enable on demand connections on high speed networks.
        *
        * These function are the weak implementation to be overriden
        * by actual ones in the target implementation.
         */

/***********
 * MPC PMI *
 ***********/

/**
 * @addtogroup pmi
 * @{
 */

/*************************
 * PMI ERROR DEFINITIONS *
 *************************/

#define MPC_LAUNCH_PMI_SUCCESS 0 /**< Wrapper for PMI_SUCCESS */
#define MPC_LAUNCH_PMI_FAIL -1 /**< Wrapper for PMI_FAIL */

/***************************
 * INITIALIZATION/FINALIZE *
 ***************************/

/**
 * @brief Main initialization point for MPI support
 *
 * @return int SCTK_PMI_SUCCESS / _FAIL
 */
int mpc_launch_pmi_init();

/**
 * @brief Main release point for PMI support
 *
 * @return int SCTK_PMI_SUCCESS / _FAIL
 */
int mpc_launch_pmi_finalize();

/******************
 * SIZE RETRIEVAL *
 ******************/

/**
 * @brief Maximum lenght of a PMI key
 *
 * @return int max key len
 */
int mpc_launch_pmi_get_max_key_len();

/**
 * @brief Maximum lenght of a PMI value
 *
 * @return int max value len
 */
int mpc_launch_pmi_get_max_val_len();

/***********************
 * PMI STATE AND ABORT *
 ***********************/

/**
 * @brief Check if PMI was initialized
 *
 * @return int 1 if initialized 0 otherwise
 */
int mpc_launch_pmi_is_initialized();

/**
 * @brief Abort the process using PMI
 *
 */
void mpc_launch_pmi_abort();

/*******************
 * SYNCHRONIZATION *
 *******************/

/**
 * @brief Execute a PMI barrier between UNIX processes
 *
 * @return int SCTK_PMI_SUCCESS / _FAIL
 */
int mpc_launch_pmi_barrier();

/*************************
 * INFORMATION DIFFUSION *
 *************************/

/**
 * @brief Put a value in KVS for rank using an integer tag
 *
 * @param value Data to be stored in the PMI
 * @param tag identifier of the stored value
 * @return int SCTK_PMI_SUCCESS / _FAIL
 */
int mpc_launch_pmi_put_as_rank( char *value, int tag, int is_local );

/**
 * @brief Get a value in KVS for rank using an integer tag
 *
 * @param value Data to be retrieved from the PMI
 * @param size maximum size of the expected data
 * @param tag tag of the data to be retrieved
 * @param rank rank to retrieve data for
 * @return int SCTK_PMI_SUCCESS / _FAIL
 */
int mpc_launch_pmi_get_as_rank( char *value, size_t size, int tag, int rank );

/**
 * @brief Set a value at key in the KVS
 *
 * @param value value to be set
 * @param key corresponding key
 * @param is_local if the value is to be stored locally not over net
 * @return int SCTK_PMI_SUCCESS / _FAIL
 */
int mpc_launch_pmi_put( char *value, char *key, int is_local );

/**
 * @brief Get a value at key in the KVS
 *
 * @param value value to be retrieved
 * @param size maximum value size
 * @param key corresponding key
 * @param remote remote rank to query
 * @return int SCTK_PMI_SUCCESS / _FAIL
 */
int mpc_launch_pmi_get( char *value, size_t size, char *key, int remote);

/*********************
 * OS PROCESS LAYOUT *
 *********************/

/**
 * @brief Defines an UTHASH hash-table to retrieve per node process layout
 *
 */
struct mpc_launch_pmi_process_layout
{
	int node_rank;		/**< Node info for rank */
	int nb_process;		/**< Number of processes on the node */
	int *process_list;	/**< List of processes ID on the node */
	UT_hash_handle hh;	/**< UTHash handle */
};

/**
 * @brief Retrieve a UTHASH hash-table gathering process layout on nodes
 *
 * @param layout [OUT] where to store the pointer (not to free)
 * @return int SCTK_PMI_SUCCESS / _FAIL
 */
int mpc_launch_pmi_get_process_layout( struct mpc_launch_pmi_process_layout **layout );

/**********************************
 * NUMBERING/TOPOLOGY INFORMATION *
 **********************************/

/*! \brief Get the job id
 * @param id Pointer to store the job id
*/
int mpc_launch_pmi_get_job_id( uint64_t *id );

/* End Process Management Interface */
/**
 * @}
 */

/* End MPC_Launch */
/**
 * @}
 */


int mpc_launch_pmi_get_univ_size(int* univsize);

int mpc_launch_pmi_get_app_rank(int* appname);

int mpc_launch_pmi_get_app_size(int* appsize);

int mpc_launch_pmi_get_app_num(int* appnum);

int mpc_launch_pmi_get_pset(char** pset);

int mpc_launch_pmi_get_pset_list(char** psetlist);

#ifdef __cplusplus
}
#endif

#endif // MPC_LAUNCH_INCLUDE_MPC_LAUNCH_PMI_H_
