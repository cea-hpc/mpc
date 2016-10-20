#include <sctk_bool.h>
#include <sctk_int.h>
#include <sctk_asm.h>
#include "mpcomp_internal.h"
#include "sctk_runtime_config_struct.h"

#include "mpcomp_task.h"
#include "mpcomp_task_tree.h"
#include "mpcomp_task_macros.h"
#include "mpcomp_task_list.h"
#include "mpcomp_task_utils.h"
#include "mpcomp_tree_utils.h"
#include "mpcomp_task_stealing.h"

#include "mpcomp_task_dep.h"
#include "MurmurHash_64.h"

static int total_task = 0;
static float max_percent = 0.0;

int mpcomp_resolve_task_deps( void **depend )
{
    return 0;
}
#if 0
static inline int
__mpcomp_debug_extract_task_deps( unsigned flags, void **depend )
{
    int i, j;
    int tot_filter_dep_num = 0;
	 mpcomp_task_hash_elt_t *new_entry;
    mpcomp_task_dep_entry_t* filter_dep_list = NULL;

    if( !mpcomp_task_dep_is_flag_with_deps( flags )  || depend == NULL ) 
    {
        sctk_error( "Deps task flag not define (depend:%p)", depend );
        return tot_filter_dep_num;
    }

    const size_t tot_deps_num = (uintptr_t) depend[0];
    const size_t out_deps_num = (uintptr_t) depend[1];

    if( !tot_deps_num )
    {
        return tot_filter_dep_num;
    }

    filter_dep_list = ( mpcomp_task_dep_entry_t*) sctk_malloc( sizeof( mpcomp_task_dep_entry_t) * tot_deps_num );
     
    for( i = 0 ; i < tot_deps_num; i++ )
    {
        	int found = 0;
        	const uintptr_t cur_dep_addr = (uintptr_t) depend[ 2 + i ];
        	const int cur_dep_type = ( i < out_deps_num ) ? MPCOMP_TASK_DEP_OUT : MPCOMP_TASK_DEP_IN;
 
			uint64_t	hash = simple_mix_hash( cur_dep_addr ) % MPCOMP_MAX_DEP_PER_TASK; 
			
			if( htable->buckets[hash] == NULL )
			{
				mpcomp_task_hash_elt_t *new_entry = sctk_malloc(sizeof(mpcomp_task_hash_elt_t));
				new_entry->base_addr = cur_dep_addr;
				htable->buckets[hash] = new_entry;
			}

        for( j = 0; j < tot_filter_dep_num; j++ )
        {
            if( (uintptr_t) filter_dep_list[j].base_addr == cur_dep_addr )
            {
                /* Update dep type  IN < OUT (INOUT == OUT) */
                if( filter_dep_list[j].flag < cur_dep_type )
                {
                    filter_dep_list[j].flag = cur_dep_type;
                }

                filter_dep_list[j].redundant++; 
                found = 1;
				
                break;
            }
        }

        if( !found)
        {
                filter_dep_list[tot_filter_dep_num].base_addr = cur_dep_addr;
                filter_dep_list[tot_filter_dep_num].flag = cur_dep_type; 
                filter_dep_list[tot_filter_dep_num].redundant = 0; 
                tot_filter_dep_num++;
       }

    } 
	
	
/*    
    sctk_error("  === DEPS LIST === ");
    for( j = 0; j < tot_filter_dep_num; j++ )
    {
        sctk_error("[%d] task_addr: %p dep_type: %s redundant: %-3s hash1: %lu", j, 
            filter_dep_list[j].base_addr, 
            mpcomp_task_dep_type_to_string[filter_dep_list[j].flag],
            (filter_dep_list[j].redundant) ? "true" : "false",
				simple_mix_hash(filter_dep_list[j].base_addr) % MPCOMP_MAX_DEP_PER_TASK);
                
    }
*/
    return tot_filter_dep_num;    
}
#endif

static int 
__mpcomp_task_process_deps( mpcomp_task_dep_node_t* task_node, mpcomp_task_dep_ht_table_t* htable, void **depend )
{

	int i, j, predecessors_num;
	mpcomp_task_dep_ht_entry_t* entry;
	mpcomp_task_dep_node_t* last_out;

   const size_t tot_deps_num = (uintptr_t) depend[0];
   const size_t out_deps_num = (uintptr_t) depend[1];
	
	// Filter redundant value
	int task_already_process_num = 0;
	uintptr_t* task_already_process_list = sctk_malloc( sizeof( uintptr_t ) * tot_deps_num );
	sctk_assert( task_already_process_list );	
	
	predecessors_num = 0;
	last_out = entry->last_out;

   for( i = 0; i < tot_deps_num; i++ )
   {
		int redundant = 0;

		/* FIND HASH IN HTABLE */
		const uintptr_t addr =  (uintptr_t) depend[ 2 + i ];
		const int type = ( i < out_deps_num ) ? MPCOMP_TASK_DEP_OUT : MPCOMP_TASK_DEP_IN;

		for( j = 0; j < task_already_process_num; j++ ) 
		{
			if( task_already_process_list[i] ) 
			{
				redundant = 1;
				break;
			}
		}
		
		/** OUT are in first position en OUT > IN deps */
		if( redundant ) continue;
	
		task_already_process_list[task_already_process_num++] = addr;

		entry = mpcomp_task_dep_add_entry( htable, addr );
		sctk_assert( entry );

		// NEW [out] dep must be after all [in] deps
		if( type == MPCOMP_TASK_DEP_OUT && entry->last_in != NULL )
		{
			mpcomp_task_dep_node_list_t* node_list;
			for( node_list = entry->last_in; node_list; node_list = node_list->next )
			{
				mpcomp_task_dep_node_t* node = node_list->node;
				if( node->task != NULL )
				{
					MPCOMP_TASK_DEP_LOCK_NODE( node );
					if( node->task != NULL )
					{
						node->successors = mpcomp_task_dep_add_node( node->successors, task_node ); 
						predecessors_num++;
					}
					MPCOMP_TASK_DEP_UNLOCK_NODE( node );
				}
			}
		}
		else
		{
			/** Non executed OUT dependency**/
			if( last_out && last_out->task ) 
			{
				MPCOMP_TASK_DEP_LOCK_NODE( last_out );	
				if( last_out->task )
				{
					last_out->successors = mpcomp_task_dep_add_node( last_out->successors, task_node );
					predecessors_num++;
				}
				MPCOMP_TASK_DEP_UNLOCK_NODE( last_out );	
			}
		}

		if( type == MPCOMP_TASK_DEP_OUT )
		{
			mpcomp_task_dep_node_unref( last_out );
			last_out = mpcomp_task_dep_node_ref( task_node );
		}
		else
		{
			sctk_assert( type == MPCOMP_TASK_DEP_IN );
			entry->last_in = mpcomp_task_dep_add_node( entry->last_in, task_node );
		}
	}
	
	sctk_free( task_already_process_list );	
	return predecessors_num;
}

void
__mpcomp_task_finalize_deps( mpcomp_task_t* task )
{

	return;
}

void
__mpcomp_task_with_deps( void (*fn) (void *), void *data, void (*cpyfn) (void *, void *), long arg_size, long arg_align, bool if_clause, unsigned flags, void **depend)
{
	int predecessors_num;
   mpcomp_thread_t* thread;
	mpcomp_task_dep_node_t* task_node;
   mpcomp_task_t *current_task, *new_task;

   thread = ( mpcomp_thread_t*) sctk_openmp_thread_tls;
   current_task = (mpcomp_task_t*) MPCOMP_TASK_THREAD_GET_CURRENT_TASK( thread );  
	
   if( mpcomp_task_dep_is_flag_with_deps( flags ) || ( depend == NULL ) )
	{
		__mpcomp_task( fn, data, cpyfn, arg_size, arg_align, if_clause, flags );
		return;
	}

	/* Is it possible ?! See GOMP source code	*/ 
	sctk_assert( (uintptr_t) depend[0] > (uintptr_t) 0 );

	if( !( current_task->task_dep_infos->htable ) )
	{
		current_task->task_dep_infos->htable = mpcomp_task_dep_alloc_task_htable( &mpcomp_task_dep_mpc_hash_func );
		sctk_assert( current_task->task_dep_infos->htable );
	}
	
	task_node = mpcomp_task_dep_new_node();	
	sctk_assert( task_node );

	new_task = __mpcomp_task_alloc( fn, data, cpyfn, arg_size, arg_align, if_clause, flags, 0 ); 	
	task_node->task = new_task;
	predecessors_num  = __mpcomp_task_process_deps( task_node, current_task->task_dep_infos->htable, depend );		

	if( !predecessors_num )
	{
		__mpcomp_task_process( fn, data, cpyfn, arg_size, arg_align, if_clause, flags );
	}
}
