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
#ifndef __SCTK_PMI_H_
#define __SCTK_PMI_H_

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
#define SCTK_PMI_ERR_INIT                 1 //PMI_ERR_INIT
#define SCTK_PMI_ERR_NOMEM                2 //PMI_ERR_NOMEM
#define SCTK_PMI_ERR_INVALID_ARG          3 //PMI_ERR_INVALID_ARG
#define SCTK_PMI_ERR_INVALID_KEY          4 //PMI_ERR_INVALID_KEY
#define SCTK_PMI_ERR_INVALID_KEY_LENGTH   5 //PMI_ERR_INVALID_KEY_LENGTH
#define SCTK_PMI_ERR_INVALID_VAL          6 //PMI_ERR_INVALID_VAL
#define SCTK_PMI_ERR_INVALID_VAL_LENGTH   7 //PMI_ERR_INVALID_VAL_LENGTH
#define SCTK_PMI_ERR_INVALID_LENGTH       8 //PMI_ERR_INVALID_LENGTH
#define SCTK_PMI_ERR_INVALID_NUM_ARGS     9 //PMI_ERR_INVALID_NUM_ARGS
#define SCTK_PMI_ERR_INVALID_ARGS        10 //PMI_ERR_INVALID_ARGS
#define SCTK_PMI_ERR_INVALID_NUM_PARSED  11 //PMI_ERR_INVALID_NUM_PARSED
#define SCTK_PMI_ERR_INVALID_KEYVALP     12 //PMI_ERR_INVALID_KEYVALP
#define SCTK_PMI_ERR_INVALID_SIZE        13 //PMI_ERR_INVALID_SIZE
#define SCTK_PMI_ERR_INVALID_KVS         14 //PMI_ERR_INVALID_KVS

/*!
 * SCTK_PMI tags offset
 */
#define SCTK_PMI_TAG_TCP				1000
#define SCTK_PMI_TAG_INFINIBAND			2000
#define SCTK_PMI_TAG_MPCRUNCLIENT		3000
#define SCTK_PMI_TAG_PMI				4000
#define SCTK_PMI_TAG_SHM				5000
#define NODE_NUMBER 1024*1024

struct process_nb_from_node_rank
{
	int node_rank;
	int nb_process;
	UT_hash_handle hh;
};

typedef int SCTK_PMI_BOOL;
#define SCTK_PMI_TRUE     1
#define SCTK_PMI_FALSE    0

/*! \brief Max size for an hostname
 *
*/
#define SCTK_PMI_NAME_SIZE 1024
typedef struct
{
	int port_nb; /*!< TCP port used for communications*/
	char name[SCTK_PMI_NAME_SIZE]; /*!< Name of the host or IP adress*/
} sctk_pmi_tcp_t;

sctk_pmi_tcp_t *host_list;

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
int sctk_pmi_get_job_id ( int *id );

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
int sctk_pmi_put_connection_info ( void *info, size_t size, int tag );
/*! \brief Get information required for connection initialization
 * @param info The place to store the information to retrieve
 * @param size Size in bytes of the information
 * @param tag An identifier to distinguish information
 * @param rank Rank of the process the information comes from
*/
int sctk_pmi_get_connection_info ( void *info, size_t size, int tag, int rank );

/*! \brief Register information required for connection initialization
 * @param info The information to store
 * @param size Size in bytes of the information
 * @param tag A key string to distinguish information
*/
int sctk_pmi_put_connection_info_str ( void *info, size_t size, char tag[] );
/*! \brief Get information required for connection initialization
 * @param info The place to store the information to retrieve
 * @param size Size in bytes of the information
 * @param tag A key string to distinguish information
*/
int sctk_pmi_get_connection_info_str ( void *info, size_t size, char tag[] );


/******************************************************************************
NUMBERING/TOPOLOGY INFORMATION
******************************************************************************/

int sctk_pmi_get_process_number_from_node_rank ( struct process_nb_from_node_rank **process_number_from_node_rank );

/*! \brief Get the number of processes
 * @param size Pointer to store the number of processes
*/
int sctk_pmi_get_process_number ( int *size );

/*! \brief Get the rank of this process
 * @param rank Pointer to store the rank of the process
*/
int sctk_pmi_get_process_rank ( int *rank );

/*! \brief Get the number of nodes
 * @param size Pointer to store the number of nodes
*/
int sctk_pmi_get_node_number ( int *size );

/*! \brief Get the rank of this node
 * @param rank Pointer to store the rank of the node
*/
int sctk_pmi_get_node_rank ( int *rank );

/*! \brief Get the number of processes on the current node
 * @param size Pointer to store the number of processes
*/
int sctk_pmi_get_process_on_node_number ( int *size );

/*! \brief Get the rank of this process on the current node
 * @param rank Pointer to store the rank of the process
*/
int sctk_pmi_get_process_on_node_rank ( int *rank );

/******************************************************************************
PT2PT COMMUNICATIONS
******************************************************************************/
/*! \brief Send a message
 * @param info The message
 * @param size Size of the message
 * @param dest Destination of the message
*/
int sctk_pmi_send ( void *info, size_t size, int dest );

/*! \brief Receive a message
 * @param info The message
 * @param size Size of the message
 * @param src Source of the message
*/
int sctk_pmi_recv ( void *info, size_t size, int src );


int sctk_pmi_get_max_key_len();
int sctk_pmi_get_max_val_len();

void sctk_net_abort();

SCTK_PMI_BOOL
sctk_pmi_is_initialized();

#ifdef __cplusplus
}
#endif
#endif
