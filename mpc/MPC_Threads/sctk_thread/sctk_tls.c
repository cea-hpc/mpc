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


#define MAX_ACTIVE_SINGLE (16)
/* fix -> adapt to hls scope
   # of VPs in this scope */

typedef struct
{
  tls_level level ;
  size_t toenter ;
  /* total number of VPs on this level */
  volatile size_t barrier_entered[2] ;
  volatile size_t barrier_generation ;
  /* number of VPs that entered the hls barrier */ 
  volatile size_t single_entered[MAX_ACTIVE_SINGLE];
  volatile size_t single_generation[MAX_ACTIVE_SINGLE];
} hls_level;

static __thread hls_level* sctk_hls[sctk_hls_max_scope];
static hls_level *sctk_hls_repository ;

static __thread size_t single_nowait_counter[sctk_hls_max_scope] ;

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
	level->barrier_entered[0] = 0 ;
	level->barrier_entered[1] = 0 ;
	level->barrier_generation = 0 ;
	level->toenter = 0 ;
	/* toenter is to be filled by sctk_hls_build_repository */
	for ( i = 0 ; i < MAX_ACTIVE_SINGLE ; ++i ) {
		level->single_entered[i] = 0 ;
		level->single_generation[i] = 0 ;
	}
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
  tls_level *extls;
  tls_level *new_extls;
  int i;

  extls = sctk_extls;
  if (extls == NULL)
    {
      sctk_extls_create ();
    }

  new_extls = sctk_malloc (sctk_extls_max_scope * sizeof (tls_level));
  *new = new_extls;

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
  /* TODO: take values from sctk_topology */
  int i, j ;
  const int numa_number   = sctk_get_numa_node_number() ;
  const int socket_number = numa_number ;
  const int cache_number  = numa_number ;
  const int core_number   = sctk_get_cpu_number() ;
  const int total_level_number = 1 + numa_number + socket_number
                                 + cache_number + core_number ;

  sctk_hls_repository = sctk_malloc ( total_level_number*sizeof(hls_level) );
  for ( i = 0; i < total_level_number; ++i )
    sctk_hls_init_level ( &sctk_hls_repository[i] ) ;

  /* for now we assume 1 VP per core */
  /* TODO: modify with real values defined at startup */
  i = 0 ;
  sctk_hls_repository[i++].toenter = core_number ; 
  for ( j=0; j < numa_number; ++j)
	  sctk_hls_repository[i++].toenter = core_number / numa_number ; 
  for ( j=0; j < socket_number; ++j)
	  sctk_hls_repository[i++].toenter = core_number / socket_number ; 
  for ( j=0; j < cache_number; ++j)
	  sctk_hls_repository[i++].toenter = core_number / cache_number ; 
  for ( j=0; j < core_number; ++j)
	  sctk_hls_repository[i++].toenter = 1 ; 
  
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
  /* TODO: take values from sctk_topology */
  const int numa_number   = sctk_get_numa_node_number() ;
  const int socket_number = numa_number ;
  const int cache_number  = numa_number ;
  const int core_number   = sctk_get_cpu_number() ;
  const int core_id       = sctk_thread_get_vp() ;
  const int numa_id       = sctk_get_node_from_cpu(core_id) ;
  const int socket_id     = numa_id ;
  const int cache_id      = numa_id ;
  int offset = 0 ;

  /* check if already initialized */  
  if ( sctk_hls[sctk_hls_node_scope] != NULL )
    return ; 

  sctk_hls[sctk_hls_node_scope]   = &sctk_hls_repository[0] ;
  offset += 1 ;
  sctk_hls[sctk_hls_numa_scope]   = &sctk_hls_repository[offset+numa_id] ;  
  offset += numa_number ;  
  sctk_hls[sctk_hls_socket_scope] = &sctk_hls_repository[offset+socket_id] ;  
  offset += socket_number ;  
  sctk_hls[sctk_hls_cache_scope]  = &sctk_hls_repository[offset+cache_id] ;  
  offset += cache_number ;  
  sctk_hls[sctk_hls_core_scope]   = &sctk_hls_repository[offset+core_id] ;  
}

/*
 * Not yet called
 */
void sctk_hls_free ()
{
  free ( sctk_hls_repository ) ;
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
__sctk__tls_get_addr__numa_scope (tls_index * tmp)
{
  void *res;
  tls_level **hls;
  sctk_nodebug ("__sctk__tls_get_addr__numa_scope on numa node %d",
    sctk_get_node_from_cpu(sctk_thread_get_vp()));
  res =
    __sctk__tls_get_addr__generic_scope (tmp->ti_module, tmp->ti_offset,
					 &sctk_hls[sctk_hls_numa_scope]->level);
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
__sctk__tls_get_addr__cache_scope (tls_index * tmp)
{
  void *res;
  sctk_nodebug ("__sctk__tls_get_addr__cache_scope");
  res =
    __sctk__tls_get_addr__generic_scope (tmp->ti_module, tmp->ti_offset,
					 &sctk_hls[sctk_hls_cache_scope]->level);
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
	/* TODO: several tasks on same VP ? */
	hls_level * const level = sctk_hls[scope];
	const size_t generation = level->barrier_generation;
	
	sctk_nodebug ("call to hls single with scope %d", scope) ;
	
	if ( scope == sctk_hls_core_scope )
		return 1;
	
	if ( scope == sctk_hls_node_scope && sctk_get_numa_node_number() > 1 ) {
		const size_t toenter = sctk_get_numa_node_number() ;
		if ( __sctk__hls_single ( sctk_hls_numa_scope ) ) {
			sctk_spinlock_lock ( &level->level.lock ) ;
			if ( level->barrier_entered[generation] == toenter - 1 ) {
				sctk_spinlock_unlock ( &level->level.lock ) ;
				return 1;
			}else{
				level->barrier_entered[generation] = 1 ;
				sctk_spinlock_unlock ( &level->level.lock ) ;
				while ( level->barrier_entered[generation] != 0 )
					sctk_thread_yield();
				__sctk__hls_single_done ( sctk_hls_numa_scope ) ;
				return 0 ;
			}
		}else{
			return 0 ;
		}
	}

	sctk_spinlock_lock ( &level->level.lock ) ;
	if ( level->barrier_entered[generation] == level->toenter - 1 ) {
		sctk_spinlock_unlock ( &level->level.lock ) ;
		return 1;
	}else{
		level->barrier_entered[generation] += 1 ;
		sctk_spinlock_unlock ( &level->level.lock ) ;
		while ( level->barrier_entered[generation] != 0 )
			sctk_thread_yield() ;
		return 0;
	}
}

void
__sctk__hls_single_done ( sctk_hls_scope_t scope ) {
	hls_level * const level = sctk_hls[scope];
	const size_t generation = level->barrier_generation;
	
	sctk_nodebug ("call to hls single done with scope %d", scope) ;
	
	if ( scope == sctk_hls_core_scope )
		return ;

	level->barrier_generation = 1 - generation ;
	asm volatile("" ::: "memory");
	level->barrier_entered[generation] = 0 ;
	
	if ( scope == sctk_hls_node_scope )
		__sctk__hls_single_done ( sctk_hls_numa_scope ) ;

	return ;
}

void 
__sctk__hls_barrier ( sctk_hls_scope_t scope ) {
	/* TODO: synchronize tasks on same VP */
	/* we assume only one task per VP */
	hls_level * const level = sctk_hls[scope];
	const size_t generation = level->barrier_generation ;
	
	sctk_nodebug ("call to hls barrier with scope %d", scope ) ;

	if ( scope == sctk_hls_core_scope )
		return ;

	if ( scope == sctk_hls_node_scope && sctk_get_numa_node_number() > 1 ) {
		const size_t toenter = sctk_get_numa_node_number() ;
		if ( __sctk__hls_single ( sctk_hls_numa_scope ) ) {
			sctk_spinlock_lock ( &level->level.lock ) ;
			if ( level->barrier_entered[generation] == toenter - 1 ) {
				level->barrier_generation = 1 - generation ;
				asm volatile("" ::: "memory");
				level->barrier_entered[generation] = 0 ;
				sctk_spinlock_unlock ( &level->level.lock ) ;
			}else{
				level->barrier_entered[generation] += 1 ;
				sctk_spinlock_unlock ( &level->level.lock ) ;
				while ( level->barrier_entered[generation] != 0 )
					sctk_thread_yield();
			}
			__sctk__hls_single_done ( sctk_hls_numa_scope ) ;
		}
		return ;
	}

	sctk_spinlock_lock ( &level->level.lock ) ;
	if ( level->barrier_entered[generation] == level->toenter - 1 ) {
		level->barrier_generation = 1 - generation ;
		asm volatile("" ::: "memory");
		level->barrier_entered[generation] = 0 ;
		sctk_spinlock_unlock ( &level->level.lock ) ;
	}else{
		level->barrier_entered[generation] += 1 ;
		sctk_spinlock_unlock ( &level->level.lock ) ;
		while ( level->barrier_entered[generation] != 0 )
			sctk_thread_yield() ;
	}
}

int
__sctk__hls_single_nowait ( sctk_hls_scope_t scope ) {
	/* TODO: several tasks on same VP ? */
	hls_level * const level = sctk_hls[scope];
	const size_t nowait_index = single_nowait_counter[scope] % MAX_ACTIVE_SINGLE ;
	volatile size_t	* const entered = &level->single_entered[nowait_index];
	volatile size_t	* const generation = &level->single_generation[nowait_index];
	size_t entered_cached ; /* avoid an additional read in last if branch */
	size_t generation_cached ; /* avoid an additional read in while loop */
	++single_nowait_counter[scope] ;

	sctk_nodebug ("call to hls single nowait with scope %d", scope) ;
	
	if ( scope == sctk_hls_core_scope)
		return 1 ;

	sctk_spinlock_lock ( &level->level.lock ) ;
	if ( *entered > 0 && single_nowait_counter[scope] > *generation ) {
		/* in advance, wait for last thread of corresponding single */
		sctk_spinlock_unlock ( &level->level.lock ) ;
		/* same as *generation != 0 && *generation != single_nowait_counter[scope] */
		while ( ( generation_cached = *generation ) != 0 && generation_cached != single_nowait_counter[scope] )
			sctk_thread_yield();
		/* this cannot happen twice */
		sctk_spinlock_lock ( &level->level.lock ) ;
	}

	entered_cached = *entered ;
	if ( entered_cached == 0 ) {
		*entered = 1 ;
		*generation = single_nowait_counter[scope] ;
		sctk_spinlock_unlock ( &level->level.lock ) ;
		sctk_nodebug ("hls single nowait executed (first thread)") ;
		return 1 ;
	}else if ( entered_cached == level->toenter-1 ) {
		*entered = 0 ;
		asm volatile("" ::: "memory");
		*generation = 0 ;
		sctk_spinlock_unlock ( &level->level.lock ) ;
		sctk_nodebug ("hls single nowait not executed (last thread)") ;
		return 0 ;
	}else{
		*entered = entered_cached + 1 ;
		sctk_spinlock_unlock ( &level->level.lock ) ;
		sctk_nodebug ("hls single nowait not executed") ;
		return 0 ;
	}
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
