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
#ifndef __SCTK__ALLOC__LIST__
#define __SCTK__ALLOC__LIST__

#ifdef __cplusplus
extern "C"
{
#endif
#ifndef __SCTK__ALLOC__C__
#error "For internal use only"
#endif

#include <stdlib.h>
#include "sctk_config.h"
#include "sctk_alloc_types.h"
#include "sctk_alloc_debug.h"
  static sctk_size_t sctk_sizeof_sctk_page_t = 0;
  static sctk_size_t sctk_sizeof_sctk_malloc_chunk_t = 0;

  static mpc_inline void
    sctk_remove_free_chunk (sctk_free_chunk_t * chunk,
			    sctk_free_chunk_t ** list)
  {
    if (chunk->prev_chunk == NULL)
      {
	sctk_alloc_assert (*list == chunk);
	*list = chunk->next_chunk;
	if (chunk->next_chunk != NULL)
	  {
	    sctk_alloc_assert (chunk->next_chunk->prev_chunk == chunk);
	    chunk->next_chunk->prev_chunk = NULL;
	  }
      }
    else
      {
	sctk_alloc_assert (*list != chunk);
	if (chunk->next_chunk == NULL)
	  {
	    chunk->prev_chunk->next_chunk = NULL;
	  }
	else
	  {
	    sctk_alloc_assert (chunk->next_chunk->prev_chunk == chunk);
	    chunk->prev_chunk->next_chunk = chunk->next_chunk;
	    chunk->next_chunk->prev_chunk = chunk->prev_chunk;
	  }
      }
    chunk->prev_chunk = NULL;
    chunk->next_chunk = NULL;
  }

  static mpc_inline void
    sctk_remove_free_page (sctk_page_t * chunk, sctk_page_t ** list)
  {
    if (chunk->prev_chunk == NULL)
      {
	*list = chunk->next_chunk;
	if (chunk->next_chunk != NULL)
	  {
	    chunk->next_chunk->prev_chunk = NULL;
	  }
      }
    else
      {
	if (chunk->next_chunk == NULL)
	  {
	    chunk->prev_chunk->next_chunk = NULL;
	  }
	else
	  {
	    chunk->prev_chunk->next_chunk = chunk->next_chunk;
	    chunk->next_chunk->prev_chunk = chunk->prev_chunk;
	  }
      }
    chunk->prev_chunk = NULL;
    chunk->next_chunk = NULL;
  }

  static mpc_inline void
    sctk_insert_free_page (sctk_page_t * chunk, sctk_page_t ** list)
  {
    chunk->prev_chunk = NULL;
    chunk->next_chunk = *list;
    if (chunk->next_chunk != NULL)
      {
	chunk->next_chunk->prev_chunk = chunk;
      }

    *list = chunk;

  }

  static mpc_inline void
    sctk_insert_free_page_last (sctk_page_t * chunk, sctk_page_t ** list,
				sctk_page_t ** last)
  {
    chunk->prev_chunk = *last;
    chunk->next_chunk = NULL;
    if (*last != NULL)
      {
	(*last)->next_chunk = chunk;
      }
    if (*list == NULL)
      {
	*list = chunk;
      }
    *last = chunk;
  }

  static mpc_inline void
    sctk_remove_used_page (sctk_page_t * chunk, sctk_tls_t * tls)
  {
    tls->used_pages_number--;
    if (chunk->prev_chunk_used == NULL)
      {
	tls->used_pages = chunk->next_chunk_used;
	if (chunk->next_chunk_used != NULL)
	  {
	    chunk->next_chunk_used->prev_chunk_used = NULL;
	  }
      }
    else
      {
	if (chunk->next_chunk_used == NULL)
	  {
	    chunk->prev_chunk_used->next_chunk_used = NULL;
	  }
	else
	  {
	    chunk->prev_chunk_used->next_chunk_used = chunk->next_chunk_used;
	    chunk->next_chunk_used->prev_chunk_used = chunk->prev_chunk_used;
	  }
      }
  }

  static mpc_inline void
    sctk_insert_used_page (sctk_page_t * chunk, sctk_tls_t * tls)
  {
    tls->used_pages_number++;

    chunk->prev_chunk_used = NULL;
    chunk->next_chunk_used = tls->used_pages;
    if (chunk->next_chunk_used != NULL)
      {
	chunk->next_chunk_used->prev_chunk_used = chunk;
      }

    tls->used_pages = chunk;
  }

  static mpc_inline sctk_page_t
    * sctk_get_page_from_chunk (sctk_free_chunk_t * chunk)
  {
    sctk_page_t *tmp;
    tmp = (sctk_page_t *) (((char *) chunk) - sctk_sizeof_sctk_page_t);
    return tmp;
  }

  static mpc_inline sctk_free_chunk_t
    * sctk_get_chunk_from_page (sctk_page_t * page)
  {
    sctk_free_chunk_t *tmp;
    tmp = (sctk_free_chunk_t *) (((char *) page) + sctk_sizeof_sctk_page_t);
    return tmp;
  }

  static mpc_inline sctk_malloc_chunk_t
    * sctk_next_malloc_chunk (sctk_malloc_chunk_t * chunk)
  {
    sctk_malloc_chunk_t *tmp;
    tmp = (sctk_malloc_chunk_t *) (((char *) chunk) + chunk->cur_size +
				   sctk_sizeof_sctk_malloc_chunk_t);
    return tmp;
  }

  static mpc_inline sctk_malloc_chunk_t
    * sctk_next_malloc_chunk_size (sctk_malloc_chunk_t * chunk,
				   sctk_size_t size)
  {
    sctk_malloc_chunk_t *tmp;
    tmp = (sctk_malloc_chunk_t *) (((char *) chunk) + size +
				   sctk_sizeof_sctk_malloc_chunk_t);
    return tmp;
  }

  static mpc_inline void sctk_print_chunk (sctk_malloc_chunk_t * a)
  {
    char *msg;
    sctk_mem_error ("Chunk %p-%p next %p\n", a,
		    (char *) a + a->cur_size, sctk_next_malloc_chunk (a));
    sctk_mem_error ("\t- cur_size %lu\n", (unsigned long) a->cur_size);
/*     sctk_mem_error("\t- related %p\n",a->related); */
    switch (a->state)
      {
      case sctk_inuse_state:
	msg = "sctk_inuse_state";
	break;
      case sctk_free_state:
	msg = "sctk_free_state";
	break;
      case sctk_disable_consolidate_state:
	msg = "sctk_disable_consolidate_state";
	break;
      case sctk_free_state_use:
	msg = "sctk_free_state_use";
	break;
      default:
	msg = "unknown";
      }
    sctk_mem_error ("\t- state %d (%s)\n", a->state, msg);
    switch (a->type)
      {
      case sctk_chunk_small_type:
	msg = "sctk_chunk_small_type";
	break;
      case sctk_chunk_big_type:
	msg = "sctk_chunk_big_type";
	break;
      case sctk_chunk_new_type:
	msg = "sctk_chunk_new_type";
	break;
      case sctk_chunk_page_type:
	msg = "sctk_chunk_page_type";
	break;
      case sctk_chunk_buffered_type:
	msg = "sctk_chunk_buffered_type";
	break;
      default:
	msg = "unknown";
      }
    sctk_mem_error ("\t- type %d (%s)\n", a->type, msg);
  }

  static mpc_inline void sctk_print_page (sctk_page_t * a)
  {
    char *msg;
    sctk_mem_error ("Page %p-%p\n", a, (char *) a + a->real_size);
    sctk_mem_error ("\t- cur_size %lu\n", (unsigned long) a->cur_size);
    sctk_mem_error ("\t- real_size %lu\n", (unsigned long) a->real_size);
/*     sctk_mem_error("\t- related %p\n",a->related); */
    switch (a->state)
      {
      case sctk_inuse_state:
	msg = "sctk_inuse_state";
	break;
      case sctk_free_state:
	msg = "sctk_free_state";
	break;
      case sctk_disable_consolidate_state:
	msg = "sctk_disable_consolidate_state";
	break;
      case sctk_free_state_use:
	msg = "sctk_free_state_use";
	break;
      default:
	msg = "unknown";
      }
    sctk_mem_error ("\t- state %d (%s)\n", a->state, msg);
    switch (a->page_state)
      {
      case sctk_inuse_state:
	msg = "sctk_inuse_state";
	break;
      case sctk_free_state:
	msg = "sctk_free_state";
	break;
      case sctk_disable_consolidate_state:
	msg = "sctk_disable_consolidate_state";
	break;
      case sctk_free_state_use:
	msg = "sctk_free_state_use";
	break;
      default:
	msg = "unknown";
      }
    sctk_mem_error ("\t- page_state %d (%s)\n", a->state, msg);
    switch (a->type)
      {
      case sctk_chunk_small_type:
	msg = "sctk_chunk_small_type";
	break;
      case sctk_chunk_big_type:
	msg = "sctk_chunk_big_type";
	break;
      case sctk_chunk_new_type:
	msg = "sctk_chunk_new_type";
	break;
      case sctk_chunk_page_type:
	msg = "sctk_chunk_page_type";
	break;
      case sctk_chunk_buffered_type:
	msg = "sctk_chunk_buffered_type";
	break;
      case sctk_chunk_memalign_type:
	msg = "sctk_chunk_memalign_type";
	break;
      default:
	msg = "unknown";
      }
    sctk_mem_error ("\t- type %d (%s)\n", a->type, msg);
  }

#ifndef NO_INTERNAL_ASSERT
  static mpc_inline void
    sctk_check_chunk_coherency_intern (sctk_malloc_chunk_t * a,
				       const char *file, const int line)
  {
    int error = 0;
    char error_msg[4096];
    char *msg;
    if (a == NULL)
      return;
    switch (a->type)
      {
      case sctk_chunk_small_type:
	msg = "sctk_chunk_small_type";
	break;
      case sctk_chunk_big_type:
	msg = "sctk_chunk_big_type";
	break;
      case sctk_chunk_new_type:
	msg = "sctk_chunk_new_type";
	break;
      case sctk_chunk_page_type:
	msg = "sctk_chunk_page_type";
	break;
      case sctk_chunk_buffered_type:
	msg = "sctk_chunk_buffered_type";
	break;
      case sctk_chunk_memalign_type:
	msg = "sctk_chunk_memalign_type";
	break;
      default:
	msg = "unknown";
      }

    sctk_check_alignement (a);

    if (a->cur_size <= 0)
      {
	sprintf (error_msg, "%s invalid size %lu", msg,a->cur_size);
	error = 1;
      }

    if (error)
      {
	sctk_mem_error ("Coherency error %s at line %d in file %s\n",
			error_msg, line, file);
	sctk_abort ();
      }
  }
#endif


  static mpc_inline sctk_malloc_chunk_t
    * sctk_chunk_init (sctk_malloc_chunk_t * chunk, sctk_size_t size)
  {
    sctk_malloc_chunk_t *next;

    SCTK_MAKE_WRITABLE (chunk, sizeof (sctk_malloc_chunk_t));
    chunk->cur_size = size;

    next = sctk_next_malloc_chunk_size (chunk, size);

    return next;
  }

  static mpc_inline sctk_malloc_chunk_t
    * sctk_chunk_init_two (sctk_malloc_chunk_t * chunk,
			   sctk_size_t size, sctk_size_t size2)
  {
    sctk_malloc_chunk_t *next;

    SCTK_MAKE_WRITABLE (chunk, sizeof (sctk_malloc_chunk_t));
    chunk->cur_size = size;
    next = sctk_next_malloc_chunk_size (chunk, size);
    SCTK_MAKE_WRITABLE (next, sizeof (sctk_malloc_chunk_t));
    next->cur_size = size2;

    return next;
  }

  static mpc_inline void sctk_chunk_merge (sctk_malloc_chunk_t * a)
  {
    sctk_size_t new_size;
    sctk_malloc_chunk_t *b;


    b = sctk_next_malloc_chunk (a);

    sctk_check_chunk_coherency (a);
    sctk_check_chunk_coherency (b);

    new_size = a->cur_size + b->cur_size + sctk_sizeof_sctk_malloc_chunk_t;


    SCTK_MAKE_NOACCESS (a, new_size);
    sctk_chunk_init (a, new_size);

    sctk_check_chunk_coherency (a);
  }

  static mpc_inline sctk_malloc_chunk_t
    * sctk_chunk_split (sctk_malloc_chunk_t * a, sctk_size_t size)
  {
    sctk_size_t old_size;
    sctk_malloc_chunk_t *b;
    sctk_size_t ref_size;

    ref_size = size + sctk_sizeof_sctk_malloc_chunk_t;
    old_size = a->cur_size;

    if (old_size > (SCTK_SPLIT_THRESHOLD + ref_size))
      {
	b = sctk_chunk_init_two (a, size, old_size - ref_size);

	b->state = a->state;
	b->type = a->type;
      }
    else
      {
	b = sctk_next_malloc_chunk (a);
      }
    return b;
  }

  static mpc_inline sctk_malloc_chunk_t
    * sctk_chunk_split_verif (sctk_malloc_chunk_t * a, sctk_size_t size)
  {
    sctk_size_t old_size;
    sctk_malloc_chunk_t *b;
    sctk_size_t ref_size;

    ref_size = size + sctk_sizeof_sctk_malloc_chunk_t;
    old_size = a->cur_size;

    if (old_size > (SCTK_SPLIT_THRESHOLD + ref_size))
      {
	b = sctk_chunk_init_two (a, size, old_size - ref_size);

	b->state = a->state;
	b->type = a->type;
      }
    else
      {
	b = NULL;
      }
    return b;
  }


  static mpc_inline void *sctk_get_ptr (sctk_malloc_chunk_t * chunk)
  {
    SCTK_DEBUG (sctk_mem_error ("sctk_get_ptr sctk_check_chunk_coherency\n"));
    sctk_check_chunk_coherency (chunk);
    SCTK_DEBUG (sctk_mem_error
		("sctk_get_ptr sctk_check_chunk_coherency DONE\n"));
    return (void *) (((char *) chunk) + sctk_sizeof_sctk_malloc_chunk_t);
  }

  static mpc_inline sctk_malloc_chunk_t *sctk_get_chunk (void *chunk)
  {
    sctk_malloc_chunk_t *a;

    a = (sctk_malloc_chunk_t *) (((char *) chunk) -
				 sctk_sizeof_sctk_malloc_chunk_t);

    sctk_check_chunk_coherency (a);
    return a;
  }

  static mpc_inline sctk_page_t *sctk_split_page (sctk_page_t * page,
						  sctk_size_t size)
  {
    sctk_size_t init_size;
    sctk_page_t *page_end = NULL;

    SCTK_DEBUG (sctk_mem_error ("SPLIT PAGE\n"); sctk_print_page (page));
    sctk_mem_assert (page->real_size >= size);
    init_size = page->real_size;
    if (init_size > size)
      {
	page_end = (sctk_page_t *) ((char *) page + size);

	page->real_size = size;

	page_end->real_size = init_size - size;
      }
    SCTK_DEBUG (sctk_mem_error ("SPLIT PAGE DONE\n");
		sctk_print_page (page); sctk_print_page (page_end));
    return page_end;
  }


#ifdef __cplusplus
}				/* end of extern "C" */
#endif
#endif
