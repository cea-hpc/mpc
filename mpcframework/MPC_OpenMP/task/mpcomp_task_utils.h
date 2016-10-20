
#if (!defined( __SCTK_MPCOMP_TASK_UTILS_H__) && MPCOMP_TASK)
#define __SCTK_MPCOMP_TASK_UTILS_H__

#include "sctk_runtime_config.h"
#include "mpcomp_internal.h"

#include "mpcomp_alloc.h"
#include "mpcomp_task.h"
#include "mpcomp_task_list.h"
#include "mpcomp_tree_utils.h"
#include "mpcomp_task_macros.h"

// Round up a size to a power of two specified by val
// Used to insert padding between structures co-allocated using a single malloc() call
static size_t
__kmp_round_up_to_val( size_t size, size_t val ) 
{
    if ( size & ( val - 1 ) ) 
    {
        size &= ~ ( val - 1 );
        if ( size <= KMP_SIZE_T_MAX - val ) 
            size += val;    // Round up if there is no overflow.
    }
    return size;
} // __kmp_round_up_to_va

/*** Task property primitives ***/

static inline void 
mpcomp_task_reset_property(mpcomp_task_property_t *property)
{
	*property = 0;
}

static inline void 
mpcomp_task_set_property(mpcomp_task_property_t *property, mpcomp_task_property_t mask)
{
	*property |= mask;
}

static inline void 
mpcomp_task_unset_property(mpcomp_task_property_t *property, mpcomp_task_property_t mask)
{
	*property &= ~( mask );
}

static inline int 
mpcomp_task_property_isset(mpcomp_task_property_t property, mpcomp_task_property_t mask)
{
	return ( property & mask );
}

/* Is Serialized Task Context if_clause not handle */
static inline int 
mpcomp_task_no_deps_is_serialized( mpcomp_thread_t* thread )
{
    mpcomp_task_t* current_task = NULL;
    sctk_assert( thread );

    current_task = MPCOMP_TASK_THREAD_GET_CURRENT_TASK( thread );
    sctk_assert( current_task );

    if ( ( current_task && mpcomp_task_property_isset (current_task->property, MPCOMP_TASK_FINAL ) )
        || ( thread->info.num_threads == 1 )
        || ( current_task && current_task->depth > 8/*t->instance->team->task_nesting_max*/ ) )
    {
        return 1;
    }

    return 0;
}


static inline void
mpcomp_task_infos_reset( mpcomp_task_t* task )
{
	sctk_assert( task );
	memset( task, 0,  sizeof( mpcomp_task_t ) );
}

static inline int
mpcomp_task_is_final( unsigned int flags, mpcomp_task_t* parent )
{
	if( flags & 2 )
	{
		return 1;
	}

	if( parent && mpcomp_task_property_isset ( parent->property, MPCOMP_TASK_FINAL ) )
	{
		return 1;
	}

	return 0;
}
  
/* Initialization of a task structure */
static inline void 
mpcomp_task_infos_init( mpcomp_task_t *task, void (*func) (void *), void *data, struct mpcomp_thread_s *thread )
{
	sctk_assert( task != NULL );
	sctk_assert( thread != NULL );

	/* Reset all task infos to NULL */
	mpcomp_task_infos_reset( task );

	/* Set non null task infos field */
  	task->func = func;
 	task->data = data;
	task->icvs = thread->info.icvs;

  	task->parent = MPCOMP_TASK_THREAD_GET_CURRENT_TASK( thread );
   task->depth = ( task->parent ) ? task->parent->depth + 1 : 0;
   task->children_lock = SCTK_SPINLOCK_INITIALIZER;
}

static inline void
mpcomp_task_node_task_infos_reset( struct mpcomp_node_s* node )
{
	sctk_assert( node );
	memset( &( node->task_infos ), 0, sizeof( mpcomp_task_node_infos_t ) );
}

static inline void
mpcomp_task_mvp_task_infos_reset( struct mpcomp_mvp_s* mvp )
{
	sctk_assert( mvp );
	memset( &( mvp->task_infos ), 0, sizeof( mpcomp_task_mvp_infos_t ) );
}

static inline int
mpcomp_task_thread_get_numa_node_id( struct mpcomp_thread_s* thread )
{
	sctk_assert( thread );
	
	if( thread->info.num_threads > 1 )
	{
		sctk_assert( thread->mvp && thread->mvp->father );
		return thread->mvp->father->id_numa;
	}
	return sctk_get_node_from_cpu(sctk_get_init_vp(sctk_get_task_rank()));
}

static inline void
mpcomp_task_thread_task_infos_reset( struct mpcomp_thread_s* thread )
{
	sctk_assert( thread );
	memset( &( thread->task_infos ), 0, sizeof( mpcomp_task_thread_infos_t ) );
}

static inline void
mpcomp_task_thread_infos_init( struct mpcomp_thread_s* thread )
{
	sctk_assert( thread );	

	if( !MPCOMP_TASK_THREAD_IS_INITIALIZED( thread ) )
	{
		mpcomp_task_t* implicite_task;
		mpcomp_task_list_t* tied_tasks_list;

		const int numa_node_id = mpcomp_task_thread_get_numa_node_id( thread );

		/* Allocate the default current task (no func, no data, no parent) */
		implicite_task = mpcomp_malloc( 1, sizeof(mpcomp_task_t), numa_node_id );
		sctk_assert( implicite_task );
		MPCOMP_TASK_THREAD_SET_CURRENT_TASK( thread, NULL );

		mpcomp_task_infos_init( implicite_task, NULL, NULL, thread );

		/* Allocate private task data structures */ 
		tied_tasks_list = mpcomp_malloc( 1, sizeof(mpcomp_task_list_t), numa_node_id );
		sctk_assert( tied_tasks_list );

		MPCOMP_TASK_THREAD_SET_CURRENT_TASK( thread, implicite_task );
		MPCOMP_TASK_THREAD_SET_TIED_TASK_LIST_HEAD( thread, tied_tasks_list );

		/* Change tasking_init_done to avoid multiple thread init */ 
		MPCOMP_TASK_THREAD_CMPL_INIT( thread );
	}
	
}

static inline void
mpcomp_task_instance_task_infos_reset( struct mpcomp_instance_s* instance )
{
	sctk_assert( instance );
	memset( &( instance->task_infos ), 0, sizeof( mpcomp_task_instance_infos_t ) );
}

static inline void
mpcomp_task_instance_task_infos_init( struct mpcomp_instance_s* instance ) 
{
	int i, array_tree_total_size;
	int* tree_base, *array_tree_level_size, *array_tree_level_first;

	sctk_assert( instance && instance->tree_base );	
	tree_base = instance->tree_base;

	const int depth = instance->tree_depth;
	array_tree_level_size = ( int* ) mpcomp_malloc( 0, sizeof(int) * ( depth + 1 ), 0 );	
	sctk_assert( array_tree_level_size );
	
	array_tree_level_first = ( int* ) mpcomp_malloc( 0, sizeof(int) * ( depth + 1 ), 0 );
   sctk_assert( array_tree_level_first );

	array_tree_total_size = 1;
	array_tree_level_size[0] = 1;
	array_tree_level_first[0] = 0;

	for( i = 1; i < depth + 1; i++ ) 
	{
		const int prev_tree_level_size = array_tree_level_size[i-1];
		array_tree_level_size[i] = prev_tree_level_size * tree_base[i-1]; 
		array_tree_level_first[i] = array_tree_level_first[i-1] + prev_tree_level_size; 
		array_tree_total_size += array_tree_level_size[i];
	}		

	MPCOMP_TASK_INSTANCE_SET_ARRAY_TREE_TOTAL_SIZE( instance, array_tree_total_size );	
	MPCOMP_TASK_INSTANCE_SET_ARRAY_TREE_LEVEL_SIZE( instance, array_tree_level_size );	
	MPCOMP_TASK_INSTANCE_SET_ARRAY_TREE_LEVEL_FIRST( instance, array_tree_level_first );	
}

static inline void
mpcomp_task_team_infos_reset( struct mpcomp_team_s* team )
{
	sctk_assert( team );
	memset( &( team->task_infos ), 0, sizeof( mpcomp_task_team_infos_t ) );
} 

static inline void
mpcomp_task_team_infos_init( struct mpcomp_team_s* team, int depth )
{
    mpcomp_task_team_infos_reset( team );

	const int new_tasks_depth_value 		= sctk_runtime_config_get()->modules.openmp.omp_new_task_depth;
	const int untied_tasks_depth_value 	= sctk_runtime_config_get()->modules.openmp.omp_untied_task_depth;
	const int omp_task_nesting_max		= sctk_runtime_config_get()->modules.openmp.omp_task_nesting_max;
	const int omp_task_larceny_mode 		= sctk_runtime_config_get()->modules.openmp.omp_task_larceny_mode;	

	/* Ensure tasks lists depths are correct */
	const int new_tasks_depth 		= ( new_tasks_depth_value < depth ) ? new_tasks_depth_value : depth;
	const int untied_tasks_depth 	= ( untied_tasks_depth_value < depth ) ? untied_tasks_depth_value : depth;

	/* Set variable in team task infos */
	MPCOMP_TASK_TEAM_SET_TASKLIST_DEPTH( team, MPCOMP_TASK_TYPE_NEW, new_tasks_depth );	
	MPCOMP_TASK_TEAM_SET_TASKLIST_DEPTH( team, MPCOMP_TASK_TYPE_UNTIED, untied_tasks_depth );	
	MPCOMP_TASK_TEAM_SET_TASK_NESTING_MAX( team, omp_task_nesting_max );		
	MPCOMP_TASK_TEAM_SET_TASK_LARCENY_MODE( team, omp_task_larceny_mode );	
}

/* mpcomp_task.c */
int mpcomp_task_all_task_executed( void );
void __mpcomp_task_schedule( int filter );
void __mpcomp_task_exit( void );
int paranoiac_test_task_exit_check(void);
int mpcomp_task_get_task_left_in_team( struct mpcomp_team_s*);

/* Return list of 'type' tasks contained in element of rank 'globalRank' */
static inline struct mpcomp_task_list_s *
mpcomp_task_get_list( int globalRank, mpcomp_tasklist_type_t type )
{
	mpcomp_thread_t* thread = NULL;
	struct mpcomp_task_list_s * list = NULL;

	sctk_assert( globalRank >= 0 );
 	sctk_assert( sctk_openmp_thread_tls != NULL );
	sctk_assert( type >= 0 && type < MPCOMP_TASK_TYPE_COUNT );

	/* Retrieve the current thread information */
	thread = ( mpcomp_thread_t * ) sctk_openmp_thread_tls;

	sctk_assert( thread->mvp != NULL );
	sctk_assert( globalRank < MPCOMP_TASK_INSTANCE_GET_ARRAY_TREE_TOTAL_SIZE( thread->instance ) );

   if( !MPCOMP_TASK_MVP_GET_TREE_ARRAY_NODES( thread->mvp ) ) 
   {
   	return NULL;
   }

	if (mpcomp_is_leaf( thread->instance, globalRank ) ) 
	{
   	struct mpcomp_mvp_s* mvp = (struct mpcomp_mvp_s *) ( MPCOMP_TASK_MVP_GET_TREE_ARRAY_NODE( thread->mvp, globalRank ) );
		list = mvp->task_infos.tasklist[type];
  	}
	else
	{
		struct mpcomp_node_s *node = (struct mpcomp_node_s *) ( MPCOMP_TASK_MVP_GET_TREE_ARRAY_NODE( thread->mvp, globalRank ) );
		list = node->task_infos.tasklist[type];
	}
	
	sctk_assert( list != NULL );
   return list;
}

#endif /* __SCTK_MPCOMP_TASK_UTILS_H__ */
