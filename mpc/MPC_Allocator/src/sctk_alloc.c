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
#define __SCTK__ALLOC__C__
#define SCTK_SKIP_SIZE ((unsigned long)1024 * 1024 * 1024)
#define SCTK_BOTTOM_SKIP ((unsigned long)0x3ce00000)
#define SCTK_TOP_SKIP (SCTK_BOTTOM_SKIP + SCTK_SKIP_SIZE)

#include "sctk_config.h"
#include "sctk_alloc_api.h"
#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "sctk.h"
#include "sctk_io_helper.h"
#include <sys/mman.h>
#include "mpcalloc.h"
#include "sctk.h"
#include <sched.h>
#include "sctk_dummy.h"

#ifdef MPC_Message_Passing
#include "sctk_low_level_comm.h"
#endif

#ifdef MPC_Threads
#include "sctk_thread.h"
#endif

#ifdef SCTK_VIEW_FACTIONNEMENT
#warning "Fractionnement detection enable"
#endif

#include "sctk_debug.h"

#include "sctk_alloc_debug.h"
#include "sctk_alloc_options.h"
#include "sctk_alloc.h"
#include "sctk_alloc_thread.h"
#include "sctk_tabs.h"
#include "sctk_alloc_types.h"
#include "sctk_alloc_debug.h"
#include "sctk_alloc_list.h"
#include "sctk_alloc_meminfo.h"

#ifdef MPC_Threads
#include "sctk_topology.h"
#endif

/* #ifdef MPC_Message_Passing */
/* #include "sctk_low_level_comm.h" */
/* #endif */

/*Page Migration*/
#ifdef MPC_USE_PAGE_MIGRATION
#include <numaif.h>

#ifndef MPOL_MF_MOVE
#define MPOL_MF_MOVE    (1<<1)
#endif

#ifndef MPOL_MF_MOVE_ALL
#define MPOL_MF_MOVE_ALL    (1<<2)
#endif

#endif

#define SCTK_USE_COMPRESSION_FILE

/* #define SCTK_USE_BRK_INSTED_MMAP */
#ifdef SCTK_COHERENCY_CHECK
static sctk_size_t sctk_coherency_check_extend_size (sctk_size_t size);
static void *sctk_coherency_check_prepare_chunk (void *tmp, sctk_size_t size);
static void *sctk_coherency_check_verif_chunk (void *tmp);
#endif
/*******************************************/
/*************** variables     *************/
/*******************************************/
int SCTK_FULL_PAGE_NUMBER = SCTK_FULL_PAGE_NUMBER_DEFAULT;

static char *global_address_start = NULL;
static char *global_address_end = (char *) -1;

 /*BRK*/
static char *init_pre_brk_pointer = NULL;
static char *max_pre_brk_pointer = NULL;

static volatile char *brk_pointer = NULL;
static char *init_brk_pointer = NULL;
static char *max_brk_pointer = NULL;
static char *first_free = NULL;

/* static sctk_alloc_spinlock_t sctk_sbrk_lock = SCTK_ALLOC_SPINLOCK_INITIALIZER; */
static sctk_spin_rwlock_t sctk_sbrk_rwlock = SCTK_SPIN_RWLOCK_INITIALIZER;

static sctk_size_t page_size = 0;

static volatile int sctk_no_alloc_land = 0;

/* TLS reuse */
static sctk_spinlock_t old_tls_list_spinlock = SCTK_SPINLOCK_INITIALIZER;
static volatile sctk_tls_t *old_tls_list = NULL;

static mpc_inline sctk_page_t *sctk_init_page (sctk_page_t * tmp,
					       sctk_size_t size);

static void *sctk_real_mmap (void *start, size_t length, int prot, int flags,
			     int fd, off_t offset);
static int sctk_real_munmap (void *start, size_t length);

static mpc_inline void *
sctk_mmap (void *ptr, sctk_size_t size)
{
  void *tmp;
  SCTK_TRACE_START (sctk_mmap, ptr, size, NULL, NULL);
  SCTK_DEBUG (sctk_mem_error
	      ("PREPARE MMAP %p-%p %lu\n", ptr, (char *) ptr + size, size));
  tmp = sctk_real_mmap ((void *) (ptr), (size),
	      PROT_READ | PROT_WRITE,
	      MAP_PRIVATE | MAP_FIXED | MAP_ANONYMOUS, -1, 0);
  if (ptr != tmp)
    {
      perror ("MMAP FAIL");
    }
  assume (ptr == tmp);
  SCTK_DEBUG (memset (ptr, 1, size));
  SCTK_DEBUG (sctk_mem_error
	      ("MMAP %p-%p %lu\n", ptr, (char *) ptr + size, size));
  SCTK_TRACE_END (sctk_mmap, tmp, size, NULL, NULL);
  return tmp;
}
static mpc_inline int
sctk_munmap (void *start, size_t length)
{
  int tmp;
  tmp = sctk_real_munmap (start, length);
  return tmp;
}

char *
sctk_alloc_mode (void)
{
  return "MPC allocator";
}

static mpc_inline void
sctk_delete_not_migrable_page (sctk_page_t * cursor)
{
  SCTK_DEBUG (sctk_mem_error
	      ("Unmap not migrable %p-%p\n",
	       ((char *) cursor + page_size),
	       (char *) cursor + cursor->real_size));
  sctk_munmap ((void *) cursor, cursor->real_size);
}

static mpc_inline void
sctk_empty_free_page (sctk_page_t * cursor)
{
  SCTK_TRACE_START (sctk_empty_free_page, cursor, NULL, NULL, NULL);

#ifndef SCTK_DO_NOT_FREE
  sctk_munmap ((void *) ((char *) cursor + page_size),
	       cursor->real_size - page_size);
#endif
  SCTK_DEBUG (sctk_mem_error
	      ("Unmap %p-%p\n", ((char *) cursor + page_size),
	       (char *) cursor + cursor->real_size));
  SCTK_TRACE_END (sctk_empty_free_page, cursor, NULL, NULL, NULL);
}

static mpc_inline void
sctk_realloc_free_page (sctk_page_t * cursor)
{
  void *tmp;
  sctk_size_t return_size;
  SCTK_TRACE_START (sctk_realloc_free_page, cursor, NULL, NULL, NULL);

  return_size = cursor->real_size;

/*   sctk_munmap ((void *) cursor, cursor->real_size); */

#ifndef SCTK_DO_NOT_FREE
  tmp = sctk_mmap (cursor, return_size);
  sctk_mem_assert (tmp == cursor);
#endif

  cursor->real_size = return_size;
  SCTK_DEBUG (sctk_mem_error
	      ("Remap %p-%p\n", ((char *) cursor),
	       (char *) cursor + cursor->real_size));
  cursor->dirty = 0;
  SCTK_TRACE_END (sctk_realloc_free_page, cursor, NULL, NULL, NULL);
}

#if 1
#define sctk_refresh_free_page(cursor) (void)(0)
#else
static mpc_inline void
sctk_refresh_free_page (sctk_page_t * cursor)
{
  void *tmp;
  char *tmp_cur;
  sctk_size_t return_size;

  return_size = cursor->real_size - page_size;

  tmp_cur = ((char *) cursor + page_size);
  SCTK_DEBUG (sctk_mem_error
	      ("TRY Refresh %p-%p (%p-%p)\n", ((char *) cursor),
	       (char *) cursor + cursor->real_size, tmp_cur,
	       tmp_cur + return_size));

  sctk_munmap ((void *) tmp_cur, return_size);
  tmp = sctk_mmap (tmp_cur, return_size);
  sctk_mem_assert (tmp == tmp_cur);
  SCTK_DEBUG (sctk_mem_error
	      ("Refresh %p-%p (%p-%p)\n", ((char *) cursor),
	       (char *) cursor + cursor->real_size, tmp_cur,
	       tmp_cur + return_size));
}
#endif

static mpc_inline void
sctk_refresh_free_page_force (sctk_page_t * cursor)
{
  if (cursor->dirty >= SCTK_PAGES_ALLOC_SIZE * 4)
    {
      void *tmp;
      char *tmp_cur;
      sctk_size_t return_size;

      return_size = cursor->real_size - page_size;

      tmp_cur = ((char *) cursor + page_size);
      SCTK_DEBUG (sctk_mem_error
		  ("TRY Refresh %p-%p (%p-%p)\n", ((char *) cursor),
		   (char *) cursor + cursor->real_size, tmp_cur,
		   tmp_cur + return_size));

#ifndef SCTK_DO_NOT_FREE
      sctk_munmap ((void *) tmp_cur, return_size);
      tmp = sctk_mmap (tmp_cur, return_size);
      sctk_mem_assert (tmp == tmp_cur);
#endif

      SCTK_DEBUG (sctk_mem_error
		  ("Refresh %p-%p (%p-%p)\n", ((char *) cursor),
		   (char *) cursor + cursor->real_size, tmp_cur,
		   tmp_cur + return_size));
      cursor->dirty = 0;
    }
}


#define sctk_compute_real_size(init_size) \
  (SCTK_PAGES_ALLOC_SIZE + (((init_size)-1) & (~(SCTK_PAGES_ALLOC_SIZE-1))))

#define sctk_compute_real_size_big(init_size) \
  (SCTK_xMO_TAB_SIZE*SCTK_PAGES_ALLOC_SIZE + (((init_size)-1) & (~((SCTK_xMO_TAB_SIZE*SCTK_PAGES_ALLOC_SIZE)-1))))

static unsigned long
sctk_small_page_align (sctk_free_chunk_t * chunk)
{
  sctk_page_t *page;
  sctk_size_t size;


  size = sctk_compute_real_size (SCTK_SMALL_PAGES_ALLOC_SIZE);

  sctk_mem_assert (size % SCTK_PAGES_ALLOC_SIZE == 0);

  page = (sctk_page_t *) ((unsigned long) chunk & (~(size - 1)));

  SCTK_DEBUG (sctk_mem_error
	      ("%010p page %010p chunk %010p\n", size, page, chunk));
  return (unsigned long) page;
}

static unsigned long
sctk_big_page_align (sctk_page_t * chunk)
{
  return (unsigned long) (chunk->init_page);
}

/*******************************************/
/***************  PROTOTYPES   *************/
/*******************************************/
static mpc_inline sctk_page_t *sctk_sbrk_compaction (sctk_size_t size);
static sctk_size_t sctk_sbrk (sctk_size_t init_size, void **ptr);
static sctk_page_t *sctk_alloc_page_sbrk (sctk_size_t size, sctk_tls_t * tls);

static sctk_page_t *sctk_alloc_page_big (sctk_size_t size, sctk_tls_t * tls);
static void sctk_free_page_big (sctk_page_t * page, sctk_tls_t * tls);
static void *sctk_alloc_ptr_big (sctk_size_t size, sctk_tls_t * tls);
static void
sctk_free_big (sctk_free_chunk_t * chunk, sctk_size_t size,
	       sctk_tls_t * my_tls);

static sctk_page_t *sctk_alloc_page_small (sctk_size_t size,
					   sctk_tls_t * tls);
static void *sctk_alloc_ptr_small (sctk_size_t size, sctk_tls_t * tls);
static void
sctk_free_small (sctk_free_chunk_t * chunk, sctk_size_t size,
		 sctk_tls_t * my_tls);
static void sctk_free_chunk_small (sctk_free_chunk_t * ptr, sctk_tls_t * tls);


static void sctk_alloc_page_big_flush (sctk_tls_t * tls);
/*******************************************/
/***************    INIT       *************/
/*******************************************/
static int sctk_alloc_init_done = 0;
static volatile int sctk_alloc_todo_init_tls = 0;

static int
sctk_alloc_check_heap (char *ptr)
{
  sctk_size_t size;
  void *tmp;
  int res;

  size = SCTK_xMO_TAB_SIZE * SCTK_PAGES_ALLOC_SIZE;
  assume (size > 0);

  tmp = sctk_real_mmap ((void *) (ptr), (size),
	      PROT_READ | PROT_WRITE,
	      MAP_PRIVATE | MAP_FIXED | MAP_ANONYMOUS, -1, 0);
  if (tmp == ptr)
    {
      res = 1;
      sctk_real_munmap (tmp, size);
    }
  else
    {
/*     sctk_noalloc_fprintf(stderr,"Disable thread migration\n");  */
      res = 0;
    }
  return res;
}

static void
sctk_init_memory_allocation ()
{
  static int done = 0;

  LOG_IN ();

  if (sctk_alloc_todo_init_tls == 1)
    {
      sctk_init_tls_from_thread ();
      sctk_alloc_init_done = 1;
      SCTK_DEBUG (sctk_mem_error ("KEY %d\n", sctk_tls_key));
    }
  else
    {
      sctk_mem_assert (done == 0);
      sctk_alloc_init_done = 1;


      sctk_mem_assert (max_chunk_sizes_max >= max_chunk_sizes);
      done = 1;
      sctk_sizeof_sctk_page_t = sctk_sizeof (sctk_page_t);

      sctk_sizeof_sctk_malloc_chunk_t = sctk_sizeof (sctk_malloc_chunk_t);
      SCTK_DEBUG (sctk_mem_error ("sctk_sizeof_sctk_malloc_chunk_t %lu\n",
				  sctk_sizeof (sctk_malloc_chunk_t)));
      SCTK_DEBUG (sctk_mem_error ("sizeof(sctk_malloc_chunk_t) %lu\n",
				  sizeof (sctk_malloc_chunk_t)));
      SCTK_DEBUG (sctk_mem_error
		  ("%lu %lu\n", sctk_sizeof (char), sctk_sizeof (int)));
      sctk_mem_assert (sctk_sizeof (sctk_malloc_chunk_t) ==
		       sctk_sizeof_sctk_malloc_chunk_t);

      page_size = getpagesize ();
      sctk_mem_assert (page_size < SCTK_RELOCALISE_BUFFER_SIZE);

      SCTK_DEBUG (sctk_mem_error
		  ("%d >= %d\n", chunk_sizes[0],
		   (sctk_sizeof (sctk_free_chunk_t) -
		    sctk_sizeof (sctk_malloc_chunk_t))));
      sctk_mem_assert (chunk_sizes[0] >=
		       (sctk_sizeof (sctk_free_chunk_t) -
			sctk_sizeof (sctk_malloc_chunk_t)));

      sctk_mem_assert (sctk_sizeof (sctk_malloc_chunk_t) +
		       SCTK_SPLIT_THRESHOLD >=
		       sctk_sizeof (sctk_free_chunk_t));

      sctk_mem_assert (SCTK_SPLIT_THRESHOLD >= chunk_sizes[0]);

      sctk_init_tls_from_thread ();
      sctk_alloc_todo_init_tls = 1;

      /*   page_size = 1000*page_size; */

#ifdef MPC_Message_Passing
#ifdef SCTK_64_BIT_ARCH
      sctk_set_net_migration_available (1);
#else
      sctk_set_net_migration_available (0);
#endif
#endif

      brk_pointer = sbrk (0);
#ifdef SCTK_x86_64_ARCH_SCTK
      if ((unsigned long) brk_pointer < SCTK_TOP_SKIP)
	{
	  brk_pointer = (char *) SCTK_TOP_SKIP;
	}
#endif
      init_brk_pointer = (char *) brk_pointer;

      init_brk_pointer =
	(char *) sctk_small_page_align ((sctk_free_chunk_t *) brk_pointer);
      if (((unsigned long) init_brk_pointer) < ((unsigned long) brk_pointer))
	{
	  init_brk_pointer +=
	    sctk_compute_real_size (SCTK_SMALL_PAGES_ALLOC_SIZE);
	}
      sctk_mem_assert (((unsigned long) init_brk_pointer) >=
		       ((unsigned long) brk_pointer));

      SCTK_DEBUG (sctk_mem_error ("init_brk %p, brk %p\n",
				  init_brk_pointer, brk_pointer));

      brk_pointer = (char *) init_brk_pointer;
      init_brk_pointer = (char *) brk_pointer;
      first_free = (char *) brk_pointer;
      init_pre_brk_pointer = init_brk_pointer;

      SCTK_DEBUG (sctk_mem_error
		  ("init_brk %p, brk %p\n", init_brk_pointer, brk_pointer));

#ifdef SCTK_i686_ARCH_SCTK
      max_brk_pointer = (char *) 0xc0000000;
#else
      max_brk_pointer = (char *) ((unsigned long) -1);
#endif

      /*   sctk_exception_catch_with (11, sctk_exception_handler); */
      /*   sctk_exception_catch_with (SIGFPE, sctk_exception_handler); */

#if defined(SCTK_ALLOC_INFOS) || 0
      sctk_mem_error ("INIT BRK %p\n", init_brk_pointer);
      sctk_mem_error ("BRK %p\n", brk_pointer);
      sctk_mem_error ("MAX BRK %p\n", max_brk_pointer);
      sctk_mem_error ("Page Size %luKo\n", page_size / 1024);
      sctk_mem_error ("MALLOC SIZE %lu\n", sctk_sizeof_sctk_malloc_chunk_t);
      sctk_mem_error ("PAGE SIZE %lu\n", sctk_sizeof_sctk_page_t);
      sctk_mem_error ("sizeof(sctk_chunk_state_t) %lu\n",
		      sizeof (sctk_chunk_state_t));
      sctk_mem_error ("sizeof(sctk_chunk_type_t) %lu\n",
		      sizeof (sctk_chunk_type_t));
      sctk_mem_error ("sizeof(sctk_malloc_chunk_t) %lu\n",
		      sizeof (sctk_malloc_chunk_t));
      sctk_mem_error ("sizeof(sctk_free_chunk_t) %lu\n",
		      sizeof (sctk_free_chunk_t));
      sctk_mem_error ("sizeof(sctk_page_t) %lu\n", sizeof (sctk_page_t));
      sctk_mem_error ("sizeof(struct sctk_tls_s) %lu\n",
		      sizeof (struct sctk_tls_s));
      sctk_mem_error ("sizeof(struct sctk_alloc_block_s) %lu\n",
		      sizeof (struct sctk_alloc_block_s));
      sctk_mem_error ("SKIP %p - %p\n", (void *) SCTK_BOTTOM_SKIP,
		      (void *) SCTK_TOP_SKIP);
#endif
    }
#ifdef MPC_Threads
  if (sctk_multithreading_initialised == 0)
    {
      sctk_alloc_todo_init_tls = 1;
      sctk_alloc_init_done = 0;
    }
#endif
  SCTK_DEBUG (sctk_mem_error
	      ("sctk_alloc_todo_init_tls %d sctk_alloc_init_done %d\n",
	       sctk_alloc_todo_init_tls, sctk_alloc_init_done));
  LOG_OUT ();
}

/*******************************************/
/***************    PAGES      *************/
/*******************************************/

static mpc_inline sctk_page_t *
sctk_init_page (sctk_page_t * tmp, sctk_size_t size)
{
  sctk_free_chunk_t *chunk;


  tmp->cur_size = size;
  tmp->related = NULL;
  tmp->type = sctk_chunk_page_type;
  tmp->prev_chunk = tmp;
  tmp->next_chunk = tmp;
  tmp->real_size = size;
  tmp->page_state = sctk_inuse_state;
  tmp->state = sctk_inuse_state;
  tmp->is_remote_page = 0;

  chunk = sctk_get_chunk_from_page (tmp);

  sctk_chunk_init ((sctk_malloc_chunk_t *) chunk, size -
		   (sctk_sizeof_sctk_page_t +
		    sctk_sizeof_sctk_malloc_chunk_t));
  chunk->state = sctk_disable_consolidate_state;
  chunk->state = sctk_free_state;
  return tmp;
}

/* static mpc_inline sctk_page_t * */
/* sctk_init_remote_page (sctk_page_t * tmp, sctk_size_t size) */
/* { */
/*   tmp->is_remote_page = 1; */
/*   return tmp; */
/* } */

static sctk_page_t *
sctk_create_page (sctk_size_t init_size)
{
  sctk_size_t size;
  void *tmp;
  size = init_size;
  size += sctk_sizeof_sctk_page_t + sctk_sizeof_sctk_malloc_chunk_t;
  size = sctk_sbrk (size, &tmp);
  if (tmp == NULL)
    {
#ifdef MPC_Threads
      sctk_thread_yield ();
#else
      sched_yield ();
#endif
      size = init_size;
      size += sctk_sizeof_sctk_page_t + sctk_sizeof_sctk_malloc_chunk_t;
      size = sctk_sbrk (size, &tmp);
    }

  if (tmp == NULL)
    return NULL;

  tmp = sctk_init_page ((sctk_page_t *) tmp, size);
  ((sctk_page_t *) tmp)->dirty = 0;
  ((sctk_page_t *) tmp)->init_page = ((sctk_page_t *) tmp);
  if (((sctk_page_t *) tmp)->real_size !=
      (SCTK_xMO_TAB_SIZE * SCTK_PAGES_ALLOC_SIZE))
    {
      ((sctk_page_t *) tmp)->init_page = NULL;
    }
  return (sctk_page_t *) tmp;
}



/*******************************************/
/***************    TLS        *************/
/*******************************************/
static sctk_tls_t *
sctk_create_tls ()
{
  sctk_tls_t *tls;


#ifdef SCTK_TRACE_ALLOC_BEHAVIOR
  {
    static int nb_tls = 0;
    nb_tls++;
    sctk_mem_error ("\n[ALLOC TRACE %12d %12p] New TLS\n",
		    getpid (), sctk_get_tls_from_thread ());
  }
#endif

  sctk_spinlock_lock (&old_tls_list_spinlock);
  if (old_tls_list != NULL)
    {
      tls = (sctk_tls_t *) old_tls_list;
      old_tls_list = tls->next;
#ifdef MPC_Threads
      __sctk_relocalise_tls (tls);
#endif
    }
  else
    {
      int i;
      sctk_page_t *page;
      sctk_malloc_chunk_t *chunk;
      sctk_free_chunk_t *next;
      sctk_mutex_t lock = SCTK_MUTEX_INITIALIZER;
      page = sctk_create_page (SCTK_SMALL_PAGES_ALLOC_SIZE);

      /*allocation de la page */
      chunk = (sctk_malloc_chunk_t *) sctk_get_chunk_from_page (page);

      sctk_chunk_split (chunk, sctk_sizeof (sctk_tls_t));
      chunk->state = sctk_disable_consolidate_state;

      tls = (sctk_tls_t *) sctk_get_ptr (chunk);
      SCTK_MAKE_WRITABLE (tls, sizeof (sctk_tls_t));
      page->related = tls;

      for (i = 0; i < max_chunk_sizes_max; i++)
	{
	  tls->chunks[i] = NULL;
	}

      tls->chunks_distant = NULL;
      tls->big_chunks_distant = NULL;
      tls->used_pages = NULL;
      tls->used_pages_number = 0;
      tls->static_vars = NULL;
      tls->static_vars_number = 0;
      tls->full_page_number = 0;
      tls->last_small = NULL;
      tls->last_small2 = NULL;


      for (i = 0; i < SCTK_xMO_TAB_SIZE; i++)
	{
	  tls->big_empty_pages_xMO[i] = NULL;
	}

      sctk_insert_used_page (page, tls);

      tls->lock = lock;

      next = (sctk_free_chunk_t *) sctk_next_malloc_chunk (chunk);
      next->type = sctk_chunk_small_type;

      sctk_free_chunk_small (next, tls);

      tls->spinlock = SCTK_SPINLOCK_INITIALIZER;
    }
  sctk_spinlock_unlock (&old_tls_list_spinlock);
  return tls;
}

static mpc_inline void
sctk_get_set_tls_init ()
{
  static sctk_mutex_t lock = SCTK_MUTEX_INITIALIZER;
  static volatile int once_done = 0;

#ifdef MPC_Threads
  static int done = 0;
  if (done == 0)
    {
      if (sctk_multithreading_initialised == 0)
	{
	  sctk_init_memory_allocation ();
	}
      else
	{
	  if (once_done == 0)
	    {
	      sctk_mutex_lock (&lock);
	      if (once_done == 0)
		{
		  sctk_init_memory_allocation ();
		  once_done = 1;
		}
	      sctk_mutex_unlock (&lock);
	    }
	  done = 1;
	}
    }
#else
  if (once_done == 0)
    {
      sctk_mutex_lock (&lock);
      if (once_done == 0)
	{
	  sctk_init_memory_allocation ();
	  once_done = 1;
	}
      sctk_mutex_unlock (&lock);
    }
#endif
}

static mpc_inline sctk_tls_t *__sctk_get_tls_init_to_do (void);
static mpc_inline sctk_tls_t *__sctk_get_tls_init_done (void);

static sctk_tls_t *(*__sctk_get_tls_ptr) () = __sctk_get_tls_init_to_do;

static mpc_inline sctk_tls_t *
__sctk_get_tls_init_to_do ()
{
  sctk_tls_t *tls;
  tls = sctk_get_tls_from_thread ();

  if ((tls == NULL) || (sctk_alloc_init_done == 0))
    {
      if (sctk_alloc_init_done == 0)
	sctk_get_set_tls_init ();

      tls = sctk_get_tls_from_thread ();
      if (tls == NULL)
	{
	  SCTK_DEBUG (sctk_mem_error ("Create new tls\n"));
	  tls = sctk_create_tls ();
	  sctk_set_tls_from_thread (tls);
	  SCTK_DEBUG (sctk_mem_error ("tls %p created\n", tls));
	}
    }

  if (sctk_alloc_init_done != 0)
    {
      __sctk_get_tls_ptr = __sctk_get_tls_init_done;
    }

  return tls;
}

static mpc_inline sctk_tls_t *
__sctk_get_tls_init_done ()
{
  sctk_tls_t *tls;

  tls = sctk_get_tls_from_thread_no_check ();
  if (tls == NULL)
    {
      SCTK_DEBUG (sctk_mem_error ("Create new tls\n"));
      tls = sctk_create_tls ();
      sctk_set_tls_from_thread (tls);
      SCTK_DEBUG (sctk_mem_error ("tls %p created\n", tls));
    }
  return tls;
}

static mpc_inline sctk_tls_t *
__sctk_get_tls ()
{
  return __sctk_get_tls_ptr ();
}

static mpc_inline void
__sctk_set_tls (sctk_tls_t * tls)
{
  sctk_get_set_tls_init ();
  SCTK_DEBUG (sctk_mem_error ("Set tls %p\n", tls));
  sctk_set_tls_from_thread (tls);
}

static inline void
sctk_refresh_chunk_alloc (sctk_free_chunk_t * chunk)
{
#if 0
  char *ptr;
  unsigned long offset;
  unsigned long size;

  ptr = (char *) sctk_get_ptr ((sctk_malloc_chunk_t *) chunk);
  size = chunk->cur_size;

  if (size >= page_size)
    {
      offset = (page_size - (((unsigned long) ptr) % page_size));

      ptr += offset;
      size -= offset;

      size = size / page_size;
      size = size * page_size;

/*     sctk_debug("Free %p(%p) %lu(%lu)",ptr,sctk_get_ptr ((sctk_malloc_chunk_t *) chunk),size,chunk->cur_size); */

      sctk_munmap (ptr, size);
      sctk_mmap (ptr, size);
    }
#endif
}


static inline void
sctk_refresh_page_alloc (sctk_page_t * page)
{
  if (page->page_state == sctk_free_state)
    {
      sctk_refresh_free_page_force (page);
    }
  else
    {
      if (page->type == sctk_chunk_small_type)
	{
	  char *ptr_small;
	  char *page_end;
	  sctk_free_chunk_t *chunk;
	  chunk = sctk_get_chunk_from_page (page);
	  ptr_small = (char *) chunk;
	  page_end = (char *) page + SCTK_PAGES_ALLOC_SIZE;
	  while ((unsigned long) ptr_small < (unsigned long) page_end)
	    {
	      chunk = (sctk_free_chunk_t *) ptr_small;
	      if (chunk->state == sctk_free_state_use)
		{
		  sctk_refresh_chunk_alloc (chunk);
		}
	      chunk = (sctk_free_chunk_t *)
		sctk_next_malloc_chunk ((sctk_malloc_chunk_t *) chunk);
	      ptr_small = (char *) chunk;
	    }
	}
      else
	{
	  sctk_free_chunk_t *chunk;
	  char *ptr_small;
	  chunk = sctk_get_chunk_from_page (page);
	  ptr_small = (char *) chunk;
	  if (chunk->state == sctk_free_state_use)
	    {
	      sctk_refresh_chunk_alloc (chunk);
	    }
	  else
	    {
	      chunk = (sctk_free_chunk_t *)
		sctk_next_malloc_chunk ((sctk_malloc_chunk_t *) chunk);
	      sctk_refresh_chunk_alloc (chunk);
	    }
	}
    }
}

/*******************************************/
/***************    BRK        *************/
/*******************************************/
static void
sctk_dump_memory_heap_fd (int fd);

static int sctk_check_address_range(void* addr, size_t n,void*  a_start_block, void* a_end_block){
  unsigned long start;
  unsigned long end;
  unsigned long  start_block;
  unsigned long  end_block;
  int res = 0;

  start = (unsigned long)addr;
  end = (unsigned long)addr + n;
  start_block = (unsigned long)a_start_block;
  end_block = (unsigned long)a_end_block;


  if((start >= start_block) && (start < end_block)){
    res = 1;
  }
  if((end < end_block) && (end >= start_block)){
    res = 1;
  }

  return res;
}

void sctk_check_address(void* addr, size_t n){
  char *local_brk_pointer;
  sctk_page_t *cursor;
  char *ptr;
  int i = 0;
  static int step = 0;

  sctk_nodebug("Check %p-%p",addr,(char*)addr + n);

  sctk_spinlock_read_lock(&sctk_sbrk_rwlock);

  local_brk_pointer = (char *) brk_pointer;

  step++;
  ptr = init_brk_pointer;
  cursor = (sctk_page_t *) ptr;
  while ((unsigned long) ptr < (unsigned long) local_brk_pointer)
    {
      if (sctk_check_address_range(addr,n,ptr,ptr + cursor->real_size)){
	if ((cursor->page_state == sctk_free_state) || (cursor->page_state == sctk_free_state_use))
	  {
	    if (sctk_check_address_range(addr,n,ptr,ptr + cursor->real_size)){
	      goto error_found;
	    }
	  }
	else
	  {
	    sctk_tls_t * tls;

	    tls = cursor->related;

	    sctk_spinlock_lock(&(tls->spinlock));
	    if (cursor->type == sctk_chunk_small_type)
	      {
		char *ptr_small;
		char *page_end;
		sctk_free_chunk_t *chunk;
		int block = 0;
		sctk_mem_assert (cursor->real_size == SCTK_PAGES_ALLOC_SIZE);

		chunk = sctk_get_chunk_from_page (cursor);
		ptr_small = (char *) chunk;
		page_end = (char *) cursor + SCTK_PAGES_ALLOC_SIZE;

		while ((unsigned long) ptr_small < (unsigned long) page_end)
		  {
		    chunk = (sctk_free_chunk_t *) ptr_small;
		    if (chunk->state == sctk_free_state_use)
		      {
			if (sctk_check_address_range(addr,n,ptr_small,
						     ptr_small + chunk->cur_size)){
			  sctk_spinlock_unlock(&(tls->spinlock));
			  goto error_found;
			}
		      }
		    else
		      {
			if (sctk_check_address_range(addr,n,ptr_small,
						     ptr_small + chunk->cur_size)){
			  sctk_spinlock_unlock(&(tls->spinlock));
			  goto end;
			}
		      }
		    block++;
		    chunk = (sctk_free_chunk_t *)
		      sctk_next_malloc_chunk ((sctk_malloc_chunk_t *) chunk);
		    ptr_small = (char *) chunk;
		  }
	      }
	    else
	      {
		sctk_free_chunk_t *chunk;
		char *ptr_small;
		chunk = sctk_get_chunk_from_page (cursor);
		ptr_small = (char *) chunk;
		if (chunk->state == sctk_free_state_use)
		  {
		    if (sctk_check_address_range(addr,n,ptr_small,
						 ptr_small + chunk->cur_size)){
		      sctk_spinlock_unlock(&(tls->spinlock));
		      goto error_found;
		    }
		  }
		else
		  {
		    if (sctk_check_address_range(addr,n,ptr_small,
						 ptr_small + chunk->cur_size)){
		      sctk_spinlock_unlock(&(tls->spinlock));
		      goto end;
		    }
		  }
	      }
	    sctk_spinlock_unlock(&(tls->spinlock));
	  }
      }
      cursor = (sctk_page_t *) (ptr + cursor->real_size);
      ptr = (char *) cursor;
      i++;
    }

#warning "We miss somme cases"
  /*
    We miss:
        - Out of scope memory
	- Allocations previous init
	- Code section
  */

 end:
  sctk_nodebug("Check %p-%p VALID",addr,(char*)addr + n);
/*   sctk_alloc_spinlock_unlock (&sctk_sbrk_lock); */
  sctk_spinlock_read_unlock(&sctk_sbrk_rwlock);

  return ;

 error_found:
/*   sctk_alloc_spinlock_unlock (&sctk_sbrk_lock); */
  sctk_spinlock_read_unlock(&sctk_sbrk_rwlock);
  sctk_error("invalid access to %p-%p",addr,(char*)addr + n);
  sctk_abort();
}

void
sctk_dump_memory_heap_fd (int fd)
{
  char *local_brk_pointer;
  sctk_page_t *cursor;
  char *ptr;
  int i = 0;
  int nb_free = 0;
  int nb_used = 0;
  static char buf[4096];
  static int step = 0;
  unsigned long mem_lost = 0;
  int to_close = 0;

/*   sctk_alloc_spinlock_lock (&sctk_sbrk_lock); */
  sctk_spinlock_read_lock(&sctk_sbrk_rwlock);

  local_brk_pointer = (char *) brk_pointer;

  sprintf (buf, "/tmp/mpc_memory_dump_%010d", step);
  step++;
  if(fd == -1){
    fd = open (buf, O_WRONLY | O_CREAT | O_TRUNC,
	       S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    to_close = 1;
  }
  ptr = init_brk_pointer;
  cursor = (sctk_page_t *) ptr;
  sprintf (buf, "\nDUMP PAGES\n");
  assume(write (fd, buf, strlen (buf)) ==  strlen (buf));
  while ((unsigned long) ptr < (unsigned long) local_brk_pointer)
    {
      if (cursor->page_state == sctk_free_state)
	{
	  sprintf (buf, "[%10d] %p-%p FREE %lu %p %p\n", i, ptr,
		   ptr + cursor->real_size,
		   cursor->real_size / (1024 * 1024), cursor->init_page,
		   cursor->related);
	  assume(write (fd, buf, strlen (buf)) ==  strlen (buf));
	  nb_free++;
	}
      else
	{
	  sprintf (buf, "[%10d] %p-%p USED %lu %p %p %d %d\n", i, ptr,
		   ptr + cursor->real_size,
		   cursor->real_size / (1024 * 1024), cursor->init_page,
		   cursor->related, cursor->page_state, cursor->type);
	  assume(write (fd, buf, strlen (buf)) ==  strlen (buf));
	  nb_used++;
	  if (cursor->type == sctk_chunk_small_type)
	    {
	      char *ptr_small;
	      char *page_end;
	      sctk_free_chunk_t *chunk;
	      unsigned long mem_used = 0;
	      int block = 0;
	      sctk_mem_assert (cursor->real_size == SCTK_PAGES_ALLOC_SIZE);

	      chunk = sctk_get_chunk_from_page (cursor);
	      ptr_small = (char *) chunk;
	      page_end = (char *) cursor + SCTK_PAGES_ALLOC_SIZE;

	      while ((unsigned long) ptr_small < (unsigned long) page_end)
		{
		  chunk = (sctk_free_chunk_t *) ptr_small;
		  if (chunk->state == sctk_free_state_use)
		    {
		      sprintf (buf,
			       "[%10d] \t[%10d] %p-%p FREE %7lu tree %u\n",
			       i, block, ptr_small,
			       ptr_small + chunk->cur_size,
			       chunk->cur_size, chunk->tree);
		      assume(write (fd, buf, strlen (buf)) ==  strlen (buf));
		    }
		  else
		    {
		      sprintf (buf,
			       "[%10d] \t[%10d] %p-%p USED %7lu tree %u\n",
			       i, block, ptr_small,
			       ptr_small + chunk->cur_size,
			       chunk->cur_size, chunk->tree);
		      assume(write (fd, buf, strlen (buf)) ==  strlen (buf));
		      mem_used += chunk->cur_size;
		    }
		  block++;
		  chunk = (sctk_free_chunk_t *)
		    sctk_next_malloc_chunk ((sctk_malloc_chunk_t *) chunk);
		  ptr_small = (char *) chunk;
		}
	      mem_lost += cursor->real_size - mem_used;
	      sprintf (buf, "[%10d] %p-%p USED %lu LOST nb ref %u\n", i,
		       ptr, ptr + cursor->real_size,
		       cursor->real_size - mem_used, cursor->nb_ref);
	      assume(write (fd, buf, strlen (buf)) ==  strlen (buf));
	    }
	  else
	    {
	      sctk_free_chunk_t *chunk;
	      int block = 0;
	      char *ptr_small;
	      unsigned long mem_used = 0;
	      chunk = sctk_get_chunk_from_page (cursor);
	      ptr_small = (char *) chunk;
	      if (chunk->state == sctk_free_state_use)
		{
		  sprintf (buf,
			   "[%10d] \t[%10d] %p-%p FREE %7lu tree %u\n",
			   i, block, ptr_small,
			   ptr_small + chunk->cur_size,
			   chunk->cur_size, chunk->tree);
		  assume(write (fd, buf, strlen (buf)) ==  strlen (buf));
		}
	      else
		{
		  sprintf (buf,
			   "[%10d] \t[%10d] %p-%p USED %7lu tree %u\n",
			   i, block, ptr_small,
			   ptr_small + chunk->cur_size,
			   chunk->cur_size, chunk->tree);
		  assume(write (fd, buf, strlen (buf)) ==  strlen (buf));
		  mem_used += chunk->cur_size;
		}
	    }
	}
      cursor = (sctk_page_t *) (ptr + cursor->real_size);
      ptr = (char *) cursor;
      i++;
    }
  sprintf (buf, "MEM_LOST %lu (%luMo)\n", mem_lost, mem_lost / (1024 * 1024));
  assume(write (fd, buf, strlen (buf)) ==  strlen (buf));
  sprintf (buf, "FREE %d\n", nb_free);
  assume(write (fd, buf, strlen (buf)) ==  strlen (buf));
  sprintf (buf, "USED %d\n", nb_used);
  assume(write (fd, buf, strlen (buf)) ==  strlen (buf));
  sprintf (buf, "TOTAL %d\n", nb_used + nb_free);
  assume(write (fd, buf, strlen (buf)) ==  strlen (buf));
  if(to_close){
    close (fd);
  }
/*   sctk_alloc_spinlock_unlock (&sctk_sbrk_lock); */
  sctk_spinlock_read_unlock(&sctk_sbrk_rwlock);
}

void
sctk_dump_memory_heap ()
{
  sctk_dump_memory_heap_fd(-1);
}


static mpc_inline sctk_page_t *
sctk_sbrk_compaction (sctk_size_t size)
{
  char *local_brk_pointer;
  sctk_page_t *cursor;
  sctk_page_t *last = NULL;
  char *ptr;
  sctk_page_t *results = NULL;

  SCTK_TRACE_START (sctk_sbrk_compaction, NULL, NULL, NULL, NULL);

  local_brk_pointer = (char *) brk_pointer;

  ptr = first_free;
  first_free = init_brk_pointer;
  cursor = (sctk_page_t *) ptr;
  while ((unsigned long) ptr < (unsigned long) local_brk_pointer)
    {
      sctk_size_t real_size;

      real_size = cursor->real_size;
      if (cursor->init_page == cursor)
	{
	  if ((cursor->page_state == sctk_free_state) &&
	      (real_size == SCTK_xMO_TAB_SIZE * SCTK_PAGES_ALLOC_SIZE))
	    {
	      sctk_page_t *next_page;

	      if (first_free)
		{
		  first_free = (char *) cursor;
		}

	      next_page = (sctk_page_t *) (ptr + real_size);

	      if ((unsigned long) next_page <
		  (unsigned long) local_brk_pointer)
		{
		  if (next_page->page_state == sctk_free_state)
		    {
		      cursor->real_size += next_page->real_size;
		      cursor->init_page = NULL;
		    }
		  else
		    {
		      last = cursor;
		      if (real_size >= size)
			{
			  results = cursor;
			  goto fin_compaction;
			}
		      cursor = (sctk_page_t *) (ptr + real_size);
		    }

		}
	      else
		{
		  last = cursor;
		  if (real_size >= size)
		    {
		      results = cursor;
		      goto fin_compaction;
		    }
		  cursor = (sctk_page_t *) (ptr + real_size);
		}

	    }
	  else
	    {
	      real_size = SCTK_xMO_TAB_SIZE * SCTK_PAGES_ALLOC_SIZE;
	      cursor = (sctk_page_t *) (ptr + real_size);
	    }
	}
      else
	{
	  if (cursor->page_state == sctk_free_state)
	    {
	      sctk_page_t *next_page;

	      if (first_free)
		{
		  first_free = (char *) cursor;
		}

	      next_page = (sctk_page_t *) (ptr + real_size);

	      if ((unsigned long) next_page <
		  (unsigned long) local_brk_pointer)
		{
		  if (next_page->page_state == sctk_free_state)
		    {
		      cursor->real_size += next_page->real_size;
		      cursor->init_page = NULL;
		    }
		  else
		    {
		      last = cursor;
		      if (real_size >= size)
			{
			  results = cursor;
			  goto fin_compaction;
			}
		      cursor = (sctk_page_t *) (ptr + real_size);
		    }

		}
	      else
		{
		  last = cursor;
		  if (real_size >= size)
		    {
		      results = cursor;
		      goto fin_compaction;
		    }
		  cursor = (sctk_page_t *) (ptr + real_size);
		}

	    }
	  else
	    {
	      cursor = (sctk_page_t *) (ptr + real_size);
	    }
	}

      ptr = (char *) cursor;
    }

  if (last != NULL)
    {
      if ((char *) last + last->real_size == (char *) local_brk_pointer)
	{
	  sctk_size_t real_size;
	  if (last == results)
	    {
	      results = NULL;
	    }
	  real_size = last->real_size;
	  local_brk_pointer = (char *) last;
	  sctk_munmap ((void *) last, real_size);
	  sctk_del_mem (real_size);
	}
    }

fin_compaction:
  brk_pointer = local_brk_pointer;

  if (first_free == NULL)
    {
      first_free = (char *) local_brk_pointer;
    }

  SCTK_TRACE_END (sctk_sbrk_compaction, NULL, NULL, NULL, NULL);
  return results;
}

static sctk_size_t
sctk_sbrk (sctk_size_t init_size, void **ptr)
{
  sctk_size_t size;
  sctk_size_t return_size;
  void *return_ptr;
  char *local_brk_pointer;
  sctk_page_t *cursor;
  char *tmp;

  sctk_page_t *result = NULL;
  sctk_size_t real_size;

  SCTK_TRACE_START (sctk_sbrk, init_size, ptr, NULL, NULL);

  size = sctk_compute_real_size (init_size);

  while (sctk_no_alloc_land != 0){
    sched_yield();
  }

  SCTK_DEBUG (sctk_dump_memory_heap ());

/*   sctk_alloc_spinlock_lock (&sctk_sbrk_lock); */
  sctk_spinlock_write_lock(&sctk_sbrk_rwlock);

  result = sctk_sbrk_compaction (size);

  local_brk_pointer = (char *) brk_pointer;

  cursor = result;
  if (cursor != NULL)
    {
      real_size = cursor->real_size;

      if (expect_false (real_size > size))
	{
	  sctk_page_t *end;
	  void *tmp_sbrk;

	  end = (sctk_page_t *) ((char *) cursor + (real_size - size));

#ifndef SCTK_DO_NOT_FREE
	  tmp_sbrk = sctk_mmap (end, size);
	  sctk_mem_assert (tmp_sbrk == end);
#endif

	  end->real_size = size;
	  end->page_state = sctk_free_state;
	  cursor->real_size = real_size - size;
	  cursor = end;
	}
      else
	{
	  sctk_realloc_free_page (cursor);
	}
      return_ptr = cursor;
      return_size = cursor->real_size;

      goto fin_sbrk;
    }

  /*New allocation */
  tmp = local_brk_pointer;
  local_brk_pointer = (char *) (local_brk_pointer + size);

  SCTK_DEBUG (sctk_mem_error ("CURRENT BRK %p %d %d\n", tmp,
			      (((unsigned long) tmp >= SCTK_BOTTOM_SKIP) &&
			       ((unsigned long) tmp < SCTK_TOP_SKIP)),
			      (((unsigned long) local_brk_pointer >=
				SCTK_BOTTOM_SKIP)
			       && ((unsigned long) local_brk_pointer <
				   SCTK_TOP_SKIP))));
  if ((((unsigned long) tmp >= SCTK_BOTTOM_SKIP)
       && ((unsigned long) tmp < SCTK_TOP_SKIP))
      || (((unsigned long) local_brk_pointer >= SCTK_BOTTOM_SKIP)
	  && ((unsigned long) local_brk_pointer < SCTK_TOP_SKIP)))
    {
      void *tmp_sbrk;
      tmp_sbrk = sctk_mmap (tmp, page_size);
      sctk_mem_assert (tmp_sbrk == tmp);
      sctk_init_page ((sctk_page_t *) tmp_sbrk, SCTK_SKIP_SIZE);
      ((sctk_page_t *) tmp_sbrk)->type = sctk_chunk_big_type;
      ((sctk_page_t *) tmp_sbrk)->page_state = sctk_inuse_state;
      tmp = tmp + ((sctk_page_t *) tmp_sbrk)->real_size;
      local_brk_pointer = tmp;

      tmp = local_brk_pointer;
      local_brk_pointer = (char *) (local_brk_pointer + size);

      sctk_mem_error ("Skip pages\n");
    }

  SCTK_DEBUG (sctk_mem_error
	      ("SBRK NEW BRK %p -> %lu\n", local_brk_pointer,
	       ((unsigned long) local_brk_pointer -
		(unsigned long) init_brk_pointer) / (1024 * 1024)));

  if ((unsigned long) local_brk_pointer >= (unsigned long) max_brk_pointer)
    {
      sctk_mem_error ("Not Enough Memory\n");
/*       sctk_alloc_spinlock_unlock (&sctk_sbrk_lock); */
      sctk_spinlock_write_unlock(&sctk_sbrk_rwlock);
      abort ();
    }

  return_ptr = sctk_mmap ((void *) tmp, size);
  return_size = size;

  sctk_mem_assert (return_ptr != (void *) (-1));
  sctk_add_mem (size);

fin_sbrk:
  SCTK_MAKE_NOACCESS (return_ptr, return_size);
  SCTK_MAKE_WRITABLE (return_ptr, sizeof (sctk_page_t));
  ((sctk_page_t *) return_ptr)->page_state = sctk_inuse_state;

  brk_pointer = local_brk_pointer;
/*   sctk_mem_error("New brk %p \n",local_brk_pointer); */
/*   sctk_alloc_spinlock_unlock (&sctk_sbrk_lock); */
  sctk_spinlock_write_unlock(&sctk_sbrk_rwlock);

  *ptr = return_ptr;

  SCTK_TRACE_END (sctk_sbrk, init_size, ptr, local_brk_pointer,
		  max_brk_pointer);
  return return_size;
}

static sctk_page_t *
sctk_alloc_page_sbrk (sctk_size_t size, sctk_tls_t * tls)
{
  sctk_alloc_page_big_flush (tls);
  sctk_mem_assert (size % (SCTK_xMO_TAB_SIZE * SCTK_PAGES_ALLOC_SIZE) == 0);
  SCTK_DEBUG (sctk_mem_error ("New block %lu\n", size / (1024 * 1024)));
  size -= sctk_sizeof_sctk_page_t + sctk_sizeof_sctk_malloc_chunk_t;
  return sctk_create_page (size);
}

/*******************************************/
/*************** ALLOC  BIG    *************/
/*******************************************/

static void
sctk_free_page_big_insert (sctk_page_t * page, sctk_tls_t * tls)
{
  int i;
  sctk_size_t size;

  size = page->real_size;
  i = size / SCTK_PAGES_ALLOC_SIZE;
  i--;
  sctk_mem_assert (i >= 0);
  sctk_mem_assert (i < SCTK_xMO_TAB_SIZE);
  sctk_mem_assert (page->real_size == SCTK_PAGES_ALLOC_SIZE * (1 + i));
  page->page_state = sctk_free_state_use;
  sctk_insert_free_page (page,
			 (sctk_page_t **) & (tls->big_empty_pages_xMO[i]));
  SCTK_DEBUG (sctk_mem_error
	      ("Return Big Page %d %p %lu\n", i, page,
	       page->real_size / (1024 * 1024)));
}

static void
sctk_free_page_big_compaction_iter (sctk_page_t * page,
				    sctk_size_t size, sctk_tls_t * tls)
{
  if ((size > SCTK_PAGES_ALLOC_SIZE) && (page->real_size < size))
    {
      sctk_page_t *next;

      size = size / 2;
      next = (sctk_page_t *) ((char *) page + size);

      sctk_free_page_big_compaction_iter (page, size, tls);
      sctk_free_page_big_compaction_iter (next, size, tls);

      SCTK_DEBUG (sctk_mem_error
		  ("TRY MERGE %p %lu %d and %p %lu %d (%lu)\n", page,
		   page->real_size, page->page_state, next,
		   next->real_size, next->page_state, size));
      if (((page->real_size == size)
	   && (page->page_state == sctk_free_state_use))
	  && ((next->real_size == size)
	      && (next->page_state == sctk_free_state_use)))
	{
	  int pos;


	  SCTK_DEBUG (sctk_mem_error
		      ("MERGE %p %lu %d and %p %lu %d (%lu)\n", page,
		       page->real_size, page->page_state, next,
		       next->real_size, next->page_state, size));

	  pos = page->real_size / SCTK_PAGES_ALLOC_SIZE;
	  pos--;

	  sctk_remove_free_page ((sctk_page_t *) page,
				 (sctk_page_t **) & (tls->
						     big_empty_pages_xMO
						     [pos]));
	  sctk_remove_free_page ((sctk_page_t *) next,
				 (sctk_page_t **) & (tls->
						     big_empty_pages_xMO
						     [pos]));

	  sctk_remove_used_page (next, tls);
	  SCTK_MAKE_NOACCESS (page, 2 * size);
	  SCTK_MAKE_READABLE (page, sizeof (sctk_page_t));
	  page->real_size = 2 * size;
	  page->type = sctk_chunk_big_type;

	  page->dirty = page->dirty + next->dirty;

	  sctk_free_page_big_insert (page, tls);

	  SCTK_DEBUG (sctk_mem_error
		      ("MERGE New page merged %p %lu\n", page,
		       page->real_size / (1024 * 1024)));
	}
    }
}

static void
sctk_alloc_page_big_flush (sctk_tls_t * tls)
{
  int pos;
  sctk_page_t *page;
  for (pos = 0; pos < SCTK_xMO_TAB_SIZE; pos++)
    {
      page = tls->big_empty_pages_xMO[pos];
      while (page != NULL)
	{
	  sctk_refresh_free_page_force (page);
	  page = page->next_chunk;
	}
    }
}

/* void */
/* sctk_alloc_clean () */
/* { */
/*   sctk_tls_t *tls; */
/*   tls = __sctk_get_tls (); */
/*   sctk_alloc_page_big_flush (tls); */
/* } */

static void
sctk_free_page_big_compaction (sctk_page_t * page, sctk_tls_t * tls)
{
  SCTK_TRACE_START (sctk_free_page_big_compaction, page, tls, NULL, NULL);
  if (page->real_size < SCTK_xMO_TAB_SIZE * SCTK_PAGES_ALLOC_SIZE)
    {
      sctk_page_t *page_init;

      sctk_refresh_free_page_force (page);
      page_init = (sctk_page_t *) sctk_big_page_align (page);

      SCTK_DEBUG (sctk_mem_error ("BEGIN COMPACTION %p\n", page_init));
      sctk_free_page_big_compaction_iter (page_init,
					  SCTK_xMO_TAB_SIZE *
					  SCTK_PAGES_ALLOC_SIZE, tls);

      if (page_init->real_size >= SCTK_xMO_TAB_SIZE * SCTK_PAGES_ALLOC_SIZE)
	{
	  int pos;
	  SCTK_DEBUG (sctk_mem_error ("Return page %p\n", page_init));
	  pos = SCTK_xMO_TAB_SIZE;
	  pos--;

	  if (tls->full_page_number >= SCTK_FULL_PAGE_NUMBER)
	    {
	      sctk_remove_free_page ((sctk_page_t *) page_init,
				     (sctk_page_t **) & (tls->
							 big_empty_pages_xMO
							 [pos]));
	      sctk_remove_used_page (page_init, tls);

	      sctk_empty_free_page (page_init);

	      page_init->page_state = sctk_free_state;

	      if ((unsigned long) first_free > (unsigned long) page_init)
		{
		  first_free = (char *) page_init;
		}
	    }
	  else
	    {
	      sctk_refresh_free_page_force (page_init);
	    }
	}
    }
  SCTK_TRACE_END (sctk_free_page_big_compaction, page, tls, NULL, NULL);
}


static void
sctk_alloc_page_big_split (sctk_size_t init_size, sctk_page_t * page,
			   sctk_tls_t * tls)
{
  sctk_size_t size;
  sctk_size_t new_size;
  sctk_size_t real_size;

  if (page->init_page == NULL)
    {
      return;
    }

  size = sctk_compute_real_size (init_size);
  SCTK_DEBUG (sctk_mem_error
	      ("Request size %lu in TLS %p, %lu\n", size / (1024 * 1024), tls,
	       page->real_size / (1024 * 1024)));


  new_size = page->real_size / 2;
  while (new_size >= size)
    {
      sctk_page_t *end;

      real_size = page->real_size;
      sctk_mem_assert (real_size == new_size * 2);
      sctk_mem_assert (page->page_state == sctk_free_state_use);
      SCTK_DEBUG (sctk_mem_error
		  ("Old page %p %lu\n", page,
		   page->real_size / (1024 * 1024)));

      end = (sctk_page_t *) ((char *) page + new_size);
      SCTK_MAKE_WRITABLE (end, sizeof (sctk_page_t));

      end->page_state = sctk_free_state_use;
      page->page_state = sctk_free_state_use;
      end->init_page = page->init_page;
      end->dirty = page->dirty / 2;
      page->dirty = page->dirty / 2;

      end->related = tls;
      page->related = tls;

      end->type = sctk_chunk_big_type;
      page->type = sctk_chunk_big_type;

      end->real_size = new_size;
      page->real_size = new_size;

      SCTK_DEBUG (sctk_mem_error
		  ("SPLIT New page %p %lu\n", page,
		   page->real_size / (1024 * 1024)));
      SCTK_DEBUG (sctk_mem_error
		  ("SPLIT New page %p %lu\n", end,
		   end->real_size / (1024 * 1024)));

      sctk_insert_used_page (end, tls);
      sctk_alloc_page_big_split (init_size, end, tls);
      sctk_free_page_big_insert (end, tls);

      new_size = page->real_size / 2;
    }
}

static sctk_page_t *
sctk_alloc_page_big (sctk_size_t size, sctk_tls_t * tls)
{
  sctk_page_t *page = NULL;
  int pos;
  sctk_size_t aligned_size;

  aligned_size = sctk_compute_real_size (size);

  pos = aligned_size / SCTK_PAGES_ALLOC_SIZE;
  pos--;

  SCTK_DEBUG (sctk_mem_error
	      ("Request size %lu int TLS %p\n", aligned_size / (1024 * 1024),
	       tls));

  sctk_mem_assert (pos >= 0);

  if (pos < SCTK_xMO_TAB_SIZE)
    {
      sctk_page_t **big_empty_pages_xMO;
      sctk_page_t *item_page;

      big_empty_pages_xMO = tls->big_empty_pages_xMO;

      page = big_empty_pages_xMO[pos];
      if (page != NULL)
	{
	  item_page = page;
	  while (item_page != NULL)
	    {
	      if (page->dirty)
		{
		  if (((unsigned long) page > (unsigned long) item_page)
		      && item_page->dirty)
		    {
		      page = item_page;
		    }
		}
	      else
		{
		  if ((unsigned long) page > (unsigned long) item_page)
		    {
		      page = item_page;
		    }
		}
	      item_page = item_page->next_chunk;
	    }

	  sctk_remove_free_page ((sctk_page_t *) page,
				 (sctk_page_t **) &
				 (big_empty_pages_xMO[pos]));

	  sctk_mem_assert (page->real_size ==
			   SCTK_PAGES_ALLOC_SIZE * (1 + pos));
	  SCTK_DEBUG (sctk_mem_error ("Found Big Page\n"));
	  if (pos == SCTK_xMO_TAB_SIZE - 1)
	    tls->full_page_number--;
	  goto fin;
	}

      pos++;

      for (; pos < SCTK_xMO_TAB_SIZE; pos++)
	{
	  page = big_empty_pages_xMO[pos];
	  if (page != NULL)
	    {
	      item_page = page;
	      while (item_page != NULL)
		{
		  if (page->dirty)
		    {
		      if (((unsigned long) page > (unsigned long) item_page)
			  && item_page->dirty)
			{
			  page = item_page;
			}
		    }
		  else
		    {
		      if ((unsigned long) page > (unsigned long) item_page)
			{
			  page = item_page;
			}
		    }
		  item_page = item_page->next_chunk;
		}

	      sctk_remove_free_page ((sctk_page_t *) page,
				     (sctk_page_t **) &
				     (big_empty_pages_xMO[pos]));
	      sctk_mem_assert (page->real_size ==
			       SCTK_PAGES_ALLOC_SIZE * (1 + pos));

	      sctk_refresh_free_page_force (page);
	      sctk_alloc_page_big_split (size, page, tls);
	      SCTK_DEBUG (sctk_mem_error ("Found Big Page\n"));
	      if (pos == SCTK_xMO_TAB_SIZE - 1)
		tls->full_page_number--;
	      goto fin;

	    }
	}
      tls->full_page_number = 0;
    }
  SCTK_DEBUG (sctk_mem_error ("NOT Found Big Page %d\n", pos));

  page =
    sctk_alloc_page_sbrk (sctk_compute_real_size_big
			  (size + sctk_sizeof_sctk_page_t +
			   sctk_sizeof_sctk_malloc_chunk_t), tls);
  page->page_state = sctk_free_state_use;
  if (page != NULL)
    {
      sctk_insert_used_page (page, tls);
      sctk_alloc_page_big_split (size, page, tls);
    }

fin:
  page->type = sctk_chunk_big_type;
  page->page_state = sctk_inuse_state;
  SCTK_DEBUG (sctk_mem_error
	      ("NEW page %p %p %lu\n", page, (char *) page + page->real_size,
	       page->real_size / (1024 * 1024)));
  return page;
}

static void
sctk_free_page_big (sctk_page_t * page, sctk_tls_t * tls)
{
  SCTK_TRACE_START (sctk_free_page_big, page, tls, NULL, NULL);
  page->type = sctk_chunk_big_type;
  if (page->cur_dirty > page->dirty)
    {
      page->dirty = page->cur_dirty;
    }
  page->cur_dirty = 0;
  SCTK_DEBUG (sctk_mem_error
	      ("FREE page %p %p\n", page, (char *) page + page->real_size));
  if ((sctk_size_t) page >= (sctk_size_t) init_brk_pointer)
    {
      if (page->real_size < SCTK_xMO_TAB_SIZE * SCTK_PAGES_ALLOC_SIZE)
	{
	  SCTK_DEBUG (sctk_mem_error
		      ("page->real_size < SCTK_xMO_TAB_SIZE*SCTK_PAGES_ALLOC_SIZE\n"));
	  sctk_free_page_big_insert (page, tls);
	  sctk_free_page_big_compaction (page, tls);
	}
      else
	{
	  SCTK_DEBUG (sctk_mem_error
		      ("page->real_size >= SCTK_xMO_TAB_SIZE*SCTK_PAGES_ALLOC_SIZE\n"));
	  sctk_remove_used_page (page, tls);
	  sctk_empty_free_page (page);
	  page->page_state = sctk_free_state;
	}
    }
  else
    {
      sctk_delete_not_migrable_page (page);
    }
  SCTK_TRACE_END (sctk_free_page_big, page, tls, NULL, NULL);
}

static void
sctk_free_chunk_big (sctk_free_chunk_t * ptr, sctk_tls_t * tls)
{
  sctk_page_t *page;

  page = sctk_get_page_from_chunk (ptr);
/*   sctk_refresh_free_page(page); */
  sctk_free_page_big (page, tls);

}

static void *
sctk_alloc_ptr_big (sctk_size_t size, sctk_tls_t * tls)
{
  sctk_size_t real_size;
  sctk_page_t *page;
  sctk_malloc_chunk_t *chunk;

  real_size = sctk_compute_real_size (size + 4 * sctk_sizeof_sctk_page_t);

  page = sctk_alloc_page_big (real_size, tls);
  if (page == NULL)
    return NULL;

  sctk_init_page (page, page->real_size);

  page->cur_dirty = real_size;

  page->type = sctk_chunk_big_type;
  page->related = tls;

  /*allocation de la page */
  chunk = (sctk_malloc_chunk_t *) sctk_get_chunk_from_page (page);

  sctk_mem_assert (size <= chunk->cur_size);

  page->state = sctk_inuse_state;
  chunk->state = sctk_inuse_state;

  chunk->type = sctk_chunk_big_type;
  sctk_mem_assert (chunk->cur_size >= size);

  sctk_chunk_split (chunk, size);

  return sctk_get_ptr ((sctk_malloc_chunk_t *) chunk);
}

static void
sctk_free_big (sctk_free_chunk_t * chunk, sctk_size_t size,
	       sctk_tls_t * my_tls)
{
  sctk_tls_t *tls;
  sctk_page_t *page;
  LOG_IN ();
  page = sctk_get_page_from_chunk (chunk);

  tls = page->related;

  if (tls == my_tls)
    {
      sctk_free_chunk_big (chunk, tls);
    }
  else
    {
      /*Gestion des libération distantes */
      sctk_mutex_lock (&tls->lock);
/*       tls->big_chunks_distant_number++; */
      chunk->next_chunk = (sctk_free_chunk_t *) tls->big_chunks_distant;
      tls->big_chunks_distant = chunk;
      sctk_mutex_unlock (&tls->lock);
    }

  LOG_OUT ();
}

static void
sctk_free_big_distant (sctk_tls_t * tls)
{
  if (tls->big_chunks_distant != NULL)
    {
      sctk_free_chunk_t *big_chunks_distant;
      sctk_free_chunk_t *tmp;
      /*Gestion des libération distantes */
      sctk_mutex_lock (&tls->lock);
      if (tls->big_chunks_distant != NULL)
	{
	  big_chunks_distant =
	    (sctk_free_chunk_t *) (tls->big_chunks_distant);
	  tls->big_chunks_distant = NULL;
	  sctk_mutex_unlock (&tls->lock);
	  while (big_chunks_distant != NULL)
	    {
	      tmp = big_chunks_distant->next_chunk;
	      sctk_free_chunk_big (big_chunks_distant, tls);
	      big_chunks_distant = tmp;
	    }
	}
      else
	{
	  sctk_mutex_unlock (&tls->lock);
	}
    }
}

/*******************************************/
/*************** ALLOC SMALL   *************/
/*******************************************/
static mpc_inline void
sctk_chunks_remove_intern (int i, sctk_tls_t * tls)
{
  sctk_free_chunk_t *chunk;
  sctk_free_chunk_t *next;

  chunk = tls->chunks[i];
  sctk_alloc_assert (chunk->cur_size >= chunk_sizes[i]);
  sctk_alloc_assert (tls->chunks[i] != NULL);

  tls->chunks[i] = chunk->next_chunk;
  next = tls->chunks[i];

  if (next)
    {
      next->prev_chunk = NULL;
    }

  chunk->next_chunk = NULL;
  chunk->prev_chunk = NULL;
}

static mpc_inline void
sctk_chunks_remove_inside (sctk_free_chunk_t * chunk, sctk_tls_t * tls)
{
  int i;

  i = chunk->small_pos;

/*   sctk_debug("(chunk->cur_size %lu >= chunk_sizes[%d] %lu) || (%d == %d)", */
/* 	     chunk->cur_size,i,chunk_sizes[i],i,max_chunk_sizes); */

  sctk_mem_assert ((chunk->cur_size >= chunk_sizes[i])
		   || (i == max_chunk_sizes));
  sctk_mem_assert (tls->chunks[i] != NULL);

  sctk_remove_free_chunk ((sctk_free_chunk_t *) chunk,
			  (sctk_free_chunk_t **) & (tls->chunks[i]));
  chunk->next_chunk = NULL;
  chunk->prev_chunk = NULL;
}

static mpc_inline void
sctk_chunks_insert (sctk_free_chunk_t * chunk, unsigned short i,
		    sctk_tls_t * tls)
{
  sctk_free_chunk_t *next;

  SCTK_MAKE_READABLE (chunk, sizeof (sctk_free_chunk_t));

  SCTK_DEBUG (sctk_mem_error
	      ("sctk_chunks_insert %lu %d\n", chunk->cur_size, i));
  next = tls->chunks[i];
  sctk_mem_assert ((chunk->cur_size >= chunk_sizes[i])
		   || (i == max_chunk_sizes));

  chunk->prev_chunk = NULL;
  chunk->next_chunk = next;
  if (next)
    {
      next->prev_chunk = chunk;
    }

  tls->chunks[i] = chunk;
  chunk->small_pos = i;
}

static mpc_inline sctk_free_chunk_t *
sctk_chunks_remove (int i, sctk_tls_t * tls)
{
  sctk_free_chunk_t *chunk;

  SCTK_DEBUG (sctk_mem_error ("sctk_chunks_remove %d\n", i));
  chunk = tls->chunks[i];
  if (chunk != NULL)
    {
      sctk_mem_assert ((chunk->cur_size >= chunk_sizes[i])
		       || (i == max_chunk_sizes));
      sctk_chunks_remove_intern (i, tls);
    }
  return chunk;
}

#define sctk_init_tree()     (~(1))
#define sctk_left_branch(i)  ((((unsigned int)i) << 1))
#define sctk_right_branch(i) ((((unsigned int)i) << 1) + (unsigned int)1)
#define sctk_merge_leaf(i)   ((((unsigned int)i) >> 1) + (unsigned int)(1 << 31))

static mpc_inline void
sctk_small_page_compaction (sctk_free_chunk_t * chunk, sctk_tls_t * tls)
{
  sctk_page_t *page;
  char *page_end;
  char *ptr;
  sctk_free_chunk_t *next;

  page = (sctk_page_t *) sctk_small_page_align (chunk);
  page->nb_ref--;
  page_end = (char *) page + SCTK_PAGES_ALLOC_SIZE;

  sctk_mem_assert (page->page_state == sctk_inuse_state);
  sctk_mem_assert (page->real_size == SCTK_PAGES_ALLOC_SIZE);

  if (page->nb_ref == 0)
    {
      SCTK_DEBUG (sctk_mem_error ("RETURN %p\n", page));
      chunk = sctk_get_chunk_from_page (page);
      ptr = (char *) chunk;
      while ((unsigned long) ptr < (unsigned long) page_end)
	{
	  sctk_mem_assert (chunk->state == sctk_free_state_use);
	  sctk_chunks_remove_inside (chunk, tls);
	  SCTK_DEBUG (sctk_mem_error ("\tREMOVE %p\n", chunk));
	  chunk =
	    (sctk_free_chunk_t *)
	    sctk_next_malloc_chunk ((sctk_malloc_chunk_t *) chunk);
	  ptr = (char *) chunk;
	}

      sctk_mem_assert (page->real_size == SCTK_PAGES_ALLOC_SIZE);
      if (tls->last_small)
	{
	  if ((unsigned long) tls->last_small > (unsigned long) page)
	    {
	      if (tls->last_small2)
		{
		  sctk_refresh_free_page (tls->last_small2);
		  sctk_free_page_big (tls->last_small2, tls);
		}
	      tls->last_small2 = tls->last_small;
	      tls->last_small = page;
	    }
	  else
	    {
	      if ((unsigned long) tls->last_small2 > (unsigned long) page)
		{
		  sctk_refresh_free_page (tls->last_small2);
		  sctk_free_page_big (tls->last_small2, tls);
		  tls->last_small2 = page;
		}
	      else
		{
		  sctk_refresh_free_page (page);
		  sctk_free_page_big (page, tls);
		}
	    }
	}
      else
	{
	  tls->last_small = page;
	}
      return;
    }

  next =
    (sctk_free_chunk_t *)
    sctk_next_malloc_chunk ((sctk_malloc_chunk_t *) chunk);

  SCTK_DEBUG (sctk_mem_error ("%p < %p <%p\n", page, chunk, page_end));
  ptr = (char *) next;
  while ((unsigned long) ptr < (unsigned long) page_end)
    {
      SCTK_DEBUG (sctk_mem_error
		  ("TRY MERGE %p %p %p\n", chunk, next, page_end));
      if ((chunk->state == sctk_free_state_use)
	  && (next->state == sctk_free_state_use)
	  && ((next->tree) == (chunk->tree + (unsigned int) 1))
	  && (chunk->tree % 2 == 0))
	{
	  SCTK_DEBUG (sctk_mem_error
		      ("MERGE %p %lu and %p %lu (%u,%u) -> %u\n", chunk,
		       chunk->cur_size, next, next->cur_size, chunk->tree,
		       next->tree, sctk_merge_leaf (chunk->tree)));
	  SCTK_DEBUG (sctk_mem_error
		      ("MERGE %p %lu and %p %lu (%u,%u) -> %u\n", chunk,
		       chunk->cur_size, next, next->cur_size, chunk->tree,
		       next->tree, sctk_merge_leaf (chunk->tree));
		      getchar ());
	  sctk_chunks_remove_inside (next, tls);
	  sctk_chunks_remove_inside (chunk, tls);
	  sctk_chunk_merge ((sctk_malloc_chunk_t *) chunk);
	  chunk->tree = sctk_merge_leaf (chunk->tree);
	  sctk_free_chunk_small (chunk, tls);
	  next =
	    (sctk_free_chunk_t *)
	    sctk_next_malloc_chunk ((sctk_malloc_chunk_t *) chunk);
	  ptr = (char *) next;
	}
      else
	{
	  break;
	}
    }
}

static sctk_page_t *
sctk_alloc_page_small (sctk_size_t size, sctk_tls_t * tls)
{
  sctk_page_t *page;
  if (tls->last_small)
    {
      page = tls->last_small;
      tls->last_small = tls->last_small2;
      tls->last_small2 = NULL;
    }
  else
    {
      page = sctk_alloc_page_big (SCTK_SMALL_PAGES_ALLOC_SIZE, tls);
    }
  sctk_init_page (page, page->real_size);
  sctk_mem_assert (page->real_size == SCTK_PAGES_ALLOC_SIZE);
  SCTK_DEBUG (sctk_mem_error
	      ("Small page %010p %lu\n", page,
	       page->real_size / (1024 * 1024)));
  page->type = sctk_chunk_small_type;
  page->cur_dirty = 0;
  return page;
}

static void
sctk_free_chunk_small (sctk_free_chunk_t * ptr, sctk_tls_t * tls)
{
  int j;

  SCTK_DEBUG (sctk_mem_error
	      ("FREE_SMALL page %p\n",
	       ((sctk_page_t *) sctk_small_page_align (ptr))));
  sctk_mem_assert (((sctk_page_t *) sctk_small_page_align (ptr))->
		   page_state == sctk_inuse_state);
  if (ptr->cur_size > chunk_sizes[max_chunk_sizes - 1])
    {
      j = max_chunk_sizes;
    }
  else
    {
      j = sctk_get_chunk_tab (ptr->cur_size);
      SCTK_DEBUG (sctk_mem_error ("j = %d %lu\n", j, ptr->cur_size));
      if (chunk_sizes[j] > ptr->cur_size)
	{
	  j--;
	}
      SCTK_DEBUG (sctk_mem_error ("ADJUSTED j = %d\n", j));
    }

  ptr->state = sctk_free_state_use;
  sctk_chunks_insert (ptr, j, tls);
}

static void
sctk_free_chunk_small_local (sctk_free_chunk_t * ptr, sctk_tls_t * tls)
{
  int j;
  sctk_page_t *page;

  j = ptr->small_pos;
  ptr->state = sctk_free_state_use;
  page = (sctk_page_t *) sctk_small_page_align (ptr);
  page->cur_dirty -= ptr->cur_size;
  sctk_chunks_insert (ptr, j, tls);
}



static sctk_free_chunk_t *
sctk_alloc_ptr_small_search (sctk_size_t size, sctk_tls_t * tls, int i,
			     sctk_size_t real_size)
{
  int cursor;
  sctk_free_chunk_t *chunk;
  sctk_page_t *page;
  sctk_free_chunk_t *next;
  sctk_size_t new_size;
  sctk_size_t stop_size;

  /* if (size > 512) */
  if (size > 1024)
    {
      sctk_free_chunk_t *tmp;
      sctk_free_chunk_t **chunks_tab;
      chunk = NULL;

      chunks_tab = tls->chunks;

      cursor = i - 1;
      tmp = chunks_tab[cursor];
      if (tmp != NULL)
	{
	  sctk_size_t tmp_size = 2 * SCTK_PAGES_ALLOC_SIZE;
	  do
	    {
	      sctk_free_chunk_t *tmp_next;
	      sctk_size_t cur_size;

	      tmp_next = tmp->next_chunk;
	      cur_size = tmp->cur_size;

	      if ((cur_size > real_size) && (tmp_size >= cur_size))
		{
		  tmp_size = cur_size;
		  chunk = tmp;
		}
	      tmp = tmp_next;
	    }
	  while (tmp != NULL);
	  if (chunk != NULL)
	    {
	      sctk_chunks_remove_inside (chunk, tls);
	      SCTK_DEBUG (sctk_mem_error
			  ("USE page %p\n",
			   ((sctk_page_t *) sctk_small_page_align (chunk))));
	      SCTK_DEBUG (sctk_mem_assert
			  (((sctk_page_t *) sctk_small_page_align (chunk))->
			   page_state == sctk_inuse_state));
	      goto fin;
	    }
	}

      tmp = NULL;
      for (cursor = i; (tmp == NULL) && (cursor < max_chunk_sizes); cursor++)
	{
	  tmp = chunks_tab[cursor];
	}

      if (tmp != NULL)
	{
	  sctk_size_t tmp_size = 2 * SCTK_PAGES_ALLOC_SIZE;
	  do
	    {
	      sctk_free_chunk_t *tmp_next;
	      sctk_size_t cur_size;

	      tmp_next = tmp->next_chunk;
	      cur_size = tmp->cur_size;

	      if (tmp_size >= cur_size)
		{
		  tmp_size = cur_size;
		  chunk = tmp;
		}
	      tmp = tmp_next;
	    }
	  while (tmp != NULL);
	  sctk_chunks_remove_inside (chunk, tls);
	  SCTK_DEBUG (sctk_mem_error
		      ("USE page %p\n",
		       ((sctk_page_t *) sctk_small_page_align (chunk))));
	  SCTK_DEBUG (sctk_mem_assert
		      (((sctk_page_t *) sctk_small_page_align (chunk))->
		       page_state == sctk_inuse_state));
	  goto fin;
	}
    }
  else
    {
      sctk_free_chunk_t **chunks_tab;

      chunks_tab = tls->chunks;
      chunk = NULL;
      for (cursor = i; cursor < max_chunk_sizes; cursor++)
	{
	  chunk = chunks_tab[cursor];
	  if (chunk != NULL)
	    {
	      sctk_chunks_remove_inside (chunk, tls);
	      SCTK_DEBUG (sctk_mem_assert
			  (((sctk_page_t *) sctk_small_page_align (chunk))->
			   page_state == sctk_inuse_state));
	      goto fin;
	    }
	}
    }

  page = sctk_alloc_page_small (size, tls);
  SCTK_DEBUG (sctk_mem_error ("GET %p\n", page));
  SCTK_DEBUG (sctk_mem_assert (page->page_state == sctk_inuse_state));
  if (page == NULL)
    {
      goto fail_to_alloc;
    }

  page->related = tls;
  page->type = sctk_chunk_small_type;
  page->nb_ref = 0;

  chunk = sctk_get_chunk_from_page (page);
  chunk->tree = sctk_init_tree ();
  SCTK_DEBUG (sctk_mem_assert
	      (((sctk_page_t *) sctk_small_page_align (chunk))->
	       page_state == sctk_inuse_state));

fin:
  SCTK_DEBUG (sctk_mem_assert
	      (((sctk_page_t *) sctk_small_page_align (chunk))->page_state ==
	       sctk_inuse_state));

  new_size =
    sctk_aligned_size (((chunk->cur_size -
			 sctk_sizeof_sctk_malloc_chunk_t) / 2));

  SCTK_DEBUG (sctk_mem_error
	      ("CHUNK %lu(%lu)  NEXT %lu \n", chunk->cur_size, size,
	       new_size));

  stop_size = real_size;
  if (real_size < 1024)
    {
      stop_size = real_size * 4;
    }

  while (new_size > stop_size)
    {
      unsigned int tree;
      SCTK_DEBUG (sctk_mem_error
		  ("SPLIT CHUNK %lu(%lu)  NEXT %lu \n", chunk->cur_size, size,
		   new_size));
      tree = chunk->tree;

      next =
	(sctk_free_chunk_t *) sctk_chunk_split_verif ((sctk_malloc_chunk_t *)
						      chunk, new_size);
      if (next != NULL)
	{
	  SCTK_DEBUG (sctk_mem_error
		      ("SPLIT CHUNK %u -> (%u,%u) \n", tree,
		       sctk_left_branch (tree), sctk_right_branch (tree)));
	  chunk->tree = sctk_left_branch (tree);
	  next->tree = sctk_right_branch (tree);
	  sctk_free_chunk_small (next, tls);
	}
      else
	{
	  break;
	}

      new_size =
	sctk_aligned_size (((chunk->cur_size -
			     sctk_sizeof_sctk_malloc_chunk_t) / 2));

      SCTK_DEBUG (sctk_mem_error
		  ("CHUNK %lu(%lu)  NEXT %lu \n", chunk->cur_size, size,
		   new_size));
    }

  if ((chunk->cur_size > real_size + (real_size / 8)))
    {
      unsigned int tree;

      new_size = sctk_aligned_size (real_size);

      sctk_mem_assert (real_size <= new_size);

      SCTK_DEBUG (sctk_mem_error
		  ("RESPLIT CHUNK %lu(%lu)  NEXT %lu \n", chunk->cur_size,
		   size, new_size));
      tree = chunk->tree;

      next =
	(sctk_free_chunk_t *) sctk_chunk_split_verif ((sctk_malloc_chunk_t *)
						      chunk, new_size);
      if (next != NULL)
	{
	  SCTK_DEBUG (sctk_mem_error
		      ("SPLIT CHUNK %u -> (%u,%u) \n", tree,
		       sctk_left_branch (tree), sctk_right_branch (tree)));
	  chunk->tree = sctk_left_branch (tree);
	  next->tree = sctk_right_branch (tree);
	  sctk_free_chunk_small (next, tls);
	}
    }

  SCTK_DEBUG (sctk_mem_error ("%lu lost\n", chunk->cur_size - real_size));

  chunk->state = sctk_inuse_state;
  sctk_mem_assert (real_size <= chunk->cur_size);

  sctk_mem_assert (((sctk_page_t *) sctk_small_page_align (chunk))->
		   page_state == sctk_inuse_state);

  return chunk;

fail_to_alloc:
  return NULL;
}

static void *
sctk_alloc_ptr_small (sctk_size_t real_size, sctk_tls_t * tls)
{
  sctk_free_chunk_t *chunk;
  sctk_size_t size;
  sctk_page_t *page;
  int i;

  if (real_size < chunk_sizes[0])
    {
      real_size = chunk_sizes[0];
    }

  /*Compute rank in buffer tab */
  if (real_size == 0)
    i = 0;
  else
    i = sctk_get_chunk_tab (real_size);


  sctk_mem_assert (i < max_chunk_sizes);
  if (chunk_sizes[i] < real_size)
    {
      return sctk_alloc_ptr_big (real_size, tls);
    }

  size = chunk_sizes[i];

  /* if (size < 512) */
  if (size <= 1024)
    {
      chunk = sctk_chunks_remove (i, tls);

      if (chunk == NULL)
	{
	  SCTK_DEBUG (sctk_mem_error ("search %lu %d\n", size, i));
	  chunk = sctk_alloc_ptr_small_search (size, tls, i, real_size);
	  if (chunk == NULL)
	    {
	      return NULL;
	    }
	}
    }
  else
    {
      chunk = sctk_alloc_ptr_small_search (size, tls, i, real_size);
      if (chunk == NULL)
	{
	  return NULL;
	}
    }

  chunk->state = sctk_inuse_state;
  chunk->type = sctk_chunk_small_type;

  SCTK_DEBUG (sctk_mem_error
	      ("LOST SPACE %lu real size %lu %5.2f size %lu %d %lu\n",
	       chunk->cur_size - real_size, real_size,
	       (double) chunk->cur_size / (double) real_size, chunk->cur_size,
	       i, chunk_sizes[i]));

  i = sctk_get_chunk_tab (chunk->cur_size);
  if (chunk_sizes[i] > chunk->cur_size)
    i--;
  SCTK_DEBUG (sctk_mem_error ("FOUND %lu %d\n", chunk->cur_size, i));
  chunk->small_pos = i;

  sctk_mem_assert (chunk->cur_size >= real_size);
  SCTK_DEBUG (sctk_mem_error
	      ("FOUND %lu  %lu %lu %f%%\n", real_size, chunk->cur_size,
	       chunk->cur_size - real_size,
	       ((double) (chunk->cur_size - real_size)) / (double) real_size *
	       100));

  page = (sctk_page_t *) sctk_small_page_align (chunk);
  page->nb_ref++;
  page->cur_dirty += chunk->cur_size;
  if (page->cur_dirty > page->dirty)
    {
      page->dirty = page->cur_dirty;
    }

  return sctk_get_ptr ((sctk_malloc_chunk_t *) chunk);
}

static void
sctk_free_small (sctk_free_chunk_t * chunk, sctk_size_t size,
		 sctk_tls_t * my_tls)
{
  sctk_tls_t *tls;
  sctk_page_t *page;
  LOG_IN ();

  page = (sctk_page_t *) sctk_small_page_align (chunk);
  tls = page->related;

  if (tls == my_tls)
    {
      SCTK_DEBUG (sctk_mem_error ("libere %p\n", chunk));
      sctk_free_chunk_small_local (chunk, my_tls);
      sctk_small_page_compaction (chunk, my_tls);
    }
  else
    {
      /*Gestion des libération distantes */
      sctk_mutex_lock (&tls->lock);
      chunk->next_chunk = (sctk_free_chunk_t *) tls->chunks_distant;
      tls->chunks_distant = chunk;
      sctk_mutex_unlock (&tls->lock);
    }

  LOG_OUT ();
}

static void
sctk_free_small_distant (sctk_tls_t * tls)
{
  if (tls->chunks_distant != NULL)
    {
      sctk_free_chunk_t *chunks_distant;
      sctk_free_chunk_t *tmp;
      /*Gestion des libération distantes */
      sctk_mutex_lock (&tls->lock);
      if (tls->chunks_distant != NULL)
	{
	  chunks_distant = (sctk_free_chunk_t *) (tls->chunks_distant);
	  tls->chunks_distant = NULL;
	  sctk_mutex_unlock (&tls->lock);
	  while (chunks_distant != NULL)
	    {
	      tmp = chunks_distant->next_chunk;
	      sctk_free_chunk_small (chunks_distant, tls);
	      chunks_distant = tmp;
	    }
	}
      else
	{
	  sctk_mutex_unlock (&tls->lock);
	}
    }
}

/*******************************************/
/*************** ALLOC TINY    *************/
/*******************************************/



/*******************************************/
/***************    ALLOC GEN  *************/
/*******************************************/

static void *sctk_alloc_ptr_new (sctk_size_t size, sctk_tls_t * tls);

static void *
sctk_alloc_ptr (sctk_size_t size, sctk_tls_t * tls)
{
  void *tmp;
  size = sctk_aligned_size (size);
  SCTK_DEBUG (sctk_mem_error ("%lu\n", size));

  if ((unsigned long) size <=
      (unsigned long) chunk_sizes[max_chunk_sizes - 1])
    {
      tmp = sctk_alloc_ptr_small (size, tls);
    }
  else
    {
      tmp = sctk_alloc_ptr_big (size, tls);
    }
  return tmp;
}

static void *
sctk_alloc_ptr_new (sctk_size_t size, sctk_tls_t * tls)
{
  sctk_malloc_chunk_t *chunk;
  sctk_page_t *page = NULL;
  void *tmp;

#ifdef SCTK_COHERENCY_CHECK
  sctk_size_t init_size;
#endif

  if (chunk_sizes[0] > size)
    {
      size = chunk_sizes[0];
    }

#ifdef SCTK_COHERENCY_CHECK
  init_size = size;
  size = sctk_coherency_check_extend_size (size);
#endif

  page = sctk_create_page (size);
  page->related = tls;
  sctk_insert_used_page (page, tls);
  sctk_mem_assert (page->real_size > 0);

  chunk = (sctk_malloc_chunk_t *) sctk_get_chunk_from_page (page);

  sctk_chunk_split (chunk, size);

  chunk->state = sctk_inuse_state;

  chunk->type = sctk_chunk_new_type;
  sctk_check_alignement (chunk);
  tmp = sctk_get_ptr ((sctk_malloc_chunk_t *) chunk);

#ifdef SCTK_COHERENCY_CHECK
  tmp = sctk_coherency_check_prepare_chunk (tmp, init_size);
#endif
  return tmp;
}

static void
sctk_free_new (sctk_free_chunk_t * chunk, sctk_size_t size,
	       sctk_tls_t * my_tls)
{
  sctk_tls_t *tls;
  sctk_page_t *page;
  LOG_IN ();

  page = sctk_get_page_from_chunk (chunk);
  tls = page->related;

  page = sctk_init_page (page, page->real_size);
  page->related = tls;
  page->next_chunk = NULL;
  page->prev_chunk = NULL;
  sctk_mem_assert (page->real_size > 0);

  sctk_remove_used_page (page, tls);
  sctk_empty_free_page (page);
  page->page_state = sctk_free_state;

  LOG_OUT ();
}

/*******************************************/
/*************** __sctk_*alloc *************/
/*******************************************/

#ifdef SCTK_COHERENCY_CHECK
#warning "Enable hard coherency checking"


#define SCTK_COHERENCY_CHECK_SIZE 24
/* #define SCTK_COHERENCY_CHECK_SIZE 240 */
typedef struct
{
  sctk_size_t size;
  char word[SCTK_COHERENCY_CHECK_SIZE];
} sctk_coherency_check_t;
static char *sctk_coherency_check_init_val = "Je suis un chunk correct";

static void
sctk_coherency_check_init_check (sctk_coherency_check_t * check,
				 sctk_size_t size)
{
  size_t i;
  size_t init_size;

  init_size = strlen (sctk_coherency_check_init_val);

  check->size = size;

  for (i = 0; (i < SCTK_COHERENCY_CHECK_SIZE) && (i < init_size); i++)
    {
      check->word[i] = sctk_coherency_check_init_val[i];
    }
  for (; i < SCTK_COHERENCY_CHECK_SIZE; i++)
    {
      check->word[i] = '@';
    }
}

static void
sctk_coherency_check_verif_check (sctk_coherency_check_t * check, void *tmp)
{
  size_t i;
  size_t init_size;

  init_size = strlen (sctk_coherency_check_init_val);

  for (i = 0; (i < SCTK_COHERENCY_CHECK_SIZE) && (i < init_size); i++)
    {
      if (check->word[i] != sctk_coherency_check_init_val[i])
	{
	  sctk_mem_error ("Check fail for chunk %p in %d size %lu\n", tmp, i,
			  check->size);
/* 	  sctk_debug("|%s| != |%s|",check->word,sctk_coherency_check_init_val); */
/* 	  while(1); */
	  sctk_mem_assert (0);
	  abort ();
	}
    }
  for (; i < SCTK_COHERENCY_CHECK_SIZE; i++)
    {
      if (check->word[i] != '@')
	{
	  sctk_mem_error ("Check fail for chunk %p in %d\n", tmp, i);
	  sctk_mem_assert (0);
	  abort ();
	}
    }
}

static sctk_size_t
sctk_coherency_check_extend_size (sctk_size_t size)
{

  size = sctk_aligned_size (size) + 2 * sctk_sizeof (sctk_coherency_check_t);
  return size;
}

static void *
sctk_coherency_check_prepare_chunk (void *tmp, sctk_size_t size)
{
  char *tmp_val;
  size_t chunk_size;
  SCTK_MAKE_WRITABLE (tmp, sizeof (sctk_coherency_check_t));
  size = sctk_aligned_size (size);

  chunk_size = sctk_sizeof (sctk_coherency_check_t);

  sctk_coherency_check_init_check ((sctk_coherency_check_t *) tmp, size);
  tmp_val = (char *) tmp + chunk_size;

  SCTK_MAKE_WRITABLE (tmp_val + size, sizeof (sctk_coherency_check_t));
  sctk_coherency_check_init_check ((sctk_coherency_check_t *) (tmp_val +
							       size), size);
  SCTK_MAKE_NOACCESS (tmp_val + size, sizeof (sctk_coherency_check_t));

  SCTK_MAKE_NOACCESS (tmp, sizeof (sctk_coherency_check_t));
  return tmp_val;
}
static void *
sctk_coherency_check_verif_chunk (void *tmp)
{
  char *tmp_val;
  size_t chunk_size;
  sctk_size_t size;
  sctk_coherency_check_t *start;
  sctk_coherency_check_t *end;
  sctk_free_chunk_t *chunk;

  chunk_size = sctk_sizeof (sctk_coherency_check_t);
  tmp_val = (char *) tmp - chunk_size;
  chunk = (sctk_free_chunk_t *) sctk_get_chunk (tmp_val);


  start = (sctk_coherency_check_t *) tmp_val;
  SCTK_MAKE_READABLE (start, sizeof (sctk_coherency_check_t));
  size = start->size;

  end = (sctk_coherency_check_t *) (((char *) tmp) + size);
  SCTK_DEBUG (sctk_mem_error ("CHECK %p %p %p\n", tmp, start, end));

  SCTK_MAKE_READABLE (end, sizeof (sctk_coherency_check_t));
  sctk_coherency_check_verif_check (start, tmp);
  sctk_coherency_check_verif_check (end, tmp);
  SCTK_MAKE_NOACCESS (start, sizeof (sctk_coherency_check_t));
  SCTK_MAKE_NOACCESS (end, sizeof (sctk_coherency_check_t));

  return tmp_val;
}
#endif

static mpc_inline void *
__intern_sctk_malloc__ (sctk_size_t size, sctk_tls_t * tls)
{
  void *tmp;
  sctk_size_t init_size;
  init_size = size;

  sctk_spinlock_lock(&(tls->spinlock));

  if (size >= (unsigned long) LONG_MAX)
    {
      errno = ENOMEM;
      sctk_spinlock_unlock(&(tls->spinlock));
      return NULL;
    }

#ifdef SCTK_COHERENCY_CHECK
  size = sctk_coherency_check_extend_size (size);
#endif

  sctk_free_small_distant (tls);
  sctk_free_big_distant (tls);

  tmp = sctk_alloc_ptr (size, tls);
#ifdef SCTK_COHERENCY_CHECK
  sctk_add_block (init_size, sctk_get_chunk (tmp));
#else
  sctk_add_block (size, sctk_get_chunk (tmp));
#endif

#ifdef SCTK_COHERENCY_CHECK
  tmp = sctk_coherency_check_prepare_chunk (tmp, init_size);
#endif
  SCTK_DEBUG (sctk_mem_error ("Create %p, size %lu\n", tmp, size));
  SCTK_DEBUG (sctk_dump_memory_heap ());
  sctk_spinlock_unlock(&(tls->spinlock));
  return tmp;
}

static mpc_inline void *
__intern_sctk_malloc (sctk_size_t size, sctk_tls_t * tls)
{
  void *tmp;
  tmp = __intern_sctk_malloc__ (size, tls);
  SCTK_MALLOCLIKE_BLOCK (tmp, size, sizeof (sctk_malloc_chunk_t), 0);
  return tmp;
}

static void *
__intern_sctk_calloc (sctk_size_t nmemb, sctk_size_t size, sctk_tls_t * tls)
{
  void *tmp;
  tmp = __intern_sctk_malloc (size * nmemb, tls);
  memset (tmp, 0, size * nmemb);
#ifdef SCTK_COHERENCY_CHECK
  sctk_coherency_check_verif_chunk (tmp);
#endif
  return tmp;
}

static void
__intern_sctk_free__ (void *ptr, sctk_tls_t * my_tls)
{
  sctk_free_chunk_t *chunk;
  sctk_size_t size;

  SCTK_DEBUG (sctk_mem_error ("Free %p\n", ptr));
  sctk_spinlock_lock(&(my_tls->spinlock));

#ifdef SCTK_COHERENCY_CHECK
  ptr = sctk_coherency_check_verif_chunk (ptr);
#endif

  chunk = (sctk_free_chunk_t *) sctk_get_chunk (ptr);
  SCTK_MAKE_READABLE (chunk, sizeof (sctk_free_chunk_t));
  SCTK_MAKE_NOACCESS (chunk, chunk->cur_size);
  SCTK_MAKE_READABLE (chunk, sizeof (sctk_free_chunk_t));

  sctk_check_chunk_coherency ((sctk_malloc_chunk_t *) chunk);

  size = chunk->cur_size;
  sctk_del_block (chunk->real_size_frac);

  if (size <= 0)
    {
      sctk_print_chunk ((sctk_malloc_chunk_t *) chunk);
      sctk_mem_assert (size > 0);
    }

  if (chunk->type == sctk_chunk_small_type)
    {
      SCTK_TRACE_START (__intern_sctk_free_small, ptr, my_tls, NULL, NULL);
      SCTK_DEBUG (sctk_mem_error ("--- Small type %p\n", ptr));
      sctk_free_small (chunk, size, my_tls);

      SCTK_DEBUG (sctk_mem_error
		  ("<-- Free chunk %p size %lu DONE\n", chunk,
		   chunk->cur_size));
      SCTK_TRACE_END (__intern_sctk_free_small, ptr, my_tls, NULL, NULL);
    }
  else if (chunk->type == sctk_chunk_big_type)
    {
      SCTK_TRACE_START (__intern_sctk_free_big, ptr, my_tls, NULL, NULL);
      SCTK_DEBUG (sctk_mem_error ("--- Big type %p\n", ptr));
      sctk_mem_assert (chunk->type == sctk_chunk_big_type);
      sctk_free_big (chunk, size, my_tls);
      SCTK_TRACE_END (__intern_sctk_free_big, ptr, my_tls, NULL, NULL);
    }
  else if (chunk->type == sctk_chunk_new_type)
    {
      SCTK_TRACE_START (__intern_sctk_free_new, ptr, my_tls, NULL, NULL);
      SCTK_DEBUG (sctk_mem_error ("--- New type\n"));
      sctk_mem_assert (chunk->type == sctk_chunk_new_type);
      sctk_free_new (chunk, size, my_tls);
      SCTK_TRACE_END (__intern_sctk_free_new, ptr, my_tls, NULL, NULL);
    }
  else if (chunk->type == sctk_chunk_memalign_type)
    {
      void *new_ptr;
      unsigned long *val;
      SCTK_TRACE_START (__intern_sctk_free_memalign, ptr, my_tls, NULL, NULL);

      val = (unsigned long *) ((char *) chunk - sizeof (unsigned long));

#ifdef SCTK_COHERENCY_CHECK
      ptr = (char *) ptr + sctk_sizeof (sctk_coherency_check_t);
#endif
      new_ptr = (char *) ptr - (*val);

      sctk_spinlock_unlock(&(my_tls->spinlock));
      __intern_sctk_free__ (new_ptr, my_tls);
      sctk_spinlock_lock(&(my_tls->spinlock));
      SCTK_TRACE_END (__intern_sctk_free_memalign, ptr, my_tls, NULL, NULL);
    }
  else
    {
      not_reachable ();
    }

  sctk_spinlock_unlock(&(my_tls->spinlock));
  SCTK_DEBUG (sctk_mem_error ("FREE %p done\n", ptr));
}
static void
__intern_sctk_free (void *ptr, sctk_tls_t * my_tls)
{
  SCTK_FREELIKE_BLOCK (ptr, sizeof (sctk_free_chunk_t));
  __intern_sctk_free__ (ptr, my_tls);
}

static void *
__intern_sctk_realloc (void *ptr, sctk_size_t size, sctk_tls_t * tls)
{
  void *tmp;

  if (size == 0)
    {
      sctk_free (ptr);
      return NULL;
    }

  if (ptr != NULL)
    {
      sctk_free_chunk_t *chunk;
      sctk_size_t old_size;

#ifdef SCTK_COHERENCY_CHECK
      sctk_size_t init_size;
      SCTK_FREELIKE_BLOCK (ptr, sizeof (sctk_free_chunk_t));
      init_size = size;
      size = sctk_coherency_check_extend_size (size);
      ptr = sctk_coherency_check_verif_chunk (ptr);
#else
      SCTK_FREELIKE_BLOCK (ptr, sizeof (sctk_free_chunk_t));
#endif

      chunk = (sctk_free_chunk_t *) sctk_get_chunk (ptr);

      assume((chunk->type == sctk_chunk_small_type) || (chunk->type == sctk_chunk_big_type));

      SCTK_MAKE_READABLE (chunk, sizeof (sctk_free_chunk_t));
      old_size = chunk->cur_size;

      SCTK_DEBUG (sctk_mem_error ("Old size %lu  new size %lu\n", old_size,size));

      if (old_size >= size)
	{
	  if (((old_size < 2 * size) ||
	       ((chunk->type == sctk_chunk_big_type)
		&& (old_size < size * 2))))
	    {
	      sctk_del_block (chunk->real_size_frac);
#ifdef SCTK_COHERENCY_CHECK
	      sctk_add_block (init_size, (sctk_malloc_chunk_t *) chunk);
#else
	      sctk_add_block (size, (sctk_malloc_chunk_t *) chunk);
#endif
	      /*Nothing to do */
#ifdef SCTK_COHERENCY_CHECK
	      ptr = sctk_coherency_check_prepare_chunk (ptr, init_size);
#endif
	    }
	  else
	    {
	      SCTK_DEBUG (sctk_mem_error ("Realloc pointer\n"));

	      tmp = __intern_sctk_malloc__ (size, tls);
	      SCTK_DEBUG (sctk_mem_error
			  ("New size %lu \n",
			   sctk_get_chunk (tmp)->cur_size));

#ifdef SCTK_COHERENCY_CHECK
	      sctk_coherency_check_verif_chunk (tmp);
	      ptr = (char *) ptr + sctk_sizeof (sctk_coherency_check_t);
	      memcpy (tmp, ptr, init_size);
	      sctk_coherency_check_verif_chunk (tmp);
#else
	      memcpy (tmp, ptr, size);
#endif
	      __intern_sctk_free__ (ptr, tls);
	      ptr = tmp;
	    }
	  SCTK_MALLOCLIKE_BLOCK (ptr, size, sizeof (sctk_malloc_chunk_t), 0);
	  return ptr;
	}
      else
	{
	  if((chunk->type == sctk_chunk_big_type)){
	    sctk_page_t *page;
	    page = sctk_get_page_from_chunk (chunk);
	    if(page->real_size > (size + 4 * sctk_sizeof_sctk_page_t)){
	      sctk_chunk_merge((sctk_malloc_chunk_t *)chunk);
	      sctk_chunk_split ((sctk_malloc_chunk_t *)chunk, size);
#ifdef SCTK_COHERENCY_CHECK
	      ptr = sctk_coherency_check_prepare_chunk (ptr, init_size);
#endif
	      return ptr;
	    }
	  }
	  SCTK_DEBUG (sctk_mem_error
		      ("Realloc pointer %lu -> %lu\n", old_size, size));

	  tmp = __intern_sctk_malloc__ (size, tls);
	  SCTK_DEBUG (sctk_mem_error
			    ("Old size %lu New size %lu \n", old_size,sctk_get_chunk (tmp)->cur_size));

#ifdef SCTK_COHERENCY_CHECK
	  ptr = (char *) ptr + sctk_sizeof (sctk_coherency_check_t);
#endif
	  memcpy (tmp, ptr, old_size);

	  __intern_sctk_free__ (ptr, tls);
	  SCTK_MALLOCLIKE_BLOCK (tmp, size, sizeof (sctk_malloc_chunk_t), 0);
	}
    }
  else
    {
      tmp = __intern_sctk_malloc (size, tls);
    }
  return tmp;
}

static int
__intern_sctk_posix_memalign (void **memptr, sctk_size_t alignment,
			      sctk_size_t size, sctk_tls_t * tls)
{
  void *tmp;
  unsigned long alignment_tmp;
  sctk_size_t init_size;
  sctk_size_t min_offset;
  init_size = size;
#ifdef SCTK_COHERENCY_CHECK
  size =
    sctk_coherency_check_extend_size (size +
				      4 *
				      sctk_sizeof (sctk_coherency_check_t));
#endif

  if ((alignment % sizeof (void *) != 0) || (alignment == 0))
    return EINVAL;

  alignment_tmp = alignment;
  while (alignment_tmp >= 2)
    {
      if (alignment_tmp % 2 != 0)
	return EINVAL;
      alignment_tmp = alignment_tmp / 2;
    }

  tmp =
    __intern_sctk_malloc (size + 2 * alignment + 2 * chunk_sizes[0] +
			  5 * sctk_sizeof_sctk_page_t, tls);
  if (((unsigned long) tmp % (unsigned long) alignment != 0) && (tmp != NULL))
    {
      void *tmp_begin;
      sctk_malloc_chunk_t *chunk;
      sctk_malloc_chunk_t chunk_sav;
      unsigned long offset;
      sctk_malloc_chunk_t *chunk_new;
      unsigned long *val;

      tmp_begin = tmp;
#ifdef SCTK_COHERENCY_CHECK
      tmp_begin = sctk_coherency_check_verif_chunk (tmp_begin);
#endif

      chunk = sctk_get_chunk (tmp_begin);

      chunk_sav = *chunk;

      SCTK_DEBUG (sctk_mem_error ("Align %lu rest %lu\n", alignment,
				  (unsigned long) tmp %
				  (unsigned long) alignment));

      offset =
	(unsigned long) alignment -
	((unsigned long) tmp % (unsigned long) alignment);

      min_offset = 2 * sctk_sizeof_sctk_malloc_chunk_t;
#ifdef SCTK_COHERENCY_CHECK
      min_offset = 2 * (sctk_sizeof_sctk_malloc_chunk_t + sctk_sizeof (sctk_coherency_check_t));
#endif

      if (offset < min_offset)
	{
	  offset += min_offset;
	  offset +=
	    (unsigned long) alignment -
	    (((unsigned long) tmp + offset) % (unsigned long) alignment);
	}


      SCTK_DEBUG (sctk_mem_error
		  ("Align %lu rest %lu offset %lu\n", alignment,
		   (unsigned long) tmp % (unsigned long) alignment, offset));

      tmp_begin = (char *) tmp_begin + offset;
      chunk_new = (sctk_malloc_chunk_t *) (((char *) tmp_begin) -
					   sctk_sizeof_sctk_malloc_chunk_t);
      *chunk_new = chunk_sav;
      chunk_new->type = sctk_chunk_memalign_type;
      val = (unsigned long *) ((char *) chunk_new - sizeof (unsigned long));
      *val = offset;

#ifdef SCTK_COHERENCY_CHECK
      tmp_begin = sctk_coherency_check_prepare_chunk (tmp_begin, init_size);
#endif
      tmp = tmp_begin;

    }

  sctk_mem_assert ((unsigned long) tmp % (unsigned long) alignment == 0);

  *memptr = tmp;
  SCTK_DEBUG (sctk_mem_error ("Create memalign %p, size %lu\n", tmp, size));

  if (tmp != NULL)
    return 0;
  else
    return ENOMEM;
}

void *
__sctk_malloc (sctk_size_t size, sctk_tls_t * tls)
{
  void *tmp;
  SCTK_PROFIL_START (sctk_malloc);
  tmp = __intern_sctk_malloc (size, tls);
  SCTK_PROFIL_END (sctk_malloc);
  return tmp;
}

void *
__sctk_calloc (sctk_size_t nmemb, sctk_size_t size, sctk_tls_t * tls)
{
  void *tmp;
  SCTK_PROFIL_START (sctk_calloc);
  tmp = __intern_sctk_calloc (nmemb, size, tls);
  SCTK_PROFIL_END (sctk_calloc);
  return tmp;
}

void
__sctk_free (void *ptr, sctk_tls_t * tls)
{
  SCTK_PROFIL_START (sctk_free);
  __intern_sctk_free (ptr, tls);
  SCTK_PROFIL_END (sctk_free);
}

void *
__sctk_realloc (void *ptr, sctk_size_t size, sctk_tls_t * tls)
{
  void *tmp;
  SCTK_PROFIL_START (sctk_realloc);
  tmp = __intern_sctk_realloc (ptr, size, tls);
  SCTK_PROFIL_END (sctk_realloc);
  return tmp;
}

int
__sctk_posix_memalign (void **memptr, sctk_size_t alignment, sctk_size_t size,
		       sctk_tls_t * tls)
{
  return __intern_sctk_posix_memalign (memptr, alignment, size, tls);
}


/*******************************************/
/*************** dump_*alloc ***************/
/*******************************************/
void
sctk_show_used_pages ()
{
  sctk_tls_t *tls;
  sctk_page_t *page;
  size_t i;
  tls = __sctk_get_tls ();

  sctk_mem_error ("Pages list %d:\n", tls->used_pages_number);
  page = tls->used_pages;
  for (i = 0; i < tls->used_pages_number; i++)
    {
      sctk_mem_error ("    - %p size %luKo\n", (void *) page,
		      (page->real_size) / 1024);
      page = page->next_chunk_used;
    }
}


static mpc_inline void
sctk_write_ptr (int fd, void *buf, sctk_size_t count)
{
  SCTK_DEBUG (sctk_mem_error
	      ("WRITE PTR %p alloc done copy %lu\n", buf, count));
  sctk_safe_write (fd, &buf, sizeof (void *));
  sctk_safe_write (fd, &count, sizeof (sctk_size_t));

#ifdef SCTK_USE_COMPRESSION_FILE
  if (count % page_size == 0)
    {
      size_t i;
      char *ptr;
      ptr = (char *) buf;
      for (i = 0; i < count / page_size; i++)
	{
	  unsigned char vec;
	  static char empty_page[SCTK_COMPRESSION_BUFFER_SIZE];

	  vec = memcmp (ptr, empty_page, page_size);

	  sctk_safe_write (fd, &vec, sizeof (unsigned char));
	  if (vec)
	    {
	      sctk_safe_write (fd, ptr, page_size);
	    }
	  else
	    {
	      sctk_munmap (ptr, page_size);
	      sctk_mmap (ptr, page_size);
	    }
	  ptr += page_size;
	}
    }
  else
    {
      sctk_safe_write (fd, buf, count);
    }
#else
  sctk_safe_write (fd, buf, count);
#endif
}

static mpc_inline int
sctk_check_read (int fd, void *buf, sctk_size_t count)
{
  sctk_size_t readed;
  readed = read (fd, buf, count);
  return (count == readed);
}

static sctk_page_t *
sctk_alloc_block_get_owner_page (void *block_ptr)
{
  sctk_page_t *cursor;
  sctk_page_t *cursor_next;
  char *local_brk_pointer;
  char *ptr;
  local_brk_pointer = (char *) brk_pointer;
  ptr = init_brk_pointer;
  cursor = (sctk_page_t *) ptr;
  while ((unsigned long) ptr < (unsigned long) local_brk_pointer)
    {
      cursor_next = (sctk_page_t *) (ptr + cursor->real_size);
      if (((unsigned long) block_ptr) < ((unsigned long) cursor_next))
	{
	  assume ((unsigned long) block_ptr < (unsigned long) cursor_next);
	  assume ((unsigned long) block_ptr >= (unsigned long) cursor);
	  return cursor;
	}
      cursor = cursor_next;
      ptr = (char *) cursor;
    }
  not_reachable ();
  return NULL;
}

static mpc_inline void
sctk_alloc_block_init_new_page (sctk_page_t * new_page,
				sctk_size_t new_page_size)
{
  new_page = sctk_init_page (new_page, new_page_size);
  new_page->dirty = 0;
  new_page->init_page = NULL;
}

static mpc_inline sctk_page_t *
sctk_alloc_block (void *ptr, sctk_size_t size)
{
  char *tmp_ptr;
  sctk_page_t *cursor = NULL;
  sctk_page_t *owner_page = NULL;
  char *local_brk_pointer;
  sctk_size_t real_size;

  SCTK_DEBUG (sctk_mem_error
	      ("NEW ptr %p-%p size %fMO\n", ptr, (char *) ptr + size,
	       size / (1024.0 * 1024.0)));

/*   sctk_alloc_spinlock_lock (&sctk_sbrk_lock); */
  sctk_spinlock_write_lock(&sctk_sbrk_rwlock);

/*   cursor = sctk_sbrk_compaction (-1); */
/*   assume (cursor == NULL); */

  tmp_ptr = init_brk_pointer;
  local_brk_pointer = (char *) brk_pointer;

  cursor = (sctk_page_t *) tmp_ptr;

  if (((unsigned long) ptr + size <= (unsigned long) init_brk_pointer) ||
      ((unsigned long) ptr >= (unsigned long) max_brk_pointer))
    {
      void *tmp;
      char *to_free;
      /*Allocation avant le pointeur local ou apres le max */

      to_free = ptr;
      real_size = size;

      tmp = sctk_mmap ((void *) to_free, real_size);
      cursor = (sctk_page_t *) tmp;
    }
  else
    {
      if ((unsigned long) ptr < (unsigned long) init_brk_pointer)
	{
	  not_reachable ();
	}
      else if (((unsigned long) ptr < (unsigned long) max_brk_pointer) &&
	       ((unsigned long) ptr + size > (unsigned long) max_brk_pointer))
	{
	  not_reachable ();
	}
      else
	if (((unsigned long) ptr + size <= (unsigned long) local_brk_pointer))
	{
	  /*init_brk_pointer <= block <= local_brk_pointer */

	  /*retrouver la page contenant block */
	  owner_page = sctk_alloc_block_get_owner_page (ptr);

	  /*la decouper si elle est libre */
	  if (owner_page->page_state == sctk_free_state)
	    {
	      sctk_page_t *new_page;
	      sctk_size_t new_page_size;
	      sctk_size_t old_size;
	      SCTK_DEBUG (sctk_mem_error ("Owner page %p\n", owner_page));

	      old_size = owner_page->real_size;

	      new_page = owner_page;
	      new_page_size =
		((unsigned long) ptr - (unsigned long) owner_page);
	      if (new_page_size > 0)
		{
		  sctk_alloc_block_init_new_page (new_page, new_page_size);
		  new_page->page_state = sctk_free_state;
		}

	      new_page = ptr;
	      new_page_size = size;
	      sctk_alloc_block_init_new_page (new_page, new_page_size);
	      cursor = new_page;

	      new_page = (void *) ((char *) ptr + size);
	      new_page_size =
		(((unsigned long) owner_page + old_size) -
		 ((unsigned long) ptr + size));
	      if (new_page_size > 0)
		{
		  sctk_alloc_block_init_new_page (new_page, new_page_size);
		  new_page->page_state = sctk_free_state;
		}
	    }
	  else
	    {
	      cursor = ptr;
	      /*Si elle est occupée alors la taille doit etre bonne */
	      if (owner_page->real_size != size)
		{
		  not_reachable ();
		}
	    }
	}
      else if (((unsigned long) ptr < (unsigned long) local_brk_pointer) &&
	       ((unsigned long) ptr + size >=
		(unsigned long) local_brk_pointer))
	{
	  /*cas ou block a cheval sur local_brk_pointer */

	  /*La dernière page doit être libre!!!! */
	  owner_page = sctk_alloc_block_get_owner_page (ptr);

	  if (owner_page->page_state == sctk_free_state)
	    {
	      not_implemented ();
	    }
	  else
	    {
	      not_reachable ();
	    }
	}
      else
	{
	  /*Cas ou block apres local_brk_pointer */

	  /*
	     Creation de deux pages:
	     - une de local_brk_pointer à ptr
	     - une pour le block
	   */
	  sctk_page_t *new_page;
	  sctk_size_t new_page_size;
	  void *tmp;

	  SCTK_DEBUG (sctk_mem_error
		      ("ptr %p-%p\n", ptr, (char *) ptr + size));
	  SCTK_DEBUG (sctk_mem_error
		      ("local_brk_pointer %p\n", local_brk_pointer));

	  new_page = (sctk_page_t *) local_brk_pointer;
	  new_page_size =
	    ((unsigned long) ptr + size) -
	    ((unsigned long) local_brk_pointer);

	  tmp = sctk_mmap ((void *) local_brk_pointer, new_page_size);
	  local_brk_pointer = (void *) ((char *) ptr + size);
	  new_page_size -= size;

	  SCTK_DEBUG (sctk_mem_error
		      ("new page size %fMo\n",
		       (new_page_size) / (1024.0 * 1024.0)));
	  sctk_alloc_block_init_new_page (new_page, new_page_size);
	  new_page->page_state = sctk_free_state;

	  new_page = ptr;
	  new_page_size = size;
	  SCTK_DEBUG (sctk_mem_error
		      ("new page size %fMo\n",
		       (new_page_size) / (1024.0 * 1024.0)));
	  sctk_alloc_block_init_new_page (new_page, new_page_size);
	  cursor = new_page;
	}
    }

  brk_pointer = local_brk_pointer;
/*   sctk_alloc_spinlock_unlock (&sctk_sbrk_lock); */
  sctk_spinlock_write_unlock(&sctk_sbrk_rwlock);
  return cursor;
}

static mpc_inline void
sctk_read_ptr (int fd, void **buf, sctk_size_t * count)
{
  sctk_page_t *cursor;
  sctk_safe_read (fd, buf, sizeof (void *));
  sctk_safe_read (fd, count, sizeof (sctk_size_t));

  SCTK_DEBUG (sctk_mem_error ("READ PTR %p\n", *buf));
  cursor = sctk_alloc_block (*buf, *count);

  SCTK_DEBUG (sctk_mem_error
	      ("READ PTR %p alloc done copy %lu\n", *buf, *count));

#ifdef SCTK_USE_COMPRESSION_FILE
  memset (cursor, 0, *count);
  cursor->page_state = sctk_free_state;
  assume (cursor == *buf);
  if (*count % page_size == 0)
    {
      size_t i;
      char *ptr;
      ptr = (char *) (*buf);
      for (i = 0; i < (*count) / page_size; i++)
	{
	  unsigned char vec;
	  sctk_safe_read (fd, &vec, sizeof (unsigned char));
	  if (vec)
	    {
	      sctk_safe_read (fd, ptr, page_size);
	    }
	  else
	    {
	      sctk_munmap (ptr, page_size);
	      sctk_mmap (ptr, page_size);
	    }
	  ptr += page_size;
	}
    }
  else
    {
      sctk_safe_read (fd, *buf, *count);
    }
#else
  memset (cursor, 0, *count);
  cursor->page_state = sctk_free_state;
  assume (cursor == *buf);
  sctk_safe_read (fd, *buf, *count);
#endif
  assume (cursor->page_state != sctk_free_state);
}

static mpc_inline void
sctk_alloc_block_update (void *ptr, sctk_size_t size)
{
  sctk_page_t *page;
  page = sctk_alloc_block (ptr, size);
/*   sctk_munmap (ptr, size); */
}

static mpc_inline void
sctk_read_ptr_update (int fd, void **buf, sctk_size_t * count)
{
  sctk_safe_read (fd, buf, sizeof (void *));
  sctk_safe_read (fd, count, sizeof (sctk_size_t));

  SCTK_DEBUG (sctk_mem_error
	      ("TRY Update pages %p %lu\n", *buf, (unsigned long) *count));
  sctk_alloc_block_update (*buf, *count);
  SCTK_DEBUG (sctk_mem_error
	      ("DONE Update pages %p %lu\n", *buf, (unsigned long) *count));
#ifdef SCTK_USE_COMPRESSION_FILE
  if (*count % page_size == 0)
    {
      size_t i;
      char *ptr;
      ptr = (char *) (*buf);
      for (i = 0; i < (*count) / page_size; i++)
	{
	  unsigned char vec;
	  sctk_safe_read (fd, &vec, sizeof (unsigned char));
	  if (vec)
	    {
	      sctk_safe_read (fd, ptr, page_size);
	    }
	  ptr += page_size;
	}
    }
  else
    {
      sctk_safe_read (fd, *buf, *count);
    }
#else
  sctk_safe_read (fd, *buf, *count);
#endif

}

static mpc_inline int
sctk_read_ptr_check (int fd, void **buf, sctk_size_t * count)
{
  sctk_size_t i;
  char c[4096];
  int tmp;
  sctk_size_t max_size;
  sctk_size_t total;
  tmp = sctk_check_read (fd, buf, sizeof (void *));
  if (!tmp)
    return 0;
  tmp = sctk_check_read (fd, count, sizeof (sctk_size_t));
  if (!tmp)
    return 0;

  total = *count;

  SCTK_DEBUG (sctk_mem_error ("READ %lu\n", total));

  max_size = (total) / 4096;
  max_size = max_size * 4096;
  for (i = 0; i < max_size; i += 4096)
    {
#ifdef SCTK_USE_COMPRESSION_FILE
      if (*count % page_size == 0)
	{
	  char *ptr;
	  unsigned char vec;
	  ptr = (char *) (*buf);
	  sctk_safe_read (fd, &vec, sizeof (unsigned char));
	  tmp = 4096;
	  if (vec)
	    {
	      tmp = sctk_check_read (fd, c, 4096);
	    }
	  ptr += page_size;
	}
      else
	{
	  tmp = sctk_check_read (fd, c, 4096);
	}
#else
      tmp = sctk_check_read (fd, c, 4096);
      if (!tmp)
	return 0;
#endif
    }

  if ((total) - i > 0)
    {
      SCTK_DEBUG (sctk_mem_error ("READ LEFT %lu\n", total));
#ifdef SCTK_USE_COMPRESSION_FILE
      if (*count % page_size == 0)
	{
	  char *ptr;
	  unsigned char vec;
	  ptr = (char *) (*buf);
	  sctk_safe_read (fd, &vec, sizeof (unsigned char));
	  tmp = (total) - i;
	  if (vec)
	    {
	      tmp = sctk_check_read (fd, c, (total) - i);
	    }
	  ptr += page_size;
	}
      else
	{
	  tmp = sctk_check_read (fd, c, (total) - i);
	}
#else
      tmp = sctk_check_read (fd, c, (total) - i);
      if (!tmp)
	return 0;
#endif
    }

  return 1;
}

static mpc_inline void
sctk_read_ptr_no_alloc (int fd, void **buf, sctk_size_t * count)
{
  sctk_safe_read (fd, buf, sizeof (void *));
  sctk_safe_read (fd, count, sizeof (sctk_size_t));

  SCTK_DEBUG (sctk_mem_error ("Write %lu octets at %p\n", *count, *buf));
#ifdef SCTK_USE_COMPRESSION_FILE
  if (*count % page_size == 0)
    {
      size_t i;
      char *ptr;
      ptr = (char *) (*buf);
      for (i = 0; i < (*count) / page_size; i++)
	{
	  unsigned char vec;
	  sctk_safe_read (fd, &vec, sizeof (unsigned char));
	  if (vec)
	    {
	      sctk_safe_read (fd, ptr, page_size);
	    }
	  ptr += page_size;
	}
    }
  else
    {
      sctk_safe_read (fd, *buf, *count);
    }
#else
  sctk_safe_read (fd, *buf, *count);
#endif
}

static mpc_inline int
sctk_read_ptr_no_alloc_check (int fd, void **buf, sctk_size_t * count)
{
  return sctk_read_ptr_check (fd, buf, count);
}

static mpc_inline void
__sctk_dump_used_pages (int fd, void *user_data, sctk_size_t user_data_size,
			sctk_tls_t * tls)
{
  sctk_page_t *page;
  long nb;
  size_t i;
  sctk_static_var_t *static_var;

  SCTK_DEBUG (sctk_mem_error ("Dump pages tls %p\n", tls));
  sctk_safe_write (fd, &tls, sizeof (void *));
  nb = tls->used_pages_number;
  sctk_safe_write (fd, &nb, sizeof (long));

  page = tls->used_pages;
  SCTK_DEBUG (sctk_mem_error ("Dump %d pages\n", nb));
  for (i = 0; i < tls->used_pages_number; i++)
    {
      SCTK_DEBUG (sctk_mem_error
		  ("Dump pages %p %lu\n", page,
		   (unsigned long) page->real_size));
      sctk_write_ptr (fd, page, page->real_size);
      sctk_refresh_page_alloc (page);
      page = page->next_chunk_used;
    }

  sctk_write_ptr (fd, user_data, user_data_size);
  SCTK_DEBUG (sctk_mem_error
	      ("Dump user pages %p %lu\n", user_data,
	       (unsigned long) user_data_size));

  nb = tls->static_vars_number;
  sctk_safe_write (fd, &nb, sizeof (long));

  static_var = tls->static_vars;
  for (i = 0; i < tls->static_vars_number; i++)
    {
      SCTK_DEBUG (sctk_mem_error
		  ("Dump static data %p %lu\n", static_var->adr,
		   (unsigned long) static_var->size));
      sctk_write_ptr (fd, static_var->adr, static_var->size);
      static_var = static_var->next;
    }

  SCTK_DEBUG (sctk_mem_error ("Dump pages tls %p DONE\n", tls));
}

void
sctk_dump_used_pages (int fd, void *user_data, sctk_size_t user_data_size)
{
  sctk_tls_t *tls;
  tls = __sctk_get_tls ();
  __sctk_dump_used_pages (fd, user_data, user_data_size, tls);
}

int
sctk_check_used_pages (int fd)
{
  int i;
  long nb;
  void *page = NULL;
  int tmp;
  sctk_size_t page_size = 0;
  off_t offset;

  offset = lseek (fd, 0, SEEK_CUR);

  tmp = sctk_check_read (fd, &page, sizeof (void *));
  if (!tmp)
    {
      SCTK_DEBUG (sctk_mem_error ("QUIT 1\n"));
      return 0;
    }
  tmp = sctk_check_read (fd, &nb, sizeof (long));
  if (!tmp)
    {
      SCTK_DEBUG (sctk_mem_error ("QUIT 2\n"));
      return 0;
    }
  SCTK_DEBUG (sctk_mem_error ("Check %d pages\n", nb));

  for (i = 0; i < nb; i++)
    {
      tmp = sctk_read_ptr_check (fd, &page, &page_size);
      if (!tmp)
	{
	  SCTK_DEBUG (sctk_mem_error ("QUIT 3\n"));
	  return 0;
	}
      SCTK_DEBUG (sctk_mem_error
		  ("Check pages %p %lu\n", page, (unsigned long) page_size));
    }

  tmp = sctk_read_ptr_no_alloc_check (fd, &page, &page_size);
  if (!tmp)
    {
      SCTK_DEBUG (sctk_mem_error ("QUIT 4\n"));
      return 0;
    }

  tmp = sctk_check_read (fd, &nb, sizeof (long));
  if (!tmp)
    {
      SCTK_DEBUG (sctk_mem_error ("QUIT 5\n"));
      return 0;
    }
  for (i = 0; i < nb; i++)
    {
      tmp = sctk_read_ptr_check (fd, &page, &page_size);
      if (!tmp)
	{
	  SCTK_DEBUG (sctk_mem_error ("QUIT 6\n"));
	  return 0;
	}
    }



  tmp = (read (fd, &page, 1) == 0);
  if (!tmp)
    {
      SCTK_DEBUG (sctk_mem_error ("QUIT 7\n"));
      return 0;
    }

  sctk_mem_assert (lseek (fd, offset, SEEK_SET) == offset);
  return 1;
}

int
sctk_check_file (char *name)
{
  int fd;
  int tmp;
  fd = open (name, O_RDONLY);
  tmp = sctk_check_used_pages (fd);
  close (fd);
  return !tmp;
}

void
sctk_restore_used_pages (int fd, void **user_data,
			 sctk_size_t * user_data_size, sctk_tls_t ** tls)
{
  int i;
  long nb;
  void *page = NULL;
  sctk_size_t page_size = 0;

/*   sctk_alloc_spinlock_lock (&sctk_sbrk_lock); */
/*   sctk_sbrk_compaction (-1); */
/*   sctk_alloc_spinlock_unlock (&sctk_sbrk_lock); */


  SCTK_DEBUG (sctk_mem_error ("START RESTORE\n"));
  sctk_safe_read (fd, tls, sizeof (void *));
  SCTK_DEBUG (sctk_mem_error ("RESTORE pages tls %p\n", *tls));
  sctk_safe_read (fd, &nb, sizeof (long));
  SCTK_DEBUG (sctk_mem_error ("Restore %d pages\n", nb));

  for (i = 0; i < nb; i++)
    {
      sctk_read_ptr (fd, &page, &page_size);
      SCTK_DEBUG (sctk_mem_error
		  ("Restore pages %p %lu\n", page,
		   (unsigned long) page_size));
      sctk_refresh_page_alloc (page);
    }

  sctk_read_ptr_no_alloc (fd, user_data, user_data_size);
  SCTK_DEBUG (sctk_mem_error
	      ("Restore user pages %p %lu\n", *user_data,
	       (unsigned long) *user_data_size));

  sctk_safe_read (fd, &nb, sizeof (long));
  SCTK_DEBUG (sctk_mem_error ("Restore %d global vars\n", nb));

  for (i = 0; i < nb; i++)
    {
      sctk_read_ptr_no_alloc (fd, &page, &page_size);
      SCTK_DEBUG (sctk_mem_error
		  ("Restore glob var %p %lu\n", page,
		   (unsigned long) page_size));
    }

  sctk_mem_assert (read (fd, &page, 1) == 0);
  SCTK_DEBUG (sctk_mem_error ("RESTORE pages tls %p DONE\n", *tls));
}

void
sctk_add_global_var (void *adr, sctk_size_t size)
{
  sctk_tls_t *tls;
  sctk_static_var_t *static_var;

  tls = __sctk_get_tls ();
  tls->static_vars_number++;

  static_var =
    (sctk_static_var_t *) sctk_malloc (sctk_sizeof (sctk_static_var_t));

  static_var->adr = adr;
  static_var->size = size;
  static_var->next = tls->static_vars;

  tls->static_vars = static_var;
}

void
__sctk_dump_tls (sctk_tls_t * tls, char *file_name)
{
  int fd;
  char name[SCTK_MAX_FILENAME_SIZE];

  SCTK_DEBUG (sctk_mem_error ("TRY to dump TLS %s %p\n", file_name, tls));
  sprintf (name, "%s_tmp", file_name);
  fd =
    open (name, O_WRONLY | O_CREAT | O_TRUNC,
	  S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);

  __sctk_dump_used_pages (fd, NULL, 0, tls);

  close (fd);
  SCTK_DEBUG (sctk_mem_error
	      ("TRY to dump TLS %s %p DONE\n", file_name, tls));
  rename (name, file_name);
}

void
sctk_dump_tls (char *file_name)
{
  sctk_tls_t *tls;
  tls = __sctk_get_tls ();
  __sctk_dump_tls (tls, file_name);
}

void
__sctk_restore_tls (sctk_tls_t ** tls, char *file_name)
{
  int fd;
  void *ptr = NULL;
  sctk_size_t size;

  fd = -1;

  SCTK_DEBUG (sctk_mem_error ("TRY to restore TLS %s\n", file_name));

  do
    {
/* #ifdef MPC_Threads */
/*       sctk_thread_yield (); */
/* #else */
/*       sched_yield (); */
/* #endif */
      sleep(1);
      fd = open (file_name, O_RDONLY);
    }
  while (fd == -1);

  SCTK_DEBUG (sctk_mem_error ("TRY to restore TLS FD OK\n"));

  sctk_restore_used_pages (fd, &ptr, &size, tls);

  close (fd);
}

void
sctk_restore_tls (char *file_name)
{
  sctk_tls_t *tls;
  __sctk_restore_tls (&tls, file_name);
}

void
sctk_mem_reset_heap (sctk_size_t start, sctk_size_t max_size)
{
#ifdef MPC_Threads
#ifdef MPC_Message_Passing
  if (sctk_is_net_migration_available ())
    {
#endif
      sctk_size_t per_slot_mem_size;
      sctk_size_t start_tmp;
      char *new_brk;

      __sctk_get_tls ();

      start_tmp = sctk_compute_real_size ((unsigned long) start);
      if (start_tmp < start)
	{
	  start_tmp += SCTK_PAGES_ALLOC_SIZE;
	}

      per_slot_mem_size = max_size / ((sctk_size_t) sctk_process_number);
      per_slot_mem_size = per_slot_mem_size / page_size;
      per_slot_mem_size = per_slot_mem_size * page_size;
      per_slot_mem_size = sctk_compute_real_size (per_slot_mem_size);

      new_brk =
	(char *) (start + (((size_t) sctk_process_rank) * per_slot_mem_size));
      if (sctk_alloc_check_heap (new_brk) == 1)
	{
/* 	  sctk_alloc_spinlock_lock (&sctk_sbrk_lock); */
	  sctk_spinlock_write_lock(&sctk_sbrk_rwlock);
	  sctk_mem_assert (NULL != brk_pointer);
	  assume ((sctk_size_t) start >= (sctk_size_t) brk_pointer);

	  brk_pointer = new_brk;
	  max_pre_brk_pointer = init_brk_pointer;
	  init_brk_pointer = (char *) brk_pointer;

	  sctk_mem_assert (((unsigned long) init_brk_pointer) >=
			   ((unsigned long) brk_pointer));
	  brk_pointer = (char *) init_brk_pointer;
	  init_brk_pointer = (char *) brk_pointer;
	  first_free = (char *) brk_pointer;

	  max_brk_pointer = (char *) brk_pointer + per_slot_mem_size;

	  sctk_nodebug ("LOCAL SPACE %p-%p", init_brk_pointer,
			max_brk_pointer);

/* 	  sctk_alloc_spinlock_unlock (&sctk_sbrk_lock); */
	  sctk_spinlock_write_unlock(&sctk_sbrk_rwlock);
	}
      else
	{
	  sctk_warning ("Migration disabled");
#ifdef MPC_Message_Passing
	  sctk_set_net_migration_available (0);
#endif
	}
#ifdef MPC_Message_Passing
    }
#endif
#else
  not_available ();
#endif
#if defined(SCTK_ALLOC_INFOS) || 0
      sctk_mem_error ("RESET INIT BRK %p\n", init_brk_pointer);
      sctk_mem_error ("RESET BRK %p\n", brk_pointer);
      sctk_mem_error ("RESET MAX BRK %p\n", max_brk_pointer);
#endif
}

void
sctk_update_used_pages (int fd, void **user_data,
			sctk_size_t * user_data_size, sctk_tls_t ** tls)
{
  int i;
  long nb;
  void *page = NULL;
  sctk_size_t page_size = 0;

  sctk_safe_read (fd, tls, sizeof (void *));
  sctk_safe_read (fd, &nb, sizeof (long));
  SCTK_DEBUG (sctk_mem_error ("Update %d pages\n", nb));

  for (i = 0; i < nb; i++)
    {
      sctk_read_ptr_update (fd, &page, &page_size);
      SCTK_DEBUG (sctk_mem_error
		  ("Update pages %p %lu\n", page, (unsigned long) page_size));
    }

  sctk_read_ptr_no_alloc (fd, user_data, user_data_size);
  SCTK_DEBUG (sctk_mem_error
	      ("Update user pages %p %lu\n", *user_data,
	       (unsigned long) *user_data_size));

  sctk_safe_read (fd, &nb, sizeof (long));
  SCTK_DEBUG (sctk_mem_error ("Update %d global vars\n", nb));

  for (i = 0; i < nb; i++)
    {
      sctk_read_ptr_no_alloc_check (fd, &page, &page_size);
      SCTK_DEBUG (sctk_mem_error
		  ("Update glob var %p %lu\n", page,
		   (unsigned long) page_size));
    }

  sctk_mem_assert (read (fd, &page, 1) == 0);
}

void
__sctk_update_memory (char *file_name)
{

  int fd;
  void *ptr = NULL;
  sctk_size_t size;
  sctk_tls_t *tls;

  fd = open (file_name, O_RDONLY);

  sctk_update_used_pages (fd, &ptr, &size, &tls);

  close (fd);
}

#ifdef MPC_Threads
#ifndef MPC_USE_PAGE_MIGRATION

static mpc_inline void
__sctk_relocalise_memory (void *ptr, sctk_size_t size)
{
#warning "wrong behavior => disabled"
#if 0
  unsigned long pos;
  unsigned long i;
  void *tmp;
  int cur_cpu;

  cur_cpu = sctk_get_cpu ();
  assume (SCTK_ALLOC_MAX_CPU_NUMBER > cur_cpu);

  if (cur_cpu >= 0)
    {
      if ((sctk_is_numa_node ())
	  && (sctk_tmp_buffer_relocalise_memory[cur_cpu] != NULL))
	{
	  pos = (unsigned long) ptr;
	  pos = pos / (unsigned long) page_size;
	  pos = pos * (unsigned long) page_size;

	  for (i = pos; i < pos + size; i += (unsigned long) page_size)
	    {
	      memcpy (sctk_tmp_buffer_relocalise_memory[cur_cpu],
		      (char *) i, page_size);

	      sctk_munmap ((void *) i, page_size);
	      tmp = sctk_mmap ((void *) i, page_size);

	      sctk_mem_assert ((void *) tmp != (void *) (-1));
	      sctk_mem_assert ((void *) tmp == (void *) (i));

	      memcpy ((char *) i,
		      sctk_tmp_buffer_relocalise_memory[cur_cpu], page_size);
	    }
	}
    }
#endif
}
#else
static mpc_inline void
__sctk_relocalise_memory (void *ptr, sctk_size_t size)
{
  unsigned long pos;
  unsigned long ssize;
  unsigned long nodes;
  int node_nb;

  if (sctk_is_numa_node ())
    {
      pos = (unsigned long) ptr;
      pos = pos / (unsigned long) page_size;
      pos = pos * (unsigned long) page_size;

      ssize = (unsigned long) size;
      ssize = ssize / (unsigned long) page_size;
      ssize = ssize * (unsigned long) page_size;

      /*Compute node */
      node_nb = sctk_get_node_from_cpu (sctk_thread_get_vp ());
      nodes = 1UL << node_nb;

      if (mbind
	  ((void *) (pos), ssize, MPOL_BIND, &nodes, 64,
	   MPOL_MF_MOVE | MPOL_MF_STRICT) != 0)
	{
#ifndef NO_INTERNAL_ASSERT
	  perror ("Page Migration error");
#endif
	}
    }
}
#endif

void
sctk_relocalise_memory (void *ptr, sctk_size_t size)
{
  __sctk_relocalise_memory (ptr, size);
}

static mpc_inline void
__sctk_relocalise_used_pages (sctk_tls_t * tls)
{
  sctk_page_t *page;
  int i;
  int nb;

  if (sctk_is_numa_node ())
    {
      nb = tls->used_pages_number;

      page = tls->used_pages;
      SCTK_DEBUG (sctk_mem_error ("Dump %d pages\n", nb));
      for (i = 0; i < nb; i++)
	{
	  SCTK_DEBUG (sctk_mem_error
		      ("Relocalise pages %p %lu\n", page,
		       (unsigned long) page->real_size));
	  page->dirty = SCTK_xMO_TAB_SIZE * SCTK_PAGES_ALLOC_SIZE;
	  __sctk_relocalise_memory (page, page->real_size);
	  sctk_alloc_page_big_flush (tls);
	  page = page->next_chunk_used;
	}
    }
}

void
__sctk_relocalise_tls (sctk_tls_t * tls)
{
  __sctk_relocalise_used_pages (tls);
}

void
sctk_relocalise_tls ()
{
  sctk_tls_t *tls;
  tls = __sctk_get_tls ();
  __sctk_relocalise_used_pages (tls);
}


#endif
void
sctk_clean_memory ()
{
  sctk_tls_t *tls;
  tls = __sctk_get_tls ();
}

/*******************************************/
/*************** sctk_*alloc ***************/
/*******************************************/
void *
sctk_malloc (sctk_size_t size)
{
  void *tmp;
  sctk_tls_t *tls;

  SCTK_PROFIL_START (sctk_malloc);
  LOG_IN ();
  LOG_ARG (size, "size");
  tls = __sctk_get_tls ();
  tmp = __intern_sctk_malloc (size, tls);
#ifdef SCTK_MEMINFO
  do
    {
      unsigned long len;

      len = sctk_get_chunk (tmp)->cur_size;
      sctk_atomic_increment (&calls[idx_malloc]);
      sctk_atomic_add (&total[idx_malloc], len);
      sctk_atomic_add (&grand_total, len);
      if (len <= chunk_sizes[max_chunk_sizes - 1])
	sctk_atomic_increment (&histogram[len / step_chunk_sizes]);
      else
	sctk_atomic_increment (&large);
      sctk_atomic_increment (&calls_total);
      if (tmp == NULL)
	{
	  sctk_atomic_increment (&failed[idx_malloc]);
	}
      update_data (tmp, 0);
    }
  while (0);
#endif
  LOG_ARG (tmp, "ptr");
  LOG_OUT ();
  SCTK_PROFIL_END (sctk_malloc);
  return tmp;
}

void *
sctk_tmp_malloc (sctk_size_t size)
{
#warning "Should be optimize"
  return sctk_malloc (size);
}

void *
sctk_calloc (sctk_size_t nmemb, sctk_size_t size)
{
  void *tmp;
  sctk_tls_t *tls;
  SCTK_PROFIL_START (sctk_calloc);
  LOG_IN ();
  tls = __sctk_get_tls ();
  tmp = __intern_sctk_calloc (nmemb, size, tls);
#ifdef SCTK_MEMINFO
  do
    {
      unsigned long len;

      len = sctk_get_chunk (tmp)->cur_size;
      sctk_atomic_increment (&calls[idx_calloc]);
      sctk_atomic_add (&total[idx_calloc], len);
      sctk_atomic_add (&grand_total, len);
      if (len <= chunk_sizes[max_chunk_sizes - 1])
	sctk_atomic_increment (&histogram[len / step_chunk_sizes]);
      else
	sctk_atomic_increment (&large);
      sctk_atomic_increment (&calls_total);
      if (tmp == NULL)
	{
	  sctk_atomic_increment (&failed[idx_calloc]);
	}
      update_data (tmp, 0);
    }
  while (0);
#endif
  LOG_ARG (tmp, "ptr");
  LOG_ARG (size, "size");
  LOG_ARG (nmemb, "nmemb");
  LOG_OUT ();
  SCTK_PROFIL_END (sctk_calloc);
  return tmp;
}

void *
sctk_realloc (void *ptr, sctk_size_t size)
{
  void *tmp;
  sctk_tls_t *tls;
  SCTK_PROFIL_START (sctk_realloc);
  LOG_IN ();
  LOG_ARG (ptr, "ptr init");
  LOG_ARG (size, "size");
#ifdef SCTK_MEMINFO
  do
    {
      unsigned long len;
      unsigned long old_len;

      if (ptr)
	old_len = sctk_get_chunk (ptr)->cur_size;
      else
	old_len = 0;
      update_data (ptr, 1);
#endif
      tls = __sctk_get_tls ();
      tmp = __intern_sctk_realloc (ptr, size, tls);
#ifdef SCTK_MEMINFO

      len = sctk_get_chunk (tmp)->cur_size;
      sctk_atomic_increment (&calls[idx_realloc]);
      sctk_atomic_add (&total[idx_realloc], len);
      sctk_atomic_add (&grand_total, len);
      if (len <= chunk_sizes[max_chunk_sizes - 1])
	sctk_atomic_increment (&histogram[len / step_chunk_sizes]);
      else
	sctk_atomic_increment (&large);
      sctk_atomic_increment (&calls_total);
      if (tmp == NULL)
	{
	  sctk_atomic_increment (&failed[idx_realloc]);
	}
      else
	{
	  if (tmp == ptr)
	    sctk_atomic_increment (&inplace);

	  if (old_len > len)
	    sctk_atomic_increment (&decreasing);
	}
      update_data (tmp, 0);
    }
  while (0);
#endif
  LOG_ARG (tmp, "ptr");
  LOG_OUT ();
  SCTK_PROFIL_END (sctk_realloc);
  return tmp;
}

void
sctk_free (void *ptr)
{
  sctk_tls_t *my_tls;

  SCTK_PROFIL_START (sctk_free);
  LOG_IN ();
  LOG_ARG (ptr, "ptr");
  my_tls = __sctk_get_tls ();
  SCTK_DEBUG (sctk_mem_error ("--> Free pointer %p\n", ptr));
  if (ptr != NULL)
    {
#ifdef SCTK_MEMINFO
      sctk_atomic_increment (&calls[idx_free]);
      sctk_atomic_add (&total[idx_free], sctk_get_chunk (ptr)->cur_size);
      update_data (ptr, 1);
#endif
      __intern_sctk_free (ptr, my_tls);
    }
#ifdef SCTK_MEMINFO
  else
    {
      sctk_atomic_increment (&calls[idx_free]);
    }
#endif
  SCTK_DEBUG (sctk_mem_error ("<-- Free pointer %p\n", ptr));
  LOG_OUT ();
  SCTK_PROFIL_END (sctk_free);
}

void
sctk_cfree (void *ptr)
{
  LOG_IN ();
  LOG_ARG (ptr, "ptr");
  sctk_free (ptr);
  LOG_OUT ();
}

int
sctk_posix_memalign (void **memptr, sctk_size_t alignment, sctk_size_t size)
{
  int tmp;
  sctk_tls_t *tls;
  LOG_IN ();
  tls = __sctk_get_tls ();
  tmp = __intern_sctk_posix_memalign (memptr, alignment, size, tls);
  LOG_ARG (*memptr, "ptr");
  LOG_ARG (tmp, "res");
  LOG_OUT ();
  return tmp;
}

void *
sctk_memalign (sctk_size_t boundary, sctk_size_t size)
{
  void *tmp = NULL;
  sctk_tls_t *tls;
  LOG_IN ();
  tls = __sctk_get_tls ();
  __intern_sctk_posix_memalign (&tmp, boundary, size, tls);
  LOG_ARG (tmp, "ptr");
  LOG_ARG (size, "size");
  LOG_ARG (boundary, "boundary");
  LOG_OUT ();
  return tmp;
}

void *
sctk_valloc (sctk_size_t size)
{
  void *tmp = NULL;
  LOG_IN ();
  tmp = sctk_memalign (sysconf (_SC_PAGESIZE), size);
  LOG_ARG (tmp, "ptr");
  LOG_ARG (size, "size");
  LOG_OUT ();
  return tmp;
}

/*******************************************/
/**************** Accessors ****************/
/*******************************************/
void *
sctk_get_heap_start ()
{
  return (void *) brk_pointer;
}
unsigned long
sctk_get_heap_size ()
{
  return (unsigned long) max_brk_pointer - (unsigned long) init_brk_pointer;
}

sctk_tls_t *
sctk_get_tls ()
{
  return __sctk_get_tls ();
}

void
sctk_set_tls (sctk_tls_t * tls)
{
  __sctk_set_tls (tls);
}

/* int */
/* sctk_get_page_used (sctk_tls_t * tls) */
/* { */
/*   return tls->used_pages_number; */
/* } */

/*******************************************/
/*********** Buffered allocation ***********/
/*******************************************/

static mpc_inline sctk_alloc_block_t *
sctk_get_buffered_pointer (void *ptr)
{
  sctk_malloc_chunk_t *chunk;
  sctk_alloc_block_t *tmp;
#ifdef SCTK_COHERENCY_CHECK
  ptr = sctk_coherency_check_verif_chunk (ptr);
#endif

  chunk = sctk_get_chunk (ptr);

  tmp = (sctk_alloc_block_t *) ((char *) ptr + chunk->cur_size -
#ifdef SCTK_COHERENCY_CHECK
				sctk_sizeof (sctk_coherency_check_t) -
#endif
				sctk_sizeof (sctk_alloc_block_t));
  return tmp;
}

static mpc_inline int
__sctk_is_buffered (void *ptr)
{
  sctk_malloc_chunk_t *chunk;
#ifdef SCTK_COHERENCY_CHECK
  ptr = sctk_coherency_check_verif_chunk (ptr);
#endif

  chunk = sctk_get_chunk (ptr);
  return (chunk->type == sctk_chunk_buffered_type);
}

void
sctk_buffered_alloc_create (sctk_alloc_buffer_t * buf, size_t elemsize)
{
  sctk_mutex_t lock = SCTK_MUTEX_INITIALIZER;
#ifdef SCTK_COHERENCY_CHECK
  sctk_size_t init_size;
  init_size = elemsize;
  elemsize = sctk_coherency_check_extend_size (init_size);
#endif

  buf->lock = lock;
  buf->size = elemsize;
  buf->list = NULL;
  buf->real_size =
    sctk_aligned_size (elemsize + sctk_sizeof (sctk_alloc_block_t));
}

void
sctk_buffered_alloc_delete (sctk_alloc_buffer_t * buf)
{
  sctk_alloc_block_t *block;
  sctk_alloc_block_t *tmp;
  sctk_mutex_lock (&(buf->lock));
  block = (sctk_alloc_block_t *) buf->list;
  buf->list = NULL;
  while (block != NULL)
    {
      tmp = block->next;

      sctk_free (block->ptr);

      block = tmp;
    }
  sctk_mutex_unlock (&(buf->lock));
}

int
sctk_is_buffered (void *ptr)
{
  return __sctk_is_buffered (ptr);
}

void
sctk_buffered_free (void *ptr)
{
  sctk_alloc_block_t *block;
  sctk_alloc_buffer_t *buf;
  sctk_mem_assert (__sctk_is_buffered (ptr) == 1);

  block = sctk_get_buffered_pointer (ptr);
  buf = block->buf;
  sctk_mutex_lock (&(buf->lock));
  block->next = (sctk_alloc_block_t *) buf->list;
  buf->list = block;
  sctk_mutex_unlock (&(buf->lock));
}

void *
sctk_buffered_malloc (sctk_alloc_buffer_t * buf, size_t size)
{
  void *tmp;
#ifdef SCTK_COHERENCY_CHECK
  sctk_size_t init_size;
  init_size = size;
  size = sctk_coherency_check_extend_size (init_size);
#endif
  sctk_mem_assert (size == buf->size);
  sctk_mutex_lock (&(buf->lock));
  if (buf->list == NULL)
    {
      sctk_alloc_block_t *block;
      sctk_malloc_chunk_t *chunk;
      tmp = sctk_malloc (buf->real_size);
#ifdef SCTK_COHERENCY_CHECK
      tmp = (char *) tmp - sctk_sizeof (sctk_coherency_check_t);
      chunk = sctk_get_chunk (tmp);
      chunk->type = sctk_chunk_buffered_type;
      tmp = (char *) tmp + sctk_sizeof (sctk_coherency_check_t);
#else
      chunk = sctk_get_chunk (tmp);
      chunk->type = sctk_chunk_buffered_type;
#endif
      block = sctk_get_buffered_pointer (tmp);
      block->ptr = tmp;
      block->buf = buf;
      block->next = NULL;
    }
  else
    {
      tmp = (void *) buf->list->ptr;
      buf->list = (sctk_alloc_block_t *) buf->list->next;
    }
  sctk_mutex_unlock (&(buf->lock));
  return tmp;
}

/*******************************************/
/*********** MPC spec allocation ***********/
/*******************************************/
void
sctk_flush_tls ()
{
}
void
sctk_enter_no_alloc_land ()
{
/*   sctk_alloc_spinlock_lock (&sctk_sbrk_lock); */
  sctk_spinlock_write_lock(&sctk_sbrk_rwlock);
  sctk_no_alloc_land = 1;
/*   sctk_alloc_spinlock_unlock (&sctk_sbrk_lock); */
  sctk_spinlock_write_unlock(&sctk_sbrk_rwlock);
}

void
sctk_leave_no_alloc_land ()
{
  sctk_no_alloc_land = 0;
}

int
sctk_is_no_alloc_land ()
{
  return sctk_no_alloc_land;
}

sctk_tls_t *
__sctk_create_thread_memory_area ()
{
  return sctk_create_tls ();
}

void
__sctk_delete_thread_memory_area (sctk_tls_t * tls)
{

  sctk_spinlock_lock (&old_tls_list_spinlock);
  tls->next = (sctk_tls_t *) old_tls_list;
  old_tls_list = tls;
  sctk_spinlock_unlock (&old_tls_list_spinlock);

}

void
sctk_init_alloc (void)
{
#ifdef MPC_Threads
  sctk_mem_assert (sctk_multithreading_initialised == 1);
#endif
  sctk_init_tls_from_thread ();
  SCTK_DEBUG (sctk_mem_error
	      ("sctk_init_tls_from_thread done old %p\n", sctk_tls_default));
  sctk_get_tls ();
  SCTK_DEBUG (sctk_mem_error ("New tls %p\n", sctk_get_tls ()));
}

/*******************************************/
/*************** NUMA *alloc ***************/
/*******************************************/
#ifdef MPC_Threads
void *
__sctk_malloc_new (size_t size, sctk_tls_t * tls)
{
  void *tmp;

  if (sctk_is_numa_node ())
    {
      tmp = sctk_alloc_ptr_new (size, tls);
      SCTK_MALLOCLIKE_BLOCK (tmp, size, sizeof (sctk_malloc_chunk_t), 0);
      memset (tmp, 0, size);
    }
  else
    {
      tmp = __sctk_malloc (size, tls);
    }

  return tmp;
}

void *
__sctk_malloc_on_node (size_t size, int node, sctk_tls_t * tls)
{
  void *tmp;

  if (sctk_is_numa_node ())
    {
      int init_proc;
      int new_node;
      new_node = sctk_get_first_cpu_in_node (node);
      init_proc = sctk_get_cpu ();
      if (init_proc != new_node)
	{
#ifdef MPC_USE_PAGE_MIGRATION
	  unsigned long pos;
	  unsigned long ssize;
	  unsigned long nodes;
#endif
	  tmp = __sctk_malloc_new (size, tls);
#ifdef MPC_USE_PAGE_MIGRATION
	  pos = (unsigned long) tmp;
	  pos = pos / (unsigned long) page_size;
	  pos = pos * (unsigned long) page_size;

	  ssize = (unsigned long) size;
	  if (ssize < (unsigned long) page_size)
	    ssize = (unsigned long) page_size;
	  ssize = ssize / (unsigned long) page_size;
	  ssize = ssize * (unsigned long) page_size;

	  /*Compute node */
	  nodes = 1UL << node;
	  if (mbind
	      ((void *) (pos), ssize, MPOL_BIND, &nodes, 64,
	       MPOL_MF_MOVE | MPOL_MF_STRICT) != 0)
	    {
#ifndef NO_INTERNAL_ASSERT
	      perror ("Page Migration error");
#endif
	    }
#endif
	}
      else
	{
	  tmp = __sctk_malloc_new (size, tls);
	}
    }
  else
    {
      tmp = __sctk_malloc (size, tls);
    }

  return tmp;
}

void *
sctk_malloc_on_node (size_t size, int node)
{
  sctk_tls_t *tls;
  void *tmp;
  LOG_IN ();
  tls = __sctk_get_tls ();
  tmp = __sctk_malloc_on_node (size, node, tls);
  LOG_OUT ();
  return tmp;
}
#else
void *
__sctk_malloc_new (size_t size, sctk_tls_t * tls)
{
  not_available ();
  return NULL;
}

void *
__sctk_malloc_on_node (size_t size, int node, sctk_tls_t * tls)
{
  not_available ();
  return NULL;
}

void *
sctk_malloc_on_node (size_t size, int node)
{
  not_available ();
  return NULL;
}
#endif

/*******************************************/
/***************    STATS    ***************/
/*******************************************/


int
sctk_stats_enable ()
{
#ifdef SCTK_VIEW_FACTIONNEMENT
  char *tmp;
  sctk_stats_status = 1;
  if (mem_watchpoint != NULL)
    {
      tmp = mem_watchpoint (total_memory_allocated,
			    total_memory_used,
			    total_nb_block, 0, mpc_alloc_type_enable);
      if (tmp != NULL)
	sctk_debug_print_backtrace (tmp);
    }
  return 0;
#else
  return 1;
#endif
}

int
sctk_stats_disable ()
{
#ifdef SCTK_VIEW_FACTIONNEMENT
  char *tmp;
  sctk_stats_status = 0;
  if (mem_watchpoint != NULL)
    {
      tmp = mem_watchpoint (total_memory_allocated,
			    total_memory_used,
			    total_nb_block, 0, mpc_alloc_type_disable);
      if (tmp != NULL)
	sctk_debug_print_backtrace (tmp);
    }
  return 0;
#else
  return 1;
#endif
}

int
sctk_stats_show ()
{
#ifdef SCTK_VIEW_FACTIONNEMENT
  char *tmp;
  if (mem_watchpoint != NULL)
    {
      tmp = mem_watchpoint (total_memory_allocated,
			    total_memory_used,
			    total_nb_block, 0, mpc_alloc_type_show);
      if (tmp != NULL)
	sctk_debug_print_backtrace (tmp);
    }
  return 0;
#else
  return 1;
#endif
}

size_t
sctk_get_total_memory_allocated ()
{
#ifdef SCTK_VIEW_FACTIONNEMENT
  return total_memory_allocated;
#else
  return 0;
#endif
}

size_t
sctk_get_total_memory_used ()
{
#ifdef SCTK_VIEW_FACTIONNEMENT
  return total_memory_used;
#else
  return 0;
#endif
}

size_t
sctk_get_total_nb_block ()
{
#ifdef SCTK_VIEW_FACTIONNEMENT
  return total_nb_block;
#else
  return 0;
#endif
}

double
sctk_get_fragmentation ()
{
#ifdef SCTK_VIEW_FACTIONNEMENT
  size_t mem_alloc;
  size_t mem_used;

  mem_alloc = sctk_get_total_memory_allocated ();
  mem_used = sctk_get_total_memory_used ();

  if (mem_alloc > 0)
    {
      return ((double) ((mem_alloc - mem_used) * 100)) / ((double) mem_alloc);
    }
  else
    {
      return 100.0;
    }
#else
  return 0;
#endif
}

int
sctk_reset_watchpoint ()
{
#ifdef SCTK_VIEW_FACTIONNEMENT
  mem_watchpoint = NULL;
  sctk_stats_status = 0;
  return 0;
#else
  return 1;
#endif
}

int
sctk_set_watchpoint (char *(*func_watchpoint) (size_t mem_alloc,
					       size_t mem_used,
					       size_t nb_block,
					       size_t size,
					       sctk_alloc_type_t op_type))
{
#ifdef SCTK_VIEW_FACTIONNEMENT
  char *tmp;
  if (func_watchpoint != NULL)
    {
      tmp = func_watchpoint (total_memory_allocated,
			     total_memory_used,
			     total_nb_block, 0, mpc_alloc_type_init);
      if (tmp != NULL)
	sctk_debug_print_backtrace (tmp);
    }

  mem_watchpoint = func_watchpoint;
  return 0;
#else
  return 1;
#endif
}


int
mpc_alloc_stats_enable (void)
{
  return sctk_stats_enable ();
}

int
mpc_alloc_stats_disable (void)
{
  return sctk_stats_disable ();
}

int
mpc_alloc_stats_show (void)
{
  return sctk_stats_show ();
}

size_t
mpc_alloc_get_total_memory_allocated (void)
{
  return sctk_get_total_memory_allocated ();
}

size_t
mpc_alloc_get_total_memory_used (void)
{
  return sctk_get_total_memory_used ();
}

size_t
mpc_alloc_get_total_nb_block (void)
{
  return sctk_get_total_nb_block ();
}

double
mpc_alloc_get_fragmentation (void)
{
  return sctk_get_fragmentation ();
}

void
mpc_alloc_flush_alloc_buffers_long_life (void)
{
  return;
}

void
mpc_alloc_flush_alloc_buffers_short_life (void)
{
  return;
}

void
mpc_alloc_flush_alloc_buffers (void)
{
  return;
}

void
mpc_alloc_switch_to_short_life_alloc (void)
{
  return;
}

void
mpc_alloc_switch_to_long_life_alloc (void)
{
  return;
}

int
mpc_alloc_set_watchpoint (char *(*func_watchpoint) (size_t mem_alloc,
						    size_t mem_used,
						    size_t nb_block,
						    size_t size,
						    mpc_alloc_type_t op_type))
{
  return sctk_set_watchpoint (func_watchpoint);
}

void
mpc_dsm_prefetch (void *addr)
{
  sctk_dsm_prefetch (addr);
}

void
mpc_enable_dsm (void)
{
  sctk_enable_dsm ();
}

void
mpc_disable_dsm (void)
{
  sctk_disable_dsm ();
}

/* DSM */
#ifdef MPC_Message_Passing
static struct sigaction oldsigparam;
static size_t sctk_memory_slot_per_process = 0;
static sctk_thread_mutex_t dsm_lock = SCTK_THREAD_MUTEX_INITIALIZER;

typedef struct
{
  void *addr;
  size_t size;
} sctk_dsm_t;

static sctk_dsm_t *addresses = NULL;
static size_t addresses_number = 0;
static size_t addresses_number_max = 0;

static void
sctk_register_new_address (void *addr, size_t size)
{
  sctk_mmap (addr, size);
  if (addresses_number <= addresses_number_max)
    {
      addresses_number_max += 1024;
      addresses =
	sctk_realloc (addresses, addresses_number_max * sizeof (sctk_dsm_t));
    }
  addresses[addresses_number].addr = addr;
  addresses[addresses_number].size = size;
  addresses_number++;
}

static void
sctk_unregister_addresses (void)
{
  size_t i;
  for (i = 0; i < addresses_number; i++)
    {
      sctk_munmap (addresses[i].addr, addresses[i].size);
    }
  addresses_number = 0;
}

static void *
sctk_require_remote_address (void *addr)
{
  char *aligned_addr;
  size_t remote_owner;

  remote_owner = (unsigned long) addr - (unsigned long) global_address_start;
  remote_owner = remote_owner / sctk_memory_slot_per_process;

  sctk_nodebug ("Try to retrive %p in processes %lu", addr, remote_owner);

  aligned_addr = (void *) sctk_compute_real_size ((unsigned long) addr);
  if ((unsigned long) addr < (unsigned long) aligned_addr)
    {
      aligned_addr -= SCTK_PAGES_ALLOC_SIZE;
    }

  assume ((unsigned long) addr >= (unsigned long) aligned_addr);
  assume ((unsigned long) addr <
	  (unsigned long) aligned_addr + SCTK_PAGES_ALLOC_SIZE);

  sctk_register_new_address (aligned_addr, SCTK_PAGES_ALLOC_SIZE);

  sctk_net_get_pages (aligned_addr, SCTK_PAGES_ALLOC_SIZE, remote_owner);
  return aligned_addr;
}

void
sctk_dsm_prefetch (void *addr)
{
  not_implemented ();
  assume (addr == NULL);
}

static void
sctk_require_page_handler (int signo, siginfo_t * info, void *context)
{
  void *addr;

  addr = info->si_addr;
  sctk_thread_mutex_lock (&dsm_lock);
  sctk_nodebug ("Require ptr %p", addr);
  sctk_nodebug ("GLOBAL ADRESSES %p-%p", init_brk_pointer, max_brk_pointer);

  /*Error in local space */
  if (((unsigned long) addr >= (unsigned long) init_brk_pointer) &&
      ((unsigned long) addr <= (unsigned long) max_brk_pointer))
    {
      goto error;
    }

  if (sctk_process_number > 0)
    {
      if (((unsigned long) addr >= (unsigned long) global_address_start) &&
	  ((unsigned long) addr <= (unsigned long) global_address_end))
	{
	  sctk_require_remote_address (addr);
	}
      else
	{
	  goto error;
	}
    }
  else
    {
      goto error;
    }
  sctk_thread_mutex_unlock (&dsm_lock);
  return;

error:
  sctk_error ("Require invalid address %p", addr);
  oldsigparam.sa_sigaction (signo, info, context);
  sctk_thread_mutex_unlock (&dsm_lock);
}

void
sctk_enable_dsm (void)
{
  static volatile int enabled = 0;
  struct sigaction sigparam;

  sctk_thread_mutex_lock (&dsm_lock);
  if (!enabled)
    {
      sigparam.sa_flags = SA_SIGINFO;
      sigparam.sa_sigaction = sctk_require_page_handler;
      sigemptyset (&sigparam.sa_mask);

      sigaction (SIGSEGV, &sigparam, &oldsigparam);
      enabled = 1;
      if (sctk_process_number > 0)
	{
	  if (global_address_start == NULL)
	    {
	      size_t size;
	      size =
		(unsigned long) max_brk_pointer -
		(unsigned long) init_brk_pointer;
	      sctk_nodebug ("Memory size %lu", size);
	      sctk_memory_slot_per_process = size;
	      global_address_start =
		init_brk_pointer - (size * sctk_process_rank);
	      global_address_end =
		global_address_start + (size * sctk_process_number);
	      sctk_nodebug ("GLOBAL SPACE %p-%p", global_address_start,
			    global_address_end);
	    }
	}
    }
  sctk_thread_mutex_unlock (&dsm_lock);
}

void
sctk_disable_dsm (void)
{
  sctk_thread_mutex_lock (&dsm_lock);
  sigaction (SIGSEGV, &oldsigparam, NULL);
  sctk_unregister_addresses ();
  sctk_thread_mutex_unlock (&dsm_lock);
}
#else
void
sctk_dsm_prefetch (void *addr)
{
  not_available ();
  assume (addr == NULL);
}

void
sctk_enable_dsm (void)
{
  not_available ();
}

void
sctk_disable_dsm (void)
{
  not_available ();
}
#endif

/*******************************************************************************************/
/* MMAP/MUNMAP/MREMAP                                                                      */
/*******************************************************************************************/

/*
  Functions used to perform user mmap
*/

void *
sctk_user_mmap (void *start, size_t length, int prot, int flags,
		int fd, off_t offset)
{
  void *res = MAP_FAILED;
  if (flags & MAP_FIXED)
    {
#warning "MAP_FIXED not implemented"
    }
  else
    {
      if (sctk_posix_memalign (&res, page_size, length) != 0)
	{
	  res = MAP_FAILED;
	  goto end;
    }
      sctk_real_munmap (res, length);
      res = sctk_real_mmap (res, length, prot, flags | MAP_FIXED, fd, offset);
    }
end:
  return res;
}

int
sctk_user_munmap (void *start, size_t length)
{
#warning "munmap not implemented"
  return 1;
}

void *
sctk_user_mremap (void *old_address, size_t old_size, size_t new_size,
		  int flags)
{
      not_implemented ();
  return MAP_FAILED;
    }

#ifdef SCTK_MPC_MMAP
/*
  Linker with wrap capabilities
*/
void *
__wrap_mmap (void *start, size_t length, int prot, int flags,
	     int fd, off_t offset)
    {
  return sctk_user_mmap (start, length, prot, flags, fd, offset);
}

int
__wrap_munmap (void *start, size_t length)
	{
  return sctk_user_munmap (start, length);
	}

void *
__wrap_mremap (void *old_address, size_t old_size, size_t new_size, int flags)
	{
  return sctk_user_mremap (old_address, old_size, new_size, flags);
	}

void *__real_mmap (void *start, size_t length, int prot, int flags,
		   int fd, off_t offset);
int __real_munmap (void *start, size_t length);

static void *
sctk_real_mmap (void *start, size_t length, int prot, int flags,
		int fd, off_t offset)
{
  return __real_mmap (start, length, prot, flags, fd, offset);
    }

static int
sctk_real_munmap (void *start, size_t length)
{
  return __real_munmap (start, length);
}

#else

static void *
sctk_real_mmap (void *start, size_t length, int prot, int flags,
		int fd, off_t offset)
{
  return mmap (start, length, prot, flags, fd, offset);
}


static int
sctk_real_munmap (void *start, size_t length)
{
  return munmap (start, length);
}
#endif
