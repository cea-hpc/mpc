
#ifndef __MPCOMP_INTEL_API_H__
#define __MPCOMP_INTEL_API_H__

kmp_int32 __kmpc_global_thread_num(ident_t *);
kmp_int32 __kmpc_global_num_threads(ident_t *);
kmp_int32 __kmpc_bound_thread_num(ident_t *);
kmp_int32 __kmpc_bound_num_threads(ident_t *);
kmp_int32 __kmpc_in_parallel(ident_t *);
void __kmpc_flush(ident_t *loc, ...);

#endif /* __MPCOMP_INTEL_API_H__ */
