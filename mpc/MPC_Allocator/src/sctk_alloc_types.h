/* ############################# MPC License ############################## */
/* # Wed Nov 19 15:19:19 CET 2008                                         # */
/* # Copyright or (C) or Copr. Commissariat a l'Energie Atomique          # */
/* #                                                                      # */
/* # IDDN.FR.001.230040.000.S.P.2007.000.10000                            # */
/* # This file is part of the MPC Runtime.                                # */
/* #                                                                      # */
/* # This software is governed by the CeCILL-C license under French law   # */
/* # and abiding by the rules of distribution of free software.  You can  # */
/* # use, modify and/ or redistribute the software under the terms of     # */
/* # the CeCILL-C license as circulated by CEA, CNRS and INRIA at the     # */
/* # following URL http://www.cecill.info.                                # */
/* #                                                                      # */
/* # The fact that you are presently reading this means that you have     # */
/* # had knowledge of the CeCILL-C license and that you accept its        # */
/* # terms.                                                               # */
/* #                                                                      # */
/* # Authors:                                                             # */
/* #   - PERACHE Marc marc.perache@cea.fr                                 # */
/* #                                                                      # */
/* ######################################################################## */
#ifndef __SCTK__ALLOC__TYPES__
#define __SCTK__ALLOC__TYPES__

#ifdef __cplusplus
extern "C"
{
#endif
#ifndef __SCTK__ALLOC__C__
#error "For internal use only"
#endif

#include <stdlib.h>
#include "sctk_alloc_options.h"

#define sctk_inuse_state 0
#define sctk_free_state 1
#define sctk_disable_consolidate_state 2
#define sctk_free_state_use 3
  typedef char sctk_chunk_state_t;

#define SCTK_ADD_CASE(a) case (a): return (SCTK_STRING(a))
  static mpc_inline char *sctk_get_char_state (sctk_chunk_state_t s)
  {
    switch (s)
      {
	SCTK_ADD_CASE (sctk_inuse_state);
	SCTK_ADD_CASE (sctk_free_state);
	SCTK_ADD_CASE (sctk_disable_consolidate_state);
	SCTK_ADD_CASE (sctk_free_state_use);
	default:not_reachable ();
      }
    return "";
  }

#define sctk_chunk_small_type 0
#define sctk_chunk_big_type 1
#define sctk_chunk_new_type 2
#define sctk_chunk_page_type 3
#define sctk_chunk_buffered_type 4
#define sctk_chunk_memalign_type 5
  typedef char sctk_chunk_type_t;

  struct sctk_tls_s;


#ifdef SCTK_VIEW_FACTIONNEMENT
#define SCTK_MALLOC_STRUCT_PART()		\
  sctk_size_t cur_size;				\
    sctk_size_t real_size_frac;			\
    sctk_chunk_state_t state;			\
    sctk_chunk_type_t type;			\
    unsigned short small_pos;			\
    unsigned int tree

#else
#define SCTK_MALLOC_STRUCT_PART()		\
  sctk_size_t cur_size;				\
    sctk_chunk_state_t state;			\
    sctk_chunk_type_t type;			\
    unsigned short small_pos;			\
    unsigned int tree

#endif


  typedef struct
  {
    SCTK_MALLOC_STRUCT_PART ();
  } sctk_malloc_chunk_t;

#ifdef SCTK_VIEW_FACTIONNEMENT

  static size_t total_memory_allocated = 0;
  static size_t total_memory_used = 0;
  static size_t total_nb_block = 0;
  static sctk_alloc_spinlock_t frac_lock = SCTK_ALLOC_SPINLOCK_INITIALIZER;
  static int sctk_stats_status = 0;
  static char *(*mem_watchpoint) (size_t mem_alloc,
				  size_t mem_used,
				  size_t nb_block,
				  size_t size,
				  sctk_alloc_type_t op_type) = NULL;

  static inline void sctk_mem_watchpoint (size_t mem_alloc,
					  size_t mem_used,
					  size_t nb_block, size_t size, int t)
  {
    char *tmp = NULL;
    if (sctk_stats_status == 1)
      {
	if (mem_watchpoint != NULL)
	  {
	    if (t == 0)
	      tmp =
		mem_watchpoint (mem_alloc, mem_used, nb_block, size,
				mpc_alloc_type_alloc_block);
	    if (t == 1)
	      tmp =
		mem_watchpoint (mem_alloc, mem_used, nb_block, size,
				mpc_alloc_type_dealloc_block);
	    if (t == 2)
	      tmp =
		mem_watchpoint (mem_alloc, mem_used, nb_block, size,
				mpc_alloc_type_alloc_mem);
	    if (t == 3)
	      tmp =
		mem_watchpoint (mem_alloc, mem_used, nb_block, size,
				mpc_alloc_type_dealloc_mem);
	  }
	if (tmp != NULL)
	  sctk_debug_print_backtrace (tmp);
      }
  }

  static inline void sctk_add_block (size_t size, sctk_malloc_chunk_t * chunk)
  {
    size_t mem_alloc;
    size_t mem_used;
    size_t nb_block;

    chunk->real_size_frac = size;
    sctk_alloc_spinlock_lock (&frac_lock);
    total_memory_used += size;
    total_nb_block++;

    mem_alloc = total_memory_allocated;
    mem_used = total_memory_used;
    nb_block = total_nb_block;
    sctk_alloc_spinlock_unlock (&frac_lock);
    sctk_mem_watchpoint (mem_alloc, mem_used, nb_block, size, 0);
  }

  static inline void sctk_del_block (size_t size)
  {
    size_t mem_alloc;
    size_t mem_used;
    size_t nb_block;

    sctk_alloc_spinlock_lock (&frac_lock);
    total_memory_used -= size;
    total_nb_block--;

    mem_alloc = total_memory_allocated;
    mem_used = total_memory_used;
    nb_block = total_nb_block;
    sctk_alloc_spinlock_unlock (&frac_lock);
    sctk_mem_watchpoint (mem_alloc, mem_used, nb_block, size, 1);
  }

  static inline void sctk_add_mem (size_t size)
  {
    size_t mem_alloc;
    size_t mem_used;
    size_t nb_block;

    sctk_alloc_spinlock_lock (&frac_lock);
    total_memory_allocated += size;

    mem_alloc = total_memory_allocated;
    mem_used = total_memory_used;
    nb_block = total_nb_block;
    sctk_alloc_spinlock_unlock (&frac_lock);
    sctk_mem_watchpoint (mem_alloc, mem_used, nb_block, size, 2);
  }

  static inline void sctk_del_mem (size_t size)
  {
    size_t mem_alloc;
    size_t mem_used;
    size_t nb_block;

    sctk_alloc_spinlock_lock (&frac_lock);
    total_memory_allocated -= size;

    mem_alloc = total_memory_allocated;
    mem_used = total_memory_used;
    nb_block = total_nb_block;
    sctk_alloc_spinlock_unlock (&frac_lock);
    sctk_mem_watchpoint (mem_alloc, mem_used, nb_block, size, 3);
  }
#else

#define sctk_add_block(a,b) (void)(0)
#define sctk_del_block(a) (void)(0)
#define sctk_add_mem(a) (void)(0)
#define sctk_del_mem(a) (void)(0)

#endif

  struct sctk_free_chunk_s;
  typedef struct sctk_free_chunk_s
  {
    SCTK_MALLOC_STRUCT_PART ();
    struct sctk_free_chunk_s *prev_chunk;
    struct sctk_free_chunk_s *next_chunk;
  } sctk_free_chunk_t;

  typedef struct sctk_page_s
  {
    SCTK_MALLOC_STRUCT_PART ();
    struct sctk_page_s *prev_chunk;
    struct sctk_page_s *next_chunk;
    struct sctk_tls_s *related;
    sctk_size_t real_size;
    sctk_chunk_state_t page_state;
    struct sctk_page_s *prev_chunk_used;
    struct sctk_page_s *next_chunk_used;
    struct sctk_page_s *init_page;
    unsigned int nb_ref;
    unsigned long dirty;
    unsigned long cur_dirty;
    int is_remote_page;

  } sctk_page_t;

  typedef struct sctk_static_var_s
  {
    void *adr;
    sctk_size_t size;
    struct sctk_static_var_s *next;
  } sctk_static_var_t;

  struct sctk_tls_s
  {
    sctk_free_chunk_t *chunks[max_chunk_sizes_max];
    sctk_page_t *last_small;
    sctk_page_t *last_small2;

    sctk_page_t *big_empty_pages_xMO[SCTK_xMO_TAB_SIZE];

    sctk_page_t *used_pages;
    unsigned long used_pages_number;

    sctk_static_var_t *static_vars;
    unsigned long static_vars_number;
    struct sctk_tls_s *next;

    int full_page_number;
    sctk_mutex_t lock;

    volatile sctk_free_chunk_t *chunks_distant;
    volatile sctk_free_chunk_t *big_chunks_distant;

    sctk_spinlock_t spinlock;
  };


  struct sctk_alloc_block_s
  {
    struct sctk_alloc_block_s *next;
    void *ptr;
    sctk_alloc_buffer_t *buf;
  };
#ifdef __cplusplus
}				/* end of extern "C" */
#endif
#endif
