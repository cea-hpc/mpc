
#ifdef MPCOMP_USE_TASKDEP

#ifndef __MPCOMP_TASK_DEP_H__
#define __MPCOMP_TASK_DEP_H__

#include <sctk_bool.h>
#include <sctk_int.h>
#include <sctk_asm.h>
#include <sctk_atomics.h>

#define MPCOMP_TASK_DEP_GOMP_DEPS_FLAG 8

#define MPCOMP_TASK_DEP_INTEL_HTABLE_SIZE 997
#define MPCOMP_TASK_DEP_INTEL_HTABLE_SEED 6

#define MPCOMP_TASK_DEP_MPC_HTABLE_SIZE 1001 /* 7 * 11 * 13 */
#define MPCOMP_TASK_DEP_MPC_HTABLE_SEED 2

#define MPCOMP_TASK_DEP_LOCK_NODE(node) sctk_spinlock_lock(&(node->lock))

#define MPCOMP_TASK_DEP_UNLOCK_NODE(node) sctk_spinlock_unlock(&(node->lock))

/* Datastruct from other header */
struct mpcomp_task_s; /*@see mpcomp_task.h */

/**
 * \param[in] 	addr	uintptr_t	depend addr ptr
 * \param[in] 	size	uint32_t 	max hash size value
 * \param[in] 	seed	uint32_t 	seed value (can be ignored)
 * \return 		hash 	uint32_t 	hash computed value
 */
typedef uint32_t (*mpcomp_task_dep_hash_func_t)(uintptr_t, uint32_t, uint32_t);

/**
 * Don't modify order NONE < IN < OUT */
typedef enum mpcomp_task_dep_type_e {
  MPCOMP_TASK_DEP_NONE = 0,
  MPCOMP_TASK_DEP_IN = 1,
  MPCOMP_TASK_DEP_OUT = 2,
  MPCOMP_TASK_DEP_COUNT = 3,
} mpcomp_task_dep_type_t;

static char *mpcomp_task_dep_type_to_string[MPCOMP_TASK_DEP_COUNT] = {
    "MPCOMP_TASK_DEP_NONE", /*  MPCOMP_TASK_DEP_NONE    = 0 */
    "MPCOMP_TASK_DEP_IN  ", /*  MPCOMP_TASK_DEP_IN      = 1 */
    "MPCOMP_TASK_DEP_OUT "  /*  MPCOMP_TASK_DEP_IN      = 2 */
};

typedef enum mpcomp_task_dep_htable_op_e {
  MPCOMP_TASK_DEP_HTABLE_OP_INSERT = 0,
  MPCOMP_TASK_DEP_HTABLE_OP_DELETE = 1,
  MPCOMP_TASK_DEP_HTABLE_OP_SEARCH = 2,
  MPCOMP_TASK_DEP_HTABLE_OP_COUNT = 3,
} mpcomp_task_dep_htable_op_t;

static char
    *mpcomp_task_dep_htable_op_to_string[MPCOMP_TASK_DEP_HTABLE_OP_COUNT] = {
        "MPCOMP_TASK_DEP_HTABLE_OP_INSERT", "MPCOMP_TASK_DEP_HTABLE_OP_DELETE",
        "MPCOMP_TASK_DEP_HTABLE_OP_SEARCH"};

typedef enum mpcomp_task_dep_task_status_e {
  MPCOMP_TASK_DEP_TASK_PROCESS_DEP = 0,
  MPCOMP_TASK_DEP_TASK_NOT_EXECUTE = 1,
  MPCOMP_TASK_DEP_TASK_RELEASED = 2,
  MPCOMP_TASK_DEP_TASK_FINALIZED = 3,
  MPCOMP_TASK_DEP_TASK_COUNT = 4
} mpcomp_task_dep_task_status_t;

static char *mpcomp_task_dep_task_status_to_string[MPCOMP_TASK_DEP_TASK_COUNT] =
    {"MPCOMP_TASK_DEP_TASK_PROCESS_DEP", "MPCOMP_TASK_DEP_TASK_NOT_EXECUTE",
     "MPCOMP_TASK_DEP_TASK_RELEASED", "MPCOMP_TASK_DEP_TASK_FINALIZED"}; 

typedef struct mpcomp_task_dep_node_s {
  sctk_spinlock_t lock;
  sctk_atomics_int ref_counter;
  sctk_atomics_int predecessors;
  struct mpcomp_task_s *task;
  sctk_atomics_int status;
  struct mpcomp_task_dep_node_list_s *successors;
  bool if_clause;
} mpcomp_task_dep_node_t;

typedef struct mpcomp_task_dep_node_list_s {
  mpcomp_task_dep_node_t *node;
  struct mpcomp_task_dep_node_list_s *next;
} mpcomp_task_dep_node_list_t;

typedef struct mpcomp_task_dep_ht_entry_s {
  uintptr_t base_addr;
  mpcomp_task_dep_node_t *last_out;
  mpcomp_task_dep_node_list_t *last_in;
  struct mpcomp_task_dep_ht_entry_s *next;
} mpcomp_task_dep_ht_entry_t;

typedef struct mpcomp_task_dep_ht_bucket_s {
  int num_entries;
  uint64_t base_addr;
  bool redundant_out;
  uint64_t *base_addr_list;
  mpcomp_task_dep_type_t type;
  mpcomp_task_dep_ht_entry_t *entry;
} mpcomp_task_dep_ht_bucket_t;

typedef struct mpcomp_task_dep_ht_table_s {
  uint32_t hsize;
  uint32_t hseed;
  sctk_spinlock_t hlock;
  mpcomp_task_dep_hash_func_t hfunc;
  struct mpcomp_task_dep_ht_bucket_s *buckets;
} mpcomp_task_dep_ht_table_t;

typedef struct mpcomp_task_dep_task_infos_s {
  mpcomp_task_dep_node_t *node;
  struct mpcomp_task_dep_ht_table_s *htable;
} mpcomp_task_dep_task_infos_t;

static inline int mpcomp_task_dep_is_flag_with_deps(const unsigned flags) {
  return flags & MPCOMP_TASK_DEP_GOMP_DEPS_FLAG;
}

/** HASHING INTEL */
static inline uint32_t
mpcomp_task_dep_intel_hash_func(uintptr_t addr, uint32_t size, uint32_t seed) {
  return ((addr >> seed) ^ addr) % size;
}

/** HASHING MPC */
static inline uint32_t
mpcomp_task_dep_mpc_hash_func(uintptr_t addr, uint32_t size, uint32_t seed) {
  return (addr >> seed) % size;
}

static inline mpcomp_task_dep_node_t *mpcomp_task_dep_new_node(void) {
  mpcomp_task_dep_node_t *new_node;
  new_node = sctk_malloc(sizeof(mpcomp_task_dep_node_t));
  sctk_assert(new_node);
  memset(new_node, 0, sizeof(mpcomp_task_dep_node_t));
  sctk_atomics_store_int(&(new_node->ref_counter), 1);
  return new_node;
}

static inline mpcomp_task_dep_node_t *
mpcomp_task_dep_node_ref(mpcomp_task_dep_node_t *node) {
  sctk_assert(node);
  const int prev = sctk_atomics_fetch_and_incr_int(&(node->ref_counter));
  return node;
}

static inline int mpcomp_task_dep_node_unref(mpcomp_task_dep_node_t *node) {

  if (!node)
    return 0;

  sctk_assert(sctk_atomics_load_int(&(node->ref_counter)));
  /* Fetch and decr to prevent double free */
  const int prev = sctk_atomics_fetch_and_decr_int(&(node->ref_counter)) - 1;
  if (!prev) {
    sctk_assert(!sctk_atomics_load_int(&(node->ref_counter)));
    sctk_free(node);
    node = NULL;
    return 1;
  }
  return 0;
}

static inline void
mpcomp_task_dep_free_node_list_elt(mpcomp_task_dep_node_list_t *list) {
  mpcomp_task_dep_node_list_t *node_list;

  if (!list)
    return;

  while ((node_list = list)) {
    mpcomp_task_dep_node_unref(node_list->node);
    list = node_list->next;
    sctk_free(node_list);
  }
}

static inline mpcomp_task_dep_node_list_t *
mpcomp_task_dep_alloc_node_list_elt(mpcomp_task_dep_node_list_t *list,
                                    mpcomp_task_dep_node_t *node) {
  mpcomp_task_dep_node_list_t *new_node;

  sctk_assert(node);
  //sctk_assert(list);

  new_node = sctk_malloc(sizeof(mpcomp_task_dep_node_list_t));
  sctk_assert(new_node);

  new_node->node = mpcomp_task_dep_node_ref(node);
  new_node->next = list;

  return new_node;
}

static inline int
mpcomp_task_dep_free_task_htable(mpcomp_task_dep_ht_table_t *htable) {
  unsigned int i;
  int removed_entries = 0;

  for (i = 0; htable && i < htable->hsize; i++) {
    if (htable[i].buckets->entry) {
      mpcomp_task_dep_ht_entry_t *entry;
      while ((entry = htable[i].buckets->entry)) {
        mpcomp_task_dep_free_node_list_elt(entry->last_in);
        mpcomp_task_dep_node_unref(entry->last_out);
        htable[i].buckets->entry = entry;
        sctk_free(entry);
      }
    }
  }
  return removed_entries;
}

/**
 *
 */
static inline mpcomp_task_dep_ht_table_t *
mpcomp_task_dep_alloc_task_htable(mpcomp_task_dep_hash_func_t hfunc) {
  mpcomp_task_dep_ht_table_t *new_htable;

  sctk_assert(hfunc);
  const long infos_size = mpcomp_task_align_single_malloc(
      sizeof(mpcomp_task_dep_ht_table_t), MPCOMP_TASK_DEFAULT_ALIGN);
  const long array_size =
      sizeof(mpcomp_task_dep_ht_bucket_t) * MPCOMP_TASK_DEP_MPC_HTABLE_SIZE;
  sctk_assert(MPCOMP_OVERFLOW_SANITY_CHECK((unsigned long)infos_size, (unsigned long)array_size));

  new_htable =
      (mpcomp_task_dep_ht_table_t *)sctk_malloc(infos_size + array_size);
  sctk_assert(new_htable);

  /* Better than a loop */
  memset(new_htable, 0, infos_size + array_size);

  new_htable->hsize = MPCOMP_TASK_DEP_MPC_HTABLE_SIZE;
  new_htable->hseed = MPCOMP_TASK_DEP_MPC_HTABLE_SEED;
  new_htable->hfunc = hfunc;

  new_htable->buckets =
      (mpcomp_task_dep_ht_bucket_t *)((uintptr_t)new_htable + infos_size);

  return new_htable;
}

static inline mpcomp_task_dep_ht_entry_t *
mpcomp_task_dep_add_entry(mpcomp_task_dep_ht_table_t *htable, uintptr_t addr) {
  mpcomp_task_dep_ht_entry_t *entry;
  const uint32_t hash = htable->hfunc(addr, htable->hsize, htable->hseed);

  for (entry = htable->buckets[hash].entry; entry; entry = entry->next) {
    if (entry->base_addr == addr) {
      break;
    }
  }

  if (entry) {
    return entry;
  }

  /* Allocation */
  entry = (mpcomp_task_dep_ht_entry_t *)sctk_malloc(
      sizeof(mpcomp_task_dep_ht_entry_t));
  sctk_assert(entry);

  memset(entry, 0, sizeof(mpcomp_task_dep_ht_entry_t));
  entry->base_addr = addr;

  /* No previous addr in bucket */
  if (htable->buckets[hash].num_entries > 0) {
    entry->next = htable->buckets[hash].entry;
  }

  htable->buckets[hash].entry = entry;
  htable->buckets[hash].num_entries += 1;

  return entry;
}

static inline mpcomp_task_dep_ht_entry_t *
mpcomp_task_dep_ht_find_entry(mpcomp_task_dep_ht_table_t *htable,
                              uintptr_t addr) {
  mpcomp_task_dep_ht_entry_t *entry = NULL;
  const uint32_t hash = htable->hfunc(addr, htable->hsize, htable->hseed);

  /* Search correct entry in bucket */
  if (htable->buckets[hash].num_entries > 0) {
    for (entry = htable->buckets[hash].entry; entry; entry = entry->next) {
      if (entry->base_addr == addr)
        break;
    }
  }

  return entry;
}

void mpcomp_task_with_deps(void (*fn)(void *), void *data,
                           void (*cpyfn)(void *, void *), long arg_size,
                           long arg_align, bool if_clause, unsigned flags,
                           void **depend, bool intel_alloc, mpcomp_task_t *intel_task);

void __mpcomp_task_finalize_deps(mpcomp_task_t *task);

#endif /* __MPCOMP_TASK_DEP_H__ */
#endif /* MPCOMP_USE_TASKDEP */
