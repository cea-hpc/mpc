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

#include "MPC_Launch/include/sctk_pmi.h"

#include "MPC_Common/include/sctk_alloc.h"
#include "MPC_Common/include/sctk_io_helper.h"
#include "MPC_Common/include/sctk_debug.h"
#include "MPC_Common/include/sctk_rank.h"

#include "MPC_Launch/include/sctk_launch.h"

#ifndef SCTK_LIB_MODE
#include <pmi.h>
#else
/* We have to define these constants
     * as in lib mode we have no PMI.h */
#define PMI_SUCCESS 0
#define PMI_FAIL 1
#endif /*SCTK_LIB_MODE */

struct sctk_pmi_context
{
    /** Nodes Ranking */
    int node_rank;
    int node_count;

    /** Process ranking */
    int process_rank;
    int process_count;

    /** Local Process ranking */
    int local_process_rank;
    int local_process_count;

    /** KVS Name */
    char * kvsname;

    /** If spawned */
    int spawned;

    /** PMI lengths */
    int kvsname_len;
    int max_val_len;
    int max_key_len;

    /** Convert struct */
    struct process_nb_from_node_rank *process_nb_from_node_rank;

    /** Global initialization flag */
    int initialized;
};

static struct sctk_pmi_context pmi_context;

void sctk_mpi_context_clear(void)
{
    memset(&pmi_context, 0, sizeof(struct sctk_pmi_context));
}

int sctk_pmi_get_max_key_len()
{
    return pmi_context.max_key_len;
}

int sctk_pmi_get_max_val_len()
{
    return pmi_context.max_val_len;
}

int sctk_pmi_is_initialized()
{
    return pmi_context.initialized;
}



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
void MPC_Net_hook_send_to( void *data, size_t size, int target )
{
    sctk_fatal( "You must implement the MPC_Net_send_to function to run in multiprocess" );
}

#pragma weak MPC_Net_hook_recv_from
void MPC_Net_hook_recv_from( void *data, size_t size, int source )
{
    sctk_fatal( "You must implement the MPC_Net_recv_from function to run in multiprocess" );
}
#endif

/******************************************************************************
INITIALIZATION/FINALIZE
******************************************************************************/

#ifdef SCTK_LIB_MODE
int _sctk_pmi_init_lib_mode()
{
    pmi_context.process_rank = MPC_Net_hook_rank();
    pmi_context.process_count = MPC_Net_hook_size();

    /* Consider nodes as processes */
    mpc_common_set_node_rank( pmi_context.process_rank );
    mpc_common_set_node_count( pmi_context.process_count );
    mpc_common_set_local_process_rank( 0 ) : mpc_common_set_local_process_count( 1 );
    return 0;
}
#endif

#define pmi_check_rc(retcode, ctx)do\
{\
    if ( retcode != PMI_SUCCESS )\
    {\
        fprintf( stderr, "FAILURE (sctk_pmi): %s: %d\n", ctx, retcode );\
        return rc;\
    }\
}while(0)


/*! \brief Initialization function
 *
 */
int sctk_pmi_init()
{
    sctk_mpi_context_clear();

#ifdef SCTK_LIB_MODE
    return _sctk_pmi_init_lib_mode();
#endif

    /* Note that process count is first set in sctk_lauch
       according to command line parameters */
    if ( mpc_common_get_process_count() <= 1 )
        return PMI_FAIL;

    int rc;

    // Initialized PMI/SLURM
    rc = PMI_Init( &pmi_context.spawned );
    pmi_check_rc(rc, "PMI_Init");

    // Check if PMI/SLURM is initialized
    pmi_context.initialized = 0;
    rc = PMI_Initialized( &pmi_context.initialized );
    pmi_check_rc(rc, "PMI_Initialized");

    if ( pmi_context.initialized != 1 )
    {
        fprintf( stderr, "FAILURE (sctk_pmi): PMI_Initialized returned false\n" );
        return rc;
    }

    // Get key, value max sizes and kvsname.
    // NB: Need to initialize kvsname here to avoid error in barrier with slurm
    rc = PMI_KVS_Get_name_length_max( &pmi_context.kvsname_len );
    pmi_check_rc(rc, "PMI_KVS_Get_name_length_max");

    rc = PMI_KVS_Get_key_length_max( &pmi_context.max_key_len );
    pmi_check_rc(rc, "PMI_KVS_Get_key_length_max");

    rc = PMI_KVS_Get_value_length_max( &pmi_context.max_val_len );
    pmi_check_rc(rc, "PMI_KVS_Get_value_length_max");

    // Get the kvs name
    pmi_context.kvsname = (char *) sctk_malloc( pmi_context.kvsname_len * sizeof( char ) );
    assume(pmi_context.kvsname);

    rc = PMI_KVS_Get_my_name( pmi_context.kvsname, pmi_context.kvsname_len );
    pmi_check_rc(rc, "PMI_KVS_Get_my_name");

    rc = sctk_pmi_get_process_rank( &pmi_context.process_rank );
    pmi_check_rc(rc, "sctk_pmi_get_process_rank");

    rc = sctk_pmi_get_process_count( &pmi_context.process_count );
    pmi_check_rc(rc, "sctk_pmi_get_process_count");

    // check if PMI_Get_size is equal to 1 (could mean application
    // launched without mpiexec)

    if ( pmi_context.process_count == 1 )
    {
        // no need of KVS for infos initialization
        pmi_context.local_process_rank = 0;
        pmi_context.node_rank = 0;
        pmi_context.node_count = 1;
        pmi_context.local_process_count = 1;
        return rc;
    }

    // now we need to put node info in KVS in order to determine the
    // following attributes,
    // since PMI1 does not provide direct access function:
    // nodes number, node rank, local processes number, local
    // process rank.

    // initialize infos
    pmi_context.local_process_rank = -1;
    pmi_context.node_rank = -1;
    pmi_context.node_count = 0;
    pmi_context.local_process_count = 0;

    int nodes_nb = 0, i, j;
    char *value = NULL;
    char *nodes = NULL;
    char hostname[SCTK_PMI_NAME_SIZE];

    value = sctk_malloc( SCTK_PMI_NAME_SIZE );
    assume(value);

    // Get hostname
    gethostname( hostname, SCTK_PMI_NAME_SIZE);

    // Put hostname on kvs for current process;
    snprintf(value,
             SCTK_PMI_NAME_SIZE,
             "%s",
             hostname );

    rc = sctk_pmi_put_as_rank(value,
                                      SCTK_PMI_TAG_PMI + SCTK_PMI_TAG_PMI_HOSTNAME);
    pmi_check_rc(rc, "sctk_pmi_put_as_rank");

    // wait for all processes to put their hostname in KVS
    rc = sctk_pmi_barrier();
    pmi_check_rc(rc, "sctk_pmi_barrier");

    // now retrieve hostnames for all processes and compute local infos
    nodes = sctk_calloc(pmi_context.process_count * SCTK_PMI_NAME_SIZE, sizeof(char));
    assume(nodes);

    // build nodes list and compute local ranks and size
    for ( i = 0; i < pmi_context.process_count; i++ )
    {
        j = 0;
        // get ith process hostname
        rc = sctk_pmi_get_as_rank(value, SCTK_PMI_NAME_SIZE,
                                  SCTK_PMI_TAG_PMI + SCTK_PMI_TAG_PMI_HOSTNAME, i );
        pmi_check_rc(rc, "sctk_pmi_get_as_rank");

        // compare value with current hostname. if the same, increase
        // local processes count
        if ( strcmp( hostname, value ) == 0 )
            pmi_context.local_process_count++;

        // acquire local process rank
        if ( i == pmi_context.process_rank )
            pmi_context.local_process_rank =
                pmi_context.local_process_count - 1;

        while ( strcmp( nodes + j * SCTK_PMI_NAME_SIZE, value ) != 0 && j < nodes_nb )
            j++;

        // update number of processes per node data
        struct process_nb_from_node_rank *tmp;
        HASH_FIND_INT( pmi_context.process_nb_from_node_rank, &j, tmp );

        if ( tmp == NULL )
        {
            // new node
            tmp = (struct process_nb_from_node_rank *) sctk_malloc(
                sizeof( struct process_nb_from_node_rank ) );
            tmp->node_rank = j;
            tmp->nb_process = 1;
            tmp->process_list = (int *) sctk_malloc( sizeof( int ) * 1 );
            tmp->process_list[0] = i;
            HASH_ADD_INT(pmi_context.process_nb_from_node_rank, node_rank, tmp);
        }
        else
        {
            // one more process on this node
            tmp->nb_process++;
            int *tmp_tab = sctk_realloc( tmp->process_list,
                                         sizeof( int ) * tmp->nb_process );
            assume( tmp_tab != NULL );
            tmp_tab[tmp->nb_process - 1] = i;
            tmp->process_list = tmp_tab;
        }

        if ( j == nodes_nb )
        {
            // found new node
            strncpy( nodes + j * SCTK_PMI_NAME_SIZE, value, SCTK_PMI_NAME_SIZE);
            nodes_nb++;
        }
    }

    /* Compute node rank */
    j = 0;

    while ( strcmp( nodes + j * SCTK_PMI_NAME_SIZE, hostname ) != 0 && j < nodes_nb )
        j++;

    pmi_context.node_rank = j;
    pmi_context.node_count = nodes_nb;

   /* Free temporary values */

    sctk_free( value );
    sctk_free( nodes );

    /* Set the whole context */
    mpc_common_set_process_rank( pmi_context.process_rank );
    mpc_common_set_process_count( pmi_context.process_count );

    int node_rank;
    sctk_pmi_get_node_rank( &node_rank );
    mpc_common_set_node_rank( node_rank );

    int node_count;
    sctk_pmi_get_node_count( &node_count );
    mpc_common_set_node_count( node_count );

    int local_process_rank;
    sctk_pmi_get_local_process_rank( &local_process_rank );
    mpc_common_set_local_process_rank( local_process_rank );

    int local_process_count;
    sctk_pmi_get_local_process_count( &local_process_count );
    mpc_common_set_local_process_count( local_process_count );

    return rc;
}

/*! \brief Finalization function */
int sctk_pmi_finalize()
{
#ifdef SCTK_LIB_MODE
    return PMI_SUCCESS;
#endif /* SCTK_LIB_MODE */
    int rc;

    // Finalize PMI/SLURM
    rc = PMI_Finalize();

    sctk_free( pmi_context.kvsname );
    return rc;
}

/*! \brief Get the job id
 * @param id Pointer to store the job id
*/
int sctk_pmi_get_job_id( int *id )
{
#if MPC_USE_HYDRA
    /* in mpich with pmi1, kvs name is used as job id. */
    *id = atoi( pmi_context.kvsname );
    return PMI_SUCCESS;
#elif defined( SCTK_LIB_MODE ) || defined( MPC_USE_SLURM )
    char *env = NULL;

    env = getenv( "SLURM_JOB_ID" );

    if ( env )
    {
        *id = atoi( env );
        return PMI_SUCCESS;
    }

    env = getenv( "SLURM_JOBID" );

    if ( env )
    {
        *id = atoi( env );
        return PMI_SUCCESS;
    }
    else
    {
        fprintf( stderr, "FAILURE (sctk_pmi): unable to get job ID\n" );
        *id = -1;
        return PMI_FAIL;
    }
    return PMI_SUCCESS;
#else
    not_implemented();
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
#endif /* SCTK_LIB_MODE */
    int rc;

    // Perform the barrier
    rc = PMI_Barrier();
    pmi_check_rc(rc, "PMI_Barrier");

    return rc;
}

/******************************************************************************
INFORMATION DIFFUSION
******************************************************************************/

/*! \brief Register information required for connection initialization
 * @param info The information to store
 * @param size Size in bytes of the information
 * @param key A key string to distinguish information
*/
int sctk_pmi_put( char *value, char *key )
{
#ifdef SCTK_LIB_MODE
    not_implemented();
    return PMI_FAIL;
#endif /* SCTK_LIB_MODE */
    int rc;

    // Put info in Key-Value-Space
    rc = PMI_KVS_Put( pmi_context.kvsname, key, value );
    pmi_check_rc(rc, "PMI_KVS_Put");

    // Apply changes on Key-Value-Space
    rc = PMI_KVS_Commit( pmi_context.kvsname );
    pmi_check_rc(rc, "PMI_KVS_Commit");

    return rc;
}

/*! \brief Register information required for connection initialization
 * @param info The information to store
 * @param size Size in bytes of the information
 * @param tag An identifier to distinguish information
*/
int sctk_pmi_put_as_rank( char *value, int tag )
{
#ifdef SCTK_LIB_MODE
    not_implemented();
    return PMI_FAIL;
#else  /* SCTK_LIB_MODE */
    int iRank, rc;
    char *key = NULL;

    // Get the process rank
    sctk_pmi_get_process_rank( &iRank );

    // Build the key
    key = (char *) sctk_malloc( pmi_context.max_key_len * sizeof( char ) );
    snprintf( key, pmi_context.max_key_len, "MPC_KEYS_%d_%d", tag, iRank );

    rc = sctk_pmi_put(value, key);

    sctk_free(key);
    return rc;
#endif /* SCTK_LIB_MODE */
}

/*! \brief Get information required for connection initialization
 * @param info The place to store the information to retrieve
 * @param size Size in bytes of the information
 * @param tag An identifier to distinguish information
 * @param rank Rank of the process the information comes from
*/
int sctk_pmi_get_as_rank( char *value, size_t size, int tag, int rank )
{
#ifdef SCTK_LIB_MODE
    not_implemented();
    return PMI_FAIL;
#endif /* SCTK_LIB_MODE */
    int rc;
    char *key = NULL;

    // Build the key
    key = (char *) sctk_malloc( pmi_context.max_key_len * sizeof( char ) );
    snprintf( key, pmi_context.max_key_len, "MPC_KEYS_%d_%d", tag, rank );

    // Get the value associated to the given key
    rc = PMI_KVS_Get( pmi_context.kvsname, key, value, size );

    sctk_free( key );
    return rc;
}

/*! \brief Get information required for connection initialization
 * @param info The place to store the information to retrieve
 * @param size Size in bytes of the information
 * @param tag A key string to distinguish information
*/
int sctk_pmi_get( char *value, size_t size, char *key )
{
#ifdef SCTK_LIB_MODE
    not_implemented();
    return PMI_FAIL;
#endif /* SCTK_LIB_MODE */
    int rc;

    // Get the value associated to the given key
    rc = PMI_KVS_Get( pmi_context.kvsname, key, value, size );
    pmi_check_rc(rc, "PMI_KVS_Get");

    return rc;
}
/******************************************************************************
NUMBERING/TOPOLOGY INFORMATION
******************************************************************************/
/*! \brief Get the number of processes
 * @param size Pointer to store the number of processes
*/
int sctk_pmi_get_process_count( int *size )
{
#ifdef SCTK_LIB_MODE
    *size = pmi_context.process_count;
    return PMI_SUCCESS;
#endif /* SCTK_LIB_MODE */

    int rc;
    // Get the total number of processes
    rc = PMI_Get_size( size );
    pmi_check_rc(rc, "PMI_Get_size");

    return rc;
}

/*! \brief Get the rank of this process
 * @param rank Pointer to store the rank of the process
*/
int sctk_pmi_get_process_rank( int *rank )
{
#ifdef SCTK_LIB_MODE
    *rank = pmi_context.process_rank;
    return PMI_SUCCESS;
#endif /* SCTK_LIB_MODE */
    int rc;

    // Get the rank of the current process
    rc = PMI_Get_rank( rank );
    pmi_check_rc(rc, "PMI_Get_rank");

    return rc;
}

/*! \brief Get the number of nodes
 * @param size Pointer to store the number of nodes
*/
int sctk_pmi_get_node_count( int *size )
{
#ifdef SCTK_LIB_MODE
    return sctk_pmi_get_process_count( size );
#elif defined(MPC_USE_HYDRA)
    *size = pmi_context.node_count;
    return PMI_SUCCESS;
#elif defined(MPC_USE_SLURM)
    char *env = NULL;

    env = getenv( "SLURM_NNODES" );

    if ( env )
    {
        *size = atoi( env );
    }
    else
    {
        fprintf( stderr, "FAILURE (sctk_pmi): unable to get nodes number\n" );
        *size = -1;
        return PMI_FAIL;
    }

    return PMI_SUCCESS;
#else
    not_implemented();
#endif

    return PMI_FAIL;
}

/*! \brief Get the rank of this node
 * @param rank Pointer to store the rank of the node
*/
int sctk_pmi_get_node_rank( int *rank )
{
#ifdef SCTK_LIB_MODE
    return sctk_pmi_get_process_rank( rank );
#elif defined(MPC_USE_HYDRA)
    *rank = pmi_context.node_rank;
    return PMI_SUCCESS;
#elif defined(MPC_USE_SLURM)
    char *env = NULL;

    env = getenv( "SLURM_NODEID" );

    if ( env )
    {
        *rank = atoi( env );
    }
    else
    {
        fprintf( stderr, "FAILURE (sctk_pmi): unable to get node rank\n" );
        *rank = -1;
        return PMI_FAIL;
    }

    return PMI_SUCCESS;
#else
    not_implemented();
#endif

    return PMI_FAIL;
}

/*! \brief Get the number of processes on the current node
 * @param size Pointer to store the number of processes
*/
int sctk_pmi_get_local_process_count( int *size )
{
#ifdef SCTK_LIB_MODE
    *size = mpc_common_get_local_process_count();
    return PMI_SUCCESS;
#elif defined(MPC_USE_HYDRA)
    *size = pmi_context.local_process_count;
    return PMI_SUCCESS;
#elif defined(MPC_USE_SLURM)
    int rc;

    // Get the number of processes on the current node
    if ((rc = PMI_Get_clique_size(size)) != PMI_SUCCESS)
    {
        fprintf( stderr, "FAILURE (sctk_pmi): PMI_Get_clique_size: %d\n", rc );
        *size = -1;
        return PMI_FAIL;
    }

    return PMI_SUCCESS;
#else
    not_implemented();
#endif

}

/*! \brief Get the rank of this process on the current node
 * @param rank Pointer to store the rank of the process
*/
int sctk_pmi_get_local_process_rank( int *rank )
{
#ifdef SCTK_LIB_MODE
    *rank = mpc_common_get_local_process_rank();
    return PMI_SUCCESS;
#elif defined(MPC_USE_HYDRA)
    *rank = pmi_context.local_process_rank;
    return PMI_SUCCESS;
#elif defined(MPC_USE_SLURM)
    char *env = NULL;

    env = getenv( "SLURM_LOCALID" );

    if ( env )
    {
        *rank = atoi( env );
    }
    else
    {
        fprintf( stderr, "FAILURE (sctk_pmi): unable to get process rank on the node\n" );
        *rank = -1;
        return PMI_FAIL;
    }

    return PMI_SUCCESS;
#else
    not_implemented();
#endif /* MPC_USE_SLURM */

}

int sctk_pmi_get_process_number_from_node_rank( struct process_nb_from_node_rank **process_number_from_node_rank )
{
    *process_number_from_node_rank = pmi_context.process_nb_from_node_rank;
    return PMI_SUCCESS;
}


void sctk_net_abort()
{
#if defined(SCTK_LIB_MODE)
    abort();
#else
    PMI_Abort( 6, "ABORT from sctk_net_abort" );
#endif
}
