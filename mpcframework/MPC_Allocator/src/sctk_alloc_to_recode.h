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
#include <sctk_alloc_debug.h>

typedef size_t sctk_alloc_buffer_t;

struct sctk_alloc_chain * __sctk_create_thread_memory_area(void);
void sctk_set_tls(struct sctk_alloc_chain * tls);

static inline void sctk_buffered_alloc_create (__AL_UNUSED__ sctk_alloc_buffer_t * buf, __AL_UNUSED__ size_t elemsize){*buf = elemsize;};
static inline void sctk_buffered_alloc_delete (__AL_UNUSED__ sctk_alloc_buffer_t * buf) {};
static inline void sctk_buffered_free (__AL_UNUSED__ void *ptr) {free(ptr);};
static inline void * sctk_buffered_malloc (__AL_UNUSED__ sctk_alloc_buffer_t * buf, size_t size) {return malloc(size);};
void * sctk_get_heap_start(void);
size_t sctk_get_heap_size(void);
void * __sctk_malloc_new(size_t size,struct sctk_alloc_chain * chain);
void * __sctk_malloc (size_t size,struct sctk_alloc_chain * chain);
char * sctk_alloc_mode (void);
void __sctk_free(void * ptr,struct sctk_alloc_chain * chain);
static inline void sctk_delete_thread_memory_area(__AL_UNUSED__ struct sctk_alloc_chain * chain) {};
static inline void __sctk_set_tls (__AL_UNUSED__ struct sctk_alloc_chain * tls) {
  /*sctk_get_set_tls_init ();
  SCTK_DEBUG (sctk_mem_error ("Set tls %p\n", tls));
  sctk_set_tls_from_thread (tls);*/
}
static inline void sctk_enter_no_alloc_land(void) {};
static inline void sctk_leave_no_alloc_land(void) {};
static inline void __sctk_delete_thread_memory_area(__UNUSED__ struct sctk_alloc_chain * tls) {};
void * sctk_user_mmap (void *start, size_t length, int prot, int flags,int fd, off_t offset);
static inline void sctk_init_alloc(void) {sctk_alloc_posix_base_init();};


/*******************************************/
/*************** dump_*alloc ***************/
/*******************************************/
static inline int sctk_check_file (__AL_UNUSED__ char *name) {return 0;};
static inline void __sctk_dump_tls (__AL_UNUSED__ struct sctk_alloc_chain * tls, __AL_UNUSED__ char *file_name) {};
static inline void sctk_dump_tls (__AL_UNUSED__ char *file_name) {};
static inline void __sctk_restore_tls (__AL_UNUSED__ struct sctk_alloc_chain ** tls, __AL_UNUSED__ char *file_name) {};
static inline void sctk_restore_tls (__AL_UNUSED__ char *file_name) {};
static inline void __sctk_view_local_memory (__AL_UNUSED__ struct sctk_alloc_chain * tls) {};
static inline void sctk_view_local_memory (void) {};
static inline void sctk_mem_reset_heap (__AL_UNUSED__ sctk_size_t start, __AL_UNUSED__ sctk_size_t max_size) {};
static inline void __sctk_update_memory (__AL_UNUSED__ char *file_name) {};

static inline void sctk_update_used_pages (__AL_UNUSED__ int fd, __AL_UNUSED__ void **user_data,__UNUSED__ sctk_size_t * user_data_size, __AL_UNUSED__ struct sctk_alloc_chain ** tls) {};
static inline void sctk_add_global_var (__AL_UNUSED__ void *adr, __AL_UNUSED__ sctk_size_t size) {};
static inline void sctk_restore_used_pages (__AL_UNUSED__ int fd, __AL_UNUSED__ void **user_data,__UNUSED__ sctk_size_t * user_data_size, __AL_UNUSED__ struct sctk_alloc_chain ** tls) {};
static inline int sctk_check_used_pages (__AL_UNUSED__ int fd) {return 0;};
static inline void sctk_dump_used_pages (__AL_UNUSED__ int fd, __AL_UNUSED__ void *user_data, __AL_UNUSED__ sctk_size_t user_data_size) {};
static inline void sctk_dump_memory_heap () {};

#endif
