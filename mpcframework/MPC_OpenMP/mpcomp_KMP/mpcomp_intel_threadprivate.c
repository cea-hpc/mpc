#include "sctk_debug.h"
#include "sctk_alloc.h"
#include "mpcomp_types.h"
#include "mpcomp_intel_types.h"
#include "mpcomp_intel_threadprivate.h"
#include "mpcomp_intel_global.h"
#include "mpcomp_barrier.h"

struct private_common *
__kmp_threadprivate_find_task_common(struct common_table *tbl, int gtid,
                                     void *pc_addr) {
  struct private_common *tn;
  for (tn = tbl->data[KMP_HASH(pc_addr)]; tn; tn = tn->next) {
    if (tn->gbl_addr == pc_addr) {
      return tn;
    }
  }
  return 0;
}

struct shared_common *__kmp_find_shared_task_common(struct shared_table *tbl,
                                                    int gtid, void *pc_addr) {
  struct shared_common *tn;
  for (tn = tbl->data[KMP_HASH(pc_addr)]; tn; tn = tn->next) {
    if (tn->gbl_addr == pc_addr) {
      return tn;
    }
  }
  return 0;
}

struct private_common *kmp_threadprivate_insert(int gtid, void *pc_addr,
                                                void *data_addr,
                                                size_t pc_size) {
  struct private_common *tn, **tt;
  static sctk_thread_mutex_t lock = SCTK_THREAD_MUTEX_INITIALIZER;
  mpcomp_thread_t *t;
  t = (mpcomp_thread_t *)sctk_openmp_thread_tls;

  /* critical section */
  sctk_thread_mutex_lock(&lock);
  tn = (struct private_common *)sctk_malloc(sizeof(struct private_common));
  memset(tn, 0, sizeof(struct private_common));
  tn->gbl_addr = pc_addr;
  tn->cmn_size = pc_size;

  if (gtid == 0)
    tn->par_addr = (void *)pc_addr;
  else {
    tn->par_addr = (void *)sctk_malloc(tn->cmn_size);
  }
  memcpy(tn->par_addr, pc_addr, pc_size);
  sctk_thread_mutex_unlock(&lock);
  /* end critical section */

  tt = &(t->th_pri_common->data[KMP_HASH(pc_addr)]);
  tn->next = *tt;
  *tt = tn;
  tn->link = t->th_pri_head;
  t->th_pri_head = tn;

  return tn;
}

void *__kmpc_threadprivate(ident_t *loc, kmp_int32 global_tid, void *data,
                           size_t size) {
  void *ret = NULL;
  struct private_common *tn;
  mpcomp_thread_t *t;
  t = (mpcomp_thread_t *)sctk_openmp_thread_tls;

  tn = __kmp_threadprivate_find_task_common(t->th_pri_common, global_tid, data);
  if (!tn) {
    tn = kmp_threadprivate_insert(global_tid, data, data, size);
  } else {
    if ((size_t)size > tn->cmn_size) {
      sctk_error(
          "TPCommonBlockInconsist: -> Wong size threadprivate variable found");
      sctk_abort();
    }
  }

  ret = tn->par_addr;
  return ret;
}

int __kmp_default_tp_capacity() {
  char *env;
  int nth = 128;
  int __kmp_xproc = 0, req_nproc = 0, r = 0, __kmp_max_nth = 32 * 1024;

  r = sysconf(_SC_NPROCESSORS_ONLN);
  __kmp_xproc = (r > 0) ? r : 2;

  env = getenv("OMP_NUM_THREADS");
  if (env != NULL) {
    req_nproc = strtol(env, NULL, 10);
  }

  if (nth < (4 * req_nproc))
    nth = (4 * req_nproc);
  if (nth < (4 * __kmp_xproc))
    nth = (4 * __kmp_xproc);

  if (nth > __kmp_max_nth)
    nth = __kmp_max_nth;

  return nth;
}

void __kmpc_copyprivate(ident_t *loc, kmp_int32 global_tid, size_t cpy_size,
                        void *cpy_data, void (*cpy_func)(void *, void *),
                        kmp_int32 didit) {
  mpcomp_thread_t *t; /* Info on the current thread */

  void **data_ptr;
  /* Grab the thread info */
  t = (mpcomp_thread_t *)sctk_openmp_thread_tls;
  sctk_assert(t != NULL);

  /* In this flow path, the number of threads should not be 1 */
  sctk_assert(t->info.num_threads > 0);

  /* Grab the team info */
  sctk_assert(t->instance != NULL);
  sctk_assert(t->instance->team != NULL);

  data_ptr = &(t->instance->team->single_copyprivate_data);
  if (didit)
    *data_ptr = cpy_data;

  __mpcomp_barrier();

  if (!didit)
    (*cpy_func)(cpy_data, *data_ptr);

  __mpcomp_barrier();
}

void *__kmpc_threadprivate_cached(ident_t *loc, kmp_int32 global_tid,
                                  void *data, size_t size, void ***cache) {
  static sctk_thread_mutex_t lock = SCTK_THREAD_MUTEX_INITIALIZER;
  int __kmp_tp_capacity = __kmp_default_tp_capacity();
  if (*cache == 0) {
    sctk_thread_mutex_lock(&lock);
    if (*cache == 0) {
      // handle cache to be dealt with later
      void **my_cache;
      my_cache = (void **)malloc(sizeof(void *) * __kmp_tp_capacity +
                                 sizeof(kmp_cached_addr_t));
      memset(my_cache, 0,
             sizeof(void *) * __kmp_tp_capacity + sizeof(kmp_cached_addr_t));
      kmp_cached_addr_t *tp_cache_addr;
      tp_cache_addr = (kmp_cached_addr_t *)((char *)my_cache +
                                            sizeof(void *) * __kmp_tp_capacity);
      tp_cache_addr->addr = my_cache;
      tp_cache_addr->next = __kmp_threadpriv_cache_list;
      __kmp_threadpriv_cache_list = tp_cache_addr;
      *cache = my_cache;
    }
    sctk_thread_mutex_unlock(&lock);
  }

  void *ret = NULL;
  if ((ret = (*cache)[global_tid]) == 0) {
    ret = __kmpc_threadprivate(loc, global_tid, data, (size_t)size);
    (*cache)[global_tid] = ret;
  }
  return ret;
}

void __kmpc_threadprivate_register(ident_t *loc, void *data, kmpc_ctor ctor,
                                   kmpc_cctor cctor, kmpc_dtor dtor) {
  struct shared_common *d_tn, **lnk_tn;

  /* Only the global data table exists. */
  d_tn = __kmp_find_shared_task_common(&__kmp_threadprivate_d_table, -1, data);

  if (d_tn == 0) {
    d_tn = (struct shared_common *)sctk_malloc(sizeof(struct shared_common));
    memset(d_tn, 0, sizeof(struct shared_common));
    d_tn->gbl_addr = data;

    d_tn->ct.ctor = ctor;
    d_tn->cct.cctor = cctor;
    d_tn->dt.dtor = dtor;

    lnk_tn = &(__kmp_threadprivate_d_table.data[KMP_HASH(data)]);

    d_tn->next = *lnk_tn;
    *lnk_tn = d_tn;
  }
}

void __kmpc_threadprivate_register_vec(ident_t *loc, void *data,
                                       kmpc_ctor_vec ctor, kmpc_cctor_vec cctor,
                                       kmpc_dtor_vec dtor,
                                       size_t vector_length) {
  struct shared_common *d_tn, **lnk_tn;

  d_tn = __kmp_find_shared_task_common(
      &__kmp_threadprivate_d_table, -1,
      data); /* Only the global data table exists. */
  if (d_tn == 0) {
    d_tn = (struct shared_common *)sctk_malloc(sizeof(struct shared_common));
    memset(d_tn, 0, sizeof(struct shared_common));
    d_tn->gbl_addr = data;

    d_tn->ct.ctorv = ctor;
    d_tn->cct.cctorv = cctor;
    d_tn->dt.dtorv = dtor;
    d_tn->is_vec = TRUE;
    d_tn->vec_len = (size_t)vector_length;

    lnk_tn = &(__kmp_threadprivate_d_table.data[KMP_HASH(data)]);

    d_tn->next = *lnk_tn;
    *lnk_tn = d_tn;
  }
}
