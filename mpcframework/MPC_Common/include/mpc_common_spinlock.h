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
#ifndef MPC_COMMON_INCLUDE_SPINLOCK_H_
#define MPC_COMMON_INCLUDE_SPINLOCK_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "sctk_config.h"
#include "opa_primitives.h"
#include "sctk_asm.h"

#ifndef MPC_Threads
/* For sched_yield */
#include <sched.h>
#endif

/*******************************
 * MPC SPINLOCK IMPLEMENTATION *
 *******************************/

#if defined( MPC_Threads )

extern int sctk_thread_yield( void );
#include "sctk_pthread_compatible_structures.h"

#else

/**
 * @brief This defines the mpc spinlock object
 *
 */
typedef volatile int mpc_common_spinlock_t;

/**
 * @brief Used to initialize a spinlock statically
 *
 */
#define SCTK_SPINLOCK_INITIALIZER 0

#endif

#define mpc_common_spinlock_init( a, b )          \
	do                                            \
	{                                             \
		*( (mpc_common_spinlock_t *) ( a ) ) = b; \
	} while ( 0 )

static inline int mpc_common_spinlock_lock_yield( mpc_common_spinlock_t *lock )
{
	volatile int *p = (volatile int *) lock;
	while ( expect_true( sctk_test_and_set( p ) ) )
	{
		while ( *p )
		{
			int i;
#ifdef MPC_Threads
			sctk_thread_yield();
#else
			sched_yield();
#endif
			for ( i = 0; ( *p ) && ( i < 100 ); i++ )
			{
				sctk_cpu_relax();
			}
		}
	}

	return 0;
}

static inline int mpc_common_spinlock_lock( mpc_common_spinlock_t *lock )
{
	volatile int *p = lock;
	while ( expect_true( sctk_test_and_set( p ) ) )
	{
		do
		{
			sctk_cpu_relax();
		} while ( *p );
	}
	return 0;
}

static inline int mpc_common_spinlock_trylock( mpc_common_spinlock_t *lock )
{
	return sctk_test_and_set( lock );
}

static inline int mpc_common_spinlock_unlock( mpc_common_spinlock_t *lock )
{
	*lock = 0;

	return 0;
}

/**********************************
 * MPC RW SPINLOCK IMPLEMENTATION *
 **********************************/

typedef struct
{
	mpc_common_spinlock_t writer_lock;
	OPA_int_t reader_number;
} sctk_spin_rwlock_t;

#define SCTK_SPIN_RWLOCK_INITIALIZER                          \
	{                                                         \
		SCTK_SPINLOCK_INITIALIZER, OPA_INT_T_INITIALIZER( 0 ) \
	}

#define sctk_spin_rwlock_init( a )                      \
	do                                                  \
	{                                                   \
		( a )->writer_lock = SCTK_SPINLOCK_INITIALIZER, \
			OPA_store_int( &( a )->reader_number, 0 );  \
	} while ( 0 )

static inline int mpc_common_spinlock_read_lock( sctk_spin_rwlock_t *lock )
{
	mpc_common_spinlock_lock( &( lock->writer_lock ) );
	OPA_incr_int( &( lock->reader_number ) );
	mpc_common_spinlock_unlock( &( lock->writer_lock ) );
	return 0;
}

static inline int mpc_common_spinlock_read_lock_yield( sctk_spin_rwlock_t *lock )
{
	mpc_common_spinlock_lock_yield( &( lock->writer_lock ) );
	OPA_incr_int( &( lock->reader_number ) );
	mpc_common_spinlock_unlock( &( lock->writer_lock ) );
	return 0;
}

static inline int mpc_common_spinlock_read_trylock( sctk_spin_rwlock_t *lock )
{
	if ( mpc_common_spinlock_trylock( &( lock->writer_lock ) ) == 0 )
	{
		OPA_incr_int( &( lock->reader_number ) );
		mpc_common_spinlock_unlock( &( lock->writer_lock ) );
		return 0;
	}

	return 1;
}

static inline int mpc_common_spinlock_write_lock( sctk_spin_rwlock_t *lock )
{
	mpc_common_spinlock_lock( &( lock->writer_lock ) );
	while ( expect_true( OPA_load_int( &( lock->reader_number ) ) != 0 ) )
	{
		sctk_cpu_relax();
	}
	return 0;
}

static inline int mpc_common_spinlock_write_trylock( sctk_spin_rwlock_t *lock )
{
	if ( mpc_common_spinlock_trylock( &( lock->writer_lock ) ) == 0 )
	{

		while ( expect_true( OPA_load_int( &( lock->reader_number ) ) != 0 ) )
		{
			sctk_cpu_relax();
		}

		return 0;
	}

	return 1;
}

static inline int mpc_common_spinlock_write_lock_yield( sctk_spin_rwlock_t *lock )
{
	int cnt = 0;
	mpc_common_spinlock_lock_yield( &( lock->writer_lock ) );
	while ( expect_true( OPA_load_int( &( lock->reader_number ) ) != 0 ) )
	{
		sctk_cpu_relax();
		if ( !( ++cnt % 1000 ) )
		{
#ifdef MPC_Threads
			sctk_thread_yield();
#else
			sched_yield();
#endif
		}
	}
	return 0;
}

static inline int mpc_common_spinlock_read_unlock( sctk_spin_rwlock_t *lock )
{
	return OPA_fetch_and_add_int( &( lock->reader_number ), -1 ) - 1;
}

static inline int mpc_common_spinlock_read_unlock_state( sctk_spin_rwlock_t *lock )
{
	return OPA_load_int( &( lock->reader_number ) );
}

static inline int mpc_common_spinlock_write_unlock( sctk_spin_rwlock_t *lock )
{
	mpc_common_spinlock_unlock( &( lock->writer_lock ) );
	return 0;
}

#ifdef __cplusplus
} /* end of extern "C" */
#endif

#endif /* MPC_COMMON_INCLUDE_SPINLOCK_H_ */
