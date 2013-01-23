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
#ifndef __SCTK_THREAD_KEYS_H_
#define __SCTK_THREAD_KEYS_H_

#include <stdlib.h>
#include <string.h>
#include "sctk_config.h"
#include "sctk_debug.h"
#include "sctk_thread.h"
#include "sctk_internal_thread.h"

typedef struct {
  void* keys[SCTK_THREAD_KEYS_MAX];
}sctk_thread_generic_keys_t;

void sctk_thread_generic_keys_init(); 
void sctk_thread_generic_keys_init_thread(sctk_thread_generic_keys_t* keys);
int
sctk_thread_generic_keys_setspecific (sctk_thread_key_t __key, const void *__pointer,sctk_thread_generic_keys_t* keys);
void *
sctk_thread_generic_keys_getspecific (sctk_thread_key_t __key,sctk_thread_generic_keys_t* keys);
int
sctk_thread_generic_keys_key_create (sctk_thread_key_t * __key,
				void (*__destr_function) (void *),sctk_thread_generic_keys_t* keys);
int
sctk_thread_generic_keys_key_delete (sctk_thread_key_t __key,sctk_thread_generic_keys_t* keys);

inline void
sctk_thread_generic_keys_key_delete_all ( sctk_thread_generic_keys_t* keys );

#endif
