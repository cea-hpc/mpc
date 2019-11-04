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
#include "mpcomp.h"
#include "mpc_common_spinlock.h"
#include "omp_ompt.h"


#include "sched.h"
#include "mpc_common_asm.h"
#include "sctk_thread.h"

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
 
/* Atomic emulated */
__UNUSED__ static ompt_wait_id_t __mpcomp_ompt_atomic_lock_wait_id = 0;
static OPA_int_t __mpcomp_atomic_lock_init_once 		= OPA_INT_T_INITIALIZER(0); 
/* Critical anonymous */
__UNUSED__  static ompt_wait_id_t __mpcomp_ompt_critical_lock_wait_id = 0;
static OPA_int_t __mpcomp_critical_lock_init_once 	= OPA_INT_T_INITIALIZER(0); 

static mpc_common_spinlock_t* __mpcomp_omp_global_atomic_lock = NULL;
static mpcomp_lock_t* 	__mpcomp_omp_global_critical_lock = NULL;
static mpc_common_spinlock_t 	__mpcomp_global_init_critical_named_lock = SCTK_SPINLOCK_INITIALIZER;

void __mpcomp_atomic_begin(void) 
{
	if( OPA_load_int(&__mpcomp_atomic_lock_init_once) < 2)
	{
		//prevent multi call to init 
		if( !OPA_cas_int( &__mpcomp_atomic_lock_init_once, 0, 1))
		{
			__mpcomp_omp_global_atomic_lock = (mpc_common_spinlock_t*) sctk_malloc(sizeof(mpc_common_spinlock_t));
			sctk_assert( __mpcomp_omp_global_atomic_lock );
			mpc_common_spinlock_init(__mpcomp_omp_global_atomic_lock, SCTK_SPINLOCK_INITIALIZER );

#if OMPT_SUPPORT
			if( _mpc_omp_ompt_is_enabled() )
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
			OPA_store_int( &__mpcomp_atomic_lock_init_once, 2 );
		}
		else
		{
			/* Wait lock init */
			while( OPA_load_int( &__mpcomp_atomic_lock_init_once ) != 2 )
			{
				sctk_cpu_relax();
			}
		}	
	}	
#if OMPT_SUPPORT
		if( _mpc_omp_ompt_is_enabled() )
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
			
		mpc_common_spinlock_lock(__mpcomp_omp_global_atomic_lock);

#if OMPT_SUPPORT
		if( _mpc_omp_ompt_is_enabled() )
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
  mpc_common_spinlock_unlock(__mpcomp_omp_global_atomic_lock);
#if OMPT_SUPPORT
		if( _mpc_omp_ompt_is_enabled() )
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
	if( OPA_load_int(&__mpcomp_critical_lock_init_once)  != 2)
	{
		//prevent multi call to init 
		if( !OPA_cas_int( &__mpcomp_critical_lock_init_once, 0, 1))
		{
			__mpcomp_omp_global_critical_lock = (mpcomp_lock_t *) sctk_malloc( sizeof( mpcomp_lock_t ));
			sctk_assert(__mpcomp_omp_global_critical_lock);
   		memset(__mpcomp_omp_global_critical_lock, 0, sizeof(mpcomp_lock_t));
   		sctk_thread_mutex_init(&(__mpcomp_omp_global_critical_lock->lock), 0);

#if OMPT_SUPPORT
			if( _mpc_omp_ompt_is_enabled() )
   		{
				/* Prevent non thread safe wait_id init */
				__mpcomp_omp_global_critical_lock->wait_id = mpcomp_OMPT_gen_wait_id();
				__mpcomp_omp_global_critical_lock->hint = omp_lock_hint_none;
						
      		if( OMPT_Callbacks )
      		{
        	 		ompt_callback_lock_init_t callback_init;
         		callback_init = (ompt_callback_lock_init_t) OMPT_Callbacks[ompt_callback_lock_init];
         		if( callback_init )
         		{
            		const void* code_ra = __builtin_return_address(0);
						const omp_lock_hint_t hint = __mpcomp_omp_global_critical_lock->hint;
						const ompt_wait_id_t wait_id  = __mpcomp_omp_global_critical_lock->wait_id;
            		callback_init( ompt_mutex_critical, hint, mpcomp_mutex, wait_id, code_ra);
					}
				}
			}
#endif //OMPT_SUPPORT
			OPA_store_int( &__mpcomp_critical_lock_init_once, 2 );
		}
		else
		{
			/* Wait lock init */
			while( OPA_load_int( &__mpcomp_critical_lock_init_once ) != 2 )
			{
				sctk_cpu_relax();
			}
		}	
	}	
#if OMPT_SUPPORT
		if( _mpc_omp_ompt_is_enabled() )
		{
			if( OMPT_Callbacks )
			{
				ompt_callback_mutex_acquire_t callback_acquire;
				callback_acquire = (ompt_callback_mutex_acquire_t) OMPT_Callbacks[ompt_callback_mutex_acquire];
				if( callback_acquire )
				{
					const void* code_ra = __builtin_return_address(0);
					const omp_lock_hint_t hint = __mpcomp_omp_global_critical_lock->hint;
					const ompt_wait_id_t wait_id  = __mpcomp_omp_global_critical_lock->wait_id;
					callback_acquire( ompt_mutex_critical, hint, mpcomp_mutex, wait_id, code_ra);	
				}
			}
		}
#endif //OMPT_SUPPORT
		
		sctk_thread_mutex_lock( &( __mpcomp_omp_global_critical_lock->lock));

#if  OMPT_SUPPORT
		if( _mpc_omp_ompt_is_enabled() )
		{
			if( OMPT_Callbacks )
			{
				ompt_callback_mutex_t callback_acquired;
				callback_acquired = (ompt_callback_mutex_t) OMPT_Callbacks[ompt_callback_mutex_acquired];
				if( callback_acquired )
				{
					const void* code_ra = __builtin_return_address(0);
					const ompt_wait_id_t wait_id  = __mpcomp_omp_global_critical_lock->wait_id;
					callback_acquired( ompt_mutex_critical, wait_id, code_ra);	
				}
			}
		}
#endif //OMPT_SUPPORT
}

void __mpcomp_anonymous_critical_end(void) 
{
 	 sctk_thread_mutex_unlock(&(__mpcomp_omp_global_critical_lock->lock));
#if OMPT_SUPPORT
      if( _mpc_omp_ompt_is_enabled() )
      {
         if( OMPT_Callbacks )
         {
            ompt_callback_mutex_t callback_released;
            callback_released = (ompt_callback_mutex_t) OMPT_Callbacks[ompt_callback_mutex_released];
            if( callback_released )
            {
               const void* code_ra = __builtin_return_address(0);
					const ompt_wait_id_t wait_id  = __mpcomp_omp_global_critical_lock->wait_id;
               callback_released( ompt_mutex_critical, wait_id, code_ra);
            }
         }
      }
#endif //OMPT_SUPPORT
}

void __mpcomp_named_critical_begin(void **l) 
{
  	sctk_assert(l);
  	mpcomp_lock_t* named_critical_lock;
  	if( *l == NULL ) 
	{
    	mpc_common_spinlock_lock(&(__mpcomp_global_init_critical_named_lock));
		
    	if( *l == NULL ) 
		{
			named_critical_lock = (mpcomp_lock_t *) sctk_malloc( sizeof( mpcomp_lock_t ));
			sctk_assert( named_critical_lock );
			memset(named_critical_lock, 0, sizeof(mpcomp_lock_t));
			sctk_thread_mutex_init(&(named_critical_lock->lock), 0);
		
#if OMPT_SUPPORT	
		if( _mpc_omp_ompt_is_enabled() )
		{
			named_critical_lock->wait_id = mpcomp_OMPT_gen_wait_id();
			named_critical_lock->hint = omp_lock_hint_none;
		}
#endif //OMPT_SUPPORT	

      *l = named_critical_lock;
    }
    mpc_common_spinlock_unlock(&(__mpcomp_global_init_critical_named_lock));
  }

 	named_critical_lock = (mpcomp_lock_t*)(*l);

#if OMPT_SUPPORT
	if( _mpc_omp_ompt_is_enabled() )
   {
   	if( OMPT_Callbacks )
      {
      	ompt_callback_mutex_acquire_t callback_acquire;
         callback_acquire = (ompt_callback_mutex_acquire_t) OMPT_Callbacks[ompt_callback_mutex_acquire];
         if( callback_acquire )
         {
         	const void* code_ra = __builtin_return_address(0);
            const omp_lock_hint_t hint = named_critical_lock->hint;
            const ompt_wait_id_t wait_id = named_critical_lock->wait_id;
            callback_acquire( ompt_mutex_critical, hint, mpcomp_mutex, wait_id, code_ra);
         }
     	}
	}
#endif //OMPT_SUPPORT
		
 	sctk_thread_mutex_lock(&(named_critical_lock->lock));

#if OMPT_SUPPORT
	if( _mpc_omp_ompt_is_enabled() )
   {
   	if( OMPT_Callbacks )
      {
      	ompt_callback_mutex_t callback_acquired;
         callback_acquired = (ompt_callback_mutex_t) OMPT_Callbacks[ompt_callback_mutex_acquired];
         if( callback_acquired )
         {
         	const void* code_ra = __builtin_return_address(0);
            const omp_lock_hint_t hint = named_critical_lock->hint;
            const ompt_wait_id_t wait_id = named_critical_lock->wait_id;
            callback_acquired( ompt_mutex_critical, wait_id, code_ra);
         }
     	}
	}
#endif //OMPT_SUPPORT
}

void __mpcomp_named_critical_end(void **l) {
  	sctk_assert(l);
  	sctk_assert(*l);
	
	mpcomp_lock_t* named_critical_lock = (mpcomp_lock_t*)(*l);
  	sctk_thread_mutex_unlock(&(named_critical_lock->lock));
	
#if OMPT_SUPPORT
   if( _mpc_omp_ompt_is_enabled() )
   {
      if( OMPT_Callbacks )
      {
         ompt_callback_mutex_t callback_released;
         callback_released = (ompt_callback_mutex_t) OMPT_Callbacks[ompt_callback_mutex_released];
         if( callback_released )
         {
            const void* code_ra = __builtin_return_address(0);
            const omp_lock_hint_t hint = named_critical_lock->hint;
            const ompt_wait_id_t wait_id = named_critical_lock->wait_id;
            callback_released( ompt_mutex_critical, wait_id, code_ra);
         }
      }
   }
#endif //OMPT_SUPPORT

}

/* GOMP OPTIMIZED_1_0_WRAPPING */
#ifndef NO_OPTIMIZED_GOMP_4_0_API_SUPPORT
__asm__(".symver __mpcomp_atomic_begin, GOMP_atomic_start@@GOMP_1.0");
__asm__(".symver __mpcomp_atomic_end, GOMP_atomic_end@@GOMP_1.0");
__asm__(".symver __mpcomp_anonymous_critical_begin, GOMP_critical_start@@GOMP_1.0");
__asm__(".symver __mpcomp_anonymous_critical_end, GOMP_critical_end@@GOMP_1.0");
__asm__(".symver __mpcomp_named_critical_begin, GOMP_critical_name_start@@GOMP_1.0");
__asm__(".symver __mpcomp_named_critical_end, GOMP_critical_name_end@@GOMP_1.0");
#endif /* OPTIMIZED_GOMP_API_SUPPORT */
