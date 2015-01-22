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
#include "sctk_alloc.h"
#include "sctk_pmi.h"
#include "sctk_launch.h"
#include "sctk_io_helper.h"
#include "sctk_debug.h"

#ifdef MPC_USE_HYDRA
#include <pmi.h>

static int sctk_pmi_process_on_node_rank;
static int sctk_pmi_node_rank;
static int sctk_pmi_nodes_number;
static int sctk_pmi_processes_on_node_number;




#define SCTK_PMI_TAG_PMI_HOSTNAME 1

#endif /* MPC_USE_HYDRA */

struct process_nb_from_node_rank *sctk_pmi_process_nb_from_node_rank = NULL;

#ifdef MPC_USE_SLURM
#include <pmi.h>
#endif /* MPC_USE_SLURM */

static int sctk_max_val_len;
static int sctk_max_key_len;
static char *sctk_kvsname;
SCTK_PMI_BOOL initialized;

int spawned;
/******************************************************************************
INITIALIZATION/FINALIZE
******************************************************************************/
/*! \brief Initialization function
 *
 */
int sctk_pmi_init()
{
	static int done = 0;

	if ( done == 0 )
	{
		int rc, max_kvsname_len;
		done = 1;

		// Initialized PMI/SLURM
		rc = PMI_Init ( &spawned );

		if ( rc != PMI_SUCCESS )
		{
			fprintf ( stderr, "FAILURE (sctk_pmi): PMI_Init: %d\n", rc );
			return rc;
		}

		// Check if PMI/SLURM is initialized
		initialized = SCTK_PMI_FALSE;
		rc = PMI_Initialized ( &initialized );

		if ( rc != PMI_SUCCESS )
		{
			fprintf ( stderr, "FAILURE (sctk_pmi): PMI_Initialized: %d\n", rc );
			return rc;
		}

		if ( initialized != SCTK_PMI_TRUE )
		{
			fprintf ( stderr, "FAILURE (sctk_pmi): PMI_Initialized returned false\n" );
			return rc;
		}

		// Get key, value max sizes and kvsname.
		// NB: Need to initialize kvsname here to avoid error in barrier with slurm
		if ( ( rc = PMI_KVS_Get_name_length_max ( &max_kvsname_len ) ) != PMI_SUCCESS )
		{
			printf ( "FAILURE (sctk_pmi): PMI_KVS_Get_name_length_max: %d\n", rc );
			return rc;
		}

		if ( ( rc = PMI_KVS_Get_key_length_max ( &sctk_max_key_len ) ) != PMI_SUCCESS )
		{
			printf ( "FAILURE (sctk_pmi): PMI_KVS_Get_key_length_max: %d\n", rc );
			return rc;
		}

		if ( ( rc = PMI_KVS_Get_value_length_max ( &sctk_max_val_len ) ) != PMI_SUCCESS )
		{
			printf ( "FAILURE (sctk_pmi): PMI_KVS_Get_value_length_max: %d\n", rc );
			return rc;
		}

		// Get the kvs name
		sctk_kvsname = ( char * ) sctk_malloc ( max_kvsname_len * sizeof ( char ) );

		if ( ( rc = PMI_KVS_Get_my_name ( sctk_kvsname, max_kvsname_len ) ) != PMI_SUCCESS )
		{
			printf ( "FAILURE: PMI_KVS_Get_my_name: %d\n", rc );
			free ( sctk_kvsname );
			return rc;
		}

#ifdef MPC_USE_HYDRA
		int rank, process_nb, nodes_nb = 0, i, j, size;
		char *value = NULL;
		char *nodes = NULL;
		char hostname[SCTK_PMI_NAME_SIZE];

		// check if PMI_Get_size is equal to 1 (could mean application launched without mpiexec)
		rc = PMI_Get_size ( &size );

		if ( rc != PMI_SUCCESS )
		{
			fprintf ( stderr, "unable to get size\n" );
			goto fn_exit;
		}
		else
			if ( size == 1 )
			{
				// no need of KVS for infos initialization
				sctk_pmi_process_on_node_rank = 0;
				sctk_pmi_node_rank = 0;
				sctk_pmi_nodes_number = 1;
				sctk_pmi_processes_on_node_number = 1;
				return rc;
			}


		// now we need to put node info in KVS in order to determine the following attributes,
		// since PMI1 does not provide direct access function:
		// nodes number, node rank, local processes number, local process rank.

		// initialize infos
		sctk_pmi_process_on_node_rank = -1;
		sctk_pmi_node_rank = -1;
		sctk_pmi_nodes_number = 0;
		sctk_pmi_processes_on_node_number = 0;

		value = sctk_malloc ( sctk_max_val_len );

		// Get hostname
		gethostname ( hostname, SCTK_PMI_NAME_SIZE - 1 );

		// Get the rank of the current process
		rc = sctk_pmi_get_process_rank ( &rank );

		if ( rc != PMI_SUCCESS )
		{
			fprintf ( stderr, "unable to get rank %d\n", rc );
			goto fn_exit;
		}

		// Put hostname on kvs for current process;
		sprintf ( value, "%s", hostname );
		rc = sctk_pmi_put_connection_info ( value, sctk_max_val_len, SCTK_PMI_TAG_PMI + SCTK_PMI_TAG_PMI_HOSTNAME );

		if ( rc != PMI_SUCCESS )
		{
			fprintf ( stderr, "unable to put hostname %d\n", rc );
			goto fn_exit;
		}


		// wait for all processes to put their hostname in KVS
		rc = sctk_pmi_barrier();

		if ( rc != PMI_SUCCESS )
		{
			fprintf ( stderr, "barrier failure %d\n", rc );
			goto fn_exit;
		}

		// now retrieve hostnames for all processes and compute local infos
		//rc = sctk_pmi_get_process_number(&process_nb);
		rc = PMI_Get_size ( &process_nb );

		if ( rc != PMI_SUCCESS )
		{
			fprintf ( stderr, "unable to get process number %d\n", rc );
			goto fn_exit;
		}

		nodes = sctk_malloc ( ( size_t ) process_nb * sctk_max_val_len );
		memset ( nodes, '\0', ( size_t ) process_nb * sctk_max_val_len );

		// build nodes list and compute local ranks and size
		for ( i = 0; i < process_nb; i++ )
		{
			j = 0;
			// get ith process hostname
			rc = sctk_pmi_get_connection_info ( value, sctk_max_val_len, SCTK_PMI_TAG_PMI + SCTK_PMI_TAG_PMI_HOSTNAME, i );

			if ( rc != PMI_SUCCESS )
			{
				fprintf ( stderr, "unable to get connection info %d\n", rc );
				goto fn_exit;
			}

			// compare value with current hostname. if the same, increase local processes number
			if ( strcmp ( hostname, value ) == 0 )
				sctk_pmi_processes_on_node_number++;

			// if i == rank, local rank == sctk_pmi_processes_on_node_number-1
			if ( i == rank )
				sctk_pmi_process_on_node_rank = sctk_pmi_processes_on_node_number - 1;

			while ( strcmp ( nodes + j * sctk_max_val_len, value ) != 0 && j < nodes_nb )
				j++;

			//update number of processes per node data
			struct process_nb_from_node_rank *tmp;
			HASH_FIND_INT ( sctk_pmi_process_nb_from_node_rank, &j, tmp );

			if ( tmp == NULL )
			{
				//new node
				tmp = ( struct process_nb_from_node_rank * ) sctk_malloc ( sizeof ( struct process_nb_from_node_rank ) );
				tmp->nb_process = 1;
				tmp->node_rank = j;
				HASH_ADD_INT ( sctk_pmi_process_nb_from_node_rank, node_rank, tmp );
			}
			else
			{
				//one more process on this node
				tmp->nb_process++;
			}

			if ( j == nodes_nb )
			{
				//found new node
				strcpy ( nodes + j * sctk_max_val_len, value );
				nodes_nb++;
			}
		}

		// Compute node rank
		j = 0;

		while ( strcmp ( nodes + j * sctk_max_val_len, hostname ) != 0 && j < nodes_nb )
			j++;

		sctk_pmi_node_rank = j;
		sctk_pmi_nodes_number = nodes_nb;

	fn_exit:
		free ( value );
		free ( nodes );
		return rc;
#endif


		sctk_pmi_get_node_rank ( &sctk_node_rank );
		sctk_pmi_get_node_number ( &sctk_node_number );
		sctk_pmi_get_process_on_node_rank ( &sctk_local_process_rank );
		sctk_pmi_get_process_on_node_number ( &sctk_local_process_number );
#ifdef MPC_USE_HYDRA
  sctk_pmi_get_process_number_from_node_rank(sctk_pmi_process_nb_from_node_rank);
#endif


#ifdef MPC_USE_SLURM
		return rc;
#endif
	}
	else
	{
		return 0;
	}
}

/*! \brief Finalization function
 *
 */
int sctk_pmi_finalize()
{
	int rc;

	// Finalize PMI/SLURM
	rc = PMI_Finalize();

	if ( rc != PMI_SUCCESS )
	{
		fprintf ( stderr, "FAILURE (sctk_pmi): PMI_Finalize: %d\n", rc );
	}

	free ( sctk_kvsname );
	return rc;
}

/*! \brief Get the job id
 * @param id Pointer to store the job id
*/
int sctk_pmi_get_job_id ( int *id )
{
#ifdef MPC_USE_HYDRA
	/* in mpich with pmi1, kvs name is used as job id. */
	*id = atoi ( sctk_kvsname );
	return PMI_SUCCESS;
#endif /* MPC_USE_HYDRA */

#ifdef MPC_USE_SLURM
	char *env = NULL;

	env = getenv ( "SLURM_JOB_ID" );

	if ( env )
	{
		*id = atoi ( env );
	}
	else
	{
		env = getenv ( "SLURM_JOBID" );

		if ( env )
		{
			*id = atoi ( env );
		}
		else
		{
			fprintf ( stderr, "FAILURE (sctk_pmi): unable to get job ID\n" );
			*id = -1;
			return PMI_FAIL;
		}
	}

	return PMI_SUCCESS;
#endif /* MPC_USE_SLURM */
}

/******************************************************************************
SYNCHRONIZATION
******************************************************************************/
/*! \brief Perform a barrier between all the processes of the job
 *
*/
int sctk_pmi_barrier()
{
	int rc;

	// Perform the barrier
	if ( ( rc = PMI_Barrier() ) != PMI_SUCCESS )
	{
		fprintf ( stderr, "FAILURE (sctk_pmi): barrier failed with error %d\n", rc );
	}

	return PMI_SUCCESS;
}


/******************************************************************************
INFORMATION DIFFUSION
******************************************************************************/
/*! \brief Register information required for connection initialization
 * @param info The information to store
 * @param size Size in bytes of the information
 * @param tag An identifier to distinguish information
*/
int sctk_pmi_put_connection_info ( void *info, size_t size, int tag )
{
	int iRank, rc;
	char *sKeyValue = NULL, * sValue = NULL;

	// Get the process rank
	sctk_pmi_get_process_rank ( &iRank );

	// Build the key
	sKeyValue = ( char * ) sctk_malloc ( sctk_max_key_len * sizeof ( char ) );
	sprintf ( sKeyValue, "%d_%d", tag, iRank );

	// Build the value
	sValue = ( char * ) info;

	// Put info in Key-Value-Space
	if ( ( rc = PMI_KVS_Put ( sctk_kvsname, sKeyValue, sValue ) ) != PMI_SUCCESS )
	{
		fprintf ( stderr, "FAILURE (sctk_pmi): PMI_KVS_Put: %d\n", rc );
	}
	else
	{
		// Apply changes on Key-Value-Space
		if ( ( rc = PMI_KVS_Commit ( sctk_kvsname ) ) != PMI_SUCCESS )
		{
			fprintf ( stderr, "FAILURE (sctk_pmi): PMI_KVS_Commit: %d\n", rc );
		}
	}

	free ( sKeyValue );
	return rc;
}

/*! \brief Get information required for connection initialization
 * @param info The place to store the information to retrieve
 * @param size Size in bytes of the information
 * @param tag An identifier to distinguish information
 * @param rank Rank of the process the information comes from
*/
int sctk_pmi_get_connection_info ( void *info, size_t size, int tag, int rank )
{
	int rc;
	char *sKeyValue = NULL, * sValue = NULL;

	// Build the key
	sKeyValue = ( char * ) sctk_malloc ( sctk_max_key_len * sizeof ( char ) );
	sprintf ( sKeyValue, "%d_%d", tag, rank );

	// Build the value
	sValue = ( char * ) info;

	// Get the value associated to the given key
	if ( ( rc = PMI_KVS_Get ( sctk_kvsname, sKeyValue, sValue, size ) ) != PMI_SUCCESS )
	{
		fprintf ( stderr, "FAILURE (sctk_pmi): PMI_KVS_Get: %d\n", rc );
	}

	free ( sKeyValue );
	return rc;
}

/*! \brief Register information required for connection initialization
 * @param info The information to store
 * @param size Size in bytes of the information
 * @param tag A key string to distinguish information
*/
int sctk_pmi_put_connection_info_str ( void *info, size_t size, char tag[] )
{
	int iRank, rc;
	char *sKeyValue = NULL, * sValue = NULL;

	// Get the process rank
	sctk_pmi_get_process_rank ( &iRank );

	// Build the key
	sKeyValue = tag;

	// Build the value
	sValue = ( char * ) info;

	// Put info in Key-Value-Space
	if ( ( rc = PMI_KVS_Put ( sctk_kvsname, sKeyValue, sValue ) ) != PMI_SUCCESS )
	{
		fprintf ( stderr, "FAILURE (sctk_pmi): PMI_KVS_Put: %d\n", rc );
	}
	else
	{
		// Apply changes on Key-Value-Space
		if ( ( rc = PMI_KVS_Commit ( sctk_kvsname ) ) != PMI_SUCCESS )
		{
			fprintf ( stderr, "FAILURE (sctk_pmi): PMI_KVS_Commit: %d\n", rc );
		}
	}

	return rc;
}

/*! \brief Get information required for connection initialization
 * @param info The place to store the information to retrieve
 * @param size Size in bytes of the information
 * @param tag A key string to distinguish information
*/
int sctk_pmi_get_connection_info_str ( void *info, size_t size, char tag[] )
{
	int rc;
	char *sKeyValue = NULL, * sValue = NULL;

	// Build the key
	sKeyValue = tag;

	// Build the value
	sValue = ( char * ) info;

	// Get the value associated to the given key
	if ( ( rc = PMI_KVS_Get ( sctk_kvsname, sKeyValue, sValue, size ) ) != PMI_SUCCESS )
	{
		fprintf ( stderr, "FAILURE (sctk_pmi): PMI_KVS_Get: %d\n", rc );
	}

	return rc;
}
/******************************************************************************
NUMBERING/TOPOLOGY INFORMATION
******************************************************************************/
/*! \brief Get the number of processes
 * @param size Pointer to store the number of processes
*/
int sctk_pmi_get_process_number ( int *size )
{
	int rc;
	// Get the total number of processes
	rc = PMI_Get_size ( size );

	if ( rc != PMI_SUCCESS )
	{
		fprintf ( stderr, "FAILURE (sctk_pmi): PMI_Get_universe_size: %d\n", rc );
	}

	return rc;
}

int sctk_pmi_get_process_number_from_node_rank ( struct process_nb_from_node_rank **process_number_from_node_rank )
{
#ifdef MPC_USE_HYDRA
	*process_number_from_node_rank = sctk_pmi_process_nb_from_node_rank;
	return PMI_SUCCESS;
#else
	not_implemented();
#endif
}

/*! \brief Get the rank of this process
 * @param rank Pointer to store the rank of the process
*/
int sctk_pmi_get_process_rank ( int *rank )
{
	int rc;

	// Get the rank of the current process
	if ( ( rc = PMI_Get_rank ( rank ) ) != PMI_SUCCESS )
	{
		fprintf ( stderr, "FAILURE (sctk_pmi): PMI_Get_rank: %d\n", rc );
	}
	else
	{
		//fprintf(stderr, "INFO %s: PMI_Get_rank: %d\n", name, *rank);
	}

	return rc;
}

/*! \brief Get the number of nodes
 * @param size Pointer to store the number of nodes
*/
int sctk_pmi_get_node_number ( int *size )
{
#ifdef MPC_USE_HYDRA
	*size = sctk_pmi_nodes_number;
	return PMI_SUCCESS;
#endif /* MPC_USE_HYDRA */

#ifdef MPC_USE_SLURM
	char *env = NULL;

	env = getenv ( "SLURM_NNODES" );

	if ( env )
	{
		*size = atoi ( env );
	}
	else
	{
		fprintf ( stderr, "FAILURE (sctk_pmi): unable to get nodes number\n" );
		*size = -1;
		return PMI_FAIL;
	}

	return PMI_SUCCESS;
#endif /* MPC_USE_SLURM */
}

/*! \brief Get the rank of this node
 * @param rank Pointer to store the rank of the node
*/
int sctk_pmi_get_node_rank ( int *rank )
{
#ifdef MPC_USE_HYDRA
	*rank  = sctk_pmi_node_rank;
	return PMI_SUCCESS;
#endif /* MPC_USE_HYDRA */

#ifdef MPC_USE_SLURM
	char *env = NULL;

	env = getenv ( "SLURM_NODEID" );

	if ( env )
	{
		*rank = atoi ( env );
	}
	else
	{
		fprintf ( stderr, "FAILURE (sctk_pmi): unable to get node rank\n" );
		*rank = -1;
		return PMI_FAIL;
	}

	return PMI_SUCCESS;
#endif /* MPC_USE_SLURM */
}

/*! \brief Get the number of processes on the current node
 * @param size Pointer to store the number of processes
*/
int sctk_pmi_get_process_on_node_number ( int *size )
{
#ifdef MPC_USE_SLURM
	int rc;
#endif

#ifdef MPC_USE_HYDRA
	*size  = sctk_pmi_processes_on_node_number;
	return PMI_SUCCESS;
#endif /* MPC_USE_HYDRA */

#ifdef MPC_USE_SLURM

	// Get the number of processes on the current node
	if ( ( rc = PMI_Get_clique_size ( size ) ) != PMI_SUCCESS )
	{
		fprintf ( stderr, "FAILURE (sctk_pmi): PMI_Get_clique_size: %d\n", rc );
		*size = -1;
		return PMI_FAIL;
	}

	return PMI_SUCCESS;
#endif /* MPC_USE_SLURM */
}

/*! \brief Get the rank of this process on the current node
 * @param rank Pointer to store the rank of the process
*/
int sctk_pmi_get_process_on_node_rank ( int *rank )
{
#ifdef MPC_USE_HYDRA
	*rank = sctk_pmi_process_on_node_rank;
	return PMI_SUCCESS;
#endif /* MPC_USE_HYDRA */

#ifdef MPC_USE_SLURM
	char *env = NULL;

	env = getenv ( "SLURM_LOCALID" );

	if ( env )
	{
		*rank = atoi ( env );
	}
	else
	{
		fprintf ( stderr, "FAILURE (sctk_pmi): unable to get process rank on the node\n" );
		*rank = -1;
		return PMI_FAIL;
	}

	return PMI_SUCCESS;
#endif /* MPC_USE_SLURM */
}

/******************************************************************************
PT2PT COMMUNICATIONS
******************************************************************************/

/*! \brief Send a message
 * @param info The message
 * @param size Size of the message
 * @param dest Destination of the message
*/
int sctk_pmi_send ( void *info, size_t size, int dest )
{
	if ( sctk_safe_write ( dest, info, size ) == -1 )
	{
		fprintf ( stderr, "FAILURE (sctk_pmi): sock write error\n" );
		return PMI_FAIL;
	}
	else
	{
		return PMI_SUCCESS;
	}
}

/*! \brief Receive a message
 * @param info The message
 * @param size Size of the message
 * @param src Source of the message
*/
int sctk_pmi_recv ( void *info, size_t size, int src )
{
	if ( sctk_safe_read ( src, info, size ) == -1 )
	{
		fprintf ( stderr, "FAILURE (sctk_pmi): sock read error\n" );
		return PMI_FAIL;
	}
	else
	{
		return PMI_SUCCESS;
	}
}

int
sctk_pmi_get_max_key_len()
{
	return sctk_max_key_len;
}

int
sctk_pmi_get_max_val_len()
{
	return sctk_max_val_len;
}

SCTK_PMI_BOOL
sctk_pmi_is_initialized()
{
	return initialized;
}

void sctk_net_abort()
{
	PMI_Abort ( 6, "ABORT" );
}
