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
/* #   - CARRIBAULT Patrick patrick.carribault@cea.fr                     # */
/* #   - TCHIBOUKDJIAN Marc marc.tchiboukdjian@exascale-computing.eu      # */
/* #                                                                      # */
/* ######################################################################## */


#define _GNU_SOURCE
#include "sctk_config.h"
#include "sctk_alloc.h"
#include "sctk_topology.h"
#include "sctk_accessor.h"
#include "sctk_atomics.h"
#include <sctk_debug.h>
#include <unistd.h>
#include <sctk_tls.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#if defined(SCTK_USE_TLS)
__thread void *sctk_extls = NULL;
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

typedef struct
{
  tls_level level ;
  sctk_atomics_int toenter ; /* total number of threads on this level */
  sctk_atomics_int entered ; /* number of VPs that entered the hls barrier */ 
  volatile int generation ;  /* number of barrier/single encountered */
  sctk_atomics_int nowait_generation ; /* number of single nowait encountered */
} hls_level ;

typedef struct
{
  int wait   ; /* number of barrier/single encountered on this thread */
  int nowait ; /* number of single nowait encountered on this thread */
} hls_generation_t ;

static hls_level **sctk_hls_repository ; /* global per process */
static __thread hls_level* sctk_hls[sctk_hls_max_scope] ; /* per VP */
__thread void* sctk_hls_generation ; /* per thread */
/* need to be saved and restored at context switch */

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

static inline void
sctk_hls_init_level(hls_level * level)
{
	int i ;
	sctk_tls_init_level(&level->level);
	/* toenter is to be filled by sctk_hls_checkout_on_vp */
	sctk_atomics_store_int ( &level->toenter, 0 ) ;
	sctk_atomics_store_int ( &level->entered, 0 ) ;
	level->generation = 0 ;
	sctk_atomics_store_int ( &level->nowait_generation, 0 ) ;
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

      assume(write (fd, tls_module, module_size) == module_size);
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

static char *
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
				     tls_level *tls_level)
{
  void *res;
  char *tls_module;

  sctk_nodebug ("Module %lu Offset %lu", m, offset);

  assert ( tls_level != NULL ) ;

  if (expect_false (tls_level->size < m))
    {
	  size_t i;
	  sctk_tls_write_level (tls_level);
  	  if (tls_level->size < m) {

	      tls_level->modules =
		sctk_realloc ((void *) (tls_level->modules), m * sizeof (char *));
	      for (i = tls_level->size; i < m; i++)
		{
		  tls_level->modules[i] = NULL;
		  /* sctk_alloc_module (i, tls_level); */
		}
	      sctk_nodebug ("Init modules to size %ld->%ld", tls_level->size, m);
	      tls_level->size = m;
          }
	  sctk_tls_unlock_level (tls_level);

    }
   
  if (expect_false (tls_level->modules[m-1] == NULL ))
     {
	  sctk_tls_write_level (tls_level);
          if ( tls_level->modules[m-1] == NULL )
            {
               tls_module = sctk_alloc_module (m, tls_level);
            }
	  sctk_tls_unlock_level (tls_level);
     } 

  tls_module = (char *) tls_level->modules[m - 1];
  res = tls_module + offset;

  return res;
}


static void
sctk_extls_create ()
{
  tls_level **extls;
  int i;

  /* page_size = getpagesize ();
     has been put in sctk_hls_build_repository() */

  extls = sctk_malloc (sctk_extls_max_scope * sizeof (tls_level *));

  for (i = 0; i < sctk_extls_max_scope; i++)
    {
      extls[i] = sctk_malloc (sizeof (tls_level));
      sctk_tls_init_level (extls[i]);
    }

  sctk_nodebug ("Init extls root %p", extls);
  sctk_extls = extls;
}

/*
  take TLS from father
*/
void
sctk_extls_duplicate (void **new)
{
  tls_level **extls;
  tls_level **new_extls;
  int i;

  extls = sctk_extls;
  if (extls == NULL)
    {
      sctk_extls_create ();
    }

  new_extls = sctk_malloc (sctk_extls_max_scope * sizeof (tls_level*));
  *(tls_level***)new = new_extls;

  extls = sctk_extls;
  sctk_nodebug ("Duplicate %p->%p", extls, new_extls);

  for (i = 0; i < sctk_extls_max_scope; i++) 
    {
      new_extls[i] = extls[i];
    }
}

/*
   only keep some tls and create new ones
*/
void
sctk_extls_keep (int *scopes)
{
  tls_level **extls;
  int i;
  extls = sctk_extls;

  assert (extls != NULL);

  for (i = 0; i < sctk_extls_max_scope; i++)
    {
      if (scopes[i] == 0)
	{
	  sctk_nodebug ("Remove %d in %p", i, extls);
	  extls[i] = sctk_malloc (sizeof (tls_level));
	  sctk_tls_init_level (extls[i]);
	}
    }
}

/*
  used by openmp in MPC_OpenMP/src/mpcomp_internal.h
*/
void
sctk_extls_keep_non_current_thread (void **extls, int *scopes)
{
  int i;

  assert (extls != NULL);

  for (i = 0; i < sctk_extls_max_scope; i++)
    {
      if (scopes[i] == 0)
	{
	  sctk_nodebug ("Remove %d in %p", i, extls);
	  extls[i] = sctk_malloc (sizeof (tls_level));
	  sctk_tls_init_level (extls[i]);
	}
    }
}

void
sctk_extls_delete ()
{
#warning "Pas de liberation des extls"
}

/*
 * At MPC startup, create all HLS levels
 * Called in sctk_topology_init in sctk_topology.c
 */
void sctk_hls_build_repository ()
{
  int i,j ;
  const int numa_level_2_number   = sctk_get_numa_number(2) ;
  const int numa_level_1_number   = sctk_get_numa_number(1) ;
  const int socket_number         = sctk_get_socket_number() ;
  const int cache_level_3_number  = sctk_get_cache_number(3) ;
  const int cache_level_2_number  = sctk_get_cache_number(2) ;
  const int cache_level_1_number  = sctk_get_cache_number(1) ;
  const int core_number           = sctk_get_core_number() ;

  const int total_level_below_numa_1 = numa_level_1_number + socket_number 
	  + cache_level_3_number + cache_level_2_number + cache_level_1_number
	  + core_number ;

  sctk_hls_repository = sctk_malloc ( numa_level_1_number*sizeof(hls_level*) );

  for ( i=0 ; i < numa_level_1_number ; ++i ) {
	  const int n = total_level_below_numa_1 + (i == 0)
		  + ( numa_level_2_number > 0 && i % numa_level_2_number == 0 ) ;
	  sctk_hls_repository[i] = sctk_malloc_on_node ( n * sizeof(hls_level), i ) ;
	  for ( j=0 ; j<n ; ++j )
		  sctk_hls_init_level ( sctk_hls_repository[i] + j ) ;
  }

  page_size = getpagesize ();
}

/*
 * During VP initialization, take HLS levels according to the VP topology
 * from the repository of all HLS levels created by sctk_hls_build_repository
 * at initialization.
 * Called in kthread_crate_start_routine in sctk_kernel_thread.c
 *           init_tls_start_routine_arg  in sctk_pthread.c
 *           sctk_start_func             in sctk_thread.c 
 */
void sctk_hls_checkout_on_vp ()
{
  const int numa_level_2_number   = sctk_get_numa_number(2) ;
  const int numa_level_1_number   = sctk_get_numa_number(1) ;
  const int socket_number         = sctk_get_socket_number() ;
  const int cache_level_3_number  = sctk_get_cache_number(3) ;
  const int cache_level_2_number  = sctk_get_cache_number(2) ;
  const int cache_level_1_number  = sctk_get_cache_number(1) ;
  const int core_number           = sctk_get_core_number() ;
  const int vp                    = sctk_thread_get_vp() ;
  const int numa_1_id             = sctk_get_numa_id(1,vp) ;
  int id ;
  int child_id ;
  int i ;
  int offset = 0 ;
  int size_below = socket_number + cache_level_3_number
	  + cache_level_2_number + cache_level_1_number + core_number ;

  if ( sctk_hls[0] == NULL ) {

	  /* if not done already, initialize sctk_hls on this VP */

	  for ( i = 1 ; i < sctk_hls_max_scope ; ++i )
		  sctk_hls[i] = NULL ;
	  
	  sctk_hls[sctk_hls_node_scope] = sctk_hls_repository[0] ;

	  offset += (numa_1_id == 0) ;

	  if ( numa_level_2_number > 0 ) {
		  sctk_hls[sctk_hls_numa_level_2_scope]
			  = sctk_hls_repository[ numa_1_id / numa_level_2_number ]
			  + offset ;
		  offset += (numa_1_id % numa_level_2_number == 0) ;
	  }

	  sctk_hls[sctk_hls_numa_level_1_scope]
		  = sctk_hls_repository[numa_1_id] + offset ;
	  offset += numa_level_1_number ;

	  id = sctk_get_socket_id (vp) ;
	  child_id = id % ( socket_number / numa_level_1_number ) ;
	  sctk_hls[sctk_hls_socket_scope] = sctk_hls_repository[numa_1_id]
		  + offset + child_id * size_below ;  
	  offset += child_id * size_below + 1 ; 
	  size_below -= socket_number ;

	  if (cache_level_3_number > 0 ) {
		  id = sctk_get_cache_id (3, vp) ;
		  child_id = id % ( cache_level_3_number / socket_number ) ;
		  sctk_hls[sctk_hls_cache_level_3_scope] = sctk_hls_repository[numa_1_id]
			  + offset + child_id * size_below ;  
		  offset += child_id * size_below + 1 ; 
		  size_below -= cache_level_3_number ;
	  }

	  if (cache_level_2_number > 0 ) {
		  id = sctk_get_cache_id (2, vp) ;
		  child_id = id % ( cache_level_2_number / cache_level_3_number ) ; 
		  sctk_hls[sctk_hls_cache_level_2_scope] = sctk_hls_repository[numa_1_id]
			  + offset + child_id * size_below ;  
		  offset += child_id * size_below + 1 ; 
		  size_below -= cache_level_2_number ;
	  }

	  if (cache_level_1_number > 0 ) {
		  id = sctk_get_cache_id (1, vp) ;
		  child_id = id % ( cache_level_1_number / cache_level_2_number ) ;
		  sctk_hls[sctk_hls_cache_level_1_scope] = sctk_hls_repository[numa_1_id]
			  + offset + child_id * size_below ;  
		  offset += child_id * size_below + 1 ; 
		  size_below -= cache_level_1_number ;
	  }

	  id = sctk_get_core_id(vp) ;
	  child_id = id % ( core_number / cache_level_1_number ) ;
	  sctk_hls[sctk_hls_core_scope] = sctk_hls_repository[numa_1_id]
		  + offset + child_id * size_below ;

  }

  /* update the number of threads to enter hls levels */

  for ( i = sctk_hls_cache_level_3_scope ; i < sctk_hls_max_scope ; ++i )
	  if ( sctk_hls[i] != NULL )
		  sctk_atomics_incr_int ( &sctk_hls[i]->toenter ) ;

  const int toenter = sctk_atomics_fetch_and_incr_int ( &sctk_hls[sctk_hls_numa_level_1_scope]->toenter ) ;
  if ( toenter == 0 ) {
	  for ( i = 0 ; i < sctk_hls_numa_level_1_scope ; ++i )
		  if ( sctk_hls[i] != NULL )
			  sctk_atomics_incr_int ( &sctk_hls[i]->toenter ) ;
  }
}

/*
 * Not yet called
 */
void sctk_hls_free ()
{
  int i ;
  const int numa_level_1_number   = sctk_get_numa_number(1) ;
  for ( i=0 ; i < numa_level_1_number ; ++i )
	  free ( sctk_hls_repository[i] ) ;
}

#if defined(SCTK_i686_ARCH_SCTK) || defined (SCTK_x86_64_ARCH_SCTK)

void *
__sctk__tls_get_addr__process_scope (tls_index * tmp)
{
  void *res;
  tls_level **extls;
  sctk_nodebug ("__sctk__tls_get_addr__process_scope");
  extls = sctk_extls ;
  assert (extls != NULL);
  res =
    __sctk__tls_get_addr__generic_scope (tmp->ti_module, tmp->ti_offset,
					 extls[sctk_extls_process_scope]);
  return res;
}

void *
__sctk__tls_get_addr__task_scope (tls_index * tmp)
{
  void *res;
  tls_level **extls;
  sctk_nodebug ("__sctk__tls_get_addr__task_scope");
  extls = sctk_extls ;
  assert (extls != NULL);
  res =
    __sctk__tls_get_addr__generic_scope (tmp->ti_module, tmp->ti_offset,
					 extls[sctk_extls_task_scope]);
  return res;
}

void *
__sctk__tls_get_addr__thread_scope (tls_index * tmp)
{
  void *res;
  tls_level **extls;
  sctk_nodebug ("__sctk__tls_get_addr__thread_scope");
  extls = sctk_extls ;
  assert (extls != NULL);
  res =
    __sctk__tls_get_addr__generic_scope (tmp->ti_module, tmp->ti_offset,
					 extls[sctk_extls_thread_scope]);
  return res;
}


#if defined (MPC_OpenMP)
void *
__sctk__tls_get_addr__openmp_scope (tls_index * tmp)
{
  void *res;
  tls_level **extls;
  sctk_nodebug ("__sctk__tls_get_addr__openmp_scope");
  extls = sctk_extls ;
  assert (extls != NULL);
  res =
    __sctk__tls_get_addr__generic_scope (tmp->ti_module, tmp->ti_offset,
					 extls[sctk_extls_openmp_scope]);
  return res;
}
#endif

void *
__sctk__tls_get_addr__node_scope (tls_index * tmp)
{
  void *res;
  sctk_nodebug ("__sctk__tls_get_addr__node_scope %p", &sctk_hls );
  res =
    __sctk__tls_get_addr__generic_scope (tmp->ti_module, tmp->ti_offset,
					 &sctk_hls[sctk_hls_node_scope]->level);
  return res;
}

void *
__sctk__tls_get_addr__numa_level_2_scope (tls_index * tmp)
{
  void *res;
  tls_level **hls;
  sctk_nodebug ("__sctk__tls_get_addr__numa_level_2_scope on numa node %d",
    sctk_get_node_from_cpu(sctk_thread_get_vp()));
  res =
    __sctk__tls_get_addr__generic_scope (tmp->ti_module, tmp->ti_offset,
					 &sctk_hls[sctk_hls_numa_level_2_scope]->level);
  return res;
}

void *
__sctk__tls_get_addr__numa_level_1_scope (tls_index * tmp)
{
  void *res;
  tls_level **hls;
  sctk_nodebug ("__sctk__tls_get_addr__numa_level_1_scope on numa node %d",
    sctk_get_node_from_cpu(sctk_thread_get_vp()));
  res =
    __sctk__tls_get_addr__generic_scope (tmp->ti_module, tmp->ti_offset,
					 &sctk_hls[sctk_hls_numa_level_1_scope]->level);
  return res;
}

void *
__sctk__tls_get_addr__socket_scope (tls_index * tmp)
{
  void *res;
  sctk_nodebug ("__sctk__tls_get_addr__socket_scope");
  res =
    __sctk__tls_get_addr__generic_scope (tmp->ti_module, tmp->ti_offset,
					 &sctk_hls[sctk_hls_socket_scope]->level);
  return res;
}

void *
__sctk__tls_get_addr__cache_level_3_scope (tls_index * tmp)
{
  void *res;
  sctk_nodebug ("__sctk__tls_get_addr__cache_level_3_scope");
  res =
    __sctk__tls_get_addr__generic_scope (tmp->ti_module, tmp->ti_offset,
					 &sctk_hls[sctk_hls_cache_level_3_scope]->level);
  return res;
}

void *
__sctk__tls_get_addr__cache_level_2_scope (tls_index * tmp)
{
  void *res;
  sctk_nodebug ("__sctk__tls_get_addr__cache_level_2_scope");
  res =
    __sctk__tls_get_addr__generic_scope (tmp->ti_module, tmp->ti_offset,
					 &sctk_hls[sctk_hls_cache_level_2_scope]->level);
  return res;
}

void *
__sctk__tls_get_addr__cache_level_1_scope (tls_index * tmp)
{
  void *res;
  sctk_nodebug ("__sctk__tls_get_addr__cache_level_1_scope");
  res =
    __sctk__tls_get_addr__generic_scope (tmp->ti_module, tmp->ti_offset,
					 &sctk_hls[sctk_hls_cache_level_1_scope]->level);
  return res;
}

void *
__sctk__tls_get_addr__core_scope (tls_index * tmp)
{
  void *res;
  sctk_nodebug ("__sctk__tls_get_addr__core_scope");
  res =
    __sctk__tls_get_addr__generic_scope (tmp->ti_module, tmp->ti_offset,
					 &sctk_hls[sctk_hls_core_scope]->level);
  return res;
}


#elif defined (SCTK_ia64_ARCH_SCTK)

void *
__sctk__tls_get_addr__process_scope (size_t m, size_t offset)
{
  void *res;
  tls_level **extls ;
  extls = sctk_extls ;
  assert (extls != NULL);
  res = __sctk__tls_get_addr__generic_scope (m, offset, extls[sctk_extls_process_scope]);
  return res;
}


void *
__sctk__tls_get_addr__task_scope (size_t m, size_t offset)
{
  void *res;
  tls_level **extls ;
  extls = sctk_extls ;
  assert (extls != NULL);
  res = __sctk__tls_get_addr__generic_scope (m, offset, extls[sctk_extls_task_scope]);
  return res;
}


void *
__sctk__tls_get_addr__thread_scope (size_t m, size_t offset)
{
  void *res;
  tls_level **extls ;
  extls = sctk_extls ;
  assert (extls != NULL);
  res = __sctk__tls_get_addr__generic_scope (m, offset, extls[sctk_extls_thread_scope]);
  return res;
}

#if defined (MPC_OpenMP)
void *
__sctk__tls_get_addr__openmp_scope (size_t m, size_t offset)
{
  void *res;
  tls_level **extls ;
  extls = sctk_extls ;
  assert (extls != NULL);
  res = __sctk__tls_get_addr__generic_scope (m, offset, extls[sctk_extls_openmp_scope]);
  return res;
}
#endif

void *
__sctk__tls_get_addr__node_scope (size_t m, size_t offset)
{
  void *res;
  res = __sctk__tls_get_addr__generic_scope (m, offset, sctk_hls[sctk_hls_node_scope].level);
  return res;
}

void *
__sctk__tls_get_addr__numa_scope (size_t m, size_t offset)
{
  void *res;
  res = __sctk__tls_get_addr__generic_scope (m, offset, sctk_hls[sctk_hls_numa_scope].level);
  return res;
}

void *
__sctk__tls_get_addr__socket_scope (size_t m, size_t offset)
{
  void *res;
  res = __sctk__tls_get_addr__generic_scope (m, offset, sctk_hls[sctk_hls_socket_scope].level);
  return res;
}

void *
__sctk__tls_get_addr__cache_scope (size_t m, size_t offset)
{
  void *res;
  res = __sctk__tls_get_addr__generic_scope (m, offset, sctk_hls[sctk_hls_cache_scope].level);
  return res;
}

void *
__sctk__tls_get_addr__core_scope (size_t m, size_t offset)
{
  void *res;
  res = __sctk__tls_get_addr__generic_scope (m, offset, sctk_hls[sctk_hls_core_scope].level);
  return res;
}

#else
#error "Architecture not available for TLS support"
#endif

void __sctk__hls_single_done ( sctk_hls_scope_t scope ) ;

int
__sctk__hls_single ( sctk_hls_scope_t scope ) {
	
	sctk_nodebug ("call to hls single with scope %d", scope) ;

	hls_level * const level = sctk_hls[scope];
	hls_generation_t * const hls_generation = (hls_generation_t*) sctk_hls_generation ;
	const int mygeneration = ++hls_generation[scope].wait ;
	
	if ( scope == sctk_hls_node_scope || scope == sctk_hls_numa_level_2_scope ) {
		if ( __sctk__hls_single ( sctk_hls_numa_level_1_scope ) ) {
			const int entered = sctk_atomics_fetch_and_incr_int ( &level->entered ) ;
			if ( entered == sctk_atomics_load_int(&level->toenter) - 1 ) {
				return 1 ;
			}else{
				sctk_thread_wait_for_value ( &level->generation, mygeneration ) ;
				__sctk__hls_single_done ( sctk_hls_numa_level_1_scope ) ;
				return 0 ;
			}
		}else{
			return 0 ;
		}
	}

	const int entered = sctk_atomics_fetch_and_incr_int ( &level->entered ) ;
	if ( entered == sctk_atomics_load_int(&level->toenter) - 1 ) {
		return 1 ;
	}else{
		sctk_thread_wait_for_value ( &level->generation, mygeneration ) ;
		return 0 ;
	}
}

void
__sctk__hls_single_done ( sctk_hls_scope_t scope ) {

	sctk_nodebug ("call to hls single done with scope %d", scope) ;
	
	hls_generation_t * const hls_generation = (hls_generation_t*) sctk_hls_generation ;
	hls_level * const level = sctk_hls[scope];
	
	sctk_atomics_store_int ( &level->entered, 0 ) ;
	sctk_atomics_read_barrier() ;
	level->generation = hls_generation[scope].wait ;

	if ( scope == sctk_hls_node_scope || scope == sctk_hls_numa_level_2_scope )
		__sctk__hls_single_done ( sctk_hls_numa_level_1_scope ) ;

	return ;
}

void 
__sctk__hls_barrier ( sctk_hls_scope_t scope ) {
	
	sctk_nodebug ("call to hls barrier with scope %d", scope ) ;
	
	hls_generation_t * const hls_generation = (hls_generation_t*) sctk_hls_generation ;
	hls_level * const level = sctk_hls[scope];
	const int mygeneration = ++hls_generation[scope].wait ;
	if ( scope == sctk_hls_node_scope || scope == sctk_hls_numa_level_2_scope ) {
		if ( __sctk__hls_single ( sctk_hls_numa_level_1_scope ) ) {
			const int entered = sctk_atomics_fetch_and_incr_int ( &level->entered ) ;
			if ( entered == sctk_atomics_load_int(&level->toenter) - 1 ) {
				sctk_atomics_store_int ( &level->entered, 0 ) ;
				sctk_atomics_read_barrier() ;
				level->generation = mygeneration ;
			}else{
				sctk_thread_wait_for_value ( &level->generation, mygeneration ) ;
			}
			__sctk__hls_single_done ( sctk_hls_numa_level_1_scope ) ;
		}
		return ;
	}
	
	const int entered = sctk_atomics_fetch_and_incr_int ( &level->entered ) ;
	if ( entered == sctk_atomics_load_int(&level->toenter) - 1 ) {
		sctk_atomics_store_int ( &level->entered, 0 ) ;
		sctk_atomics_read_barrier() ;
		level->generation = mygeneration ;
	}else{
		sctk_thread_wait_for_value ( &level->generation, mygeneration ) ;
	}
}

int
__sctk__hls_single_nowait ( sctk_hls_scope_t scope ) {

	sctk_nodebug ("call to hls single nowait with scope %d", scope) ;

	int execute = 0 ;
	hls_generation_t * const hls_generation = (hls_generation_t*) sctk_hls_generation ;
	hls_level * const level = sctk_hls[scope];
	const int generation = sctk_atomics_load_int ( &level->nowait_generation ) ;
	int * const mygeneration = &hls_generation[scope].nowait ;

	if ( *mygeneration == generation )
		execute = sctk_atomics_cas_int ( &level->nowait_generation, generation, generation+1 ) ; 

	*mygeneration = generation ;
	return execute ;
}

#else
#warning "Experimental Ex-TLS support desactivated"

void
sctk_extls_create ()
{
}

void
sctk_extls_duplicate (void **new)
{
  *new = NULL;
}

void
sctk_extls_keep (int *scopes)
{
}

void
sctk_extls_keep_non_current_thread (void **tls, int *scopes)
{
}

void
sctk_extls_delete ()
{
}

#endif
