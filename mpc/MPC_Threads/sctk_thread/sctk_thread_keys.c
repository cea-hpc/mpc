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
#include <string.h>
#include "sctk_config.h"
#include "sctk_debug.h"
#include "sctk_thread.h"
#include "sctk_internal_thread.h"
#include "sctk_thread_keys.h"
#include "sctk_spinlock.h"

typedef void (*__destr_function_key_t) (void *) ;

static sctk_spinlock_t key_lock = SCTK_SPINLOCK_INITIALIZER;
static char sctk_key_used[SCTK_THREAD_KEYS_MAX];
static __destr_function_key_t __destr_function_key [SCTK_THREAD_KEYS_MAX];

int
sctk_thread_generic_keys_setspecific (sctk_thread_key_t __key, const void *__pointer,
				      sctk_thread_generic_keys_t* keys)
{
  if(sctk_key_used[__key] == 1){
    keys->keys[__key] = __pointer;
    return 0;
  } else {
    return SCTK_EINVAL;
  }
}

void *
sctk_thread_generic_keys_getspecific (sctk_thread_key_t __key,
				      sctk_thread_generic_keys_t* keys)
{
  if(sctk_key_used[__key] == 1){
    return keys->keys[__key];
  } else {
    return NULL;
  }
}

int
sctk_thread_generic_keys_key_create (sctk_thread_key_t * __key,
				void (*__destr_function) (void *),sctk_thread_generic_keys_t* keys)
{
  int i;
  sctk_spinlock_lock(&key_lock);
  for(i = 0; i < SCTK_THREAD_KEYS_MAX; i++){
    if(sctk_key_used[i] == 0){
      sctk_key_used[i] = 1;
      __destr_function_key[i] = __destr_function;
      __key = i;
    }
  }
  sctk_spinlock_unlock(&key_lock);
  if(i == SCTK_THREAD_KEYS_MAX){
    return SCTK_EAGAIN;
  }
  return 0;
}

int
sctk_thread_generic_keys_key_delete (sctk_thread_key_t __key,sctk_thread_generic_keys_t* keys)
{
  not_implemented ();
  return 0;
}

void sctk_thread_generic_keys_init(){  
  int i; 
  sctk_thread_generic_check_size (int, sctk_thread_key_t);
  for(i = 0; i < SCTK_THREAD_KEYS_MAX; i++)
    {
      sctk_key_used[i] = 0;
      __destr_function_key[i] = NULL;
    } 
}

void sctk_thread_generic_keys_init_thread(sctk_thread_generic_keys_t* keys){ 
  int i; 
  
  for (i = 0; i < SCTK_THREAD_KEYS_MAX; i++)
    {
      keys->keys[i] = NULL;
    } 
}
