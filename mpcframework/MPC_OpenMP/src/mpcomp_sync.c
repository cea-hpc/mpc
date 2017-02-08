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
/* #                                                                      # */
/* ######################################################################## */

#include "sctk_debug.h"
#include "mpcomp_types.h"

#include "ompt.h"
extern ompt_callback_t* OMPT_Callbacks;

/*
   This file contains all synchronization functions related to OpenMP
 */

/* TODO move these locks (atomic and critical) in a per-task structure:
   - mpcomp_thread_info, (every instance sharing the same lock) 
   - Key
   - TLS if available
 */
TODO("OpenMP: Anonymous critical and global atomic are not per-task locks")
 
#if 1 // OMPT_SUPPORT
/* Atomic emulated */
static ompt_wait_id_t __mpcomp_ompt_atomic_lock_wait_id = 0;
static sctk_atomics_int __mpcomp_atomic_lock_init_once 		= OPA_INT_T_INITIALIZER(0); 
/* Critical anonymous */
static ompt_wait_id_t __mpcomp_ompt_critical_lock_wait_id = 0;
static sctk_atomics_int __mpcomp_critical_lock_init_once 	= OPA_INT_T_INITIALIZER(0); 
#endif // OMPT_SUPPORT

static sctk_spinlock_t global_atomic_lock = SCTK_SPINLOCK_INITIALIZER;
static sctk_thread_mutex_t global_critical_lock = SCTK_THREAD_MUTEX_INITIALIZER;
static sctk_spinlock_t global_allocate_named_lock = SCTK_SPINLOCK_INITIALIZER;

void __mpcomp_atomic_begin(void) 
{
	if( !sctk_atomics_load_int(&__mpcomp_atomic_lock_init_once) )
	{
		//prevent multi call to init 
		if( !sctk_atomics_cas_int( &__mpcomp_atomic_lock_init_once, 0, 1))
		{
			sctk_spinlock_init(&global_atomic_lock, SCTK_SPINLOCK_INITIALIZER );

#if 1 //OMPT_SUPPORT
			if( mpcomp_ompt_is_enabled() )
   		{
      		if( OMPT_Callbacks )
      		{
        	 		ompt_callback_lock_init_t callback_init;
					/* Prevent non thread safe wait_id init */
            	__mpcomp_ompt_atomic_lock_wait_id = mpcomp_OMPT_gen_wait_id();
         		callback_init = (ompt_callback_lock_init_t) OMPT_Callbacks[ompt_callback_lock_init];
         		if( callback_init )
         		{
            		const void* code_ra = __builtin_return_address(0);
            		callback_init( ompt_mutex_atomic, omp_lock_hint_none, mpcomp_spinlock, __mpcomp_ompt_atomic_lock_wait_id, code_ra);
					}
				}
			}
#endif //OMPT_SUPPORT
			sctk_atomics_store_int( &__mpcomp_atomic_lock_init_once, 2 );
		}
		else
		{
			/* Wait lock init */
			while( sctk_atomics_load_int( &__mpcomp_atomic_lock_init_once ) != 2 )
			{
				sctk_cpu_relax();
			}
		}	
	}	
#if 1 //OMPT_SUPPORT
		if( mpcomp_ompt_is_enabled() )
		{
			if( OMPT_Callbacks )
			{
				ompt_callback_mutex_acquire_t callback_acquire;
				callback_acquire = (ompt_callback_mutex_acquire_t) OMPT_Callbacks[ompt_callback_mutex_acquire];
				if( callback_acquire )
				{
					const void* code_ra = __builtin_return_address(0);
					callback_acquire( ompt_mutex_atomic, omp_lock_hint_none, mpcomp_spinlock, __mpcomp_ompt_atomic_lock_wait_id, code_ra);	
				}
			}
		}
#endif //OMPT_SUPPORT
			
		sctk_spinlock_lock(&global_atomic_lock);

#if 1 //OMPT_SUPPORT
		if( mpcomp_ompt_is_enabled() )
		{
			if( OMPT_Callbacks )
			{
				ompt_callback_mutex_t callback_acquired;
				callback_acquired = (ompt_callback_mutex_t) OMPT_Callbacks[ompt_callback_mutex_acquired];
				if( callback_acquired )
				{
					const void* code_ra = __builtin_return_address(0);
					callback_acquired( ompt_mutex_atomic, __mpcomp_ompt_atomic_lock_wait_id, code_ra);	
				}
			}
		}
#endif //OMPT_SUPPORT
}

void __mpcomp_atomic_end(void) {
  sctk_spinlock_unlock(&(global_atomic_lock));
#if 1 //OMPT_SUPPORT
		if( mpcomp_ompt_is_enabled() )
		{
			if( OMPT_Callbacks )
			{
				ompt_callback_mutex_t callback_released;
				callback_released = (ompt_callback_mutex_t) OMPT_Callbacks[ompt_callback_mutex_released];
				if( callback_released )
				{
					const void* code_ra = __builtin_return_address(0);
					callback_released( ompt_mutex_atomic, __mpcomp_ompt_atomic_lock_wait_id, code_ra);	
				}
			}
		}
#endif //OMPT_SUPPORT
}


INFO("Wrong atomic/critical behavior in case of OpenMP oversubscribing")

TODO("BUG w/ nested anonymous critical (and maybe named critical) -> need nested locks")

void __mpcomp_anonymous_critical_begin(void) 
{
	sctk_error("val : %d", sctk_atomics_load_int(&__mpcomp_critical_lock_init_once) );
	if( sctk_atomics_load_int(&__mpcomp_critical_lock_init_once)  != 2)
	{
		sctk_error("start init ...");
		//prevent multi call to init 
		if( !sctk_atomics_cas_int( &__mpcomp_critical_lock_init_once, 0, 1))
		{
			sctk_thread_mutex_init(&global_critical_lock, 0);
#if 1 //OMPT_SUPPORT
			if( mpcomp_ompt_is_enabled() )
   		{
      		if( OMPT_Callbacks )
      		{
        	 		ompt_callback_lock_init_t callback_init;
					/* Prevent non thread safe wait_id init */
            	__mpcomp_ompt_critical_lock_wait_id = mpcomp_OMPT_gen_wait_id();
         		callback_init = (ompt_callback_lock_init_t) OMPT_Callbacks[ompt_callback_lock_init];
         		if( callback_init )
         		{
            		const void* code_ra = __builtin_return_address(0);
            		callback_init( ompt_mutex_critical, omp_lock_hint_none, mpcomp_spinlock, __mpcomp_ompt_critical_lock_wait_id, code_ra);
					}
				}
			}
#endif //OMPT_SUPPORT
			sctk_atomics_store_int( &__mpcomp_critical_lock_init_once, 2 );
		}
		else
		{
			sctk_error( "START WAIT ...");
			/* Wait lock init */
			while( sctk_atomics_load_int( &__mpcomp_critical_lock_init_once ) != 2 )
			{
				sctk_cpu_relax();
			}
			sctk_error( "END WAIT ...");
		}	
	}	
#if 1 //OMPT_SUPPORT
		if( mpcomp_ompt_is_enabled() )
		{
			if( OMPT_Callbacks )
			{
				ompt_callback_mutex_acquire_t callback_acquire;
				callback_acquire = (ompt_callback_mutex_acquire_t) OMPT_Callbacks[ompt_callback_mutex_acquire];
				if( callback_acquire )
				{
					const void* code_ra = __builtin_return_address(0);
					callback_acquire( ompt_mutex_critical, omp_lock_hint_none, mpcomp_spinlock, __mpcomp_ompt_critical_lock_wait_id, code_ra);	
				}
			}
		}
#endif //OMPT_SUPPORT
		
		sctk_thread_mutex_lock(&global_critical_lock);

#if 1 //OMPT_SUPPORT
		if( mpcomp_ompt_is_enabled() )
		{
			if( OMPT_Callbacks )
			{
				ompt_callback_mutex_t callback_acquired;
				callback_acquired = (ompt_callback_mutex_t) OMPT_Callbacks[ompt_callback_mutex_acquired];
				if( callback_acquired )
				{
					const void* code_ra = __builtin_return_address(0);
					callback_acquired( ompt_mutex_critical, __mpcomp_ompt_critical_lock_wait_id, code_ra);	
				}
			}
		}
#endif //OMPT_SUPPORT
}

void __mpcomp_anonymous_critical_end(void) 
{
 	 sctk_thread_mutex_unlock(&global_critical_lock);
#if 1 //OMPT_SUPPORT
      if( mpcomp_ompt_is_enabled() )
      {
         if( OMPT_Callbacks )
         {
            ompt_callback_mutex_t callback_released;
            callback_released = (ompt_callback_mutex_t) OMPT_Callbacks[ompt_callback_mutex_released];
            if( callback_released )
            {
               const void* code_ra = __builtin_return_address(0);
               callback_released( ompt_mutex_critical, __mpcomp_ompt_critical_lock_wait_id, code_ra);
            }
         }
      }
#endif //OMPT_SUPPORT
}

void __mpcomp_named_critical_begin(void **l) {
  mpcomp_thread_t *t = (mpcomp_thread_t *)sctk_openmp_thread_tls;
  sctk_assert(t);
  sctk_assert(l);

  if (*l == NULL) {
    sctk_spinlock_lock(&(global_allocate_named_lock));
    if (*l == NULL) {
      sctk_thread_mutex_t *temp_l;
      temp_l = malloc(sizeof(sctk_thread_mutex_t));
      sctk_assert(temp_l);
      sctk_thread_mutex_init(temp_l, NULL);
		
      *l = temp_l;
      sctk_assert(*l);
    }
    sctk_spinlock_unlock(&(global_allocate_named_lock));
  }

  sctk_nodebug("[%d] %s: Before lock", __func__, t->rank);
  sctk_thread_mutex_lock((sctk_thread_mutex_t *)*l);
  sctk_nodebug("[%d] %s: After lock", __func__, t->rank);
}

void __mpcomp_named_critical_end(void **l) {
  mpcomp_thread_t *t = (mpcomp_thread_t *)sctk_openmp_thread_tls;
  sctk_assert(t);
  sctk_assert(l);
  sctk_assert(*l);

  sctk_nodebug("[%d] %s: Before unlock", __func__, t->rank);
  sctk_thread_mutex_unlock((sctk_thread_mutex_t *)*l);
  sctk_nodebug("[%d] %s: After unlock", __func__, t->rank);
}

/* GOMP OPTIMIZED_1_0_WRAPPING */
#ifndef NO_OPTIMIZED_GOMP_4_0_API_SUPPORT
__asm__(".symver __mpcomp_atomic_begin, GOMP_atomic_start@@GOMP_1.0");
__asm__(".symver __mpcomp_atomic_end, GOMP_atomic_end@@GOMP_1.0");
__asm__(
    ".symver __mpcomp_anonymous_critical_begin, GOMP_critical_start@@GOMP_1.0");
__asm__(".symver __mpcomp_anonymous_critical_end, GOMP_critical_end@@GOMP_1.0");
__asm__(".symver __mpcomp_named_critical_begin, "
        "GOMP_critical_name_start@@GOMP_1.0");
__asm__(
    ".symver __mpcomp_named_critical_end, GOMP_critical_name_end@@GOMP_1.0");
#endif /* OPTIMIZED_GOMP_API_SUPPORT */
