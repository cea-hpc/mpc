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
/* #   - VALAT Sebastien sebastien.valat@ocre.cea.fr                      # */
/* #                                                                      # */
/* ######################################################################## */

#ifndef __SCTK_ALLOC_NO_H__
#define __SCTK_ALLOC_NO_H__
#ifdef __cplusplus
extern "C"
{
#endif

/********************  HEADERS  *********************/
#include "sctk_config.h"
#ifdef MPC_PosixAllocator
#error "don't have do be included"
#endif

#include <stdlib.h>
#include <limits.h>
#include "sctk_debug.h"

/********************  MACRO  ***********************/
/*Specify data_alignemnt */
#define sctk_align SCTK_ALIGNEMENT
#define sctk_aligned_size(a) ((((a) / sctk_align) + 1) * sctk_align)
#define sctk_aligned_sizeof(a) (sctk_aligned_size(sizeof(a)))

/*********************  TYPES  **********************/
struct sctk_alloc_block_s;
typedef char sctk_alloc_type_t;
/*********************  TYPES  **********************/
typedef struct sctk_alloc_buffer_s
{
	char foo;
} sctk_alloc_buffer_t;

/*********************  TYPES  **********************/
struct sctk_alloc_chain;

/*********************  TYPES  **********************/
struct sctk_alloc_block_s
{
	char foo;
};

/*********************  TYPES  **********************/
struct sctk_alloc_page_s
{
	char foo;
};

/*******************  FUNCTION  *********************/
//Thread allocation chain management
void sctk_flush_tls (void);
struct sctk_alloc_chain *__sctk_create_thread_memory_area (void);
void __sctk_delete_thread_memory_area (struct sctk_alloc_chain * tmp);
void sctk_set_tls (struct sctk_alloc_chain * tls);
struct sctk_alloc_chain *sctk_get_tls (void);
void sctk_alloc_posix_plug_on_egg_allocator(void);
struct sctk_alloc_chain * sctk_get_current_alloc_chain(void);
void sctk_alloc_posix_numa_migrate(void);
void sctk_alloc_posix_numa_migrate_chain(struct sctk_alloc_chain * chain);
void sctk_alloc_posix_mmsrc_numa_init_phase_numa(void);

/*******************  FUNCTION  *********************/
//buffered allocation functions
void sctk_buffered_alloc_create (sctk_alloc_buffer_t * buf,
                                 size_t elemsize);
void sctk_buffered_alloc_delete (sctk_alloc_buffer_t * buf);
int sctk_is_buffered (void *ptr);
void sctk_buffered_free (void *ptr);
void *sctk_buffered_malloc (sctk_alloc_buffer_t * buf, size_t size);

/*******************  FUNCTION  *********************/
void __sctk_update_memory (char *file_name);
void *tmp_malloc (size_t size);

/*******************  FUNCTION  *********************/
//heap management
void sctk_mem_reset_heap (size_t start, size_t max_size);
void *sctk_get_heap_start (void);
unsigned long sctk_get_heap_size (void);
void sctk_view_local_memory (void);

/*******************  FUNCTION  *********************/
//dump/restore system
void __sctk_dump_tls (struct sctk_alloc_chain * tls, char *file_name);
void sctk_dump_tls (char *file_name);
void __sctk_restore_tls (struct sctk_alloc_chain ** tls, char *file_name);
void sctk_restore_tls (char *file_name);
int sctk_check_file (char *file_name);
void sctk_view_local_memory (void);
void __sctk_view_local_memory (struct sctk_alloc_chain * tls);

/*******************  FUNCTION  *********************/
//internal posix compliant allocator implementation
int __sctk_posix_memalign (void **memptr, size_t alignment, size_t size, struct sctk_alloc_chain * tls);
void *__sctk_realloc (void *ptr, size_t size, struct sctk_alloc_chain * tls);
void __sctk_free (void *ptr, struct sctk_alloc_chain * tls);
void *__sctk_calloc (size_t nmemb, size_t size, struct sctk_alloc_chain * tls);
void *__sctk_malloc (size_t size, struct sctk_alloc_chain * tls);

/*******************  FUNCTION  *********************/
//internal posix compliant allocator implementation
void *sctk_calloc (size_t nmemb, size_t size);
void *sctk_malloc (size_t size);
void sctk_free (void *ptr);
void sctk_cfree (void *ptr);
void *sctk_realloc (void *ptr, size_t size);
int sctk_posix_memalign (void **memptr, size_t alignment, size_t size);
void *sctk_memalign (size_t boundary, size_t size);
void *sctk_valloc (size_t size);

/*******************  FUNCTION  *********************/
//posix interface
void *calloc (size_t nmemb, size_t size);
void *malloc (size_t size);
void free (void *ptr);
void cfree (void *ptr);
void *realloc (void *ptr, size_t size);
int posix_memalign (void **memptr, size_t alignment, size_t size);
void *memalign (size_t boundary, size_t size);
void *valloc (size_t size);

/*******************  FUNCTION  *********************/
void sctk_enter_no_alloc_land (void);
void sctk_leave_no_alloc_land (void);
int sctk_is_no_alloc_land (void);

/*******************  FUNCTION  *********************/
//numa specialized functions
void *__sctk_malloc_on_node (size_t size, int node,struct sctk_alloc_chain * tls);
void *__sctk_malloc_new (size_t size, struct sctk_alloc_chain * tls);
void *sctk_malloc_on_node (size_t size, int node);
void sctk_init_alloc (void);
char *sctk_alloc_mode (void);

#ifdef __cplusplus
}/* end of extern "C" */
#endif

#endif //__SCTK_ALLOC_NO_H__
