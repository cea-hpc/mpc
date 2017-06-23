#include "mpcomp_types.h"
#include "mpcomp_core.h"
#include "mpcomp_barrier.h"
#include "sctk_atomics.h"

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
    prev_mvp_context = (mpcomp_mvp_saved_context_t* ) malloc( sizeof( mpcomp_mvp_saved_context_t ) );
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

static inline int*
__mpcomp_instance_get_mvps_local_rank( mpcomp_node_t *root, const int instance_depth, const int num_mvps )
{
	int i, j, next;
	int *array;

	sctk_assert( root );
	sctk_assert( instance_depth > 0 );
	sctk_assert( num_mvps > 0 );

	const int cur_degree = root->tree_base[instance_depth-2];
	const int prev_degree = root->tree_base[instance_depth-1];
	const int cumulative = root->tree_cumulative[instance_depth-1];

	const int quot = num_mvps / prev_degree;
	const int rest = num_mvps % prev_degree;	
	const int shift = root->mvp->global_rank;

	array = (int*) malloc( num_mvps * sizeof( int ) );
	sctk_assert( array );
	memset( array, 0, num_mvps * sizeof( int ) );

	for( next = 0, i = 0; i < prev_degree && next < num_mvps; i++ )
	{
		const int num_children = ( i < rest ) ? quot +1 : quot;
		for( j = 0; j < num_children; j++ )
		{
			array[next] = ( i * cur_degree + j ) * cumulative;
			array[next] += shift;
			next++;
		}
	}	
	return array;
	
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

    /* Create instance */
    instance = (mpcomp_instance_t*) malloc(sizeof(mpcomp_instance_t)); 
    sctk_assert( instance );
    memset( instance, 0, sizeof(mpcomp_instance_t) );

    /* -- Init Team information -- */
    instance->team = (mpcomp_team_t*) malloc(sizeof(mpcomp_team_t));
    sctk_assert( instance->team );
    __mpcomp_tree_array_team_reset( instance->team );

    /* -- Find next depth and real_nb_mvps -- */
    instance->tree_depth = __mpcomp_tree_rank_get_next_depth( root, expected_nb_mvps, &(instance->nb_mvps) )+1; 
    instance->mvps = (mpcomp_mvp_t**) malloc( sizeof( mpcomp_mvp_t* ) * instance->nb_mvps );
    sctk_assert( instance->mvps );
    memset( instance->mvps, 0, sizeof(mpcomp_mvp_t*) * instance->nb_mvps);
    
    instance->mvps_is_ready = (sctk_atomics_int*) malloc( sizeof(sctk_atomics_int)*instance->nb_mvps );
    sctk_assert( instance->mvps_is_ready );
    memset( instance->mvps_is_ready, 0, sizeof(sctk_atomics_int)*instance->nb_mvps );

    for( i = 0; i < instance->nb_mvps; i++ )
        assert( sctk_atomics_load_int( &( instance->mvps_is_ready[i] ) ) == 0 ); 

    /* -- First instance MVP  -- */
    instance->root = root;
    instance->mvps[0] = thread->mvp;
    root->num_threads = instance->nb_mvps;
    root->instance = instance;

    if( instance->tree_depth > 1 )
    {
        /* -- Forward Tree Array from node to instance-- */
		  instance->tree_base = root->tree_base;
		  instance->tree_nb_nodes_per_depth = root->tree_nb_nodes_per_depth;

        /* -- Allocation Tree cumulative Array -- */ 
        instance->tree_cumulative = (int*) malloc( instance->tree_depth * sizeof( int ) );
        sctk_assert( instance->tree_cumulative );

        const int next_depth = __mpcomp_spining_get_instance_max_depth(instance);

        /* -- Instance include topology tree leaf level -- */
        if( instance->tree_depth == root->tree_depth )
        {
            memcpy(instance->tree_cumulative, root->tree_cumulative, instance->tree_depth * sizeof( int ));
	         int * mvps_local = __mpcomp_instance_get_mvps_local_rank( root, instance->tree_depth, expected_nb_mvps );

            for( i = 0; i < instance->nb_mvps; i++ ) 
            {
		        const int cur_mvp = i + root->mvp->global_rank;
                instance->mvps[i] = (mpcomp_mvp_t*) root->tree_array[cur_mvp].user_pointer; 
                (void) __mpcomp_add_mvp_saved_context( instance->mvps[i], instance, (unsigned int) i );
            }
        }
        else
        {
            /* Cumulative can't be just copied */
            //TODO update utils function to trigger this use
            for (i = 0; i < instance->tree_depth ; i++) 
            {
                instance->tree_cumulative[i] = 1;
                for (j = i + 1; j < instance->tree_depth; j++)
                    instance->tree_cumulative[i] *= root->tree_base[j];
            }

            /* Find First node @ root->depth + tree_depth */
            mpcomp_node_t* cur_node = root;
            for( i = 0; i < instance->tree_depth-1; i++ )
            {
                cur_node = cur_node->children.node[0];
            }

            /* Collect instance mvps */
            for( i = 0; i < instance->nb_mvps && i < expected_nb_mvps; i++, cur_node = cur_node->next_brother )
            {
                instance->mvps[i] = cur_node->mvp;
                (void) __mpcomp_add_mvp_saved_context( cur_node->mvp, instance, (unsigned int) i );
            }
        }
    }
    return instance;
}

void __mpcomp_wakeup_mvp( mpcomp_mvp_t *mvp) 
{
    int i, ret;
    mpcomp_local_icv_t icvs;
    mpcomp_thread_t* new_thread;

    /* Sanity check */
    sctk_assert(mvp);
    ret = !sctk_atomics_cas_int( &( mvp->instance->mvps_is_ready[mvp->rank] ), 0, 1 );
	 
	 if( ret )
	 {
    	new_thread = (mpcomp_thread_t*) malloc( sizeof( mpcomp_thread_t) );
    	sctk_assert( new_thread );
    	memset( new_thread, 0, sizeof( mpcomp_thread_t ) );

    	new_thread->father = mvp->threads;
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

		 sctk_atomics_store_int( &( mvp->instance->mvps_is_ready[mvp->rank] ), 2);
	}
	else
	{
		while( sctk_atomics_load_int( &(mvp->instance->mvps_is_ready[mvp->rank])) != 2 ){};
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

    start_node->barrier_num_threads = start_node->num_threads;
    for( i = 0; i < start_node->num_threads; i++ )
    {
        __mpcomp_update_mvp_saved_context( leaves_array[i], start_node, start_node );
        leaves_array[i]->slave_running = MPCOMP_MVP_STATE_AWAKE;
    }
}

mpcomp_node_t*  __mpcomp_wakeup_node( mpcomp_node_t* start_node )
{
   int i, rest_num_threads;
   mpcomp_node_t **nodes_array; 

    sctk_assert( start_node->child_type == MPCOMP_CHILDREN_NODE );

    const int quot = start_node->num_threads / start_node->nb_children;
    const int rest = start_node->num_threads % start_node->nb_children;
    const int min = ( start_node->num_threads > start_node->nb_children ) ? start_node->nb_children :  start_node->num_threads;

    start_node->barrier_num_threads = min;

    nodes_array = start_node->children.node;
    sctk_assert( nodes_array );

    for( i = 0; i < min; i++ )
    {
	    nodes_array[i]->num_threads = ( i < rest ) ? quot +1 : quot;
        nodes_array[i]->slave_running = MPCOMP_MVP_STATE_AWAKE; 
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
        __mpcomp_update_mvp_saved_context( current_node->mvp, current_node, current_node->father );
    }

}

void __mpcomp_start_openmp_thread( mpcomp_mvp_t *mvp )
{
    mpcomp_thread_t* cur_thread = NULL;

    sctk_assert( mvp );
    __mpcomp_wakeup_mvp( mvp );

    sctk_openmp_thread_tls = mvp->threads;
    sctk_assert( sctk_openmp_thread_tls );
    cur_thread = (mpcomp_thread_t*) sctk_openmp_thread_tls; 

    /* Start parallel region */
    __mpcomp_in_order_scheduler( sctk_openmp_thread_tls );

    /* Implicite barrier */
	__mpcomp_internal_full_barrier( mvp );
 
    sctk_openmp_thread_tls = mvp->threads->father;
    mvp->threads = mvp->threads->father;

    ( void ) __mpcomp_del_mvp_saved_context( mvp );

    if( sctk_openmp_thread_tls )
    {
        mpcomp_thread_t* test = (mpcomp_thread_t*) sctk_openmp_thread_tls;
        mvp->instance = test->instance;
    }    
 
    /* Free previous thread */
    free( cur_thread );
}

/**
  Entry point for microVP in charge of passing information to other microVPs.
  Spinning on a specific node to wake up
 */
void mpcomp_slave_mvp_node( mpcomp_mvp_t *mvp, mpcomp_node_t *spinning_node ) 
{
    int num_threads, rest, quot, father_num_children;
    mpcomp_node_t* traversing_node = NULL;

    sctk_assert( mvp );

    if( spinning_node ) 
    {
        volatile int* spinning_val = &( spinning_node->slave_running );
        while( mvp->enable ) 
        {
            sctk_thread_wait_for_value_and_poll( spinning_val, MPCOMP_MVP_STATE_AWAKE, NULL, NULL ) ;
            spinning_node->instance = spinning_node->father->instance;
#if MPCOMP_TRANSFER_INFO_ON_NODES
            spinning_node->info = spinning_node->father->info;
            num_threads = spinning_node->info.num_threads;
#else   /* MPCOMP_TRANSFER_INFO_ON_NODES */
            num_threads = spinning_node->instance->team->info.num_threads;
#endif /* MPCOMP_TRANSFER_INFO_ON_NODES */
            __mpcomp_wakeup_gen_node( spinning_node, spinning_node->num_threads );
            __mpcomp_start_openmp_thread( mvp );
            *spinning_val = MPCOMP_MVP_STATE_SLEEP;
        }
    }
    else
    {
        volatile int* spinning_val = &( mvp->slave_running ) ;
        while( mvp->enable )
        {
            sctk_thread_wait_for_value_and_poll( spinning_val, MPCOMP_MVP_STATE_AWAKE, NULL, NULL ) ;
            __mpcomp_start_openmp_thread( mvp );
            *spinning_val = MPCOMP_MVP_STATE_SLEEP;
        }
    }
}
