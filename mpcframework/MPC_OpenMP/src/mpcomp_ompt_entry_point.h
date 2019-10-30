
#ifndef __MPOCMP_OMPT_ENTRY_POINT_H__
#define __MPOCMP_OMPT_ENTRY_POINT_H__

#include "ompt.h"
#include "mpcomp_ompt_types_callback.h" 

typedef int (*ompt_enumerate_states_t)(
   int current_state,
   int* next_state,
   const char **next_state_name
);

typedef int (*ompt_enumerate_mutex_impls_t)(
   int current_impl,
   int* next_impl,
   const char **next_impl_name
);

#endif /* __MPOCMP_OMPT_ENTRY_POINT_H__ */
