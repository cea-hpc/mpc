#include "mpcomp_types.h"
#include "mpcomp_core.h"
#include "mpcomp_barrier.h"
#include "sctk_atomics.h"

#include "mpcomp_alloc.h"

#include "mpcomp_scatter.h"
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

static inline void 
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

static inline void
__mpcomp_add_mvp_saved_context( mpcomp_mvp_t* mvp )
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
    prev_mvp_context->father = mvp->father;
    mvp->prev_node_father = prev_mvp_context;
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
    mpcomp_thread_t* master;
    mpcomp_instance_t* instance;

    sctk_assert( thread );
    sctk_assert( expected_nb_mvps > 0 );

    instance =  __mpcomp_scatter_instance_pre_init( thread, expected_nb_mvps );
    sctk_assert( instance );

    instance->nb_mvps = expected_nb_mvps;

    instance->team = (mpcomp_team_t*) mpcomp_alloc( sizeof( mpcomp_team_t ) );
    sctk_assert( instance->team );
    memset( instance->team, 0, sizeof( mpcomp_team_t ) );

    /* Allocate new thread context */
    master = ( mpcomp_thread_t* ) mpcomp_alloc( sizeof( mpcomp_thread_t ) );  
    sctk_assert( master );
    memset( master, 0, sizeof( mpcomp_thread_t ) );

    master->father = thread;
    thread->mvp->threads = master;

    return instance;
}

mpcomp_thread_t*
__mpcomp_wakeup_mvp( mpcomp_mvp_t *mvp ) 
{
    int i, ret;
    mpcomp_local_icv_t icvs;
    mpcomp_thread_t* new_thread, *cur_thread;

    sctk_assert(mvp);
    
    new_thread = mvp->threads;
    sctk_assert( new_thread );

    const int combined_pragma = mvp->instance->team->info.combined_pragma;
    sctk_assert( combined_pragma >= 0 && combined_pragma < MPCOMP_COMBINED_COUNT); 

    new_thread->instance = mvp->instance;

    mvp->father = new_thread->father_node;
    mvp->rank = new_thread->rank;
    mvp->instance->mvps[new_thread->instance_ghost_rank] = mvp;

#if MPCOMP_TRANSFER_INFO_ON_NODES
    new_thread->info = mvp->info;
#else /* MPCOMP_TRANSFER_INFO_ON_NODES */
    new_thread->info = mvp->instance->team->info;
#endif
    
    sctk_spinlock_init( &( new_thread->info.update_lock ), 0 ); 
    /* Reset pragma for dynamic internal */
    for (i = 0; i < MPCOMP_MAX_ALIVE_FOR_DYN + 1; i++) 
	    sctk_atomics_store_int(&(new_thread->for_dyn_remain[i].i), -1);

    /* Set thread rank */
    new_thread->mvp = mvp;
    new_thread->root = (mvp->prev_node_father ) ? mvp->prev_node_father->node : mvp->father;// mvp->father; //TODO just for test

	return new_thread;	
}

void
__mpcomp_instance_post_init( mpcomp_thread_t* thread )
{
    __mpcomp_scatter_instance_post_init( thread );
}

mpcomp_mvp_t*
__mpcomp_wakeup_node( mpcomp_node_t* node )
{
    
    return __mpcomp_scatter_wakeup( node );
}

void __mpcomp_start_openmp_thread( mpcomp_mvp_t *mvp )
{
    mpcomp_thread_t* cur_thread = NULL;
    volatile int * spin_status;

    sctk_assert( mvp );


    __mpcomp_add_mvp_saved_context( mvp );
    sctk_assert( mvp->prev_node_father );

    /* Switch thread TLS */
    cur_thread =  __mpcomp_wakeup_mvp( mvp );
    sctk_openmp_thread_tls = (void*) cur_thread; 

    /* Last call before parallel region */
    __mpcomp_instance_post_init( cur_thread );

    /* Start parallel region */
    __mpcomp_in_order_scheduler( sctk_openmp_thread_tls );
    
    /* Must be set before barrier for thread safety*/
    spin_status = ( mvp->spin_node ) ? &( mvp->spin_node->spin_status ) : &( mvp->spin_status );    
    *spin_status = MPCOMP_MVP_STATE_SLEEP;
       
    /* Implicite barrier */
	__mpcomp_internal_full_barrier( mvp );
 
    fprintf(stderr, ":: %s :: Switch TLS  > %p to %p\n", __func__, sctk_openmp_thread_tls, mvp->threads->father );
    sctk_openmp_thread_tls = mvp->threads->father;

    __mpcomp_del_mvp_saved_context( mvp );

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
#if MPCOMP_TRANSFER_INFO_ON_NODES
            spin_node->info = spin_node->father->info;
#endif /* MPCOMP_TRANSFER_INFO_ON_NODES */
            __mpcomp_wakeup_node( spin_node );
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
