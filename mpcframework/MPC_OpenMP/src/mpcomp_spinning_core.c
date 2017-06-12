
#include "mpcomp_types.h"
#include "mpcomp_core.h"
#include "mpcomp_barrier.h"
#include "sctk_atomics.h"

/** Reset mpcomp_team informations */
static void __mpcomp_tree_array_team_reset( mpcomp_team_t *team )
{
    sctk_assert(team);
    sctk_atomics_int *last_array_slot;
    last_array_slot = &(team->for_dyn_nb_threads_exited[MPCOMP_MAX_ALIVE_FOR_DYN].i);
    sctk_atomics_store_int(last_array_slot, MPCOMP_NOWAIT_STOP_SYMBOL);
}

static int __mpcomp_tree_rank_get_next_depth( mpcomp_node_t* node, const int expected_nb_mvps, int* nb_mvps )
{
    int i, next_depth; 

    if( node == NULL || expected_nb_mvps == 1 )
    {
        next_depth = 1;
        *nb_mvps = expected_nb_mvps;
    }
    else
    {
        for( i = 0; i < node->tree_depth; i++ )
        {
            if( node->tree_nb_nodes_per_depth[i] >= expected_nb_mvps )
                break;
        }
        next_depth = i;
        *nb_mvps = node->tree_nb_nodes_per_depth[next_depth];
    }
    
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

    /* Create instance */
    instance = (mpcomp_instance_t*) malloc(sizeof(mpcomp_instance_t)); 
    sctk_assert( instance );
    memset( instance, 0, sizeof(mpcomp_instance_t) );

    /* -- Init Team information -- */
    instance->team = (mpcomp_team_t*) malloc(sizeof(mpcomp_team_t));
    sctk_assert( instance->team );
    __mpcomp_tree_array_team_reset( instance->team );

    /* -- Find next depth and real_nb_mvps -- */
    instance->tree_depth = __mpcomp_tree_rank_get_next_depth( root, expected_nb_mvps, &(instance->nb_mvps) ); 
    instance->mvps = (mpcomp_mvp_t**) malloc(sizeof(mpcomp_mvp_t*)*instance->nb_mvps);
    sctk_assert( instance->mvps );
    memset( instance->mvps, 0, sizeof(mpcomp_mvp_t*) * instance->nb_mvps);

    /* -- First instance MVP  -- */
    instance->root = root;
    fprintf(stderr, "FATHER -- INSTANCE ROOT ADDR : %p\n", root );
    instance->mvps[0] = thread->mvp;

    if( instance->tree_depth > 1 )
    {
        const size_t tree_array_size = sizeof( int ) * instance->tree_depth; 

        /* -- Allocation Tree Base Array -- */
        instance->tree_base = (int*) malloc( tree_array_size );
        sctk_assert( instance->tree_base );
        memset( instance->tree_base, 0, tree_array_size );

        /* -- Allocation Tree cumulative Array -- */ 
        instance->tree_cumulative = (int*) malloc( tree_array_size );
        sctk_assert( instance->tree_cumulative );
        memset( instance->tree_cumulative, 0, tree_array_size );

        /* -- Allocation Tree Node Per Depth -- */
        instance->tree_nb_nodes_per_depth = (int*) malloc( tree_array_size ); 
        sctk_assert( instance->tree_nb_nodes_per_depth ); 
        memset( instance->tree_nb_nodes_per_depth, 0, tree_array_size );

        const int next_depth = root->depth + instance->tree_depth;
        /* -- Instance include topology tree leaf level -- */
        if( next_depth == root->tree_depth )
        {
            /* Init instance tree array informations */
            memcpy(instance->tree_base, root->tree_base, tree_array_size);
            memcpy(instance->tree_cumulative, root->tree_cumulative, tree_array_size);
            memcpy(instance->tree_nb_nodes_per_depth, root->tree_nb_nodes_per_depth, tree_array_size);
            /* Collect instance mvps */
            mpcomp_mvp_t* cur_mvp = root->mvp;
            for( i = 1; i < instance->nb_mvps; i++, cur_mvp = cur_mvp->next_brother ) 
            {
                instance->mvps[i] = cur_mvp; 
                /* Enqueue instance mvp father */
                cur_mvp->tree_array_father->prev_father = cur_mvp->father;
                cur_mvp->father = cur_mvp->tree_array_father;
            }
            sctk_assert( cur_mvp == root->mvp );
        }
        else
        {
            /* Init instance tree array informations */
            memcpy(instance->tree_base, root->tree_base, tree_array_size);
            /* -- From mpcomp_tree.c -- */
            for (i = 0; i < instance->tree_depth - 1; i++)
            {
                instance->tree_cumulative[i] = 1;
                for (j = i + 1; j < instance->tree_depth; j++) 
                    instance->tree_cumulative[i] *= instance->tree_base[j];
            }
            instance->tree_nb_nodes_per_depth[0] = 1;
            for (i = 1; i < instance->tree_depth; i++)
            {
                instance->tree_nb_nodes_per_depth[i] = instance->tree_nb_nodes_per_depth[i - 1];
                instance->tree_nb_nodes_per_depth[i] *= instance->tree_base[i - 1];    
            }
            /* Find First node @ root->depth + tree_depth */
            mpcomp_node_t* cur_node = root;
            for( i = 0; i < instance->tree_depth; i++ )
                cur_node = cur_node->children.node[0];

            /* Collect instance mvps */
            for( i = 1; i < instance->nb_mvps; i++, cur_node = cur_node->next_brother )
            {
                instance->mvps[i] = cur_node->mvp;
                /* Enqueue instance mvp father */
               // cur_node->prev_father = instance->mvps[i]->father;
               // instance->mvps[i]->father = cur_node;
            }
        }
    }
    return instance;
}

void __mpcomp_wakeup_mvp( mpcomp_mvp_t *mvp) 
{
    mpcomp_local_icv_t icvs;
    mpcomp_thread_t* new_thread;

    /* Sanity check */
    sctk_assert(mvp);

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
    new_thread->rank = (new_thread->father) ?  0 : 1;
    mvp->threads = new_thread;
    //new_thread->root = mvp->father;
    new_thread->root = (mvp->prev_node_father ) ? mvp->prev_node_father->node : mvp->father;// mvp->father; //TODO just for test
    new_thread->mvp = mvp;
    mvp->slave_running = MPCOMP_MVP_STATE_READY;
    fprintf(stderr, "Switch Thread : %p to %p <> %p -- Instance : %p || %p\n", new_thread->father, new_thread, new_thread->mvp, new_thread->instance, new_thread->root );
}

void __mpcomp_wakeup_leaf( mpcomp_node_t* start_node, const int num_threads )
{
    int i, nb_children_involved;
    mpcomp_mvp_t **leaves_array;

    /*TODO:  Remove global variable */
    const int mpcomp_affinity = mpcomp_global_icvs.affinity; 
    leaves_array = start_node->children.leaf;

    if( start_node->child_type == MPCOMP_CHILDREN_NODE )
        return;

    /* Wake up children leaf */
    //fprintf(stderr, "MVP KIND ... %s\n\n", (start_node->child_type == MPCOMP_CHILDREN_LEAF ) ? "LEAF" :"NODE" );
    //sctk_assert( start_node->child_type == MPCOMP_CHILDREN_LEAF );


#if 0
    /* Count children number involved */
    for( i = 1, nb_children_involved = 1; i < start_node->nb_children; i++ )
    {
        const int leaf_rank = leaves_array[i]->min_index[mpcomp_affinity];
        if( leaf_rank < num_threads ) nb_children_involved++;
    }

    start_node->barrier_num_threads = nb_children_involved;

    for( i = 1; i < start_node->nb_children; i++ )
    {
        const int leaf_rank = leaves_array[i]->min_index[mpcomp_affinity];
        if( leaf_rank < num_threads )
        {
            fprintf(stderr, "%s : Wake Up new leaf %p\n", __func__, leaves_array[i] );
            leaves_array[i]->slave_running = MPCOMP_MVP_STATE_AWAKE;
            
        }
    }  
#else
    //assert( start_node->nb_children <= num_threads ); 
    start_node->barrier_num_threads = num_threads;
    for( i = 0; i < num_threads; i++ )
    {
        leaves_array[i]->slave_running = MPCOMP_MVP_STATE_AWAKE;

        /* Allocate new chained elt */
        mpcomp_node_chain_elt_t* new_father_elt;
        new_father_elt = (mpcomp_node_chain_elt_t* ) malloc( sizeof( mpcomp_node_chain_elt_t ) );
        sctk_assert( new_father_elt );
        memset( new_father_elt, 0, sizeof( mpcomp_node_chain_elt_t ) );
        
        new_father_elt->prev = leaves_array[i]->prev_node_father;
        new_father_elt->elt = leaves_array[i]->father;
        leaves_array[i]->father = start_node;
        leaves_array[i]->prev_node_father = new_father_elt;

        fprintf(stderr, "[[ADD -LEAF]] SWITCH FATHER : %p TO %p \n", new_father_elt->elt, leaves_array[i]->father );
    }
#endif
}

mpcomp_node_t*  __mpcomp_wakeup_node( mpcomp_node_t* start_node, const int num_threads )
{
    int i, j, nb_children_involved;
    mpcomp_node_t *current_node;
    mpcomp_node_t **nodes_array; 

    current_node = start_node;

    if( current_node->child_type != MPCOMP_CHILDREN_NODE )
    {
        fprintf( stderr, "%s: <| %p |> SKIP NODE WAKE UP\n", __func__, current_node );
        return current_node;
    }

    nodes_array = start_node->children.node;
#if 0
    /*TODO:  Remove global variable */
    const int mpcomp_affinity = mpcomp_global_icvs.affinity; 

    for( i = 0; i < start_node->instance->tree_depth; i++ )
    {
        for( j = 1, nb_children_involved = 1; j < current_node->nb_children; j++ )
        {
            const int node_rank = nodes_array[j]->min_index[mpcomp_affinity];
            if( node_rank < num_threads ) nb_children_involved++;
        }

        current_node->barrier_num_threads = nb_children_involved; 

        for( j = 1, nb_children_involved = 1; j < current_node->nb_children; j++ ) 
        {
            const int node_rank = nodes_array[j]->min_index[mpcomp_affinity];
            if( node_rank < num_threads )
            {
                fprintf(stderr, "%s : Wake-Up node #%d - %p [%d / %d]\n", __func__, nodes_array[j]->rank, nodes_array[j], nb_children_involved, current_node->barrier_num_threads );
                nb_children_involved++;
                nodes_array[j]->slave_running = MPCOMP_MVP_STATE_AWAKE;
            }
        }


        /* Next level must be compute by __mpcomp_wakeup_leaf */ 
        if( current_node->child_type != MPCOMP_CHILDREN_NODE )
            break;

        current_node = current_node->children.node[0];
    }
#else
    assert( start_node->nb_children <= num_threads );
    start_node->barrier_num_threads = num_threads;
    for( i = 1; i < num_threads; i++ )
    {
        nodes_array[i]->slave_running = MPCOMP_MVP_STATE_AWAKE; 
        fprintf(stderr, "%s : Wake-Up node ...\n", __func__);
    }
#endif
    
    fprintf(stderr, "[[ <> ]] %s : Root node #%d - %p -- %p -- %d (no Wake-Up)\n", __func__, current_node->rank, current_node, start_node, start_node->barrier_num_threads );


    return nodes_array[0];
}

void __mpcomp_wakeup_gen_node( mpcomp_node_t* start_node, const int num_threads )
{
    mpcomp_node_t* current_node, *old, *new;

    const int instance_tree_depth = start_node->instance->tree_depth;
     
    current_node = __mpcomp_wakeup_node( start_node, num_threads );
    const int depth_already_awake = current_node->depth - start_node->instance->root->depth;
    sctk_assert( depth_already_awake < 2 );

    fprintf(stderr, "FATHER: %d -- %d -- %d\n", instance_tree_depth, current_node->depth, start_node->instance->root->depth );
 
    if( depth_already_awake < instance_tree_depth )
    {
        __mpcomp_wakeup_leaf( current_node, num_threads );
    }
    else
    {
        /* Allocate new chained elt */
        mpcomp_node_chain_elt_t* new_father_elt;
        new_father_elt = (mpcomp_node_chain_elt_t* ) malloc( sizeof( mpcomp_node_chain_elt_t ) );
        sctk_assert( new_father_elt );
        memset( new_father_elt, 0, sizeof( mpcomp_node_chain_elt_t ) );
        
        new_father_elt->prev = current_node->mvp->prev_node_father;
        new_father_elt->elt = current_node->mvp->father;
        new_father_elt->node = current_node;
        current_node->mvp->prev_node_father = new_father_elt;
        current_node->mvp->father = current_node->father;
        

        fprintf(stderr, "(( ADD - NODE)) SWITCH FATHER : %p TO %p \n", new_father_elt->elt, current_node->mvp->father );
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
    __mpcomp_internal_half_barrier( mvp );


    /* End barrier for master thread */
    if( !( cur_thread->rank ) )
    {
        mpcomp_node_t* root = cur_thread->instance->root; 
        const int expected_num_threads = root->barrier_num_threads;
        fprintf(stderr, "[<>] Waitting for %d threads on node %p ( %d ) \n", root->barrier_num_threads, root, sctk_atomics_load_int(&(root->barrier)) ); 
        while (sctk_atomics_load_int(&(root->barrier)) != expected_num_threads) 
            sctk_thread_yield();
        
       sctk_atomics_store_int( &( root->barrier ), 0); 
    }
    else
    {
        mpcomp_node_t* root = cur_thread->instance->root; 
        fprintf(stderr, "[<>] Passing for %d threads on node %p ( %d ) \n", root->barrier_num_threads, root, sctk_atomics_load_int(&(root->barrier)) );
    }

    fprintf(stderr, "||\tSwitch Thread : %p to %p <> %p -- Instance : %p || %p\n", cur_thread->father, cur_thread, cur_thread->mvp, cur_thread->instance, cur_thread->root );
    sctk_openmp_thread_tls = mvp->threads->father;

    /* Change mvp father */
    mpcomp_node_chain_elt_t* old_node_father = mvp->prev_node_father;
    
    fprintf(stderr, "<< DEL >> SWITCH FATHER : %p TO %p \n", mvp->father, old_node_father->elt );

    mvp->prev_node_father = mvp->prev_node_father->prev;
    mvp->father = old_node_father->elt;
    free( old_node_father );

    if( sctk_openmp_thread_tls )
    {
        mpcomp_thread_t* test = (mpcomp_thread_t*) sctk_openmp_thread_tls;
        mvp->instance = test->instance;
    }    
 
    /* Free previous thread */
    free( cur_thread );
    
}

/**
  Entry point for microVP working on their own
  Spinning on a variables inside the microVP.
 */
void mpcomp_slave_mvp_leaf( mpcomp_mvp_t *mvp, mpcomp_node_t *spinning_node ) 
{
    assert( mvp );
    volatile int* spinning_val = &( mvp->slave_running ) ;

    /* Sanity check */
    assert( *spinning_val == MPCOMP_MVP_STATE_UNDEF );

    /* Spin while this microVP is alive */
    while( mvp->enable ) 
    {
        /* MVP pause status */
        *spinning_val = MPCOMP_MVP_STATE_SLEEP;
        /* Spin for new parallel region */
        sctk_thread_wait_for_value_and_poll( spinning_val, MPCOMP_MVP_STATE_AWAKE, NULL, NULL ) ;
        mvp->instance = mvp->father->instance;
        __mpcomp_start_openmp_thread( mvp );
        mvp->instance = NULL;
    }
}

/**
  Entry point for microVP in charge of passing information to other microVPs.
  Spinning on a specific node to wake up
 */
void mpcomp_slave_mvp_node( mpcomp_mvp_t *mvp, mpcomp_node_t *spinning_node ) 
{
    int num_threads;
    mpcomp_node_t* traversing_node = NULL;

    sctk_assert( mvp );
    volatile int* spinning_val = &( spinning_node->slave_running );
    sctk_assert( *spinning_val == MPCOMP_MVP_STATE_UNDEF );

    while( mvp->enable ) 
    {
        *spinning_val = MPCOMP_MVP_STATE_SLEEP;
        /* Spinning on the designed node */
        fprintf(stderr, "%s : Spinning on node #%d - %p << ! >>\n", __func__, spinning_node->rank, spinning_node );
        sctk_thread_wait_for_value_and_poll( spinning_val, MPCOMP_MVP_STATE_AWAKE, NULL, NULL ) ;
        fprintf(stderr, "Wake-Up node: %d - %p\n", spinning_node->rank, spinning_node );
        spinning_node->instance = spinning_node->father->instance;
#if MPCOMP_TRANSFER_INFO_ON_NODES
        spinning_node->info = start_node->father->info;
        num_threads = spinning_node->info.num_threads;
#else   /* MPCOMP_TRANSFER_INFO_ON_NODES */
        num_threads = spinning_node->instance->team->info.num_threads;
#endif /* MPCOMP_TRANSFER_INFO_ON_NODES */
	    sctk_assert( num_threads > 0 ) ;
	    /* -- Wake up children nodes -- */
        __mpcomp_wakeup_gen_node( spinning_node, num_threads );
        /* -- Start Parallel Region -- */
        fprintf(stderr, "%s : Start Region ...\n", __func__ );
        mvp->instance = spinning_node->instance;
        __mpcomp_start_openmp_thread( mvp );
    }
}
