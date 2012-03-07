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
#include <malloc.h>


#include "sctk_config.h"
#ifdef MPC_Allocator
#include "sctk_alloc.h"
#include "mpcalloc.h"
#include "sctk.h"

/* void *(*__mpc_malloc_hook)(size_t size, const void *caller); */

/* void *(*__mpc_realloc_hook)(void *ptr, size_t size, const void *caller); */

/* void *(*__mpc_memalign_hook)(size_t alignment, size_t size, const void *caller); */

/* void (*__mpc_free_hook)(void *ptr, const void *caller); */

/* void (*__mpc_malloc_initialize_hook)(void); */

/* void (*__mpc_after_morecore_hook)(void); */

/*******************************************/
/*************** libc *alloc ***************/
/*******************************************/
void *
malloc (size_t size)
{
  void *tmp;
  SCTK_PROFIL_START (malloc);

  tmp = sctk_malloc (size);

  SCTK_PROFIL_END (malloc);
  return tmp;
}

void *
tmp_malloc (size_t size)
{
  return sctk_tmp_malloc (size);
}

void *
calloc (size_t nmemb, size_t size)
{
  void *tmp;
  SCTK_PROFIL_START (calloc);
  tmp = sctk_calloc (nmemb, size);
  SCTK_PROFIL_END (calloc);
  return tmp;
}

void *
realloc (void *ptr, size_t size)
{
  void *tmp;
  SCTK_PROFIL_START (realloc);
  tmp = sctk_realloc (ptr, size);
  SCTK_PROFIL_END (realloc);
  return tmp;
}

void
free (void *ptr)
{
  SCTK_PROFIL_START (free);
  sctk_free (ptr);
  SCTK_PROFIL_END (free);
}

void
cfree (void *ptr)
{
  sctk_cfree (ptr);
}

int
posix_memalign (void **memptr, size_t alignment, size_t size)
{
  return sctk_posix_memalign (memptr, alignment, size);
}

void *
valloc (size_t size)
{
  return sctk_valloc (size);
}

void *
memalign (size_t boundary, size_t size)
{
  return sctk_memalign (boundary, size);
}

#endif
