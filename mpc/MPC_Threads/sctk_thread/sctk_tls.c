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


#define _GNU_SOURCE
#include "sctk_config.h"
#include "sctk_alloc.h"
#include <sctk_debug.h>
#include <unistd.h>
#include <sctk_tls.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>


#if defined(SCTK_USE_TLS)
__thread void *sctk_hierarchical_tls = NULL;
#endif

#if defined(SCTK_USE_TLS) && defined(Linux_SYS)
#include <link.h>
#include <stdlib.h>
#include <stdio.h>

static __thread unsigned long p_memsz;
static __thread unsigned long p_filesz;
static __thread void *p_vaddr;
static __thread size_t current_module;
static __thread size_t total_module;
static size_t page_size = 0;

static int
callback (struct dl_phdr_info *info, size_t size, void *data)
{
  int j;


  for (j = 0; j < info->dlpi_phnum; j++)
    {
      if (info->dlpi_phdr[j].p_type == PT_TLS)
	{
	  total_module++;
	  if (total_module == current_module)
	    {
	      p_memsz = info->dlpi_phdr[j].p_memsz;
	      p_filesz = info->dlpi_phdr[j].p_filesz;
	      p_vaddr =
		(void *) ((char *) info->dlpi_addr +
			  info->dlpi_phdr[j].p_vaddr);
	      sctk_nodebug ("TLS in %d %p size %lu start %p %lu", j,
			    info->dlpi_addr, p_memsz, p_vaddr,
			    info->dlpi_phdr[j].p_filesz);
	      return 1;
	    }
	  else
	    {
	      sctk_nodebug ("TLS SKIP in %d %p %d != %d", j, info->dlpi_addr,
			    total_module, current_module);
	    }
	}
    }
  return 0;
}


typedef struct
{
  unsigned long int ti_module;
  unsigned long int ti_offset;
} tls_index;

typedef char *sctk_tls_module_t;

typedef struct
{
  volatile size_t size;
  volatile sctk_tls_module_t *volatile modules;
  sctk_spinlock_t lock;
} tls_level;

static inline void
sctk_tls_init_level (tls_level * level)
{
  level->size = 0;
  level->modules = NULL;
  level->lock = 0;
}
static inline void
sctk_tls_write_level (tls_level * level)
{
  sctk_spinlock_lock (&(level->lock));
}
static inline void
sctk_tls_unlock_level (tls_level * level)
{
  sctk_spinlock_unlock (&(level->lock));
}

static inline size_t
sctk_determine_module_size (size_t m)
{
  return p_memsz;
}

static inline void
sctk_init_module (size_t m, char *module, size_t size)
{
  /*  memset (module, 0, size);  Already done thanks to the mmap allocation */

  if (p_filesz)
    {
      memcpy (module, p_vaddr, p_filesz);
    }
}

#define SCTK_COW_TLS
static inline int
sctk_get_module_file_decr (size_t m, size_t module_size)
{
#if defined(SCTK_COW_TLS)
  static sctk_spinlock_t lock = 0;
  static int *fds = NULL;
  static size_t size = 0;
  int fd = -1;
  size_t i;
  char *tls_module;

  sctk_spinlock_lock (&(lock));
  if (size <= m)
    {
      fds = sctk_realloc (fds, m + 1);
      for (i = size; i < m + 1; i++)
	{
	  fds[i] = -1;
	}
      size = m + 1;
    }
  fd = fds[m];

  if (fd == -1)
    {
      static char name[4096];
      sprintf (name, "/tmp/cp_on_write_get_module_file_decr_%d_module_%lu",
	       getpid (), m);

      remove (name);
      fd = open (name, O_CREAT | O_RDWR | O_TRUNC,S_IRWXU);
      assert (fd > 0);

      tls_module = sctk_malloc (module_size);

      memset (tls_module, 0, module_size);
      sctk_init_module (m, tls_module, module_size);

      write (fd, tls_module, module_size);
      free (tls_module);
      remove (name);

      fds[m] = fd;
    }
  sctk_spinlock_unlock (&(lock));
  return fd;
#else
  return -1;
#endif
}

char *
sctk_alloc_module (size_t m, tls_level * tls_level)
{
  char *tls_module;
  tls_module = (char *) tls_level->modules[m - 1];
  if (tls_module == NULL)
    {
      size_t module_size;
      size_t module_size_rest;
      int fd;

      current_module = m;
      total_module = 0;
      dl_iterate_phdr (callback, NULL);

      module_size = sctk_determine_module_size (m);
      assume (page_size != 0);


      module_size_rest = module_size % page_size;
      if (module_size_rest != 0)
	{
	  module_size = module_size + (page_size - module_size_rest);
	}


      fd = sctk_get_module_file_decr (m, module_size);

      if (fd == -1)
	{
	  tls_module = sctk_user_mmap (NULL, module_size,
				       PROT_READ | PROT_WRITE,
				       MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	  sctk_init_module (m, tls_module, module_size);
	}
      else
	{
	  tls_module = sctk_user_mmap (NULL, module_size,
				       PROT_READ | PROT_WRITE,
				       MAP_PRIVATE, fd, 0);
	}

      assume (tls_module != NULL);

      tls_level->modules[m - 1] = tls_module;
      sctk_nodebug ("Init module %d size %lu, %d", m, module_size,
		    *((int *) p_vaddr));
    }
  return tls_module;
}

static inline void *
__sctk__tls_get_addr__generic_scope (size_t m, size_t offset,
				     sctk_tls_scope_t scope)
{
  void *res;
  tls_level **tls;
  tls_level *tls_level;
  char *tls_module;

  sctk_nodebug ("Module %lu Offset %lu (scope %d)", m, offset, scope);

  tls = sctk_hierarchical_tls;
  assume (tls != NULL);

  tls_level = tls[scope];

  if (expect_false (tls_level->size < m))
    {
      goto not_allocated;
    }

  tls_module = (char *) tls_level->modules[m - 1];
  res = tls_module + offset;

  return res;

not_allocated:
  sctk_tls_write_level (tls_level);

  if (tls_level->size < m)
    {
      size_t i;
      tls_level->modules =
	sctk_realloc ((void *) (tls_level->modules), m * sizeof (char *));
      for (i = tls_level->size; i < m; i++)
	{
	  tls_level->modules[i] = NULL;
	  sctk_alloc_module (i, tls_level);
	}
      sctk_nodebug ("Init modules to size %ld->%ld", tls_level->size, m);
      tls_level->size = m;
    }

  tls_module = sctk_alloc_module (m, tls_level);
  sctk_tls_unlock_level (tls_level);

  res = tls_module + offset;
  return res;
}


static void
sctk_tls_create ()
{
  tls_level **tls;
  int i;

  page_size = getpagesize ();

  tls = sctk_malloc (sctk_tls_max_scope * sizeof (tls_level *));

  for (i = 0; i < sctk_tls_max_scope; i++)
    {
      tls[i] = sctk_malloc (sizeof (tls_level));
      sctk_tls_init_level (tls[i]);
    }

  sctk_nodebug ("Init tls_hierarchical root %p", tls);
  sctk_hierarchical_tls = tls;
}

void
sctk_tls_duplicate (void **new)
{
  tls_level *tls;
  tls_level *new_tls;
  int i;

  tls = sctk_hierarchical_tls;
  if (tls == NULL)
    {
      sctk_tls_create ();
    }

  new_tls = sctk_malloc (sctk_tls_max_scope * sizeof (tls_level));
  *new = new_tls;

  tls = sctk_hierarchical_tls;
  sctk_nodebug ("Duplicate %p->%p", tls, new_tls);

  for (i = 0; i < sctk_tls_max_scope; i++)
    {
      new_tls[i] = tls[i];
    }

}

void
sctk_tls_keep (int *scopes)
{
  tls_level **tls;
  int i;
  tls = sctk_hierarchical_tls;

  assume (tls != NULL);

  for (i = 0; i < sctk_tls_max_scope; i++)
    {
      if (scopes[i] == 0)
	{
	  sctk_nodebug ("Remove %d in %p", i, tls);
	  tls[i] = sctk_malloc (sizeof (tls_level));
	  sctk_tls_init_level (tls[i]);
	}
    }
}

void
sctk_tls_keep_non_current_thread (void **tls, int *scopes)
{
  int i;

  assume (tls != NULL);

  for (i = 0; i < sctk_tls_max_scope; i++)
    {
      if (scopes[i] == 0)
	{
	  sctk_nodebug ("Remove %d in %p", i, tls);
	  tls[i] = sctk_malloc (sizeof (tls_level));
	  sctk_tls_init_level (tls[i]);
	}
    }
}

void
sctk_tls_delete ()
{
#warning "Pas de liberation des tls"
}


#if defined(SCTK_i686_ARCH_SCTK) || defined (SCTK_x86_64_ARCH_SCTK)


void *
__sctk__tls_get_addr__task_scope (tls_index * tmp)
{
  void *res;
  sctk_nodebug ("__sctk__tls_get_addr__task_scope");
  res =
    __sctk__tls_get_addr__generic_scope (tmp->ti_module, tmp->ti_offset,
					 sctk_tls_task_scope);
  return res;
}

void *
__sctk__tls_get_addr__thread_scope (tls_index * tmp)
{
  void *res;
  res =
    __sctk__tls_get_addr__generic_scope (tmp->ti_module, tmp->ti_offset,
					 sctk_tls_thread_scope);
  return res;
}

#if defined (MPC_OpenMP)
void *
__sctk__tls_get_addr__openmp_scope (tls_index * tmp)
{
  void *res;
  sctk_nodebug ("__sctk__tls_get_addr__openmp_scope");
  res =
    __sctk__tls_get_addr__generic_scope (tmp->ti_module, tmp->ti_offset,
					 sctk_tls_openmp_scope);
  return res;
}
#endif

void *
__sctk__tls_get_addr__process_scope (tls_index * tmp)
{
  void *res;
  res =
    __sctk__tls_get_addr__generic_scope (tmp->ti_module, tmp->ti_offset,
					 sctk_tls_process_scope);
  return res;
}

#elif defined (SCTK_ia64_ARCH_SCTK)

void *
__sctk__tls_get_addr__task_scope (size_t m, size_t offset)
{
  void *res;
  res = __sctk__tls_get_addr__generic_scope (m, offset, sctk_tls_task_scope);
  return res;
}

void *
__sctk__tls_get_addr__thread_scope (size_t m, size_t offset)
{
  void *res;
  res =
    __sctk__tls_get_addr__generic_scope (m, offset, sctk_tls_thread_scope);
  return res;
}

#if defined (MPC_OpenMP)
void *
__sctk__tls_get_addr__openmp_scope (size_t m, size_t offset)
{
  void *res;
  res =
    __sctk__tls_get_addr__generic_scope (m, offset, sctk_tls_openmp_scope);
  return res;
}
#endif

void *
__sctk__tls_get_addr__process_scope (size_t m, size_t offset)
{
  void *res;
  res =
    __sctk__tls_get_addr__generic_scope (m, offset, sctk_tls_process_scope);
  return res;
}
#else
#error "Architecture not available for TLS support"
#endif
#else
#warning "Experimental TLS support desactivated"

void
sctk_tls_create ()
{
}

void
sctk_tls_duplicate (void **new)
{
  *new = NULL;
}

void
sctk_tls_keep (int *scopes)
{
}

void
sctk_tls_keep_non_current_thread (void **tls, int *scopes)
{
}

void
sctk_tls_delete ()
{
}

#endif
