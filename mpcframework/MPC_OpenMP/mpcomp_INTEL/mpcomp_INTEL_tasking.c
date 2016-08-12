#include "mpcomp_internal.h"
#include "mpcomp_INTEL_types.h"
#include "mpcomp_INTEL_tasking.h"

#if MPCOMP_TASK

/********************************
  * TASKING SUPPORT
  *******************************/

// FROM kmp_os.h

#if KMP_ARCH_x86
#define KMP_SIZE_T_MAX (0xFFFFFFFF)
#else /* KMP_ARCH_x86 */
#define KMP_SIZE_T_MAX (0xFFFFFFFFFFFFFFFF)
#endif /* KMP_ARCH_x86 */

// FROM kmp_tasking.c
static size_t
__kmp_round_up_to_val(size_t size, size_t val)
{
    if( size & ( val - 1 ))
    {
        size &= ~( val - 1);
        if( size <= KMP_SIZE_T_MAX )
        {
            val += val;
        }
    }
    return size;
}

/* FOR OPENMP 3 */

kmp_int32
__kmpc_omp_task( ident_t *loc_ref, kmp_int32 gtid, kmp_task_t * new_task )
{
    return new_task->routine(0, new_task->shareds);
    //not_implemented() ;
   // return 0 ;
}

kmp_task_t*
__kmpc_omp_task_alloc( ident_t *loc_ref, kmp_int32 gtid, kmp_int32 flags,
                       size_t sizeof_kmp_task_t, size_t sizeof_shareds,
                       kmp_routine_entry_t task_entry )
{
    char *data_cpy;
    mpcomp_thread_t *t;
    struct mpcomp_task_s *task;
    struct mpcomp_task_s *parent;
    
    /* Initialize the OpenMP environment (1 per task) */
    __mpcomp_init();
    
    /* Initialize the OpenMP task environment (1 per worker) */ 
    __mpcomp_task_infos_init();

    /* Retrieve the information (microthread structure and current region) */
    t = (mpcomp_thread_t *) sctk_openmp_thread_tls;
    sctk_assert(t != NULL);

#if 0
    // Traduction des flags
    unsigned gomp_flags = 0;
    kmp_tasking_flags_t *input_flags = (kmp_tasking_flags_t *) &flags;
    
    if(input_flags->tiedness == 1)    
        gomp_flags |= 1;
    
    if(input_flags->final == 1) 
        gomp_flags |= 2;
#endif        
    size_t kmp_taskdata_size = 192; // sizeof( kmp_taskdata_t )
    size_t shareds_offset = kmp_taskdata_size + sizeof_kmp_task_t;
    shareds_offset = __kmp_round_up_to_val( shareds_offset, sizeof(void*));

    size_t size_to_alloc = shareds_offset + sizeof_shareds;
    kmp_taskdata_t* new_taskdata = mpcomp_malloc(1, size_to_alloc, 0); 
    kmp_task_t* new_task = (kmp_task_t*)((uintptr_t) new_taskdata + kmp_taskdata_size);

    /* Allocation de la tache */
    new_task->shareds = (void*)( task + 1);
    new_task->routine = task_entry;
    return new_task;
}

void
__kmpc_omp_task_begin_if0( ident_t *loc_ref, kmp_int32 gtid, kmp_task_t * task )
{
not_implemented() ;
}

void
__kmpc_omp_task_complete_if0( ident_t *loc_ref, kmp_int32 gtid, kmp_task_t *task )
{
not_implemented() ;
}

kmp_int32
__kmpc_omp_task_parts( ident_t *loc_ref, kmp_int32 gtid, kmp_task_t * new_task )
{
not_implemented() ;
return 0 ;
}

kmp_int32
__kmpc_omp_taskwait( ident_t *loc_ref, kmp_int32 gtid )
{
not_implemented() ;
return 0 ;
}

kmp_int32
__kmpc_omp_taskyield( ident_t *loc_ref, kmp_int32 gtid, int end_part )
{
not_implemented() ;
return 0 ;
}

/* Following 2 functions should be enclosed by "if TASK_UNUSED" */

void 
__kmpc_omp_task_begin( ident_t *loc_ref, kmp_int32 gtid, kmp_task_t * task )
{
not_implemented() ;
}

void 
__kmpc_omp_task_complete( ident_t *loc_ref, kmp_int32 gtid, kmp_task_t *task )
{
not_implemented() ;
}


/* FOR OPENMP 4 */

void 
__kmpc_taskgroup( ident_t * loc, int gtid )
{
not_implemented() ;
}

void 
__kmpc_end_taskgroup( ident_t * loc, int gtid )
{
not_implemented() ;
}

kmp_int32 
__kmpc_omp_task_with_deps ( ident_t *loc_ref, kmp_int32 gtid, kmp_task_t * new_task,
    kmp_int32 ndeps, kmp_depend_info_t *dep_list,
    kmp_int32 ndeps_noalias, kmp_depend_info_t *noalias_dep_list )
{
not_implemented() ;
return 0 ;
}

void 
__kmpc_omp_wait_deps ( ident_t *loc_ref, kmp_int32 gtid, kmp_int32 ndeps, kmp_depend_info_t *dep_list,
    kmp_int32 ndeps_noalias, kmp_depend_info_t *noalias_dep_list )
{
not_implemented() ;
}

void 
__kmp_release_deps ( kmp_int32 gtid, kmp_taskdata_t *task )
{
not_implemented() ;
}

#endif /* MPCOMP_TASK */
