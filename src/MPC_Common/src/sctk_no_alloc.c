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

#include <mpc_config.h>

#ifndef MPC_Allocator

/********************  HEADERS  *********************/
#ifdef HAVE_MREMAP
#define _GNU_SOURCE
#endif /*HAVE_MREMAP*/

#include <unistd.h>
#include <stdlib.h>
#include "sctk_alloc.h"
#include "mpc_common_debug.h"
#include <unistd.h>
#include <sys/mman.h>

/*******************  FUNCTION  *********************/
void sctk_dsm_prefetch (void *addr)
{
	not_available ();
	assume (addr == NULL);
}

/*******************  FUNCTION  *********************/
void sctk_enable_dsm (void)
{
	not_available ();
}

/*******************  FUNCTION  *********************/
void sctk_disable_dsm (void)
{
	not_available ();
}

/*******************  FUNCTION  *********************/
int sctk_reset_watchpoint ()
{
	return 1;
}

/*******************  FUNCTION  *********************/
int sctk_set_watchpoint (__UNUSED__ char *(*func_watchpoint) (size_t mem_alloc,
                         size_t mem_used,
                         size_t nb_block,
                         size_t size,
                         sctk_alloc_type_t op_type))
{
	return 1;
}

/*******************  FUNCTION  *********************/
int sctk_stats_show (void)
{
	return 1;
}

/*******************  FUNCTION  *********************/
int sctk_stats_enable (void)
{
	return 1;
}

/*******************  FUNCTION  *********************/
int sctk_stats_disable (void)
{
	return 1;
}

/*******************  FUNCTION  *********************/
size_t sctk_get_total_memory_allocated (void)
{
	return 0;
}

/*******************  FUNCTION  *********************/
size_t sctk_get_total_memory_used (void)
{
	return 0;
}

/*******************  FUNCTION  *********************/
size_t sctk_get_total_nb_block (void)
{
	return 0;
}

/*******************  FUNCTION  *********************/
double sctk_get_fragmentation (void)
{
	return 0.0;
}

/*******************  FUNCTION  *********************/
void sctk_flush_alloc_buffers_long_life (void)
{
}

/*******************  FUNCTION  *********************/
void sctk_flush_alloc_buffers_short_life (void)
{
}

/*******************  FUNCTION  *********************/
void sctk_flush_alloc_buffers (void)
{
}

/*******************  FUNCTION  *********************/
void sctk_relocalise_memory (__UNUSED__ void *ptr, __UNUSED__ size_t size)
{
}

/*******************  FUNCTION  *********************/
void sctk_flush_tls (void)
{
}

/*******************  FUNCTION  *********************/
char * sctk_alloc_mode (void)
{
  return "STD allocator";
}

/*******************  FUNCTION  *********************/
void sctk_init_alloc (void)
{

}

/*******************  FUNCTION  *********************/
void sctk_buffered_alloc_create (__UNUSED__ sctk_alloc_buffer_t * buf, __UNUSED__ size_t elemsize)
{
}

/*******************  FUNCTION  *********************/
void sctk_buffered_alloc_delete (__UNUSED__ sctk_alloc_buffer_t * buf)
{
}

/*******************  FUNCTION  *********************/
int sctk_is_buffered (__UNUSED__ void *ptr)
{
	return 0;
}

/*******************  FUNCTION  *********************/
void sctk_buffered_free (__UNUSED__ void *ptr)
{
	free (ptr);
}

/*******************  FUNCTION  *********************/
void * sctk_buffered_malloc (__UNUSED__ sctk_alloc_buffer_t * buf, __UNUSED__ size_t size)
{
	return malloc (size);
}

/*******************  FUNCTION  *********************/
void __sctk_update_memory (__UNUSED__ char *file_name)
{
}

/*******************  FUNCTION  *********************/
struct sctk_alloc_chain * __sctk_create_thread_memory_area (void)
{
	return (void *) __sctk_create_thread_memory_area;
}

/*******************  FUNCTION  *********************/
void __sctk_delete_thread_memory_area (__UNUSED__ struct sctk_alloc_chain * tmp)
{
}

/*******************  FUNCTION  *********************/
void sctk_alloc_posix_plug_on_egg_allocator(void)
{
}

/*******************  FUNCTION  *********************/
void sctk_alloc_posix_mmsrc_numa_init_phase_numa(void)
{
}

/*******************  FUNCTION  *********************/
struct sctk_alloc_chain * sctk_get_current_alloc_chain(void)
{
	return NULL;
}

/*******************  FUNCTION  *********************/
int __sctk_posix_memalign (void **memptr, size_t alignment, size_t size, __UNUSED__ struct sctk_alloc_chain * tls)
{
	#ifdef HAVE_POSIX_MEMALIGN
	return posix_memalign (memptr, alignment, size);
	#else
	not_available ();
	return 0;
	#endif
}

/*******************  FUNCTION  *********************/
void * __sctk_realloc (void *ptr, size_t size, __UNUSED__ struct sctk_alloc_chain * tls)
{
	return realloc (ptr, size);
}

/*******************  FUNCTION  *********************/
void __sctk_free (void *ptr, __UNUSED__ struct sctk_alloc_chain * tls)
{
	free (ptr);
}

/*******************  FUNCTION  *********************/
void * __sctk_calloc (size_t nmemb, size_t size, __UNUSED__ struct sctk_alloc_chain * tls)
{
	return calloc (nmemb, size);
}

/*******************  FUNCTION  *********************/
void * __sctk_malloc (size_t size, __UNUSED__ struct sctk_alloc_chain * tls)
{
	return malloc (size);
}

/*******************  FUNCTION  *********************/
void sctk_mem_reset_heap (__UNUSED__ size_t start,  __UNUSED__ size_t max_size)
{
}

/*******************  FUNCTION  *********************/
void * sctk_get_heap_start (void)
{
	return NULL;
}

/*******************  FUNCTION  *********************/
unsigned long sctk_get_heap_size (void)
{
	return 0;
}

/*******************  FUNCTION  *********************/
void sctk_view_local_memory (void)
{
}

/*******************  FUNCTION  *********************/
void sctk_set_tls (__UNUSED__ struct sctk_alloc_chain * tls)
{
}

/*******************  FUNCTION  *********************/
struct sctk_alloc_chain * sctk_get_tls (void)
{
	return NULL;
}

/*******************  FUNCTION  *********************/
void __sctk_dump_tls (__UNUSED__ struct sctk_alloc_chain * tls, __UNUSED__ char *file_name)
{
}

/*******************  FUNCTION  *********************/
void sctk_dump_tls (__UNUSED__ char *file_name)
{
}

/*******************  FUNCTION  *********************/
void __sctk_restore_tls (__UNUSED__ struct sctk_alloc_chain ** tls, __UNUSED__ char *file_name)
{
}

/*******************  FUNCTION  *********************/
void sctk_restore_tls (__UNUSED__ char *file_name)
{
}

/*******************  FUNCTION  *********************/
int sctk_check_file (__UNUSED__ char *file_name)
{
	return 0;
}

/*******************  FUNCTION  *********************/
void __sctk_view_local_memory (__UNUSED__ struct sctk_alloc_chain * tls)
{
}

/*******************  FUNCTION  *********************/
void * sctk_calloc (size_t nmemb, size_t size)
{
	return calloc (nmemb, size);
}

/*******************  FUNCTION  *********************/
void * sctk_malloc (size_t size)
{
	return malloc (size);
}

/*******************  FUNCTION  *********************/
void sctk_free (void *ptr)
{
	free (ptr);
}

/*******************  FUNCTION  *********************/
void sctk_cfree (void *ptr)
{
	#ifdef HAVE_CFREE
	cfree (ptr);
	#else
	free (ptr);
	#endif
}

/*******************  FUNCTION  *********************/
void * sctk_realloc (void *ptr, size_t size)
{
	return realloc (ptr, size);
}

/*******************  FUNCTION  *********************/
int sctk_posix_memalign (void **memptr, size_t alignment, size_t size)
{
	#ifdef HAVE_POSIX_MEMALIGN
	return posix_memalign (memptr, alignment, size);
	#else
	not_available ();
	return 0;
	#endif
}

/*******************  FUNCTION  *********************/
void * sctk_memalign (size_t boundary, size_t size)
{
	#ifdef HAVE_MEMALIGN
	return memalign (boundary, size);
	#else
	not_available ();
	return NULL;
	#endif
}

/*******************  FUNCTION  *********************/
void * sctk_valloc (size_t size)
{
	return valloc (size);
}

/*******************  FUNCTION  *********************/
void * tmp_malloc (size_t size)
{
	return malloc (size);
}

/*******************  FUNCTION  *********************/
void sctk_enter_no_alloc_land (void)
{
}

/*******************  FUNCTION  *********************/
void sctk_leave_no_alloc_land (void)
{
}

/*******************  FUNCTION  *********************/
int sctk_is_no_alloc_land (void)
{
	return 0;
}

/*******************  FUNCTION  *********************/
void * __sctk_malloc_on_node (size_t size, __UNUSED__ int node, __UNUSED__ struct sctk_alloc_chain * tls)
{
	return malloc (size);
}

/*******************  FUNCTION  *********************/
void * __sctk_malloc_new (size_t size, __UNUSED__ struct sctk_alloc_chain * tls)
{
	return malloc (size);
}

/*******************  FUNCTION  *********************/
void * sctk_malloc_on_node (size_t size, __UNUSED__ int node)
{
	return malloc (size);
}

/*******************  FUNCTION  *********************/
void * sctk_user_mmap (void *start, size_t length, int prot, int flags,
                       int fd, off_t offset)
{
	return mmap (start, length, prot, flags, fd, offset);
}

/*******************  FUNCTION  *********************/
int sctk_user_munmap (void *start, size_t length)
{
	return munmap (start, length);
}

/*******************  FUNCTION  *********************/
void * sctk_user_mremap (void *old_address, size_t old_size, size_t new_size,
                         int flags)
{
	#ifdef HAVE_MREMAP
	return mremap (old_address, old_size, new_size, flags);
	#else /* HAVE_MREMAP*/
	not_available ();
	#endif /*HAVE_MREMAP*/
}

/*******************  FUNCTION  *********************/
void sctk_alloc_posix_numa_migrate(void)
{
}

/*******************  FUNCTION  *********************/
void sctk_alloc_posix_numa_migrate_chain(__UNUSED__ struct sctk_alloc_chain * chain)
{
}

/*******************  FUNCTION  *********************/
#ifdef SCTK_MPC_MMAP
void *__real_mmap (void *start, size_t length, int prot, int flags,
                   int fd, off_t offset);
int __real_munmap (void *start, size_t length);
void *__real_mremap (void *old_address, size_t old_size, size_t new_size,
                     int flags);

/*******************  FUNCTION  *********************/
void * __wrap_mmap (void *start, size_t length, int prot, int flags,
                    int fd, off_t offset)
{
	return __real_mmap (start, length, prot, flags, fd, offset);
}

/*******************  FUNCTION  *********************/
int __wrap_munmap (void *start, size_t length)
{
	return __real_munmap (start, length);
}

/*******************  FUNCTION  *********************/
void * __wrap_mremap (void *old_address, size_t old_size, size_t new_size, int flags)
{
	return __real_mremap (old_address, old_size, new_size, flags);
}

/*******************  FUNCTION  *********************/
void sctk_alloc_posix_numa_migrate_chain(struct sctk_alloc_chain * local_chain)
{
}

/*******************  FUNCTION  *********************/
void sctk_alloc_posix_numa_migrate(void)
{
}

#endif //SCTK_MPC_MMAP

#endif //MPC_Allocator
