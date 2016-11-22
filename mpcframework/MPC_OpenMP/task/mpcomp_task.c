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
/* #   - PERACHE Marc marc.perache@cea.fr                                 # */
/* #   - CARRIBAULT Patrick patrick.carribault@cea.fr                     # */
/* #                                                                      # */
/* ######################################################################## */

#include "mpcomp_types_def.h"

#if MPCOMP_TASK

#include "sctk_bool.h"
#include <sctk_int.h>
#include <sctk_asm.h>
#include "mpcomp_types.h"
#include "sctk_runtime_config_struct.h"
#include "sctk_debug.h"
#include "mpcomp_task.h"
#include "mpcomp_task_tree.h"
#include "mpcomp_task_macros.h"
#include "mpcomp_task_list.h"
#include "mpcomp_task_utils.h"
#include "mpcomp_tree_utils.h"
#include "mpcomp_task_stealing.h"
#include "sctk_atomics.h"
#include "mpcomp_task_dep.h"
#include "mpcomp_taskgroup.h"

static sctk_atomics_int total_task_exec = OPA_INT_T_INITIALIZER( 0 );   
static sctk_atomics_int total_task_create = OPA_INT_T_INITIALIZER( 0 );   

typedef uint32_t (* kmp_routine_entry_t)( uint32_t, void * );

typedef struct kmp_task {                   /* GEH: Shouldn't this be aligned somehow? */
    void *              shareds;            /**< pointer to block of pointers to shared vars   */
    kmp_routine_entry_t routine;            /**< pointer to routine to call for executing task */
    uint32_t           part_id;            /**< part id for the task                          */
#if OMP_40_ENABLED
    kmp_routine_entry_t destructors;        /* pointer to function to invoke deconstructors of firstprivate C++ objects */
#endif // OMP_40_ENABLED
    /*  private vars  */
}kmp_task_t;
 
/*
 */
void
mpcomp_tast_clear_sister( mpcomp_task_t* task )
{
	sctk_assert( task );
	mpcomp_task_t* parent_task = task->parent;
	
	//parent_task maybe equal to NULL in between
	if( parent_task ) 
	{
		/* Remove the task from his parent children list */
	  	sctk_spinlock_lock( &( parent_task->children_lock ) );

        if( parent_task )
        {
	  	    if (parent_task->children == task)
		    {
			    parent_task->children = task->next_child;
		    }
	        else
            { 
		        if( task->next_child != NULL )
		        {
			        task->next_child->prev_child = task->prev_child;
		        }
		
		        if( task->prev_child != NULL )
		        {
		            task->prev_child->next_child = task->next_child;
		        }
            }
        }
	 	sctk_spinlock_unlock( &( parent_task->children_lock ) );
	}
}

/* 
 * Initialization of OpenMP task environment 
 */
void mpcomp_task_scheduling_infos_init( void )
{
	mpcomp_task_thread_infos_init( sctk_openmp_thread_tls );
	mpcomp_task_tree_infos_init();
}

void __mpcomp_task_intel_wrapper( void* task )
{
    kmp_task_t* kmp_task_addr = (char*) task + sizeof( mpcomp_task_t ); 
    kmp_task_addr->routine( 0, kmp_task_addr );
}

void
__mpcomp_task_execute( mpcomp_task_t* task )
{
    mpcomp_thread_t* thread;
    mpcomp_local_icv_t saved_icvs;
    mpcomp_task_t* saved_current_task;

	 sctk_assert( task );

    /* Retrieve the information (microthread structure and current region) */
    sctk_assert( sctk_openmp_thread_tls );	 
    thread = ( mpcomp_thread_t* ) sctk_openmp_thread_tls;

    /* A task can't be execute by multiple thread */
    sctk_assert( task->thread == NULL);

	/* Update task owner */
	task->thread = thread;
		
	/* Saved thread current task */
 	saved_current_task = MPCOMP_TASK_THREAD_GET_CURRENT_TASK( thread );

	/* Update thread current task */
	MPCOMP_TASK_THREAD_SET_CURRENT_TASK( thread, task );

	/* Saved thread icv environnement */	
	saved_icvs = thread->info.icvs;

	/* Get task icv environnement */
	thread->info.icvs = task->icvs;

   // __mpcomp_task_intel_wrapper( task );
	/* Execute task */
	task->func( task->data );

	/* Restore thread icv envionnement */
	thread->info.icvs = saved_icvs;

	/* Restore current task */
 	MPCOMP_TASK_THREAD_SET_CURRENT_TASK( thread, saved_current_task );

   saved_current_task = MPCOMP_TASK_THREAD_GET_CURRENT_TASK( thread );

   /* Reset task owner for implicite task */
   task->thread = NULL;

   sctk_atomics_incr_int( &( total_task_exec ) );  

	if( OPA_load_int(  &( total_task_exec ) ) > OPA_load_int(  &( total_task_create ) ) ) 
		abort();

   sctk_nodebug( "TASK DELAYED END" );
}

void
mpcomp_task_add_to_parent( mpcomp_task_t* task )
{
	sctk_assert( task );

	/* If task has a parent, add it to the children list */ 
   if( !( task->parent ))
	{
		return;
	}

	sctk_spinlock_lock( &( task->parent->children_lock ) );

	/* Parent task already had children */
	if( task->parent->children ) 
   {
		task->next_child = task->parent->children;
        sctk_assert( task->next_child );
		task->next_child->prev_child = task;
	} 
	
	/* The task became the first task */
	task->parent->children = task;

	sctk_spinlock_unlock( &( task->parent->children_lock ) );
}

void mpcomp_task_ref_parent_task( mpcomp_task_t* task )
{
    sctk_assert( task );

    if( !( task->parent ) ) return;

    sctk_atomics_fetch_and_incr_int( &( task->refcount ) );
    sctk_atomics_fetch_and_incr_int( &( task->parent->refcount ) );
    mpcomp_task_add_to_parent( task );
}

/*
 *  Delete the parent link between the task 'parent' and all its children.
 *  These tasks became orphaned.
 *	TODO: parent may be equal to NULL in between
 */
void
mpcomp_task_clear_parent( mpcomp_task_t* parent )
{
	mpcomp_task_t* child;
	sctk_assert( parent != NULL );
 	sctk_spinlock_lock( &( parent->children_lock ) );

    child = parent->children;
	while( child )
 	{
		child->parent = NULL;
	  	child = child->next_child;
	  	if( child ) 
		{
	   	    child->prev_child->next_child = NULL;
	   	    child->prev_child = NULL;
	  	}
 	}

 	parent->children = NULL;
  	sctk_spinlock_unlock( &( parent->children_lock ) );	       
}

void mpcomp_task_unref_parent_task( mpcomp_task_t* task )
{
    bool no_more_ref; 
    mpcomp_task_t *mother, *swap_value;
    sctk_assert( task );

    no_more_ref = ( sctk_atomics_fetch_and_decr_int( &( task->refcount ) ) == 1 );
    mother = task->parent;
    
    if( !mother || !no_more_ref ) return;
    
    sctk_free( task );
    
    while( mother->parent && ( sctk_atomics_fetch_and_decr_int( &( mother->refcount ) ) == 1 ) )
    {
        swap_value = mother; 
        mother = mother->parent;
        sctk_free( swap_value );
    } 

    if( !mother->parent && ( sctk_atomics_fetch_and_decr_int( &( mother->refcount ) ) == 1 ) )
    {
        mother->children = NULL;
    }
}

/* Initialization of mpcomp tasks lists (new and untied) */

void __mpcomp_task_list_infos_exit_r( mpcomp_node_t* node )
{
   int i, j;	
	mpcomp_thread_t *thread;
     
 	sctk_assert( node != NULL );
 	sctk_assert( sctk_openmp_thread_tls != NULL );

	/* Retrieve the current thread information */
	thread = ( mpcomp_thread_t * ) sctk_openmp_thread_tls;

    sctk_assert( thread->instance );
    sctk_assert( thread->instance->team );

	const int team_task_new_list_depth    = MPCOMP_TASK_TEAM_GET_TASKLIST_DEPTH( thread->instance->team, MPCOMP_TASK_TYPE_NEW );
	const int team_task_untied_list_depth = MPCOMP_TASK_TEAM_GET_TASKLIST_DEPTH( thread->instance->team, MPCOMP_TASK_TYPE_UNTIED );

   /* If the node correspond to the new list depth, release the data structure */
	if (node->depth == team_task_new_list_depth ) 
	{
		mpcomp_task_list_free( MPCOMP_TASK_NODE_GET_TASK_LIST_HEAD( node, MPCOMP_TASK_TYPE_NEW ) );
		MPCOMP_TASK_NODE_SET_TASK_LIST_HEAD( node, MPCOMP_TASK_TYPE_NEW, NULL );		
	}
		    
	/* If the node correspond to the untied list depth, release the data structure */
	if (node->depth == team_task_untied_list_depth ) 
	{
		mpcomp_task_list_free( MPCOMP_TASK_NODE_GET_TASK_LIST_HEAD( node, MPCOMP_TASK_TYPE_UNTIED ) );
		MPCOMP_TASK_NODE_SET_TASK_LIST_HEAD( node, MPCOMP_TASK_TYPE_UNTIED, NULL );		
	}

   /* If the untied or the new lists are at a lower level, look deeper */
   if( ( node->depth <= team_task_new_list_depth ) || (node->depth <= team_task_untied_list_depth ) ) 
	{	       
		switch (node->child_type) 
		{
	  			case MPCOMP_CHILDREN_NODE:
	       		for (i=0; i<node->nb_children; i++) 
					{
		    			/* Call recursively for all children nodes */
		    			__mpcomp_task_list_infos_exit_r(node->children.node[i]);
	       		}
	       		break;
	       
	  			case MPCOMP_CHILDREN_LEAF:
	       		for (i = 0; i < node->nb_children; i++) 
					{
		    			/* All the children are leafs */
		    			mpcomp_mvp_t *mvp = node->children.leaf[i];
		    			sctk_assert(mvp != NULL);
		    
		    			/* If the node correspond to the new list depth, release the data structure */
		    			if ( ( node->depth + 1 ) == team_task_new_list_depth ) 
						{
			 				mpcomp_task_list_free( MPCOMP_TASK_MVP_GET_TASK_LIST_HEAD( mvp, MPCOMP_TASK_TYPE_NEW ) );
							MPCOMP_TASK_MVP_SET_TASK_LIST_HEAD( mvp, MPCOMP_TASK_TYPE_NEW, NULL );		
		    			}
		    
		    			/* If the node correspond to the untied list depth, release the data structure */
						if ( ( node->depth + 1 ) == team_task_untied_list_depth ) 
						{	
			 				mpcomp_task_list_free( MPCOMP_TASK_MVP_GET_TASK_LIST_HEAD( mvp, MPCOMP_TASK_TYPE_UNTIED ) );
							MPCOMP_TASK_MVP_SET_TASK_LIST_HEAD( mvp, MPCOMP_TASK_TYPE_UNTIED, NULL );		
		    			}
	       		}
	       		break;
	  			default:
	       		sctk_nodebug("not reachable"); 
	  			}
     		}
	  
     return;
}
                                                 
/* The new task may be delayed, so copy arguments in a buffer */
struct mpcomp_task_s*
__mpcomp_task_alloc( void (*fn) (void *), void *data, void (*cpyfn) (void *, void *),
           long arg_size, long arg_align, bool if_clause, unsigned flags, int deps_num )
{
  	sctk_assert( sctk_openmp_thread_tls );	 
  	mpcomp_thread_t* t = (mpcomp_thread_t *) sctk_openmp_thread_tls;

	sctk_assert( t->mvp );
	sctk_assert( t->mvp->father );
	sctk_assert( t->mvp->father->id_numa );

	/* default pading */
	const long align_size = ( arg_align == 0 ) ? 8 : align_size;

   // mpcomp_task + arg_size
   const long mpcomp_task_info_size = mpcomp_task_align_single_malloc( sizeof( mpcomp_task_t), arg_align);
   const long mpcomp_task_data_size = mpcomp_task_align_single_malloc( arg_size, arg_align ); 

	/* Compute task total size */
	long mpcomp_task_tot_size = mpcomp_task_info_size;

	sctk_assert( MPCOMP_OVERFLOW_SANITY_CHECK( mpcomp_task_tot_size, mpcomp_task_data_size ));
	mpcomp_task_tot_size += mpcomp_task_data_size;

    struct mpcomp_task_s *new_task = mpcomp_malloc( 1, mpcomp_task_tot_size, 0/* t->mvp->father->id_numa */);
	sctk_assert( new_task != NULL );

	void* task_data = ( arg_size > 0 ) ? ( void*) ( (uintptr_t) new_task + mpcomp_task_info_size ) : NULL;

	/* Create new task */
    __mpcomp_task_infos_init( new_task, fn, task_data, t );
	new_task->task_dep_infos = sctk_malloc( sizeof( mpcomp_task_dep_task_infos_t ) );

	sctk_assert( new_task->task_dep_infos );
	memset( new_task->task_dep_infos, 0, sizeof( mpcomp_task_dep_task_infos_t ));
	mpcomp_task_set_property( &( new_task->property ), MPCOMP_TASK_TIED );

   if(arg_size > 0 )
   {
   	if(cpyfn) 
      {
      	cpyfn( task_data, data );
     	}
      else
      {
      	memcpy( task_data, data, arg_size );
      }
	}

	sctk_nodebug( "%s: %p -- %p -- %p", __func__, new_task, new_task->func, new_task->data ); 
   /* If its parent task is final, the new task must be final too */
	if( mpcomp_task_is_final( flags, new_task->parent ) ) 
	{
		mpcomp_task_set_property( &( new_task->property), MPCOMP_TASK_FINAL );
	}

	/* taskgroup */
	mpcomp_task_t* current_task = MPCOMP_TASK_THREAD_GET_CURRENT_TASK( t );
    mpcomp_taskgroup_add_task( new_task ); 
    mpcomp_task_ref_parent_task( new_task );
	
   return new_task;
}


mpcomp_task_list_t* 
__mpcomp_task_try_delay( bool if_clause )
{
    mpcomp_task_t* current_task = NULL;
    mpcomp_thread_t* omp_thread_tls = NULL;
    mpcomp_task_list_t* mvp_task_list = NULL;

	omp_thread_tls = (mpcomp_thread_t *) sctk_openmp_thread_tls;
	sctk_assert( omp_thread_tls );
    
    /* Is Serialized Task */
    if( !if_clause || mpcomp_task_no_deps_is_serialized( omp_thread_tls ) )
    {
        return NULL;
    }

    /* Reserved place in queue */
    assert( omp_thread_tls->mvp != NULL );
    const int node_rank = MPCOMP_TASK_MVP_GET_TASK_LIST_NODE_RANK( omp_thread_tls->mvp, MPCOMP_TASK_TYPE_NEW );
    assert( node_rank >= 0 );

    // No list for a mvp should be an error ??
    if( !( mvp_task_list = mpcomp_task_get_list( node_rank, MPCOMP_TASK_TYPE_NEW ) ) )
    {
        return NULL;
    }

    // To must delayed tasks 
    if( sctk_atomics_fetch_and_incr_int( &( mvp_task_list->nb_elements ) ) >= MPCOMP_TASK_MAX_DELAYED )
    {
        sctk_atomics_decr_int( &( mvp_task_list->nb_elements ) );   
        return NULL;
    }   
    
    return mvp_task_list;
}

void
__mpcomp_task_process( mpcomp_task_t* new_task, bool if_clause ) 
{
   mpcomp_thread_t* thread;
	mpcomp_task_list_t* mvp_task_list;

   /* Retrieve the information (microthread structure and current region) */
   sctk_assert( sctk_openmp_thread_tls );
   thread = ( mpcomp_thread_t* ) sctk_openmp_thread_tls;
   
   sctk_atomics_incr_int( &( total_task_create ) );
   mvp_task_list = __mpcomp_task_try_delay( if_clause );

    /* Push the task in the list of new tasks */	
    if( mvp_task_list )	
    {
        mpcomp_task_list_lock( mvp_task_list );
	    mpcomp_task_list_pushtohead( mvp_task_list , new_task );
 	    mpcomp_task_list_unlock( mvp_task_list );
   	    return;
    }

   sctk_nodebug( "%s: %p -- %p -- %p", __func__, new_task, new_task->func, new_task->data ); 
   /* Direct task execution */
   mpcomp_task_set_property ( &( new_task->property ), MPCOMP_TASK_UNDEFERRED );
   __mpcomp_task_execute( new_task );

   mpcomp_tast_clear_sister( new_task );
   //mpcomp_task_clear_parent( new_task );
   mpcomp_taskgroup_del_task( new_task ); 
   __mpcomp_task_finalize_deps( new_task );
   mpcomp_task_unref_parent_task( new_task );
}

/* 
 * Creation of an OpenMP task 
 * Called when encountering an 'omp task' construct 
 */
void 
__mpcomp_task(void (*fn) (void *), void *data, void (*cpyfn) (void *, void *),
		   long arg_size, long arg_align, bool if_clause, unsigned flags)
{
	mpcomp_task_t* new_task;

  	__mpcomp_init();
 	mpcomp_task_scheduling_infos_init();

   new_task = __mpcomp_task_alloc( fn, data, cpyfn, arg_size, arg_align, if_clause, flags, 0 /* no deps */ ); 
   __mpcomp_task_process( new_task, if_clause );
}


/* Steal a task in the 'type' task list of node identified by 'globalRank' */
static mpcomp_task_t* mpcomp_task_steal( mpcomp_task_list_t* list )
{
    struct mpcomp_task_s *task = NULL;

    sctk_assert(list != NULL);

    if( mpcomp_task_list_trylock(list) )
    { 
        task = mpcomp_task_list_popfromtail(list);
        if( task )
	        sctk_atomics_incr_int( &( list->nb_larcenies ) );
        mpcomp_task_list_unlock(list);
	} 
    return task;
}


/* Return the ith victim for task stealing initiated from element at 'globalRank' */
static inline int 
mpcomp_task_get_victim(int globalRank, int index, mpcomp_tasklist_type_t type)
{
   int victim;

	/* Retrieve the information (microthread structure and current region) */
   sctk_assert( sctk_openmp_thread_tls );
   mpcomp_thread_t* thread = ( mpcomp_thread_t* ) sctk_openmp_thread_tls;
	
	sctk_assert( thread->instance );
	sctk_assert( thread->instance->team );	
	mpcomp_team_t* team = thread->instance->team;

  	const int larcenyMode = MPCOMP_TASK_TEAM_GET_TASK_LARCENY_MODE( team );

     switch (larcenyMode) {

        case MPCOMP_TASK_LARCENY_MODE_HIERARCHICAL:
            victim = mpcomp_task_get_victim_hierarchical(globalRank, index, type);
	        break;

        case MPCOMP_TASK_LARCENY_MODE_RANDOM:
            victim = mpcomp_task_get_victim_random(globalRank, index, type);
	        break;
 
        case MPCOMP_TASK_LARCENY_MODE_RANDOM_ORDER:
            victim = mpcomp_task_get_victim_random_order(globalRank, index, type); 
	        break;

        case MPCOMP_TASK_LARCENY_MODE_ROUNDROBIN:
            victim = mpcomp_task_get_victim_roundrobin(globalRank, index, type);

        case MPCOMP_TASK_LARCENY_MODE_PRODUCER:
            victim = mpcomp_task_get_victim_producer(globalRank, index, type);
	        break;

        case MPCOMP_TASK_LARCENY_MODE_PRODUCER_ORDER:
	        victim = mpcomp_task_get_victim_producer_order(globalRank, index, type);
	        break;

        default:
	        victim = mpcomp_task_get_victim_default(globalRank, index, type);
	        break;	  
     }
     
     return victim;
}

/* Look in new and untied tasks lists of others */
static struct mpcomp_task_s * 
__mpcomp_task_larceny( void )
{
    int i, type;
    mpcomp_thread_t *thread;
    struct mpcomp_task_s *task = NULL;
	struct mpcomp_mvp_s *mvp;
	struct mpcomp_team_s* team;
    struct mpcomp_task_list_s *list;

  	/* Retrieve the information (microthread structure and current region) */
	sctk_assert( sctk_openmp_thread_tls );
   thread = ( mpcomp_thread_t* ) sctk_openmp_thread_tls;

 	mvp = thread->mvp;
	sctk_assert( mvp );

	sctk_assert( thread->instance );
	team = thread->instance->team;
	sctk_assert( team );

 	const int larcenyMode = MPCOMP_TASK_TEAM_GET_TASK_LARCENY_MODE( team );
	const int isMonoVictim = ( larcenyMode == MPCOMP_TASK_LARCENY_MODE_RANDOM || larcenyMode == MPCOMP_TASK_LARCENY_MODE_PRODUCER );

	/* Check first for NEW tasklists, then for UNTIED tasklists */
	for( type = 0; type < MPCOMP_TASK_TYPE_COUNT; type++ ) 
	{
	  
	  	/* Retrieve informations about task lists */
	  	const int tasklistDepth = MPCOMP_TASK_TEAM_GET_TASKLIST_DEPTH( team, type );
	  	const int nbTasklists   = MPCOMP_TASK_INSTANCE_GET_ARRAY_TREE_LEVEL_SIZE( thread->instance, tasklistDepth );
		
		//sctk_nodebug( "tasklistDepth: %d -- nbTasklists: %d", tasklistDepth, nbTasklists);

	  	/* If there are task lists in which we could steal tasks */
	  	if( nbTasklists > 1 )  
		{

	        /* Try to steal inside the last stolen list*/
	        if( ( list = MPCOMP_TASK_MVP_GET_LAST_STOLEN_TASK_LIST( mvp, type ) ) ) 
	        {
		   	   if( ( task = mpcomp_task_steal(list) ) )
					{
						return task;
		    		}
	        }

	     	/* Get the rank of the ancestor containing the task list */
	    	int rank = MPCOMP_TASK_MVP_GET_TASK_LIST_NODE_RANK( mvp, type );
	     	int nbVictims = ( isMonoVictim ) ? 1 : nbTasklists;
	      
	      /* Look for a task in all victims lists */
	     	for( i = 1; i < nbVictims + 1; i++) 
			{
		   	int victim = mpcomp_task_get_victim( rank, i, type );
		    	sctk_assert( victim != rank );
		    	list = mpcomp_task_get_list( victim, type );
		    	task = mpcomp_task_steal( list );
		    	if( task ) 
				{
					MPCOMP_TASK_MVP_SET_LAST_STOLEN_TASK_LIST( mvp, type, list );
			 		return task;
		    	}
	   	}
		}
	}
	return NULL;
}

/* 
 * Schedule remaining tasks in the different task lists (tied, untied, new) 
 * Called at several schedule points : 
 *     - in taskyield regions
 *     - in taskwait regions
 *     - in implicit and explicit barrier regions
 */
void mpcomp_task_schedule( int depth )
{
	int type;
    mpcomp_mvp_t* mvp = NULL;
	mpcomp_task_t* task = NULL;
    mpcomp_thread_t* thread = NULL;

    sctk_assert( sctk_openmp_thread_tls );	 
    thread = ( mpcomp_thread_t* ) sctk_openmp_thread_tls;
	mvp = (mpcomp_mvp_t*) thread->mvp;
 
    if( !mvp ) return;

	sctk_assert( thread->instance );
	sctk_assert( thread->instance->team );

    /* If only one thread is running, tasks are not delayed. No need to schedule */
	if( ( thread->info.num_threads == 1 ) 
     		|| !MPCOMP_TASK_THREAD_IS_INITIALIZED( thread )
       		|| !MPCOMP_TASK_TEAM_IS_INITIALIZED( thread->instance->team ) )
	{
        sctk_nodebug("sequential or no task");
   	    return;
    }
 
	sctk_assert( !task );
    mpcomp_task_t* current_task = MPCOMP_TASK_THREAD_GET_CURRENT_TASK( thread );

  	/* Find a remaining tied task */
  	mpcomp_task_list_t* list = MPCOMP_TASK_THREAD_GET_TIED_TASK_LIST_HEAD( thread ) ;
	sctk_assert( list );
         
    task = mpcomp_task_list_popfromhead( list, 0 );        

	for( type = 0; !task && type < MPCOMP_TASK_TYPE_COUNT; type++)
	{
        const int node_rank = MPCOMP_TASK_MVP_GET_TASK_LIST_NODE_RANK( mvp, type );
	    list = mpcomp_task_get_list( node_rank, type );
        sctk_assert( list );
	    if( mpcomp_task_list_trylock(list) )
            continue;
        task = mpcomp_task_list_popfromhead( list, current_task->depth );
	    mpcomp_task_list_unlock(list);
    } 

  	/* If no task found previously, try to thieve a task somewhere */
  	if( task == NULL ) task = __mpcomp_task_larceny();
    /* All tasks lists are empty, so exit task scheduling function */
	if( task == NULL ) return;
      
	__mpcomp_task_execute( task );

    // mpcomp_taskwait( task );
    
	/* Clean function */
	mpcomp_tast_clear_sister( task );
	//mpcomp_task_clear_parent( task );
    mpcomp_taskgroup_del_task( task ); 
	__mpcomp_task_finalize_deps( task );
    mpcomp_task_unref_parent_task( task );
}

void mpcomp_taskwait( void )
{
    mpcomp_task_t* current_task = NULL;         /* Current task execute */
    mpcomp_thread_t* omp_thread_tls = NULL;     /* thread private data  */

  	mpcomp_task_scheduling_infos_init();
     
    omp_thread_tls = ( mpcomp_thread_t* ) sctk_openmp_thread_tls;
	sctk_assert( omp_thread_tls );

    sctk_assert( omp_thread_tls->info.num_threads > 0 );

 	if( omp_thread_tls->info.num_threads > 1 )
	{	
        current_task = MPCOMP_TASK_THREAD_GET_CURRENT_TASK( omp_thread_tls );
  	    sctk_assert( current_task );

  	    /* Look for a children tasks list */
        while( current_task->children != NULL ) 
        {
	        mpcomp_task_schedule( 0 );
 	    }
    }
}
	
/*
 * The current may be suspended in favor of execution of
 * a different task
 * Called when encountering a taskyield construct
 */
void mpcomp_taskyield( void )
{
     /* Actually, do nothing */
}

void __mpcomp_task_coherency_entering_parallel_region() {
#if 0
  struct mpcomp_team_s *team;
  struct mpcomp_mvp_s **mvp;
  mpcomp_thread_t *t;
  mpcomp_thread_t *lt;
  int i, nb_mvps;

  t = (mpcomp_thread_t *)sctk_openmp_thread_tls;
  sctk_assert(t != NULL);

  if (t->children_instance->nb_mvps > 1) {
    team = t->children_instance->team;
    sctk_assert(team != NULL);

    /* Check team tasking cohenrency */
    sctk_debug("__mpcomp_task_coherency_entering_parallel_region: "
               "Team init %d and tasklist init %d with %d nb_tasks\n",
               sctk_atomics_load_int(&team->tasking_init_done),
               sctk_atomics_load_int(&team->tasklist_init_done),
               sctk_atomics_load_int(&team->nb_tasks));

    /* Check per thread task system coherency */
    mvp = t->children_instance->mvps;
    sctk_assert(mvp != NULL);
    nb_mvps = t->children_instance->nb_mvps;

    for (i = 0; i < nb_mvps; i++) {
      lt = &mvp[i]->threads[0];
      sctk_assert(lt != NULL);

      sctk_debug("__mpcomp_task_coherency_entering_parallel_region: "
                 "Thread in mvp %d init %d in implicit task %p\n",
                 mvp[i]->rank, lt->tasking_init_done, lt->current_task);

      sctk_assert(mpcomp_task_property_isset(lt->current_task->property,
                                             MPCOMP_TASK_IMPLICIT) != 0);
    }
  }
#endif
}

void __mpcomp_task_coherency_ending_parallel_region() {
#if 0
  struct mpcomp_team_s *team;
  struct mpcomp_mvp_s **mvp;
  mpcomp_thread_t *t;
  mpcomp_thread_t *lt;
  int i_task, i, nb_mvps;

  t = (mpcomp_thread_t *)sctk_openmp_thread_tls;
  sctk_assert(t != NULL);

  if (t->children_instance->nb_mvps > 1) {
    team = t->children_instance->team;
    sctk_assert(team != NULL);

    /* Check team tasking cohenrency */
    sctk_debug("__mpcomp_task_coherency_ending_parallel_region: "
               "Team init %d and tasklist init %d with %d nb_tasks\n",
               sctk_atomics_load_int(&team->tasking_init_done),
               sctk_atomics_load_int(&team->tasklist_init_done),
               sctk_atomics_load_int(&team->nb_tasks));

    /* Check per thread and mvp task system coherency */
    mvp = t->children_instance->mvps;
    sctk_assert(mvp != NULL);
    nb_mvps = t->children_instance->nb_mvps;

    for (i = 0; i < nb_mvps; i++) {
      lt = &mvp[i]->threads[0];

      sctk_debug("__mpcomp_task_coherency_ending_parallel_region: "
                 "Thread %d init %d in implicit task %p\n",
                 lt->rank, lt->tasking_init_done, lt->current_task);

      sctk_assert(lt->current_task != NULL);
      sctk_assert(lt->current_task->children == NULL);
      sctk_assert(lt->current_task->list == NULL);
      sctk_assert(lt->current_task->depth == 0);

      if (lt->tasking_init_done) {
        if (mvp[i]->threads[0].tied_tasks)
          sctk_assert(mpcomp_task_list_isempty(mvp[i]->threads[0].tied_tasks) ==
                      1);

        for (i_task = 0; i_task < MPCOMP_TASK_TYPE_COUNT; i_task++) {
          if (mvp[i]->tasklist[i_task]) {
            sctk_assert(mpcomp_task_list_isempty(mvp[i]->tasklist[i_task]) ==
                        1);
          }
        }
      }
    }
  }
#endif
}

void __mpcomp_task_coherency_barrier() {
#if 0
  mpcomp_thread_t *t;
  struct mpcomp_task_list_s *list = NULL;

  t = (mpcomp_thread_t *)sctk_openmp_thread_tls;
  sctk_assert(t != NULL);

  sctk_debug("__mpcomp_task_coherency_barrier: "
             "Thread %d exiting barrier in implicit task %p\n",
             t->rank, t->current_task);

  sctk_assert(t->current_task != NULL);
  sctk_assert(t->current_task->children == NULL);
  sctk_assert(t->current_task->list == NULL);
  sctk_assert(t->current_task->depth == 0);

  if (t->tasking_init_done) {
    /* Check tied tasks list */
    sctk_assert(mpcomp_task_list_isempty(t->tied_tasks) == 1);

    /* Check untied tasks list */
    list =
        mpcomp_task_get_list(t->mvp->tasklistNodeRank[MPCOMP_TASK_TYPE_UNTIED],
                             MPCOMP_TASK_TYPE_UNTIED);
    sctk_assert(list != NULL);
    sctk_assert(mpcomp_task_list_isempty(list) == 1);

    /* Check New type tasks list */
    list = mpcomp_task_get_list(t->mvp->tasklistNodeRank[MPCOMP_TASK_TYPE_NEW],
                                MPCOMP_TASK_TYPE_NEW);
    sctk_assert(list != NULL);
    sctk_assert(mpcomp_task_list_isempty(list) == 1);
  }
#endif
}

#else /* MPCOMP_TASK */

/* 
 * Creation of an OpenMP task 
 * Called when encountering an 'omp task' construct 
 */
void 
__mpcomp_task(void (*fn) (void *), void *data, void (*cpyfn) (void *, void *),
		   long arg_size, long arg_align, bool if_clause, unsigned flags)
{
    if(cpyfn)
        sctk_abort();

        fn(data);
}

/* 
 * Wait for the completion of the current task's children
 * Called when encountering a taskwait construct
 * Do nothing here as task are executed directly
 */
void 
mpcomp_taskwait()
{
}

/*
 * The current may be suspended in favor of execution of
 * a different task
 * Called when encountering a taskyield construct
 */
void mpcomp_taskyield()
{
     /* Actually, do nothing */
}

#endif /* MPCOMP_TASK */
