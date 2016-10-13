#ifndef __MPCOMP_TASK_DEP_H__
#define __MPCOMP_TASK_DEP_H__

#include <sctk_bool.h>
#include <sctk_int.h>
#include <sctk_asm.h>
#include "mpcomp_internal.h"

#define MPCOMP_GOMP_DEPS_FLAG   8

typedef enum mpcomp_task_dep_type_e
{
    MPCOMP_TASK_DEP_NONE    = 0, 
    MPCOMP_TASK_DEP_IN      = 1, 
    MPCOMP_TASK_DEP_OUT     = 2, 
    MPCOMP_TASK_DEP_COUNT   = 3, 
} mpcomp_task_dep_type_t;

typedef struct mpcomp_task_dep_data_s
{
    uintptr_t base_addr;
    mpcomp_task_dep_type_t flag;
    unsigned int redundant;
} mpcomp_task_dep_data_t;

static char** 
mpcomp_task_dep_type_to_string[ MPCOMP_TASK_DEP_COUNT ] = { "MPCOMP_TASK_DEP_NONE", "MPCOMP_TASK_DEP_IN", "MPCOMP_TASK_DEP_OUT" };

static inline int
mpcomp_task_dep_is_flag_with_deps( const unsigned flags )
{
    return (flags & MPCOMP_GOMP_DEPS_FLAG);
}

#endif /* __MPCOMP_TASK_DEP_H__ */
