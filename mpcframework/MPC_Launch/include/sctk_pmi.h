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
#ifndef MPC_LAUNCH_INCLUDE_SCTK_PMI_H_
#define MPC_LAUNCH_INCLUDE_SCTK_PMI_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <uthash.h>

#ifdef __cplusplus
extern "C"
{
#endif

/*!
 * SCTK_PMI Errors definition
 */
#define SCTK_PMI_SUCCESS                  0 //PMI_SUCCESS
#define SCTK_PMI_FAIL                    -1 //PMI_FAIL

struct process_nb_from_node_rank
{
	int node_rank;
	int nb_process;
	int *process_list;
	UT_hash_handle hh;
};

#define SCTK_PMI_NAME_SIZE 128
#define SCTK_PMI_TAG_PMI 6000
#define SCTK_PMI_TAG_PMI_HOSTNAME 1

/******************************************************************************
INITIALIZATION/FINALIZE
******************************************************************************/
/*! \brief Initialization function
 *
 */
int sctk_pmi_init();


/*! \brief Finalization function
 *
 */
int sctk_pmi_finalize();

/*! \brief Get the job id
 * @param id Pointer to store the job id
*/
int sctk_pmi_get_job_id( int *id );

/******************************************************************************
SYNCHRONIZATION
******************************************************************************/
/*! \brief Perform a barrier between all the processes of the job
 *
*/
int sctk_pmi_barrier();


/******************************************************************************
INFORMATION DIFFUSION
******************************************************************************/
/*! \brief Register information required for connection initialization
 * @param info The information to store
 * @param size Size in bytes of the information
 * @param tag An identifier to distinguish information
*/
int sctk_pmi_put_as_rank( char *info, int tag );

/*! \brief Get information required for connection initialization
 * @param info The place to store the information to retrieve
 * @param size Size in bytes of the information
 * @param tag An identifier to distinguish information
 * @param rank Rank of the process the information comes from
*/
int sctk_pmi_get_as_rank( char *info, size_t size, int tag, int rank );

/*! \brief Register information required for connection initialization
 * @param info The information to store
 * @param size Size in bytes of the information
 * @param tag A key string to distinguish information
*/
int sctk_pmi_put(char *value, char *key );

/*! \brief Get information required for connection initialization
 * @param info The place to store the information to retrieve
 * @param size Size in bytes of the information
 * @param tag A key string to distinguish information
*/
int sctk_pmi_get(char *value, size_t size, char *key);


/******************************************************************************
NUMBERING/TOPOLOGY INFORMATION
******************************************************************************/

int sctk_pmi_get_process_number_from_node_rank( struct process_nb_from_node_rank **process_number_from_node_rank );

/*! \brief Get the number of processes
 * @param size Pointer to store the number of processes
*/
int sctk_pmi_get_process_count( int *size );

/*! \brief Get the rank of this process
 * @param rank Pointer to store the rank of the process
*/
int sctk_pmi_get_process_rank( int *rank );

/*! \brief Get the number of nodes
 * @param size Pointer to store the number of nodes
*/
int sctk_pmi_get_node_count( int *size );

/*! \brief Get the rank of this node
 * @param rank Pointer to store the rank of the node
*/
int sctk_pmi_get_node_rank( int *rank );

/*! \brief Get the number of processes on the current node
 * @param size Pointer to store the number of processes
*/
int sctk_pmi_get_local_process_count( int *size );

/*! \brief Get the rank of this process on the current node
 * @param rank Pointer to store the rank of the process
*/
int sctk_pmi_get_local_process_rank( int *rank );


int sctk_pmi_get_max_key_len();
int sctk_pmi_get_max_val_len();

void sctk_net_abort();

int sctk_pmi_is_initialized();

/******************************************************************************
LIBRARY MODE TOLOGY GETTERS 
******************************************************************************/

#ifdef SCTK_LIB_MODE
int MPC_Net_hook_rank();
int MPC_Net_hook_size();
void MPC_Net_hook_barrier();
void MPC_Net_hook_send_to( void * data, size_t size, int target );
void MPC_Net_hook_recv_from( void * data, size_t size, int source );
#endif


#ifdef __cplusplus
}
#endif
#endif // MPC_LAUNCH_INCLUDE_SCTK_PMI_H_
