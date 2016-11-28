
#include "mpcomp_types.h"
#include "mpcomp_intel_types.h"

int __kmp_force_reduction_method = reduction_method_not_defined;
int __kmp_init_common = 0;
kmp_cached_addr_t  *__kmp_threadpriv_cache_list = NULL; /* list for cleaning */
struct shared_table __kmp_threadprivate_d_table;

