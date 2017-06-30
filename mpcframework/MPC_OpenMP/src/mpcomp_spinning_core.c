#include "mpcomp_types.h"
#include "mpcomp_core.h"
#include "mpcomp_barrier.h"
#include "sctk_atomics.h"

#include "mpcomp_alloc.h"

#include "mpcomp_spinning_core.h"

/** Reset mpcomp_team informations */
static void __mpcomp_tree_array_team_reset( mpcomp_team_t *team )
{
    sctk_assert(team);
    sctk_atomics_int *last_array_slot;
	 memset( team, 0, sizeof( mpcomp_team_t ) );

    last_array_slot = &(team->for_dyn_nb_threads_exited[MPCOMP_MAX_ALIVE_FOR_DYN].i);
    sctk_atomics_store_int(last_array_slot, MPCOMP_NOWAIT_STOP_SYMBOL);
}

static inline int 
__mpcomp_del_mvp_saved_context( mpcomp_mvp_t* mvp )
{
    mpcomp_mvp_saved_context_t* prev_mvp_context = NULL;

    sctk_assert( mvp );

    /* Get previous context */
    prev_mvp_context =  mvp->prev_node_father; 
    sctk_assert( prev_mvp_context );

    /* Restore MVP previous status */
    mvp->rank = prev_mvp_context->rank;
    mvp->father = prev_mvp_context->father; 
    mvp->prev_node_father = prev_mvp_context->prev;

    /* Free -- TODO: reuse mechanism */
    free( prev_mvp_context );
    
    return 0; 
} 

static inline int 
__mpcomp_update_mvp_saved_context( mpcomp_mvp_t* mvp, mpcomp_node_t* node, mpcomp_node_t* father )
{
    mpcomp_mvp_saved_context_t* prev_mvp_context = NULL;

    sctk_assert( mvp );
    sctk_assert( father );
    
    /* Get previous context */
    prev_mvp_context =  mvp->prev_node_father;
   
    /* Update prev context information */
    prev_mvp_context->node = node;
    prev_mvp_context->father = mvp->father; 
    mvp->father = father;

    return 0;
}

static inline int 
__mpcomp_add_mvp_saved_context( mpcomp_mvp_t* mvp, const mpcomp_instance_t* instance, const unsigned int rank )
{
    mpcomp_mvp_saved_context_t* prev_mvp_context = NULL;

    sctk_assert( mvp );

    /* Allocate new chained elt */
    prev_mvp_context = (mpcomp_mvp_saved_context_t* ) mpcomp_alloc( sizeof( mpcomp_mvp_saved_context_t ) );
    sctk_assert( prev_mvp_context );
    memset( prev_mvp_context, 0, sizeof( mpcomp_mvp_saved_context_t ) );

    /* Get previous context */
    prev_mvp_context->prev = mvp->prev_node_father;
    prev_mvp_context->rank = mvp->rank;
    mvp->rank = rank;
    mvp->prev_node_father = prev_mvp_context;

#if MPCOMP_TRANSFER_INFO_ON_NODES
	 mvp->info = instance->team->info;
#endif /* MPCOMP_TRANSFER_INFO_ON_NODES */

	/* Old value can be restore by mpcomp_thread */
 	 mvp->instance = instance;  

    return 0; 
}

static int __mpcomp_tree_rank_get_next_depth( mpcomp_node_t* node, const int expected_nb_mvps, int* nb_mvps )
{
    int next_depth, num_nodes; 

    /* No more nested parallelism 
     * TODO: Add OMP_NESTED value checking */
    if( node == NULL || expected_nb_mvps == 1 )
    {
        next_depth = 1; 
        num_nodes = 1;
    }
    else
    {
        for( next_depth = 0; next_depth < node->tree_depth; next_depth++ )
        {
            num_nodes = node->tree_nb_nodes_per_depth[next_depth]; 
            /* break avoid auto increment when boolean condition is true */
            if( num_nodes >= expected_nb_mvps ) break;
        }
    }
   
    *nb_mvps = ( num_nodes < expected_nb_mvps ) ? num_nodes : expected_nb_mvps;
    return next_depth;
}

mpcomp_instance_t* 
__mpcomp_tree_array_instance_init( mpcomp_thread_t* thread, const int expected_nb_mvps )
{
    int i, j;
    mpcomp_node_t* root;
    mpcomp_instance_t* instance;

    sctk_assert( thread );
    sctk_assert( expected_nb_mvps );

    root = thread->root; 
    
    fprintf(stderr, ":: %s :: root> %p threads> %d\n", __func__, root, expected_nb_mvps );
    __mpcomp_scatter_instance_init( thread, expected_nb_mvps );

    /* Create instance */
    instance = ( mpcomp_instance_t* ) mpcomp_alloc(sizeof(mpcomp_instance_t)); 
    sctk_assert( instance );
    memset( instance, 0, sizeof( mpcomp_instance_t ) );

    /* -- Init Team information -- */
    instance->team = (mpcomp_team_t*) mpcomp_alloc(sizeof(mpcomp_team_t));
    sctk_assert( instance->team );
    __mpcomp_tree_array_team_reset( instance->team );

    /* -- Find next depth and real_nb_mvps -- */
    instance->tree_depth = __mpcomp_tree_rank_get_next_depth( root, expected_nb_mvps, &(instance->nb_mvps) )+1; 
    
    instance->mvps = (mpcomp_mvp_t**) mpcomp_alloc( sizeof( mpcomp_mvp_t* ) * instance->nb_mvps );
    sctk_assert( instance->mvps );
    memset( instance->mvps, 0, sizeof(mpcomp_mvp_t*) * instance->nb_mvps);
    
    instance->mvps_is_ready = (sctk_atomics_int*) mpcomp_alloc( sizeof(sctk_atomics_int)*instance->nb_mvps );
    sctk_assert( instance->mvps_is_ready );
    memset( instance->mvps_is_ready, 0, sizeof(sctk_atomics_int)*instance->nb_mvps );

    for( i = 0; i < instance->nb_mvps; i++ )
        assert( sctk_atomics_load_int( &( instance->mvps_is_ready[i] ) ) == 0 ); 

    /* -- First instance MVP  -- */
    instance->root = root;
    instance->mvps[0] = thread->mvp;

    if( root && instance->tree_depth > 1 )
    {
        sctk_assert( root );

        root->instance = instance;
        root->num_threads = instance->nb_mvps;

        /* -- Forward Tree Array from node to instance-- */
		instance->tree_base = root->tree_base;
		instance->tree_nb_nodes_per_depth = root->tree_nb_nodes_per_depth;

        /* -- Allocation Tree cumulative Array -- */ 
        instance->tree_cumulative = (int*) mpcomp_alloc( instance->tree_depth * sizeof( int ) );
        sctk_assert( instance->tree_cumulative );

        /* -- Instance include topology tree leaf level -- */
        if( instance->tree_depth == root->tree_depth )
        {
            memcpy(instance->tree_cumulative, root->tree_cumulative, instance->tree_depth * sizeof( int ));
        }
        else
        {
            /* Cumulative can't be just copied */
            for (i = 0; i < instance->tree_depth ; i++) 
            {
                instance->tree_cumulative[i] = 1;
                for (j = i + 1; j < instance->tree_depth; j++)
                    instance->tree_cumulative[i] *= root->tree_base[j];
            }
        }
    }
    else
    {
        int* singleton = (int*) mpcomp_alloc( sizeof( int ) );
        sctk_assert( singleton );
        singleton[0] = 1;

        instance->tree_base = singleton;
        instance->tree_cumulative = singleton;
        instance->tree_nb_nodes_per_depth = singleton;
    }
    
    return instance;
}

void __mpcomp_wakeup_mvp( mpcomp_mvp_t *mvp ) 
{
    int i, ret;
    mpcomp_local_icv_t icvs;
    mpcomp_thread_t* new_thread;

    mpcomp_thread_t* cur_thread = sctk_openmp_thread_tls;
    /* Sanity check */
    sctk_assert(mvp);
    
	mpcomp_node_t* father = __mpcomp_spinning_get_mvp_father_node( mvp, mvp->instance ); 
    const int rank = __mpcomp_scatter_compute_instance_rank_from_mvp( mvp->instance, mvp );
    const int instance_rank = __mpcomp_scatter_compute_global_rank_from_instance_rank( mvp->instance, rank );

    fprintf(stderr, ":: %s :: MVP> %d instance> %d mvp> %d\n", __func__, mvp->global_rank, rank, instance_rank );

    if( 1 )
	{
        new_thread = mvp->threads;

        /* Allocate new chained elt */
        mpcomp_mvp_saved_context_t* prev_mvp_context;
        prev_mvp_context = (mpcomp_mvp_saved_context_t* ) mpcomp_alloc( sizeof( mpcomp_mvp_saved_context_t ) );                                        
        sctk_assert( prev_mvp_context );
        memset( prev_mvp_context, 0, sizeof( mpcomp_mvp_saved_context_t ) );                                                                     
    
        /* Get previous context */
        prev_mvp_context->prev = mvp->prev_node_father;                                                                                          
        prev_mvp_context->rank = mvp->rank;                                                                                                      
        mvp->rank = rank;
        prev_mvp_context->father = mvp->father;
        mvp->father = father;
        mvp->prev_node_father = prev_mvp_context; 

    	new_thread->info.num_threads = 1; 
    	new_thread->instance = mvp->instance;

#if MPCOMP_TRANSFER_INFO_ON_NODES
    	new_thread->info = mvp->info;
#else /* MPCOMP_TRANSFER_INFO_ON_NODES */
    	new_thread->info = mvp->instance->team->info;
#endif

    	/* Set thread rank */
    	new_thread->rank = mvp->rank;
    	mvp->threads = new_thread;
    	new_thread->root = (mvp->prev_node_father ) ? mvp->prev_node_father->node : mvp->father;// mvp->father; //TODO just for test
    	new_thread->mvp = mvp;

	    sctk_spinlock_init( &( new_thread->info.update_lock ), 0 );
		/* Reset pragma for dynamic internal */
		for (i = 0; i < MPCOMP_MAX_ALIVE_FOR_DYN + 1; i++) 
			sctk_atomics_store_int(&(new_thread->for_dyn_remain[i].i), -1);

        fprintf(stderr, "Update instance value %d %d %p\n", mvp->global_rank, rank, mvp);
        mvp->instance->mvps[rank] = mvp;
		sctk_atomics_store_int( &( mvp->instance->mvps_is_ready[rank] ), 2);
	}
	else
	{
		while( sctk_atomics_load_int( &(mvp->instance->mvps_is_ready[rank])) != 2 )
            sctk_thread_yield();
	}
	
	return;	
}

void __mpcomp_wakeup_leaf( mpcomp_node_t* start_node )
{
    int i;
    mpcomp_mvp_t **leaves_array;

    leaves_array = start_node->children.leaf;

    if( start_node->child_type == MPCOMP_CHILDREN_NODE )
        return;

    sctk_assert( start_node->num_threads <= start_node->nb_children );
    const int quot = start_node->nb_children / start_node->num_threads;
    const int rest = start_node->nb_children % start_node->num_threads;
    const int shift = rest / start_node->nb_children;
   
    sctk_assert( !rest );

    start_node->barrier_num_threads = start_node->num_threads;
    for( i = 0; i < start_node->num_threads; i++ )
    {
        const int cur_mvp = i * quot;        
        const int instance_mvp_rank = start_node->mvp_first_id + i;
        leaves_array[cur_mvp]->instance = start_node->instance;
        leaves_array[cur_mvp]->spin_status = MPCOMP_MVP_STATE_AWAKE;
    }
}

mpcomp_node_t*  __mpcomp_wakeup_node( mpcomp_node_t* start_node )
{
    int i, rest_num_threads, local_mvp_id;
    mpcomp_node_t **nodes_array; 

    sctk_assert( start_node->child_type == MPCOMP_CHILDREN_NODE );

    const int quot = start_node->num_threads / start_node->nb_children;
    const int rest = start_node->num_threads % start_node->nb_children;
    const int min = ( start_node->num_threads > start_node->nb_children ) ? start_node->nb_children :  start_node->num_threads;
    start_node->barrier_num_threads = min;

    nodes_array = start_node->children.node;
    sctk_assert( nodes_array );

    const int node_first_mvp = start_node->mvp_first_id;
    for( i = 0; i < min; i++ )
    {
	    nodes_array[i]->num_threads = ( i < rest ) ? quot +1 : quot;
        nodes_array[i]->mvp_first_id = ( i < rest ) ? (quot+1) * i: quot * i + rest;
        nodes_array[i]->mvp_first_id += node_first_mvp;
        nodes_array[i]->spin_status = MPCOMP_MVP_STATE_AWAKE; 
    }

    nodes_array[0]->instance = start_node->instance; 

    return nodes_array[0];
}

void __mpcomp_wakeup_gen_node( mpcomp_node_t* start_node, const int num_threads )
{
    mpcomp_mvp_t* mvp;
    mpcomp_instance_t* instance;
    mpcomp_node_t* current_node, *old, *new;

    sctk_assert( start_node );

    instance = start_node->instance;

    sctk_assert( instance ); 
    sctk_assert( instance->root );

    mvp = start_node->mvp;
    sctk_assert( mvp );

    const int depth_target = instance->tree_depth + instance->root->depth - 1; 

    /* Continue Wake-Up */
    current_node = start_node;
    while( current_node->depth < depth_target )
    {
	    if( current_node->child_type != MPCOMP_CHILDREN_NODE ) 
		    break;
        current_node = __mpcomp_wakeup_node( current_node );
    }
     
    mvp = current_node->mvp;
    if( current_node->depth < depth_target )
    {
        __mpcomp_wakeup_leaf( current_node );
    }
    else
    {
        current_node->mvp->instance = current_node->instance;
    //    __mpcomp_update_mvp_saved_context( current_node->mvp, current_node, current_node->father );
    }

}

void __mpcomp_start_openmp_thread( mpcomp_mvp_t *mvp )
{
    mpcomp_thread_t* cur_thread = NULL;
    volatile int * spin_status;

    sctk_assert( mvp );
    __mpcomp_wakeup_mvp( mvp );

    sctk_openmp_thread_tls = mvp->threads;
    sctk_assert( sctk_openmp_thread_tls );
    cur_thread = (mpcomp_thread_t*) sctk_openmp_thread_tls; 

    /* Start parallel region */
    __mpcomp_in_order_scheduler( sctk_openmp_thread_tls );
    
    /* Must be set before barrier for thread safety*/
    spin_status = ( mvp->spin_node ) ? &( mvp->spin_node->spin_status ) : &( mvp->spin_status );    
    *spin_status = MPCOMP_MVP_STATE_SLEEP;
       
    /* Implicite barrier */
	__mpcomp_internal_full_barrier( mvp );
 
    sctk_openmp_thread_tls = mvp->threads->father;

    ( void ) __mpcomp_del_mvp_saved_context( mvp );

    if( sctk_openmp_thread_tls )
    {
        mpcomp_thread_t* test = (mpcomp_thread_t*) sctk_openmp_thread_tls;
        mvp->instance = test->instance;
    }    
}

/**
  Entry point for microVP in charge of passing information to other microVPs.
  Spinning on a specific node to wake up
 */
void mpcomp_slave_mvp_node( mpcomp_mvp_t *mvp ) 
{
    int num_threads, rest, quot, father_num_children;
    mpcomp_node_t* spin_node;

    sctk_assert( mvp );
    spin_node = mvp->spin_node;
    
    if( spin_node ) 
    {
        volatile int* spin_status = &( spin_node->spin_status );
        while( mvp->enable ) 
        {
            sctk_thread_wait_for_value_and_poll( spin_status, MPCOMP_MVP_STATE_AWAKE, NULL, NULL ) ;
            spin_node->instance = spin_node->father->instance;
#if MPCOMP_TRANSFER_INFO_ON_NODES
            spin_node->info = spin_node->father->info;
            num_threads = spin_node->info.num_threads;
#else   /* MPCOMP_TRANSFER_INFO_ON_NODES */
            num_threads = spin_node->instance->team->info.num_threads;
#endif /* MPCOMP_TRANSFER_INFO_ON_NODES */
            __mpcomp_wakeup_gen_node( spin_node, spin_node->num_threads );
            __mpcomp_start_openmp_thread( mvp );
        }
    }
    else
    {
        volatile int* spin_status = &( mvp->spin_status ) ;
        while( mvp->enable )
        {
            sctk_thread_wait_for_value_and_poll( spin_status, MPCOMP_MVP_STATE_AWAKE, NULL, NULL ) ;
            __mpcomp_start_openmp_thread( mvp );
        }
    }
}
