
#ifndef __MPCOMP_INTEL_THREADPRIVATE_H__
#define __MPCOMP_INTEL_THREADPRIVATE_H__

#include "mpcomp_intel_types.h"

typedef struct common_table {
  struct private_common *data[KMP_HASH_TABLE_SIZE];
} mpcomp_kmp_common_table_t;

struct private_common *
__kmp_threadprivate_find_task_common(struct common_table *tbl, kmp_int32,
                                     void *);
struct shared_common *__kmp_find_shared_task_common(struct shared_table *,
                                                    kmp_int32, void *);
struct private_common *kmp_threadprivate_insert(kmp_int32, void *, void *,
                                                size_t);
void *__kmpc_threadprivate(ident_t *, kmp_int32, void *, size_t);
kmp_int32 __kmp_default_tp_capacity(void);
void __kmpc_copyprivate(ident_t *, kmp_int32, size_t, void *,
                        void (*cpy_func)(void *, void *), kmp_int32);
void *__kmpc_threadprivate_cached(ident_t *, kmp_int32, void *, size_t,
                                  void ***);
void __kmpc_threadprivate_register(ident_t *, void *, kmpc_ctor, kmpc_cctor,
                                   kmpc_dtor);
void __kmpc_threadprivate_register_vec(ident_t *, void *, kmpc_ctor_vec,
                                       kmpc_cctor_vec, kmpc_dtor_vec, size_t);

#endif /* __MPCOMP_INTEL_THREADPRIVATE_H__ */
