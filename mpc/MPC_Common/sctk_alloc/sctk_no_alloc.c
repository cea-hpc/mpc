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
#include <stdlib.h>
#ifndef MPC_Allocator
#include "sctk_alloc_api.h"
#include "sctk_alloc.h"
#include "sctk_debug.h"
#include <sys/mman.h>

#define SCTK_LOCAL_VERSION_MAJOR 0
#define SCTK_LOCAL_VERSION_MINOR 1


void
sctk_dsm_prefetch (void *addr)
{
  not_available ();
  assume (addr == NULL);
}

void
sctk_enable_dsm ()
{
  not_available ();
}

void
sctk_disable_dsm ()
{
  not_available ();
}

int
sctk_reset_watchpoint ()
{
  return 1;
}

int
sctk_set_watchpoint (char *(*func_watchpoint) (size_t mem_alloc,
					       size_t mem_used,
					       size_t nb_block,
					       size_t size,
					       sctk_alloc_type_t op_type))
{
  return 1;
}

int
sctk_stats_show ()
{
  return 1;
}

int
sctk_stats_enable ()
{
  return 1;
}

int
sctk_stats_disable ()
{
  return 1;
}

size_t
sctk_get_total_memory_allocated ()
{
  return 0;
}

size_t
sctk_get_total_memory_used ()
{
  return 0;
}

size_t
sctk_get_total_nb_block ()
{
  return 0;
}

double
sctk_get_fragmentation ()
{
  return 0.0;
}

void
sctk_flush_alloc_buffers_long_life ()
{
}

void
sctk_flush_alloc_buffers_short_life ()
{
}

void
sctk_flush_alloc_buffers ()
{
}

void
sctk_relocalise_memory (void *ptr, sctk_size_t size)
{

}

void
sctk_flush_tls (void)
{

}

char *
sctk_alloc_mode (void)
{
  return "STD allocator";
}

void
sctk_relocalise_tls ()
{

}

void
sctk_clean_memory ()
{

}
void
__sctk_relocalise_tls (sctk_alloc_thread_data_t * tls)
{

}

void
sctk_init_alloc ()
{
  sctk_print_version ("Init Standard Alloc", SCTK_LOCAL_VERSION_MAJOR,
		      SCTK_LOCAL_VERSION_MINOR);
}

void
sctk_buffered_alloc_create (sctk_alloc_buffer_t * buf, size_t elemsize)
{
}

void
sctk_buffered_alloc_delete (sctk_alloc_buffer_t * buf)
{
}
int
sctk_is_buffered (void *ptr)
{
  return 0;
}

void
sctk_buffered_free (void *ptr)
{
  free (ptr);
}

void *
sctk_buffered_malloc (sctk_alloc_buffer_t * buf, size_t size)
{
  return malloc (size);
}

void
__sctk_update_memory (char *file_name)
{
}

sctk_alloc_thread_data_t *
__sctk_create_thread_memory_area (void)
{
  return (void *) __sctk_create_thread_memory_area;
}

void
__sctk_delete_thread_memory_area (sctk_alloc_thread_data_t * tmp)
{

}

int
__sctk_posix_memalign (void **memptr, size_t alignment, size_t size,
		       sctk_alloc_thread_data_t * tls)
{
#ifdef HAVE_POSIX_MEMALIGN
  return posix_memalign (memptr, alignment, size);
#else
  not_available ();
  return 0;
#endif
}

void *
__sctk_realloc (void *ptr, size_t size, sctk_alloc_thread_data_t * tls)
{
  return realloc (ptr, size);
}

void
__sctk_free (void *ptr, sctk_alloc_thread_data_t * tls)
{
  free (ptr);
}

void *
__sctk_calloc (size_t nmemb, size_t size, sctk_alloc_thread_data_t * tls)
{
  return calloc (nmemb, size);
}

void *
__sctk_malloc (size_t size, sctk_alloc_thread_data_t * tls)
{
  return malloc (size);
}

void
sctk_mem_reset_heap (size_t start, size_t max_size)
{
}
void *
sctk_get_heap_start (void)
{
  return NULL;
}
unsigned long
sctk_get_heap_size (void)
{
  return 0;
}

void
sctk_view_local_memory (void)
{
}
void
sctk_set_tls (sctk_alloc_thread_data_t * tls)
{
}
sctk_alloc_thread_data_t *
sctk_get_tls (void)
{
  return NULL;
}

void
__sctk_dump_tls (sctk_alloc_thread_data_t * tls, char *file_name)
{
}
void
sctk_dump_tls (char *file_name)
{
}
void
__sctk_restore_tls (sctk_alloc_thread_data_t ** tls, char *file_name)
{
}
void
sctk_restore_tls (char *file_name)
{
}
int
sctk_check_file (char *file_name)
{
  return 0;
}

void
__sctk_view_local_memory (sctk_alloc_thread_data_t * tls)
{
}

void *
sctk_calloc (size_t nmemb, size_t size)
{
  return calloc (nmemb, size);
}

void *
sctk_malloc (size_t size)
{
  return malloc (size);
}

void
sctk_free (void *ptr)
{
  free (ptr);
}

void
sctk_cfree (void *ptr)
{
#ifdef HAVE_CFREE
  cfree (ptr);
#else
  free (ptr);
#endif
}

void *
sctk_realloc (void *ptr, size_t size)
{
  return realloc (ptr, size);
}

int
sctk_posix_memalign (void **memptr, size_t alignment, size_t size)
{
#ifdef HAVE_POSIX_MEMALIGN
  return posix_memalign (memptr, alignment, size);
#else
  not_available ();
  return 0;
#endif
}

void *
sctk_memalign (size_t boundary, size_t size)
{
#ifdef HAVE_MEMALIGN
  return memalign (boundary, size);
#else
  not_available ();
  return NULL;
#endif
}

void *
sctk_valloc (size_t size)
{
  return valloc (size);
}

void *
tmp_malloc (size_t size)
{
  return malloc (size);
}

void
sctk_enter_no_alloc_land (void)
{
}
void
sctk_leave_no_alloc_land (void)
{
}

int
sctk_is_no_alloc_land (void)
{
  return 0;
}

  /*NUMA aware */
void *
__sctk_malloc_on_node (size_t size, int node, sctk_alloc_thread_data_t * tls)
{
  return malloc (size);
}

void *
__sctk_malloc_new (size_t size, sctk_alloc_thread_data_t * tls)
{
  return malloc (size);
}

void *
sctk_malloc_on_node (size_t size, int node)
{
  return malloc (size);
}

void *
sctk_user_mmap (void *start, size_t length, int prot, int flags,
		int fd, off_t offset)
{
  return mmap (start, length, prot, flags, fd, offset);
}

int
sctk_user_munmap (void *start, size_t length)
{
  return munmap (start, length);
}

void *
sctk_user_mremap (void *old_address, size_t old_size, size_t new_size,
		  int flags)
{
  return mremap (old_address, old_size, new_size, flags);
}

#ifdef SCTK_MPC_MMAP
void *__real_mmap (void *start, size_t length, int prot, int flags,
		   int fd, off_t offset);

int __real_munmap (void *start, size_t length);
void *__real_mremap (void *old_address, size_t old_size, size_t new_size,
		     int flags);
void *
__wrap_mmap (void *start, size_t length, int prot, int flags,
	     int fd, off_t offset)
{
  return __real_mmap (start, length, prot, flags, fd, offset);
}

int
__wrap_munmap (void *start, size_t length)
{
  return __real_munmap (start, length);
}

void *
__wrap_mremap (void *old_address, size_t old_size, size_t new_size, int flags)
{
  return __real_mremap (old_address, old_size, new_size, flags);
}
#endif

#endif
