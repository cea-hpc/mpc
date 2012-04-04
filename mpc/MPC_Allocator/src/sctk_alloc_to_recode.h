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
/* #   - Valat Sébastien sebastien.valat@cea.fr                           # */
/* #                                                                      # */
/* ######################################################################## */

#ifndef SCTK_ALLOC_TO_RECODE_H
#define SCTK_ALLOC_TO_RECODE_H

#include <stdlib.h>

typedef size_t sctk_alloc_buffer_t;

sctk_alloc_chain_t * __sctk_create_thread_memory_area(void);
void sctk_set_tls(sctk_alloc_chain_t * tls);

static inline void sctk_buffered_alloc_create (sctk_alloc_buffer_t * buf, size_t elemsize){*buf = elemsize;};
static inline void sctk_buffered_alloc_delete (sctk_alloc_buffer_t * buf) {};
static inline void sctk_buffered_free (void *ptr) {free(ptr);};
static inline void * sctk_buffered_malloc (sctk_alloc_buffer_t * buf, size_t size) {return malloc(size);};
static inline void * sctk_malloc_on_node(size_t size,int node_id) {return malloc(size);};
static inline void * sctk_get_heap_start(void) {return (void*)SCTK_ALLOC_HEAP_BASE;};
static inline size_t sctk_get_heap_size(void) {return SCTK_ALLOC_HEAP_SIZE;};
static inline void * __sctk_malloc_new(size_t size,sctk_alloc_chain_t * chain) {return sctk_alloc_chain_alloc(chain,size);};
static inline void * __sctk_malloc (size_t size,sctk_alloc_chain_t * chain) {return sctk_alloc_chain_alloc(chain,size);};
static inline char * sctk_alloc_mode (void){return "MPC allocator";};
static inline void __sctk_free(void * ptr,sctk_alloc_chain_t * chain) {sctk_alloc_chain_free(chain,ptr);};
static inline void sctk_delete_thread_memory_area(sctk_alloc_chain_t * chain) {};
static inline void __sctk_set_tls (sctk_alloc_chain_t * tls) {
  /*sctk_get_set_tls_init ();
  SCTK_DEBUG (sctk_mem_error ("Set tls %p\n", tls));
  sctk_set_tls_from_thread (tls);*/
}
static inline void sctk_enter_no_alloc_land(void) {};
static inline void sctk_leave_no_alloc_land(void) {};
static inline void __sctk_delete_thread_memory_area(sctk_alloc_chain_t * tls) {};
void * sctk_user_mmap (void *start, size_t length, int prot, int flags,int fd, off_t offset);
static inline void sctk_init_alloc(void) {sctk_alloc_posix_base_init();};


/*******************************************/
/*************** dump_*alloc ***************/
/*******************************************/
static inline int sctk_check_file (char *name) {return 0;};
static inline void __sctk_dump_tls (sctk_alloc_chain_t * tls, char *file_name) {};
static inline void sctk_dump_tls (char *file_name) {};
static inline void __sctk_restore_tls (sctk_alloc_chain_t ** tls, char *file_name) {};
static inline void sctk_restore_tls (char *file_name) {};
static inline void __sctk_view_local_memory (sctk_alloc_chain_t * tls) {};
static inline void sctk_view_local_memory (void) {};
static inline void sctk_mem_reset_heap (sctk_size_t start, sctk_size_t max_size) {};
static inline void __sctk_update_memory (char *file_name) {};
static inline void sctk_relocalise_tls (void) {};
static inline void __sctk_relocalise_tls (sctk_alloc_chain_t * tls) {};
static inline void sctk_clean_memory (void) {};
static inline void sctk_relocalise_memory (void *ptr, sctk_size_t size) {};

static inline void sctk_update_used_pages (int fd, void **user_data,sctk_size_t * user_data_size, sctk_alloc_chain_t ** tls) {};
static inline void sctk_add_global_var (void *adr, sctk_size_t size) {};
static inline void sctk_restore_used_pages (int fd, void **user_data,sctk_size_t * user_data_size, sctk_alloc_chain_t ** tls) {};
static inline int sctk_check_used_pages (int fd) {return 0;};
static inline void sctk_dump_used_pages (int fd, void *user_data,sctk_size_t user_data_size) {};
static inline void sctk_dump_memory_heap () {};

#endif
