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
#ifndef __SCTK__ALLOC__INTERNAL__
#define __SCTK__ALLOC__INTERNAL__

#ifdef __cplusplus
extern "C"
{
#endif
#include <stdlib.h>
#include <malloc.h>
#include "sctk_config.h"
#include "sctk_alloc_thread.h"

/* #ifndef SCTK_ALLOC_EXTERN */
#define sctk_align SCTK_ALIGNEMENT
#define SCTK_ALIGNEMENT_MASK (SCTK_ALIGNEMENT - 1)
#define SCTK_IS_ALIGNED_OK(m)  (((sctk_size_t)(m) & SCTK_ALIGNEMENT_MASK) == 0)
#define sctk_aligned_size(a) (((a) | SCTK_ALIGNEMENT_MASK) + 1)
#define sctk_sizeof(a) (sizeof(a) & SCTK_ALIGNEMENT_MASK ? \
			(((sizeof(a)) | SCTK_ALIGNEMENT_MASK) + 1) : ((sizeof(a))))
/* #endif */

/*******************************************/
/****************** Types ******************/
/*******************************************/
  typedef unsigned long sctk_size_t;
  struct sctk_tls_s;
  typedef struct sctk_tls_s sctk_tls_t;
  struct sctk_alloc_block_s;
  typedef struct sctk_alloc_block_s sctk_alloc_block_t;
  typedef struct sctk_alloc_buffer_s
  {
    sctk_size_t size;
    sctk_size_t real_size;
    sctk_mutex_t lock;
    volatile sctk_alloc_block_t *list;
  } sctk_alloc_buffer_t;
  typedef sctk_tls_t sctk_alloc_thread_data_t;
#define sctk_aligned_sizeof(a) sizeof(a)

/*******************************************/
/*************** __sctk_*alloc *************/
/*******************************************/
  void *__sctk_malloc (sctk_size_t size, sctk_tls_t * tls);
  void *__sctk_calloc (sctk_size_t nmemb, sctk_size_t size, sctk_tls_t * tls);
  void __sctk_free (void *ptr, sctk_tls_t * tls);
  void *__sctk_realloc (void *ptr, sctk_size_t size, sctk_tls_t * tls);
  int
    __sctk_posix_memalign (void **memptr, sctk_size_t alignment,
			   sctk_size_t size, sctk_tls_t * tls);


/*******************************************/
/**************** Accessors ****************/
/*******************************************/
  void sctk_print_tls (void);
  void *sctk_get_heap_start (void);
  unsigned long sctk_get_heap_size (void);
  void sctk_set_tls (sctk_tls_t * tls);
  sctk_tls_t *sctk_get_tls (void);

/*******************************************/
/*************** sctk_*alloc ***************/
/*******************************************/
  void *sctk_calloc (sctk_size_t nmemb, sctk_size_t size);
  void *sctk_malloc (sctk_size_t size);
  void *sctk_tmp_malloc (sctk_size_t size);
  void sctk_free (void *ptr);
  void sctk_cfree (void *ptr);
  void *sctk_realloc (void *ptr, sctk_size_t size);

  int sctk_posix_memalign (void **memptr, sctk_size_t alignment,
			   sctk_size_t size);
  void *sctk_valloc (sctk_size_t size);
  void *sctk_memalign (sctk_size_t boundary, sctk_size_t size);


/*******************************************/
/*************** dump_*alloc ***************/
/*******************************************/
  int sctk_check_file (char *name);
  void __sctk_dump_tls (sctk_tls_t * tls, char *file_name);
  void sctk_dump_tls (char *file_name);
  void __sctk_restore_tls (sctk_tls_t ** tls, char *file_name);
  void sctk_restore_tls (char *file_name);
  void __sctk_view_local_memory (sctk_tls_t * tls);
  void sctk_view_local_memory (void);
  void sctk_mem_reset_heap (sctk_size_t start, sctk_size_t max_size);
  void __sctk_update_memory (char *file_name);
  void sctk_relocalise_tls (void);
  void __sctk_relocalise_tls (sctk_tls_t * tls);
  void sctk_clean_memory (void);
  void sctk_relocalise_memory (void *ptr, sctk_size_t size);

  void
    sctk_update_used_pages (int fd, void **user_data,
			    sctk_size_t * user_data_size, sctk_tls_t ** tls);
  void sctk_add_global_var (void *adr, sctk_size_t size);
  void
    sctk_restore_used_pages (int fd, void **user_data,
			     sctk_size_t * user_data_size, sctk_tls_t ** tls);
  int sctk_check_used_pages (int fd);
  void
    sctk_dump_used_pages (int fd, void *user_data,
			  sctk_size_t user_data_size);

  void sctk_dump_memory_heap ();
/*******************************************/
/*************** libc *alloc ***************/
/*******************************************/
  void *calloc (size_t nmemb, size_t size);
  void *malloc (size_t size);
  void free (void *ptr);
  void cfree (void *ptr);
  void *realloc (void *ptr, size_t size);

  int posix_memalign (void **memptr, size_t alignment, size_t size);
  void *valloc (size_t size);
  void *memalign (size_t boundary, size_t size);


/*******************************************/
/*********** Buffered allocation ***********/
/*******************************************/
  void sctk_buffered_alloc_create (sctk_alloc_buffer_t * buf,
				   size_t elemsize);
  void sctk_buffered_alloc_delete (sctk_alloc_buffer_t * buf);
  int sctk_is_buffered (void *ptr);
  void sctk_buffered_free (void *ptr);
  void *sctk_buffered_malloc (sctk_alloc_buffer_t * buf, size_t size);

/*******************************************/
/*********** MPC spec allocation ***********/
/*******************************************/
  void sctk_enter_no_alloc_land (void);
  void sctk_leave_no_alloc_land (void);
  int sctk_is_no_alloc_land (void);


  sctk_tls_t *__sctk_create_thread_memory_area (void);
  void __sctk_delete_thread_memory_area (sctk_tls_t * tls);

  void sctk_init_alloc (void);
  char *sctk_alloc_mode (void);
  void sctk_flush_tls (void);
  void sctk_check_address(void* addr, size_t n);

/*******************************************/
/*************** NUMA *alloc ***************/
/*******************************************/
  void *__sctk_malloc_on_node (size_t size, int node, sctk_tls_t * tls);
  void *__sctk_malloc_new (size_t size, sctk_tls_t * tls);
  void *sctk_malloc_on_node (size_t size, int node);

/*******************************************/
/******************* DSM *******************/
/*******************************************/
  void sctk_dsm_prefetch (void *addr);
  void sctk_enable_dsm ();
  void sctk_disable_dsm ();

#ifdef __cplusplus
}				/* end of extern "C" */
#endif
#endif
