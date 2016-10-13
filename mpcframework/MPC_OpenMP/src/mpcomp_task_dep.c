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

int mpcomp_resolve_task_deps( void **depend )
{
    return 0;
}

static inline int
__mpcomp_debug_extract_task_deps( unsigned flags, void **depend )
{
    int i, j;
    int tot_filter_dep_num = 0;
    mpcomp_task_dep_data_t* filter_dep_list = NULL;

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

    filter_dep_list = ( mpcomp_task_dep_data_t*) sctk_malloc( sizeof( mpcomp_task_dep_data_t) * tot_deps_num );
     
    for( i = 0 ; i < tot_deps_num; i++ )
    {
        int found = 0;
        const uintptr_t cur_dep_addr = (uintptr_t) depend[ 2 + i ];
        const int cur_dep_type = ( i < out_deps_num ) ? MPCOMP_TASK_DEP_OUT : MPCOMP_TASK_DEP_IN;
  
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
    
    sctk_error("  === DEPS LIST === ");
    for( j = 0; j < tot_filter_dep_num; j++ )
    {
        sctk_error("[%d] task_addr: %p dep_type: %s redundant: %s", j, 
            filter_dep_list[j].base_addr, 
            mpcomp_task_dep_type_to_string[filter_dep_list[j].flag],
            (filter_dep_list[j].redundant) ? "true" : "false");
                
    }

    return tot_filter_dep_num;    
}

void
__mpcomp_task_with_deps( void (*fn) (void *), void *data, void (*cpyfn) (void *, void *), long arg_size, long arg_align, bool if_clause, unsigned flags, void **depend)
{
    mpcomp_thread_t* thread;
    mpcomp_task_t* current_task;

    __mpcomp_debug_extract_task_deps( flags, depend );

    thread = ( mpcomp_thread_t*) sctk_openmp_thread_tls;
    current_task = (mpcomp_task_t*) 

    if( mpcomp_task_dep_is_flag_with_deps( flags )  && ( depend != NULL ) )
    {
    }

    __mpcomp_task( fn, data, cpyfn, arg_size, arg_align, if_clause, flags );
}


