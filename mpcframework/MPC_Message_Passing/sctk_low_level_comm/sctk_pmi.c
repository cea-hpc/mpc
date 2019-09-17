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
#include "sctk.h"


#ifndef SCTK_LIB_MODE
	#include <pmi.h>
#else
	/* We have to define these constants
	 * as in lib mode we have no PMI.h */
	#define PMI_SUCCESS 0
	#define PMI_FAIL    1
#endif /*SCTK_LIB_MODE */

static int sctk_pmi_process_on_node_rank;
static int sctk_pmi_node_rank;
static int sctk_pmi_nodes_number;
static int sctk_pmi_processes_on_node_number;
static int sctk_pmi_process_rank;
static int sctk_pmi_process_number;

struct process_nb_from_node_rank *sctk_pmi_process_nb_from_node_rank = NULL;


static int max_kvsname_len  = 0;
static int sctk_max_val_len = 0;
static int sctk_max_key_len = 0;
static char *sctk_kvsname = "0";
SCTK_PMI_BOOL initialized;

int spawned;

/******************************************************************************
LIBRARY MODE TOLOGY GETTERS 
******************************************************************************/
#ifdef SCTK_LIB_MODE
	/* Here are the hook used by the host MPI when running in libmode */
	#pragma weak MPC_Net_hook_rank
	int MPC_Net_hook_rank()
	{
		return 0;
	}
	
	#pragma weak MPC_Net_hook_size
	int MPC_Net_hook_size()
	{
		return 1;
	}
	
	#pragma weak MPC_Net_hook_barrier
	void MPC_Net_hook_barrier()
	{
		
	}
	
	#pragma weak MPC_Net_hook_send_to
	void MPC_Net_hook_send_to( void * data, size_t size, int target )
	{
		sctk_fatal("You must implement the MPC_Net_send_to function to run in multiprocess");
	}
	
	#pragma weak MPC_Net_hook_recv_from
	void MPC_Net_hook_recv_from( void * data, size_t size, int source )
	{
		sctk_fatal("You must implement the MPC_Net_recv_from function to run in multiprocess");
	}
#endif

/******************************************************************************
INITIALIZATION/FINALIZE
******************************************************************************/

/*! \brief Initialization function
 *
 */
int sctk_pmi_init()
{
	/* let the MPC PMI interface choose if it need to be initialized */
#ifndef SCTK_LIB_MODE
	if(get_process_count() <= 1)
		return PMI_FAIL;
#endif
	/*static int done = 0;*/
	
	/*if ( done == 0 )*/
	/*{*/
		int rc;
		/*done = 1;*/
#ifdef SCTK_LIB_MODE
		sctk_pmi_process_rank = MPC_Net_hook_rank();
		sctk_pmi_process_number = MPC_Net_hook_size();

		/* Consider nodes as processes */
		set_node_rank(sctk_pmi_process_rank);
		set_node_count(sctk_pmi_process_number);
		set_local_process_rank(0):
		set_local_process_count(1);
		return 0;
#else /* SCTK_LIB_MODE */

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
                        sctk_free(sctk_kvsname);
                        return rc;
                }

                int rank, process_nb, nodes_nb = 0, i, j, size;
                char *value = NULL;
                char *nodes = NULL;
                char hostname[SCTK_PMI_NAME_SIZE];

                // check if PMI_Get_size is equal to 1 (could mean application
                // launched without mpiexec)
                rc = sctk_pmi_get_process_number(&size);

                if (rc != PMI_SUCCESS) {
                  fprintf(stderr, "unable to get size\n");
                  goto fn_exit;
                } else if (size == 1) {
                  // no need of KVS for infos initialization
                  sctk_pmi_process_on_node_rank = 0;
                  sctk_pmi_node_rank = 0;
                  sctk_pmi_nodes_number = 1;
                  sctk_pmi_processes_on_node_number = 1;
                  return rc;
                }

                // now we need to put node info in KVS in order to determine the
                // following attributes,
                // since PMI1 does not provide direct access function:
                // nodes number, node rank, local processes number, local
                // process rank.

                // initialize infos
                sctk_pmi_process_on_node_rank = -1;
                sctk_pmi_node_rank = -1;
                sctk_pmi_nodes_number = 0;
                sctk_pmi_processes_on_node_number = 0;

                value = sctk_malloc(sctk_max_val_len);

                // Get hostname
                gethostname(hostname, SCTK_PMI_NAME_SIZE - 1);

                // Get the rank of the current process
                rc = sctk_pmi_get_process_rank(&rank);

                if (rc != PMI_SUCCESS) {
                  fprintf(stderr, "unable to get rank %d\n", rc);
                  goto fn_exit;
                }

                // Put hostname on kvs for current process;
                sprintf(value, "%s", hostname);
                rc = sctk_pmi_put_connection_info(
                    value,
                    SCTK_PMI_TAG_PMI + SCTK_PMI_TAG_PMI_HOSTNAME);

                if (rc != PMI_SUCCESS) {
                  fprintf(stderr, "unable to put hostname %d\n", rc);
                  goto fn_exit;
                }

                // wait for all processes to put their hostname in KVS
                rc = sctk_pmi_barrier();

                if (rc != PMI_SUCCESS) {
                  fprintf(stderr, "barrier failure %d\n", rc);
                  goto fn_exit;
                }

                // now retrieve hostnames for all processes and compute local
                // infos
                // rc = sctk_pmi_get_process_number(&process_nb);
                rc = sctk_pmi_get_process_number(&process_nb);

                if (rc != PMI_SUCCESS) {
                  fprintf(stderr, "unable to get process number %d\n", rc);
                  goto fn_exit;
                }

                nodes = sctk_malloc((size_t)process_nb * sctk_max_val_len);
                memset(nodes, '\0', (size_t)process_nb * sctk_max_val_len);

                // build nodes list and compute local ranks and size
                for (i = 0; i < process_nb; i++) {
                  j = 0;
                  // get ith process hostname
                  rc = sctk_pmi_get_connection_info(
                      value, sctk_max_val_len,
                      SCTK_PMI_TAG_PMI + SCTK_PMI_TAG_PMI_HOSTNAME, i);

                  if (rc != PMI_SUCCESS) {
                    fprintf(stderr, "unable to get connection info %d\n", rc);
                    goto fn_exit;
                  }

                  // compare value with current hostname. if the same, increase
                  // local processes number
                  if (strcmp(hostname, value) == 0)
                    sctk_pmi_processes_on_node_number++;

                  // if i == rank, local rank ==
                  // sctk_pmi_processes_on_node_number-1
                  if (i == rank)
                    sctk_pmi_process_on_node_rank =
                        sctk_pmi_processes_on_node_number - 1;

                  while (strcmp(nodes + j * sctk_max_val_len, value) != 0 &&
                         j < nodes_nb)
                    j++;

                  // update number of processes per node data
                  struct process_nb_from_node_rank *tmp;
                  HASH_FIND_INT(sctk_pmi_process_nb_from_node_rank, &j, tmp);

                  if (tmp == NULL) {
                    // new node
                    tmp = (struct process_nb_from_node_rank *)sctk_malloc(
                        sizeof(struct process_nb_from_node_rank));
                    tmp->node_rank = j;
                    tmp->nb_process = 1;
                    tmp->process_list = (int *)sctk_malloc(sizeof(int) * 1);
                    tmp->process_list[0] = i;
                    HASH_ADD_INT(sctk_pmi_process_nb_from_node_rank, node_rank,
                                 tmp);
                  } else {
                    // one more process on this node
                    tmp->nb_process++;
                    int *tmp_tab = sctk_realloc(tmp->process_list,
                                                sizeof(int) * tmp->nb_process);
                    assume(tmp_tab != NULL);
                    tmp_tab[tmp->nb_process - 1] = i;
                    tmp->process_list = tmp_tab;
                  }

                  if (j == nodes_nb) {
                    // found new node
                    strcpy(nodes + j * sctk_max_val_len, value);
                    nodes_nb++;
                  }
                }

                // Compute node rank
                j = 0;

                while (strcmp(nodes + j * sctk_max_val_len, hostname) != 0 &&
                       j < nodes_nb)
                  j++;

                sctk_pmi_node_rank = j;
                sctk_pmi_nodes_number = nodes_nb;

                sctk_pmi_get_process_rank(&sctk_pmi_process_rank);
                sctk_pmi_get_process_number(&sctk_pmi_process_number);

				int node_rank;
                sctk_pmi_get_node_rank(&node_rank);
				set_node_rank(node_rank);

				int node_count;
                sctk_pmi_get_node_number(&node_count);
				set_node_count(node_count);

				int local_process_rank;
                sctk_pmi_get_process_on_node_rank(&local_process_rank);
				set_local_process_rank(local_process_rank);

				int local_process_count;
                sctk_pmi_get_process_on_node_number(&local_process_count);
				set_local_process_count(local_process_count);

                sctk_pmi_get_process_number_from_node_rank(
                    &sctk_pmi_process_nb_from_node_rank);

        fn_exit:
          sctk_free(value);
          sctk_free(nodes);
          return rc;

#endif /* SCTK_LIB_MODE */
	/*}*/
	/*else*/
	/*{*/
		/*return 0;*/
	/*}*/
}

/*! \brief Finalization function
 *
 */
int sctk_pmi_finalize()
{
	int rc;

#ifdef SCTK_LIB_MODE
	return PMI_SUCCESS;
#else /* SCTK_LIB_MODE */

	// Finalize PMI/SLURM
	rc = PMI_Finalize();

	if ( rc != PMI_SUCCESS )
	{
		fprintf ( stderr, "FAILURE (sctk_pmi): PMI_Finalize: %d\n", rc );
	}

        sctk_free(sctk_kvsname);
        return rc;
#endif
}

/*! \brief Get the job id
 * @param id Pointer to store the job id
*/
int sctk_pmi_get_job_id ( int *id )
{
#if defined(SCTK_LIB_MODE) || defined(MPC_USE_SLURM)
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
#elif MPC_USE_HYDRA
	/* in mpich with pmi1, kvs name is used as job id. */
	*id = atoi ( sctk_kvsname );
	return PMI_SUCCESS;
#endif
}

/******************************************************************************
SYNCHRONIZATION
******************************************************************************/
/*! \brief Perform a barrier between all the processes of the job
 *
*/
int sctk_pmi_barrier()
{
#ifdef SCTK_LIB_MODE
	MPC_Net_hook_barrier();
	return PMI_SUCCESS;
#else /* SCTK_LIB_MODE */
	int rc;

	// Perform the barrier
	if ( ( rc = PMI_Barrier() ) != PMI_SUCCESS )
	{
		fprintf ( stderr, "FAILURE (sctk_pmi): barrier failed with error %d\n", rc );
	}

	return rc;
#endif /* SCTK_LIB_MODE */
}


/******************************************************************************
INFORMATION DIFFUSION
******************************************************************************/


/*! \brief Register information required for connection initialization
 * @param info The information to store
 * @param size Size in bytes of the information
 * @param tag A key string to distinguish information
*/
int sctk_pmi_put_connection_info_str ( void *info, char tag[] )
{
#ifdef SCTK_LIB_MODE
	not_implemented();
	return PMI_FAIL;
#else /* SCTK_LIB_MODE */
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
#endif /* SCTK_LIB_MODE */
}

/*! \brief Register information required for connection initialization
 * @param info The information to store
 * @param size Size in bytes of the information
 * @param tag An identifier to distinguish information
*/
int sctk_pmi_put_connection_info ( void *info, int tag )
{
#ifdef SCTK_LIB_MODE
	not_implemented();
	return PMI_FAIL;
#else /* SCTK_LIB_MODE */
	int iRank, rc;
	char *sKeyValue = NULL;

	// Get the process rank
	sctk_pmi_get_process_rank ( &iRank );

	// Build the key
	sKeyValue = ( char * ) sctk_malloc ( sctk_max_key_len * sizeof ( char ) );
	sprintf ( sKeyValue, "MPC_KEYS_%d_%d", tag, iRank );

	rc = sctk_pmi_put_connection_info_str( info, sKeyValue);

	sctk_free ( sKeyValue );
	return rc;
#endif /* SCTK_LIB_MODE */
}

/*! \brief Get information required for connection initialization
 * @param info The place to store the information to retrieve
 * @param size Size in bytes of the information
 * @param tag An identifier to distinguish information
 * @param rank Rank of the process the information comes from
*/
int sctk_pmi_get_connection_info ( void *info, size_t size, int tag, int rank )
{
#ifdef SCTK_LIB_MODE
	not_implemented();
	return PMI_FAIL;
#else /* SCTK_LIB_MODE */
	int rc;
	char *sKeyValue = NULL, * sValue = NULL;

	// Build the key
	sKeyValue = ( char * ) sctk_malloc ( sctk_max_key_len * sizeof ( char ) );
	sprintf ( sKeyValue, "MPC_KEYS_%d_%d", tag, rank );

	// Build the value
	sValue = ( char * ) info;

	// Get the value associated to the given key
	if ( ( rc = PMI_KVS_Get ( sctk_kvsname, sKeyValue, sValue, size ) ) != PMI_SUCCESS )
	{
		fprintf ( stderr, "FAILURE (sctk_pmi): PMI_KVS_Get: %d\n", rc );
	}

	sctk_free ( sKeyValue );
	return rc;
#endif /* SCTK_LIB_MODE */
}


/*! \brief Get information required for connection initialization
 * @param info The place to store the information to retrieve
 * @param size Size in bytes of the information
 * @param tag A key string to distinguish information
*/
int sctk_pmi_get_connection_info_str ( void *info, size_t size, char tag[] )
{
#ifdef SCTK_LIB_MODE
	not_implemented();
	return PMI_FAIL;
#else /* SCTK_LIB_MODE */
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
#endif /* SCTK_LIB_MODE */
}
/******************************************************************************
NUMBERING/TOPOLOGY INFORMATION
******************************************************************************/
/*! \brief Get the number of processes
 * @param size Pointer to store the number of processes
*/
int sctk_pmi_get_process_number ( int *size )
{
#ifdef SCTK_LIB_MODE
	*size = sctk_pmi_process_number;
	return PMI_SUCCESS;
#else /* SCTK_LIB_MODE */
	int rc;
	// Get the total number of processes
	rc = PMI_Get_size ( size );

	if ( rc != PMI_SUCCESS )
	{
		fprintf ( stderr, "FAILURE (sctk_pmi): PMI_Get_universe_size: %d\n", rc );
	}

	return rc;
#endif /* SCTK_LIB_MODE */
}

int sctk_pmi_get_process_number_from_node_rank ( struct process_nb_from_node_rank **process_number_from_node_rank )
{
	*process_number_from_node_rank = sctk_pmi_process_nb_from_node_rank;
	return PMI_SUCCESS;
}

/*! \brief Get the rank of this process
 * @param rank Pointer to store the rank of the process
*/
int sctk_pmi_get_process_rank ( int *rank )
{
#ifdef SCTK_LIB_MODE
	*rank = sctk_pmi_process_rank;
	return PMI_SUCCESS;
#else /* SCTK_LIB_MODE */
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
#endif /* SCTK_LIB_MODE */
}

/*! \brief Get the number of nodes
 * @param size Pointer to store the number of nodes
*/
int sctk_pmi_get_node_number ( int *size )
{
#ifdef SCTK_LIB_MODE
	sctk_pmi_get_process_number( size );
#else /* SCTK_LIB_MODE */
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
#endif /* SCTK_LIB_MODE */
	return PMI_FAIL;
}

/*! \brief Get the rank of this node
 * @param rank Pointer to store the rank of the node
*/
int sctk_pmi_get_node_rank ( int *rank )
{
#ifdef SCTK_LIB_MODE
	sctk_pmi_get_process_rank( rank );
#else /* SCTK_LIB_MODE */
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
#endif /* SCTK_LIB_MODE */
	return PMI_FAIL;
}

/*! \brief Get the number of processes on the current node
 * @param size Pointer to store the number of processes
*/
int sctk_pmi_get_process_on_node_number ( int *size )
{
#ifdef SCTK_LIB_MODE
	*size = get_local_process_count();
	return PMI_SUCCESS;
#else /* SCTK_LIB_MODE */
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
#endif /* SCTK_LIB_MODE */
}

/*! \brief Get the rank of this process on the current node
 * @param rank Pointer to store the rank of the process
*/
int sctk_pmi_get_process_on_node_rank ( int *rank )
{
#ifdef SCTK_LIB_MODE
	*rank = get_local_process_rank();
	return PMI_SUCCESS;
#else /* SCTK_LIB_MODE */
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
#endif /* SCTK_LIB_MODE */
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
#ifdef SCTK_LIB_MODE
	not_implemented();
	return PMI_FAIL;
#else /* SCTK_LIB_MODE */
	if ( sctk_safe_write ( dest, info, size ) == -1 )
	{
		fprintf ( stderr, "FAILURE (sctk_pmi): sock write error\n" );
		return PMI_FAIL;
	}
	else
	{
		return PMI_SUCCESS;
	}
#endif /* SCTK_LIB_MODE */
}

/*! \brief Receive a message
 * @param info The message
 * @param size Size of the message
 * @param src Source of the message
*/
int sctk_pmi_recv ( void *info, size_t size, int src )
{
#ifdef SCTK_LIB_MODE
	not_implemented();
	return PMI_FAIL;
#else /* SCTK_LIB_MODE */
	if ( sctk_safe_read ( src, info, size ) == -1 )
	{
		fprintf ( stderr, "FAILURE (sctk_pmi): sock read error\n" );
		return PMI_FAIL;
	}
	else
	{
		return PMI_SUCCESS;
	}
#endif /* SCTK_LIB_MODE */
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
#ifdef SCTK_LIB_MODE
	abort();
#else 
	PMI_Abort ( 6, "ABORT" );
#endif
}
