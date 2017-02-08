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
/* #   - CARRIBAULT Patrick patrick.carribault@cea.fr                     # */
/* #                                                                      # */
/* ######################################################################## */

#include "mpcomp.h"
#include "sctk_alloc.h"
#include "mpcomp_core.h"
#include "mpcomp_types.h"
#include "sctk_debug.h"
#include "mpcomp_openmp_tls.h"
#include "mpcomp_lock.h"

#include "ompt.h"
extern ompt_callback_t* OMPT_Callbacks;

/**
 *  \fn void omp_init_lock_with_hint( omp_lock_t *lock, omp_lock_hint_t hint)
 *  \brief These routines initialize an OpenMP lock with a hint. The effect of
 * the hint is implementation-defined. The OpenMP implementation can ignore the
 * hint without changing program semantics.
 */
static void __internal_omp_init_lock_with_hint(omp_lock_t *lock, omp_lock_hint_t hint) 
{
	mpcomp_lock_t *mpcomp_user_lock = NULL;

	__mpcomp_init();

  	mpcomp_user_lock = (mpcomp_lock_t *)sctk_malloc(sizeof(mpcomp_lock_t));
  	sctk_assert(mpcomp_user_lock);
  	memset(mpcomp_user_lock, 0, sizeof(mpcomp_lock_t));
  	sctk_thread_mutex_init(&(mpcomp_user_lock->lock), 0);

#if 1 //OMPT_SUPPORT
	if( mpcomp_ompt_is_enabled() )
	{
   	if( OMPT_Callbacks )
   	{
			ompt_callback_lock_init_t callback; 
			callback = (ompt_callback_lock_init_t) OMPT_Callbacks[ompt_callback_lock_init];
			if( callback )
			{
				const void* code_ra = __builtin_return_address(0);	
				const ompt_wait_id_t wait_id = mpcomp_OMPT_gen_wait_id();			
				mpcomp_user_lock->wait_id = wait_id;
				callback( ompt_mutex_lock, hint, mpcomp_mutex, wait_id, code_ra);
			}
		}
	}
#endif /* OMPT_SUPPORT */

  	sctk_assert(lock);
  	*lock = mpcomp_user_lock;
  	mpcomp_user_lock->hint = hint;
}

void omp_init_lock_with_hint(omp_lock_t *lock, omp_lock_hint_t hint) 
{
  	__internal_omp_init_lock_with_hint( lock, hint );
}

/**
 *  \fn void omp_init_lock(omp_lock_t *lock)
 *  \brief The effect of these routines is to initialize the lock to the
 * unlocked state; that is, no task owns the lock. (OpenMP 4.5 p272)
 *  \param omp_lock_t adress of a lock address (\see mpcomp.h)
 */
void omp_init_lock(omp_lock_t *lock) 
{
  	__internal_omp_init_lock_with_hint( lock, omp_lock_hint_none );
}

void omp_destroy_lock(omp_lock_t *lock) 
{
  mpcomp_lock_t *mpcomp_user_lock = NULL;
  sctk_assert(lock);
  mpcomp_user_lock = (mpcomp_lock_t *)*lock;

#if 1 //OMPT_SUPPORT
	if( mpcomp_ompt_is_enabled() )
	{
   	if( OMPT_Callbacks )
   	{
			ompt_callback_lock_destroy_t callback; 
			callback = (ompt_callback_lock_destroy_t) OMPT_Callbacks[ompt_callback_lock_destroy];
			if( callback )
			{
				const void* code_ra = __builtin_return_address(0);	
				const ompt_wait_id_t wait_id = mpcomp_user_lock->wait_id;			
				callback( ompt_mutex_lock, wait_id, code_ra);
			}
		}
	}
#endif /* OMPT_SUPPORT */

  sctk_thread_mutex_destroy(&(mpcomp_user_lock->lock));
  sctk_free(mpcomp_user_lock);
  *lock = NULL;
}

void omp_set_lock(omp_lock_t *lock) 
{
  mpcomp_lock_t *mpcomp_user_lock = NULL;
  sctk_assert(lock);
  mpcomp_user_lock = (mpcomp_lock_t *)*lock;

#if 1 //OMPT_SUPPORT
	if( mpcomp_ompt_is_enabled() )
	{
   	if( OMPT_Callbacks )
   	{
			ompt_callback_mutex_acquire_t callback; 
			callback = (ompt_callback_mutex_acquire_t) OMPT_Callbacks[ompt_callback_mutex_acquire];
			if( callback )
			{
				const void* code_ra = __builtin_return_address(0);	
				const ompt_wait_id_t wait_id = mpcomp_user_lock->wait_id;			
  				const unsigned hint = mpcomp_user_lock->hint;
				callback( ompt_mutex_lock, hint, mpcomp_mutex, wait_id, code_ra);
			}
		}
	}
#endif /* OMPT_SUPPORT */

  sctk_thread_mutex_lock(&(mpcomp_user_lock->lock));

#if 1 //OMPT_SUPPORT
	if( mpcomp_ompt_is_enabled() )
	{
   	if( OMPT_Callbacks )
   	{
			ompt_callback_mutex_t callback; 
			callback = (ompt_callback_mutex_t) OMPT_Callbacks[ompt_callback_mutex_acquired];
			if( callback )
			{
				const void* code_ra = __builtin_return_address(0);	
				const ompt_wait_id_t wait_id = mpcomp_user_lock->wait_id;			
				callback( ompt_mutex_lock, wait_id, code_ra);
			}
		}
	}
#endif /* OMPT_SUPPORT */

}

void omp_unset_lock(omp_lock_t *lock) 
{
  mpcomp_lock_t *mpcomp_user_lock = NULL;
  sctk_assert(lock);
  mpcomp_user_lock = (mpcomp_lock_t *)*lock;
  sctk_thread_mutex_unlock(&(mpcomp_user_lock->lock));

#if 1 //OMPT_SUPPORT
	if( mpcomp_ompt_is_enabled() )
	{
   	if( OMPT_Callbacks )
   	{
			ompt_callback_mutex_t callback; 
			callback = (ompt_callback_mutex_t) OMPT_Callbacks[ompt_callback_mutex_released];
			if( callback )
			{
				const void* code_ra = __builtin_return_address(0);	
				const ompt_wait_id_t wait_id = mpcomp_user_lock->wait_id;			
				callback( ompt_mutex_lock, wait_id, code_ra);
			}
		}
	}
#endif /* OMPT_SUPPORT */
}

int omp_test_lock(omp_lock_t *lock) 
{
  int retval; 
  mpcomp_lock_t *mpcomp_user_lock = NULL;
  sctk_assert(lock);
  mpcomp_user_lock = (mpcomp_lock_t *)*lock;

#if 1 //OMPT_SUPPORT
	if( mpcomp_ompt_is_enabled() )
	{
   	if( OMPT_Callbacks )
   	{
			ompt_callback_mutex_acquire_t callback; 
			callback = (ompt_callback_mutex_acquire_t) OMPT_Callbacks[ompt_callback_mutex_acquire];
			if( callback )
			{
				const void* code_ra = __builtin_return_address(0);	
				const ompt_wait_id_t wait_id = mpcomp_user_lock->wait_id;			
  				const unsigned hint = mpcomp_user_lock->hint;
				callback( ompt_mutex_lock, hint, mpcomp_mutex, wait_id, code_ra);
			}
		}
	}
#endif /* OMPT_SUPPORT */

  retval = !sctk_thread_mutex_trylock(&(mpcomp_user_lock->lock));

#if 1 //OMPT_SUPPORT
	if( retval && mpcomp_ompt_is_enabled() )
	{
   	if( OMPT_Callbacks )
   	{
			ompt_callback_mutex_t callback; 
			callback = (ompt_callback_mutex_t) OMPT_Callbacks[ompt_callback_mutex_acquired];
			if( callback )
			{
				const void* code_ra = __builtin_return_address(0);	
				const ompt_wait_id_t wait_id = mpcomp_user_lock->wait_id;			
				callback( ompt_mutex_lock, wait_id, code_ra);
			}
		}
	}
#endif /* OMPT_SUPPORT */

  return retval;
}

static void __internal_omp_init_nest_lock_with_hint(omp_nest_lock_t *lock, omp_lock_hint_t hint) 
{
 	mpcomp_nest_lock_t *mpcomp_user_nest_lock = NULL;
  	mpcomp_user_nest_lock = (mpcomp_nest_lock_t *)sctk_malloc(sizeof(mpcomp_nest_lock_t));
  	sctk_assert(mpcomp_user_nest_lock);
  	memset(mpcomp_user_nest_lock, 0, sizeof(mpcomp_nest_lock_t));
  	sctk_thread_mutex_init(&(mpcomp_user_nest_lock->lock), 0);

#if 1 //OMPT_SUPPORT
	if( mpcomp_ompt_is_enabled() )
	{
   	if( OMPT_Callbacks )
   	{
			ompt_callback_lock_init_t callback; 
			callback = (ompt_callback_lock_init_t) OMPT_Callbacks[ompt_callback_lock_init];
			if( callback )
			{
				const void* code_ra = __builtin_return_address(0);	
				const ompt_wait_id_t wait_id = mpcomp_OMPT_gen_wait_id();			
				mpcomp_user_nest_lock->wait_id = wait_id;
				callback( ompt_mutex_nest_lock, hint, mpcomp_mutex, wait_id, code_ra);
			}
		}
	}
#endif /* OMPT_SUPPORT */

  	sctk_assert(lock);
  	*lock = mpcomp_user_nest_lock;
   mpcomp_user_nest_lock->hint = hint;
}

/**
 *  \fn void omp_init_nest_lock(omp_lock_t *lock)
 *  \brief The effect of these routines is to initialize the lock to the
 * unlocked state; that is, no task owns the lock. (OpenMP 4.5 p272)
 * \param omp_lock_t adress of a lock address (\see mpcomp.h)
 */
void omp_init_nest_lock(omp_nest_lock_t *lock) 
{
  	__internal_omp_init_nest_lock_with_hint( lock, omp_lock_hint_none );
}

void omp_init_nest_lock_with_hint(omp_nest_lock_t *lock, omp_lock_hint_t hint) 
{
  __internal_omp_init_nest_lock_with_hint( lock, hint );
}

void omp_destroy_nest_lock(omp_nest_lock_t *lock) {
  mpcomp_nest_lock_t *mpcomp_user_nest_lock = NULL;
  sctk_assert(lock);
  mpcomp_user_nest_lock = (mpcomp_nest_lock_t *)*lock;

#if 1 //OMPT_SUPPORT
	if( mpcomp_ompt_is_enabled() )
	{
   	if( OMPT_Callbacks )
   	{
			ompt_callback_lock_destroy_t callback; 
			callback = (ompt_callback_lock_destroy_t) OMPT_Callbacks[ompt_callback_lock_destroy];
			if( callback )
			{
				const void* code_ra = __builtin_return_address(0);	
				const ompt_wait_id_t wait_id = mpcomp_user_nest_lock->wait_id;			
				callback( ompt_mutex_nest_lock, wait_id, code_ra);
			}
		}
	}
#endif /* OMPT_SUPPORT */

  sctk_thread_mutex_destroy(&(mpcomp_user_nest_lock->lock));
  sctk_free(mpcomp_user_nest_lock);
  *lock = NULL;
}

void omp_set_nest_lock(omp_nest_lock_t *lock) {
  mpcomp_nest_lock_t *mpcomp_user_nest_lock;
  __mpcomp_init();
  mpcomp_thread_t *thread = mpcomp_get_thread_tls();
  sctk_assert(lock);
  mpcomp_user_nest_lock = (mpcomp_nest_lock_t *)*lock;

#if 1 //OMPT_SUPPORT
	if( mpcomp_ompt_is_enabled() )
	{
   	if( OMPT_Callbacks )
   	{
			ompt_callback_mutex_acquire_t callback; 
			callback = (ompt_callback_mutex_acquire_t) OMPT_Callbacks[ompt_callback_mutex_acquire];
			if( callback )
			{
				const void* code_ra = __builtin_return_address(0);	
				const ompt_wait_id_t wait_id = mpcomp_user_nest_lock->wait_id;			
  				const unsigned hint = mpcomp_user_nest_lock->hint;
				callback( ompt_mutex_nest_lock, hint, mpcomp_mutex, wait_id, code_ra);
			}
		}
	}
#endif /* OMPT_SUPPORT */

#if MPCOMP_TASK
  if (mpcomp_nest_lock_test_task(thread, mpcomp_user_nest_lock))
#else
  if (mpcomp_user_nest_lock->owner_thread != (void *)thread)
#endif
  {
    sctk_thread_mutex_lock(&(mpcomp_user_nest_lock->lock));
    mpcomp_user_nest_lock->owner_thread = thread;
#if MPCOMP_TASK
    mpcomp_user_nest_lock->owner_task =
        MPCOMP_TASK_THREAD_GET_CURRENT_TASK(thread);
#endif
  }

#if 1 //OMPT_SUPPORT
	if( mpcomp_ompt_is_enabled() )
	{
   	if( OMPT_Callbacks )
   	{
			ompt_callback_mutex_t callback; 
			callback = (ompt_callback_mutex_t) OMPT_Callbacks[ompt_callback_mutex_acquired];
			if( callback )
			{
				const void* code_ra = __builtin_return_address(0);	
				const ompt_wait_id_t wait_id = mpcomp_user_nest_lock->wait_id;			
				callback( ompt_mutex_lock, wait_id, code_ra);
			}
		
			if(mpcomp_user_nest_lock->nb_nested == 0)
			{
				ompt_callback_nest_lock_t callback_nest;
				callback_nest = (ompt_callback_nest_lock_t) OMPT_Callbacks[ompt_callback_nest_lock];
				if( callback_nest )
				{
					const void* code_ra = __builtin_return_address(0);	
					const ompt_wait_id_t wait_id = mpcomp_user_nest_lock->wait_id;			
					callback_nest( ompt_scope_begin, wait_id, code_ra);
				}
			}
		}
	
	}
#endif /* OMPT_SUPPORT */

	mpcomp_user_nest_lock->nb_nested += 1;

}

void omp_unset_nest_lock(omp_nest_lock_t *lock) {
  mpcomp_nest_lock_t *mpcomp_user_nest_lock = NULL;
  sctk_assert(lock);
  mpcomp_user_nest_lock = (mpcomp_nest_lock_t *)*lock;

#if 1 //OMPT_SUPPORT
	if( mpcomp_ompt_is_enabled() )
	{
   	if( OMPT_Callbacks )
   	{
			ompt_callback_mutex_t callback; 
			callback = (ompt_callback_mutex_t) OMPT_Callbacks[ompt_callback_mutex_released];
			if( callback )
			{
				const void* code_ra = __builtin_return_address(0);	
				const ompt_wait_id_t wait_id = mpcomp_user_nest_lock->wait_id;			
				callback( ompt_mutex_lock, wait_id, code_ra);
			}
		}
	}
#endif /* OMPT_SUPPORT */

  mpcomp_user_nest_lock->nb_nested -= 1;

  if (mpcomp_user_nest_lock->nb_nested == 0) {
    mpcomp_user_nest_lock->owner_thread = NULL;
#if MPCOMP_TASK
    mpcomp_user_nest_lock->owner_task = NULL;
#endif
	if( mpcomp_ompt_is_enabled() )
	{
   	if( OMPT_Callbacks )
   	{
			ompt_callback_nest_lock_t callback_nest;
			callback_nest = (ompt_callback_nest_lock_t) OMPT_Callbacks[ompt_callback_nest_lock];
			if( callback_nest )
			{
				const void* code_ra = __builtin_return_address(0);	
				const ompt_wait_id_t wait_id = mpcomp_user_nest_lock->wait_id;			
				callback_nest( ompt_scope_end, wait_id, code_ra);
			}
		}
	 }
    sctk_thread_mutex_unlock(&(mpcomp_user_nest_lock->lock));
  }
}

int omp_test_nest_lock(omp_nest_lock_t *lock) {
  mpcomp_nest_lock_t *mpcomp_user_nest_lock;
  __mpcomp_init();
  mpcomp_thread_t *thread = mpcomp_get_thread_tls();
  sctk_assert(lock);
  mpcomp_user_nest_lock = (mpcomp_nest_lock_t *)*lock;

#if MPCOMP_TASK
  if (mpcomp_nest_lock_test_task(thread, mpcomp_user_nest_lock))
#else
  if (mpcomp_user_nest_lock->owner_thread != (void *)t)
#endif
  {
    if (sctk_thread_mutex_trylock(&(mpcomp_user_nest_lock->lock))) {
      return 0;
    }
    mpcomp_user_nest_lock->owner_thread = (void *)thread;
#if MPCOMP_TASK
    mpcomp_user_nest_lock->owner_task =
        MPCOMP_TASK_THREAD_GET_CURRENT_TASK(thread);
#endif
  }
  mpcomp_user_nest_lock->nb_nested += 1;
  return mpcomp_user_nest_lock->nb_nested;
}
