#include "mpcomp_internal.h"
#include "mpcomp_INTEL_types.h"
#include "mpcomp_INTEL_tasking.h"

/********************************
  * TASKQ INTERFACE
  *******************************/

typedef struct kmpc_thunk_t {
} kmpc_thunk_t ;

typedef void (*kmpc_task_t) (kmp_int32 global_tid, struct kmpc_thunk_t *thunk);

typedef struct kmpc_shared_vars_t {
} kmpc_shared_vars_t;

kmpc_thunk_t * 
__kmpc_taskq (ident_t *loc, kmp_int32 global_tid, kmpc_task_t taskq_task, size_t sizeof_thunk,
    size_t sizeof_shareds, kmp_int32 flags, kmpc_shared_vars_t **shareds)
{
not_implemented() ;
return NULL ;
}

void 
__kmpc_end_taskq (ident_t *loc, kmp_int32 global_tid, kmpc_thunk_t *thunk)
{
not_implemented() ;
}

kmp_int32 
__kmpc_task (ident_t *loc, kmp_int32 global_tid, kmpc_thunk_t *thunk)
{
not_implemented() ;
return 0 ;
}

void 
__kmpc_taskq_task (ident_t *loc, kmp_int32 global_tid, kmpc_thunk_t *thunk, kmp_int32 status)
{
not_implemented() ;
}

void __kmpc_end_taskq_task (ident_t *loc, kmp_int32 global_tid, kmpc_thunk_t *thunk)
{
not_implemented() ;
}

kmpc_thunk_t * 
__kmpc_task_buffer (ident_t *loc, kmp_int32 global_tid, kmpc_thunk_t *taskq_thunk, kmpc_task_t task)
{
not_implemented() ;
return NULL ;
}
