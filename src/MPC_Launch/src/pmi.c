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

#include <mpc_launch_pmi.h>

#include <mpc_common_helper.h>
#include <mpc_common_rank.h>
#include <mpc_common_types.h>
#include <mpc_common_flags.h>
#include "sctk_alloc.h"
#include "mpc_common_debug.h"

#include "mpc_launch.h"

#ifdef MPC_USE_PMIX
	#include <pmix.h>
	#define PMI_SUCCESS PMIX_SUCCESS
	#define PMI_FAIL PMIX_FAIL
#else
	#include <pmi.h>
#endif

struct mpc_pmi_context
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
	char *kvsname;

	/** If spawned */
	int spawned;

	#ifdef MPC_USE_PMIX
		pmix_proc_t pmix_proc;
	#else

		/** PMI lengths */
		int kvsname_len;
		int max_val_len;
		int max_key_len;

	#endif

	/** Convert struct */
	struct mpc_launch_pmi_process_layout *process_nb_from_node_rank;

	/** Global initialization flag */
	int initialized;
};

static struct mpc_pmi_context pmi_context;

void __mpc_launch_pmi_context_clear( void )
{
	memset( &pmi_context, 0, sizeof( struct mpc_pmi_context ) );
}

/******************************************************************************
INITIALIZATION/FINALIZE
******************************************************************************/

#define PMI_CHECK_RC(retcode, ctx)do\
{\
	if ( retcode != PMI_SUCCESS )\
	{\
		fprintf( stderr, "FAILURE (mpc_launch_pmi): %s: %d\n", ctx, retcode );\
		MPC_CRASH();\
	}\
}while(0)


#define PMI_RETURN(retcode)do\
{\
	if ( retcode != PMI_SUCCESS )\
	{\
		return MPC_LAUNCH_PMI_FAIL;\
	} else {\
		return MPC_LAUNCH_PMI_SUCCESS;\
	}\
}while(0)




/******************************************************************************
NUMBERING/TOPOLOGY INFORMATION
******************************************************************************/

#if defined(MPC_USE_PMIX)
static inline pmix_status_t __pmix_get_attribute(int rank_level, const char key[], pmix_data_type_t type, void * out)
{
	int ret = -1;
	pmix_proc_t *proc = &pmi_context.pmix_proc;

	pmix_proc_t wproc;

	if(!rank_level)
	{
		PMIX_PROC_CONSTRUCT(&wproc);
		(void)strncpy(wproc.nspace, pmi_context.pmix_proc.nspace, PMIX_MAX_NSLEN);
		wproc.rank = PMIX_RANK_WILDCARD;
		proc = &wproc;
	}

	pmix_value_t *val = NULL;

	pmix_status_t rc = PMIx_Get(proc, key, NULL, 0, &val);

	if(rc != PMI_SUCCESS)
	{
		return rc;
	}

	assume(val != NULL);

	switch (val->type)
	{
		case PMIX_INT16:
			memcpy(out, &val->data.int16, sizeof(int16_t));
		break;
		case PMIX_INT32:
			memcpy(out, &val->data.int32, sizeof(int32_t));
		break;
		case PMIX_UINT16:
			memcpy(out, &val->data.int16, sizeof(uint16_t));
		break;
		case PMIX_UINT32:
			memcpy(out, &val->data.int32, sizeof(uint32_t));
		break;
		case PMIX_PROC_RANK:
			memcpy(out, &val->data.rank, sizeof(pmix_rank_t));
		break;
	}

	if(!rank_level)
	{
		PMIX_PROC_DESTRUCT(&wproc);
	}

	PMIX_VALUE_RELEASE(val);
	PMI_RETURN( rc );
}


#endif


/*! \brief Get the rank of this process
 * @param rank Pointer to store the rank of the process
*/
static inline int __mpc_pmi_get_process_rank( int *rank )
{
#if defined(MPC_USE_PMIX)
	*rank = pmi_context.pmix_proc.rank;
	return MPC_LAUNCH_PMI_SUCCESS; 
#else
	int rc;
	// Get the rank of the current process
	rc = PMI_Get_rank( rank );
	PMI_RETURN( rc );
#endif
}

/*! \brief Get the number of processes
	* @param size Pointer to store the number of processes
*/
	static inline int __mpc_pmi_get_process_count( int *size )
{
#if defined(MPC_USE_PMIX)
	uint32_t usize;
	*size = -1;
	pmix_status_t rc = __pmix_get_attribute(0, PMIX_JOB_SIZE, PMIX_UINT32, &usize);
	PMI_CHECK_RC( rc, "__pmix_get_attribute" );
	*size = usize;
	PMI_RETURN( rc );
#else
	int rc;
	// Get the total number of processes
	rc = PMI_Get_size( size );
	PMI_RETURN( rc );
#endif
}


/*! \brief Get the rank of this process on the current node
 * @param rank Pointer to store the rank of the process
*/
static inline int __mpc_pmi_get_local_process_rank( int *rank )
{
#if defined(MPC_USE_PMIX)
	pmix_value_t *val = NULL;
	pmix_status_t rc = PMIx_Get(&pmi_context.pmix_proc, PMIX_LOCAL_RANK, NULL, 0, &val);
	PMI_CHECK_RC( rc, "PMIx_Get" );
	assume(val != NULL);
	*rank = val->data.uint16;
	PMIX_VALUE_RELEASE(val);
	PMI_RETURN( rc );
#elif defined(MPC_USE_HYDRA)
	*rank = pmi_context.local_process_rank;
	return MPC_LAUNCH_PMI_SUCCESS;
#elif defined(MPC_USE_PMI1)
	char *env = NULL;
	env = getenv( "SLURM_LOCALID" );

	if ( env )
	{
		*rank = atoi( env );
	}
	else
	{
		fprintf( stderr, "FAILURE (mpc_launch_pmi): unable to get process rank on the node\n" );
		*rank = -1;
		return MPC_LAUNCH_PMI_FAIL  ;
	}

	return MPC_LAUNCH_PMI_SUCCESS;
#else
	not_implemented();
#endif /* MPC_USE_PMI1 */
}

/*! \brief Get the number of processes on the current node
 * @param size Pointer to store the number of processes
*/
static inline int __mpc_pmi_get_local_process_count( int *size )
{
#if defined(MPC_USE_PMIX)
	uint32_t usize;
	*size = -1;
	pmix_status_t rc = __pmix_get_attribute(0, PMIX_LOCAL_SIZE, PMIX_UINT32, &usize);
	PMI_CHECK_RC( rc, "__pmix_get_attribute" );
	*size = usize;
	PMI_RETURN( rc );
#elif defined(MPC_USE_HYDRA) || defined(MPC_USE_PMI1)

	/* Testing with new PMI we had PMI_Get_clique_size returning -1
	   on PMIX compat so we rely on the manual computation
	   when running with slurm 20.02.3 and openpmi 3.1.5 */
	*size = pmi_context.local_process_count;
	return MPC_LAUNCH_PMI_SUCCESS;
#elif 0
	int rc;

	// Get the number of processes on the current node
	if ( ( rc = PMI_Get_clique_size( size ) ) != PMI_SUCCESS )
	{
		fprintf( stderr, "FAILURE (mpc_launch_pmi): PMI_Get_clique_size: %d\n", rc );
		*size = -1;
		return MPC_LAUNCH_PMI_FAIL;
	}

	return MPC_LAUNCH_PMI_SUCCESS;
#else
	not_implemented();
#endif
}

static inline int __get_node_rank_from_process_rank_fallback(int process_rank);

/*! \brief Get the rank of this node
 * @param rank Pointer to store the rank of the node
*/
static inline int __mpc_pmi_get_node_rank( int *rank )
{
#if defined(MPC_USE_HYDRA)
	*rank = pmi_context.node_rank;
	return MPC_LAUNCH_PMI_SUCCESS;
#elif defined(MPC_USE_PMI1)
	char *env = NULL;
	env = getenv( "SLURM_NODEID" );

	if ( env )
	{
		*rank = atoi( env );
	}
	else
	{
		fprintf( stderr, "FAILURE (mpc_launch_pmi): unable to get node rank\n" );
		*rank = -1;
		return MPC_LAUNCH_PMI_FAIL;
	}

	return MPC_LAUNCH_PMI_SUCCESS;
#elif defined(MPC_USE_PMIX)

	uint32_t urank;
	*rank = -1;
	pmix_status_t rc = __pmix_get_attribute(0, PMIX_NODEID, PMIX_UINT32, &urank);

	if(rc != PMI_SUCCESS)
	{
		mpc_common_tracepoint("PMI: using fallback nodeid");
		/* Note this may fail on some PMIx versions (not sure why)
		possible pointer is https://github.com/openpmix/openpmix/issues/1965
		for example on 3.2.2rc1 it works in slurm but not with prrte
		we then use the fallback lookup inside the local process map */
		int prank = -1;
		rc = __mpc_pmi_get_process_rank( &prank );
		PMI_CHECK_RC( rc, "__mpc_pmi_get_process_rank for Node Rank" );
		*rank = __get_node_rank_from_process_rank_fallback(prank);
		PMI_RETURN( rc );
	}

	*rank = urank;
	PMI_RETURN( rc );
#else
	not_implemented();
#endif
	return MPC_LAUNCH_PMI_FAIL;
}

/*! \brief Get the number of nodes
 * @param size Pointer to store the number of nodes
*/
static inline int __mpc_pmi_get_node_count( int *size )
{
#if defined(MPC_USE_HYDRA)
	*size = pmi_context.node_count;
	return MPC_LAUNCH_PMI_SUCCESS;
#elif defined(MPC_USE_PMI1)
	char *env = NULL;
	env = getenv( "SLURM_NNODES" );

	if ( env )
	{
		*size = atoi( env );
	}
	else
	{
		fprintf( stderr, "FAILURE (mpc_launch_pmi): unable to get nodes number\n" );
		*size = -1;
		return MPC_LAUNCH_PMI_FAIL;
	}

	return MPC_LAUNCH_PMI_SUCCESS;
#elif defined(MPC_USE_PMIX)
	uint32_t usize;
	*size = -1;
	pmix_status_t rc = __pmix_get_attribute(0, PMIX_NUM_NODES, PMIX_UINT32, &usize);
	PMI_CHECK_RC( rc, "__pmix_get_attribute" );
	*size = usize;
	PMI_RETURN( rc );
#else
	not_implemented();
#endif
	return MPC_LAUNCH_PMI_FAIL;
}


/******************************
 * INITIALIZATION AND RELEASE *
 ******************************/

static inline void __set_ctx_to_serial(struct mpc_pmi_context *ctx)
{
	ctx->local_process_rank = 0;
	ctx->node_rank = 0;
	ctx->node_count = 1;
	ctx->local_process_count = 1;

	/* Now proceed to add a single node to process layout */
	struct mpc_launch_pmi_process_layout *tmp = ( struct mpc_launch_pmi_process_layout * ) sctk_malloc(	sizeof( struct mpc_launch_pmi_process_layout ) );
	tmp->node_rank = 0;
	tmp->nb_process = 1;
	tmp->process_list = ( int * ) sctk_malloc( sizeof( int ) * 1 );
	tmp->process_list[0] = 0;
	HASH_ADD_INT( ctx->process_nb_from_node_rank, node_rank, tmp );
}


static inline int _pmi_initialize(struct mpc_pmi_context *ctx, int * unreachable)
{

	char * exename = mpc_common_get_flags()->exename;

	if( exename )
	{
		//If the command is mpc_print_config do not even try to bootstrap PMIx
		if(strstr(exename, "mpc_print_config"))
		{
			goto PMI_INIT_SERIAL;
		}
	}

	#ifdef MPC_USE_PMIX

		pmix_status_t rc = PMI_SUCCESS;

		rc = PMIx_Init(&ctx->pmix_proc,
                       NULL,
					   0);
		if( (rc == PMIX_ERR_UNREACH) || (rc == PMIX_ERR_INVALID_NAMESPACE) )
		{
			mpc_common_debug("PMIx was unreachable");
			goto PMI_INIT_SERIAL;
		}
		else
		{
			PMI_CHECK_RC( rc, "PMIx_Init" );
		}

		ctx->initialized = PMIx_Initialized();

		mpc_common_tracepoint_fmt("PMI: init done ? %d", ctx->initialized);


		if ( ctx->initialized != 1 )
		{
			fprintf( stderr, "FAILURE (mpc_launch_pmi): PMIx_Initialized returned false\n" );
			return MPC_LAUNCH_PMI_FAIL;
		}
	#else /* PMI / HYDRA */
		int rc;

		/* For PMI1 we do not support the Unreachable PMI case */
		if( mpc_common_get_flags()->process_number == 1)
		{
			goto PMI_INIT_SERIAL;
		}

		// Initialized PMI/SLURM
		rc = PMI_Init( &ctx->spawned );
		PMI_CHECK_RC( rc, "PMI_Init" );
		// Check if PMI/SLURM is initialized
		ctx->initialized = 0;
		rc = PMI_Initialized( &ctx->initialized );
		PMI_CHECK_RC( rc, "PMI_Initialized" );

		if ( ctx->initialized != 1 )
		{
			fprintf( stderr, "FAILURE (mpc_launch_pmi): PMI_Initialized returned false\n" );
			PMI_RETURN( rc );
		}

		// Get key, value max sizes and kvsname.
		// NB: Need to initialize kvsname here to avoid error in barrier with slurm
		rc = PMI_KVS_Get_name_length_max( &ctx->kvsname_len );
		PMI_CHECK_RC( rc, "PMI_KVS_Get_name_length_max" );
		rc = PMI_KVS_Get_key_length_max( &ctx->max_key_len );
		PMI_CHECK_RC( rc, "PMI_KVS_Get_key_length_max" );
		rc = PMI_KVS_Get_value_length_max( &ctx->max_val_len );
		PMI_CHECK_RC( rc, "PMI_KVS_Get_value_length_max" );
		// Get the kvs name
		ctx->kvsname = ( char * ) sctk_malloc( ctx->kvsname_len * sizeof( char ) );
		assume( ctx->kvsname );
		rc = PMI_KVS_Get_my_name( ctx->kvsname, ctx->kvsname_len );
		PMI_CHECK_RC( rc, "PMI_KVS_Get_my_name" );

	#endif

	/* There should be no error but PMIX could be surprising */ 

	rc = __mpc_pmi_get_process_rank( &ctx->process_rank );
	
	if(rc != PMI_SUCCESS )
	{
		goto PMI_INIT_SERIAL;
	}
	
	rc = __mpc_pmi_get_process_count( &ctx->process_count );
	
	if(rc != PMI_SUCCESS )
	{
		goto PMI_INIT_SERIAL;
	}
	return MPC_LAUNCH_PMI_SUCCESS;

PMI_INIT_SERIAL:
	*unreachable = 1;
	/* This is the case where we have no PMI server assume SERIAL */
	__set_ctx_to_serial(ctx);
	PMI_RETURN( PMI_SUCCESS );
}



#define MPC_LAUNCH_PMI_HOSTNAME_SIZE 64
#define MPC_LAUNCH_PMI_TAG_PMI 6000
#define MPC_LAUNCH_PMI_TAG_PMI_HOSTNAME 1

int __legacy_process_layout_init(void)
{
	int rc;
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
	char hostname[MPC_LAUNCH_PMI_HOSTNAME_SIZE];
	value = sctk_malloc( MPC_LAUNCH_PMI_HOSTNAME_SIZE );
	assume( value );
	// Get hostname
	gethostname( hostname, MPC_LAUNCH_PMI_HOSTNAME_SIZE );
	// Put hostname on kvs for current process;
	snprintf( value,
			MPC_LAUNCH_PMI_HOSTNAME_SIZE,
			"%s",
			hostname );
	rc = mpc_launch_pmi_put_as_rank( value,
									MPC_LAUNCH_PMI_TAG_PMI + MPC_LAUNCH_PMI_TAG_PMI_HOSTNAME, 0 );
	PMI_CHECK_RC( rc, "mpc_launch_pmi_put_as_rank" );
	// wait for all processes to put their hostname in KVS
	rc = mpc_launch_pmi_barrier();
	PMI_CHECK_RC( rc, "mpc_launch_pmi_barrier" );
	// now retrieve hostnames for all processes and compute local infos
	nodes = sctk_calloc( pmi_context.process_count * MPC_LAUNCH_PMI_HOSTNAME_SIZE, sizeof( char ) );
	assume( nodes );

	// build nodes list and compute local ranks and size
	for ( i = 0; i < pmi_context.process_count; i++ )
	{
		j = 0;
		// get ith process hostname
		rc = mpc_launch_pmi_get_as_rank( value, MPC_LAUNCH_PMI_HOSTNAME_SIZE,
										MPC_LAUNCH_PMI_TAG_PMI + MPC_LAUNCH_PMI_TAG_PMI_HOSTNAME, i );
		PMI_CHECK_RC( rc, "mpc_launch_pmi_get_as_rank" );

		// compare value with current hostname. if the same, increase
		// local processes count
		if ( strcmp( hostname, value ) == 0 )
		{
			pmi_context.local_process_count++;
		}

		// acquire local process rank
		if ( i == pmi_context.process_rank )
			pmi_context.local_process_rank =
				pmi_context.local_process_count - 1;

		while ( strcmp( nodes + j * MPC_LAUNCH_PMI_HOSTNAME_SIZE, value ) != 0 && j < nodes_nb )
		{
			j++;
		}

		// update number of processes per node data
		struct mpc_launch_pmi_process_layout *tmp;
		HASH_FIND_INT( pmi_context.process_nb_from_node_rank, &j, tmp );

		if ( tmp == NULL )
		{
			// new node
			tmp = ( struct mpc_launch_pmi_process_layout * ) sctk_malloc(
					sizeof( struct mpc_launch_pmi_process_layout ) );
			tmp->node_rank = j;
			tmp->nb_process = 1;
			tmp->process_list = ( int * ) sctk_malloc( sizeof( int ) * 1 );
			tmp->process_list[0] = i;
			HASH_ADD_INT( pmi_context.process_nb_from_node_rank, node_rank, tmp );
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
			strncpy( nodes + j * MPC_LAUNCH_PMI_HOSTNAME_SIZE, value, MPC_LAUNCH_PMI_HOSTNAME_SIZE );
			nodes_nb++;
		}
	}

	/* Compute node rank */
	j = 0;

	while ( strcmp( nodes + j * MPC_LAUNCH_PMI_HOSTNAME_SIZE, hostname ) != 0 && j < nodes_nb )
	{
		j++;
	}

	pmi_context.node_rank = j;
	pmi_context.node_count = nodes_nb;
	/* Free temporary values */
	sctk_free( value );
	sctk_free( nodes );

	return 0;
}

#ifdef MPC_USE_PMIX

static inline void __pmix_process_layout_init()
{
	char *nodelist = NULL;
	pmix_status_t rc = PMIx_Resolve_nodes(pmi_context.pmix_proc.nspace, &nodelist);
	PMI_CHECK_RC(rc, "PMIx_Resolve_nodes");

	/* Now iterate over the nodelist which is comma separated */

	int node_rank = 0;

	char * node = strtok ( nodelist, "," );
    while ( node != NULL ) {
		pmix_proc_t *procs = NULL;
		size_t nprocs = 0;

		rc = PMIx_Resolve_peers(node, pmi_context.pmix_proc.nspace, &procs, &nprocs);
		PMI_CHECK_RC(rc, "PMIx_Resolve_peers");

		struct mpc_launch_pmi_process_layout * layout = sctk_malloc(sizeof(struct mpc_launch_pmi_process_layout));
		assume(layout != NULL);

		layout->node_rank = node_rank;
		layout->nb_process = nprocs;
		layout->process_list = sctk_malloc(nprocs * sizeof(int));
		assume(layout->process_list);

		int i;
		for(i=0; i < nprocs; i++)
		{
			layout->process_list[i] = procs[i].rank;
		}

		HASH_ADD_INT( pmi_context.process_nb_from_node_rank, node_rank, layout );

		PMIX_PROC_FREE(procs, nprocs);
		/* Go to next node */
		node_rank++;
        node = strtok ( NULL, "," );
    }
}

#endif

static void __process_layout_init(void)
{
	pmi_context.process_nb_from_node_rank = NULL;

	#ifdef MPC_USE_PMIX
		__pmix_process_layout_init();
	#else
		__legacy_process_layout_init();
	#endif
}

int mpc_launch_pmi_get_process_layout( struct mpc_launch_pmi_process_layout **layout )
{
	*layout = pmi_context.process_nb_from_node_rank;
	return MPC_LAUNCH_PMI_SUCCESS;
}

/** This is the fallback code when the nodeid attribute is buggy */
static inline int __get_node_rank_from_process_rank_fallback(int process_rank)
{
	struct mpc_launch_pmi_process_layout * tmp = NULL, *current = NULL;

	HASH_ITER(hh, pmi_context.process_nb_from_node_rank, current, tmp)
	{
		int i;
		for(i = 0 ; i < current->nb_process; i++)
		{
			if(current->process_list[i] == process_rank){
				return current->node_rank;
			}
		}
	}

	not_reachable();
}


/*! \brief Initialization function
 *
 */
int mpc_launch_pmi_init()
{
	__mpc_launch_pmi_context_clear();

	int unreachable = 0;
	int rc = _pmi_initialize(&pmi_context, &unreachable);

	if( rc != MPC_LAUNCH_PMI_SUCCESS)
	{
		mpc_common_tracepoint_fmt("PMI: Failed initializing PMI (ret %d)", rc);

		return MPC_LAUNCH_PMI_FAIL;
	}

	/* In this case PMI_Init instructed to skip init */
	if(unreachable)
	{
		PMI_RETURN( rc );
	}

	__mpc_pmi_get_process_count(&pmi_context.process_count);

	/* This is a fastpath for the serial case */
	if(pmi_context.process_count == 1)
	{
		__set_ctx_to_serial(&pmi_context);
		PMI_RETURN( PMI_SUCCESS );
	}

	// check if PMI_Get_size is equal to 1 (could mean application
	// launched without mpiexec)
	__mpc_pmi_get_process_rank(&pmi_context.process_rank);


	mpc_common_tracepoint_fmt("PMI: this is process %d/%d", pmi_context.process_rank, pmi_context.process_count);

	__process_layout_init();

	/* Set the whole context */
	mpc_common_set_process_rank( pmi_context.process_rank );
	mpc_common_set_process_count( pmi_context.process_count );

	if( 0u < mpc_common_get_flags()->process_number)
		assume(mpc_common_get_flags()->process_number == (unsigned int)pmi_context.process_count);
	/* Get process number from PMI */
	mpc_common_get_flags()->process_number = pmi_context.process_count;

	/* Ensure N is at least P for bare srun configurations */
	if(mpc_common_get_flags()->task_number < mpc_common_get_flags()->process_number)
	{
		mpc_common_get_flags()->task_number = mpc_common_get_flags()->process_number;
	}

	int node_rank;
	__mpc_pmi_get_node_rank( &node_rank );
	mpc_common_set_node_rank( node_rank );
	int node_count;
	__mpc_pmi_get_node_count( &node_count );
	mpc_common_set_node_count( node_count );
	int local_process_rank;
	__mpc_pmi_get_local_process_rank( &local_process_rank );
	mpc_common_set_local_process_rank( local_process_rank );
	int local_process_count;
	__mpc_pmi_get_local_process_count( &local_process_count );
	mpc_common_set_local_process_count( local_process_count );

	mpc_common_tracepoint_fmt("DONE Starting PMI N%d/%d LPR%d/%d", node_rank, node_count, local_process_rank, local_process_count);

	PMI_RETURN( rc );
}

/*! \brief Finalization function */
int mpc_launch_pmi_finalize()
{
	mpc_common_debug("ENDING PMI");
#if defined(MPC_USE_PMIX)
	pmix_status_t rc = PMIx_Finalize(NULL, 0);
	PMI_RETURN( rc );
#else
	int rc;
	// Finalize PMI/SLURM
	rc = PMI_Finalize();
	sctk_free( pmi_context.kvsname );
	PMI_RETURN( rc );
#endif
}


/********************
 * PUBLIC INTERFACE *
 ********************/

/******************************************************************************
SYNCHRONIZATION
******************************************************************************/
/*! \brief Perform a barrier between all the processes of the job
 *
*/
int mpc_launch_pmi_barrier()
{
#if defined(MPC_USE_PMIX)
	pmix_info_t *info = NULL;
    PMIX_INFO_CREATE(info, 1);
    int flag = true;
    PMIX_INFO_LOAD(info, PMIX_COLLECT_DATA, &flag, PMIX_BOOL);
    pmix_status_t rc = PMIx_Fence(NULL, 0, info, 1);
	PMI_CHECK_RC( rc, "PMIx_Fence" );
	PMIX_INFO_FREE(info, 1);
	return MPC_LAUNCH_PMI_SUCCESS;
#else
	int rc;
	// Perform the barrier
	rc = PMI_Barrier();
	PMI_CHECK_RC( rc, "PMI_Barrier" );
	return MPC_LAUNCH_PMI_SUCCESS;
#endif
}

#ifdef MPC_USE_PMIX
	int mpc_launch_pmi_get_max_key_len()
	{
		return PMIX_MAX_KEYLEN;
	}

	int mpc_launch_pmi_get_max_val_len()
	{
		return PMIX_MAX_KEYLEN;
	}
#else
	int mpc_launch_pmi_get_max_key_len()
	{
		return pmi_context.max_key_len;
	}

	int mpc_launch_pmi_get_max_val_len()
	{
		return pmi_context.max_val_len;
	}
#endif

int mpc_launch_pmi_is_initialized()
{
	return pmi_context.initialized;
}


void mpc_launch_pmi_abort()
{
#if defined(MPC_USE_PMIX)
	PMIx_Abort(6, "ABORT from mpc_launch_pmi_abort", NULL, 0);
#else
	PMI_Abort( 6, "ABORT from mpc_launch_pmi_abort" );
#endif
}


/*! \brief Get the job id
 * @param id Pointer to store the job id
*/
int mpc_launch_pmi_get_job_id( int *id )
{
#if MPC_USE_HYDRA
	/* in mpich with pmi1, kvs name is used as job id. */
	*id = atoi( pmi_context.kvsname );
	return MPC_LAUNCH_PMI_SUCCESS;
#elif defined( MPC_USE_PMI1 )
	char *env = NULL;
	env = getenv( "SLURM_JOB_ID" );

	if ( env )
	{
		*id = atoi( env );
		return MPC_LAUNCH_PMI_SUCCESS;
	}

	env = getenv( "SLURM_JOBID" );

	if ( env )
	{
		*id = atoi( env );
		return MPC_LAUNCH_PMI_SUCCESS;
	}
	else
	{
		fprintf( stderr, "FAILURE (mpc_launch_pmi): unable to get job ID\n" );
		*id = -1;
		return MPC_LAUNCH_PMI_FAIL;
	}

	return MPC_LAUNCH_PMI_SUCCESS;
#else
	not_implemented();
#endif
}

/******************************************************************************
INFORMATION DIFFUSION
******************************************************************************/

int mpc_launch_pmi_put( char *value, char *key, int is_local)
{
#if defined(MPC_USE_PMIX)
	pmix_status_t rc;
	pmix_value_t val;
    val.type = PMIX_STRING;
    val.data.string = value;

	pmix_scope_t scope = is_local?PMIX_LOCAL:PMIX_GLOBAL;

	rc = PMIx_Put(scope, key, &val);
	PMI_CHECK_RC( rc, "PMIx_Put" );
	rc = PMIx_Commit();
	PMI_CHECK_RC( rc, "PMIx_Commit" );
	PMI_RETURN( rc );
#else
	UNUSED(is_local);
	int rc;
	// Put info in Key-Value-Space
	rc = PMI_KVS_Put( pmi_context.kvsname, key, value );
	PMI_CHECK_RC( rc, "PMI_KVS_Put" );
	// Apply changes on Key-Value-Space
	rc = PMI_KVS_Commit( pmi_context.kvsname );
	PMI_CHECK_RC( rc, "PMI_KVS_Commit" );
	PMI_RETURN( rc );
#endif
}

int mpc_launch_pmi_put_as_rank( char *value, int tag, int is_local )
{
	int my_rank, rc;
	char *key = NULL;
	// Get the process rank
	__mpc_pmi_get_process_rank( &my_rank );
	// Build the key
	key = ( char * ) sctk_malloc( mpc_launch_pmi_get_max_key_len() * sizeof( char ) );
	snprintf( key, mpc_launch_pmi_get_max_key_len(), "MPC_KEYS_%d_%d", tag, my_rank );
	rc = mpc_launch_pmi_put( value, key, is_local );
	mpc_common_debug("PUT (%s) %s @ %s as %d", is_local?"local":"global", value, key, my_rank);
	sctk_free( key );
	PMI_RETURN( rc );
}


int mpc_launch_pmi_get( char *value, size_t size, char *key, int remote)
{
#if defined(MPC_USE_PMIX)
	pmix_status_t rc;
	pmix_value_t *val = NULL;

	pmix_proc_t proc;

	PMIX_PROC_CONSTRUCT(&proc);
	(void)strncpy(proc.nspace, pmi_context.pmix_proc.nspace, PMIX_MAX_NSLEN);
	proc.rank = remote;
	mpc_common_debug("GET %s from %d", key, remote);
	rc = PMIx_Get(&proc,key, NULL, 0, &val);
	PMI_CHECK_RC( rc, "PMIx_Get" );
	assume(val != NULL);
	assume(val->type == PMIX_STRING);
	strcpy(value, val->data.string);
	PMIX_VALUE_RELEASE(val);
	PMIX_PROC_DESTRUCT(proc);
	PMI_RETURN( rc );
#else
	UNUSED(remote);
	int rc;

	// Get the value associated to the given key
	rc = PMI_KVS_Get( pmi_context.kvsname, key, value, size );
	PMI_CHECK_RC( rc, "PMI_KVS_Get" );
	PMI_RETURN( rc );
#endif
}

int mpc_launch_pmi_get_as_rank( char *value, size_t size, int tag, int rank )
{
	int rc;
	char *key = NULL;
	// Build the key
	key = ( char * ) sctk_malloc( mpc_launch_pmi_get_max_key_len() * sizeof( char ) );
	snprintf( key, mpc_launch_pmi_get_max_key_len(), "MPC_KEYS_%d_%d", tag, rank );
	// Get the value associated to the given key
	rc = mpc_launch_pmi_get(value, size, key, rank);
	sctk_free( key );
	PMI_RETURN( rc );
}

int mpc_launch_pmi_get_univ_size(int* univsize){
	#if defined(MPC_USE_PMIX)
		pmix_proc_t proc;
		pmix_status_t ret;
		pmix_value_t* val;
		PMIX_PROC_LOAD(&proc, pmi_context.pmix_proc.nspace, PMIX_RANK_WILDCARD);
		ret = PMIx_Get(&proc, PMIX_UNIV_SIZE, NULL, 0 , &val);
		if(PMIX_SUCCESS == ret) *univsize = val->data.uint32;
		return ret == PMIX_SUCCESS;
	#else
	// TODO
	#endif
}


int mpc_launch_pmi_get_app_rank(int* appname){
	#if defined(MPC_USE_PMIX)
		pmix_status_t ret;
		pmix_value_t* val;
		ret = PMIx_Get(&pmi_context.pmix_proc, PMIX_APPNUM, NULL, 0 , &val);
		if(PMIX_SUCCESS == ret) *appname = val->data.uint32;
		else printf("get appnum returned %d", ret);
		return ret == PMIX_SUCCESS;
	#else
	// TODO
	#endif
}

int mpc_launch_pmi_get_app_size(int* appsize){
	#if defined(MPC_USE_PMIX)
		pmix_proc_t proc;
		pmix_status_t ret;
		pmix_value_t* val;
		PMIX_PROC_LOAD(&proc, pmi_context.pmix_proc.nspace, PMIX_RANK_WILDCARD);
		ret = PMIx_Get(&proc, PMIX_APP_SIZE, NULL, 0 , &val);
		if(PMIX_SUCCESS == ret) *appsize = val->data.uint32;
		// else printf("get app_size returned %d", ret);
		return ret == PMIX_SUCCESS;
	#else
	// TODO
	#endif
}

// does not work

int mpc_launch_pmi_get_app_num(int* appnum){
	#if defined(MPC_USE_PMIX)
		pmix_proc_t proc;
		pmix_status_t ret;
		pmix_value_t* val;
		PMIX_PROC_LOAD(&proc, pmi_context.pmix_proc.nspace, PMIX_RANK_WILDCARD);
		ret = PMIx_Get(&proc, PMIX_JOB_NUM_APPS, NULL, 0 , &val);
		if(PMIX_SUCCESS == ret) *appnum = val->data.uint32;
		// else printf("get job_num_apps returned %d", ret);
		return ret == PMIX_SUCCESS;
	#else
	// TODO
	#endif
}

// get with PMIX_SET_NAMES does not work
int mpc_launch_pmi_get_pset(char** pset){
	#if defined(MPC_USE_PMIX) && defined(PMIx_VERSION_H)
		#if PMIX_VERSION_MAJOR >= 4
			pmix_value_t* val;
			pmix_status_t ret;
			char* my_set;

			ret = PMIx_Get(&pmi_context.pmix_proc, PMIX_PSET_NAMES, NULL, 0, &val); // fonctionne pas ?
			if(ret == PMIX_SUCCESS) my_set = val->data.string;
			// else printf("get pmix_pset_names returned %d", ret);
			return ret == PMIX_SUCCESS;
		#endif
	#else
		not_implemented();
	#endif
}


int mpc_launch_pmi_get_pset_list(char** psetlist){
	#if defined(MPC_USE_PMIX) && defined(PMIx_VERSION_H)
		#if PMIX_VERSION_MAJOR >= 4
			pmix_value_t* val;
			pmix_status_t ret;
			pmix_query_t query;

			PMIX_QUERY_CONSTRUCT(&query);
			PMIX_ARGV_APPEND(ret, query.keys, PMIX_QUERY_NUM_PSETS);
			PMIX_ARGV_APPEND(ret, query.keys, PMIX_QUERY_PSET_NAMES);
			pmix_info_t* result = NULL;
			size_t result_size;

			ret = PMIx_Query_info(&query, 1, &result, &result_size);
			if(ret == PMIX_SUCCESS) *psetlist = result[1].value.data.string;
			else printf("query returned %d", ret);
			return ret;
		#endif
	#else
		not_implemented();
	#endif
}
