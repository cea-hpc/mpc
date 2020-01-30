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

#include "mpc_config.h"
#include "opa_primitives.h"
#include "mpc_common_asm.h"
#include "sctk_alloc.h"

#ifndef MPC_Threads
/* For sched_yield */
#include <sched.h>
#endif

/*******************************
 * MPC SPINLOCK IMPLEMENTATION *
 *******************************/

#if defined( MPC_Threads )

extern int sctk_thread_yield( void );

#else

#include <pthread.h>
#define sctk_thread_yield pthread_yield

#endif

/**
 * @brief This defines the mpc spinlock object
 *
 */
typedef OPA_int_t mpc_common_spinlock_t;

/**
 * @brief Used to initialize a spinlock statically
 *
 */
#define SCTK_SPINLOCK_INITIALIZER { 0 }


static inline void mpc_common_spinlock_init( mpc_common_spinlock_t *lock, int value )
{
	OPA_int_t *p = (OPA_int_t *) lock;
	OPA_store_int(p, value);
}


static inline int mpc_common_spinlock_lock_yield( mpc_common_spinlock_t *lock )
{
	OPA_int_t *p = (OPA_int_t *) lock;
	while ( expect_true( sctk_test_and_set( p ) ) )
	{
		while ( OPA_load_int(p) )
		{
			int i;
#ifdef MPC_Threads
			sctk_thread_yield();
#else
			sched_yield();
#endif
			for ( i = 0; ( OPA_load_int(p) ) && ( i < 100 ); i++ )
			{
				sctk_cpu_relax();
			}
		}
	}

	return 0;
}

static inline int mpc_common_spinlock_lock( mpc_common_spinlock_t *lock )
{
	OPA_int_t *p =  (OPA_int_t *) lock;
	while ( expect_true( sctk_test_and_set( p ) ) )
	{
		do
		{
			sctk_cpu_relax();
		} while ( OPA_load_int(p) );
	}
	return 0;
}

static inline int mpc_common_spinlock_trylock( mpc_common_spinlock_t *lock )
{
	OPA_int_t *p =  (OPA_int_t *) lock;
	return sctk_test_and_set( p );
}

static inline int mpc_common_spinlock_unlock( mpc_common_spinlock_t *lock )
{
	mpc_common_spinlock_init(lock, 0);
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
                mpc_common_spinlock_init(&( a )->writer_lock , 0); \
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

/*******************************
 * MPC MCS LOCK IMPLEMENTATION *
 *******************************/

typedef enum { SCTK_MCSLOCK_WAIT,
			   SCTK_MCSLOCK_DELAY } sctk_mcslock_flags_t;
typedef OPA_ptr_t sctk_mcslock_t;

typedef struct
{
	OPA_int_t lock;
	OPA_ptr_t next;
} sctk_mcslock_ticket_t;

static inline void sctk_mcslock_init_ticket( sctk_mcslock_ticket_t *ticket )
{
	OPA_store_ptr( &( ticket->next ), NULL );
	OPA_store_int( &( ticket->lock ), 0 );
}

static inline sctk_mcslock_ticket_t *sctk_mcslock_alloc_ticket( void )
{
	sctk_mcslock_ticket_t *new_ticket;
	new_ticket = (sctk_mcslock_ticket_t *) sctk_malloc( sizeof( sctk_mcslock_ticket_t ) );
	OPA_store_ptr( &( new_ticket->next ), NULL );
	OPA_store_int( &( new_ticket->lock ), 0 );
	return new_ticket;
}

static inline int sctk_mcslock_push_ticket( sctk_mcslock_t *lock,
					    sctk_mcslock_ticket_t *ticket,
					    sctk_mcslock_flags_t flags )
{
	sctk_mcslock_ticket_t *prev;
	OPA_store_ptr( &( ticket->next ), NULL );
	prev = (sctk_mcslock_ticket_t *) OPA_swap_ptr( lock, ticket );

	/* Acquire lock */
	if ( !prev )
		return 1;

	/* Push ticket in queue */
	OPA_store_int( &( ticket->lock ), 0 );
	OPA_store_ptr( &( prev->next ), ticket );

	if ( flags != SCTK_MCSLOCK_WAIT )
		return 0;

	while ( !OPA_load_int( &( ticket->lock ) ) )
		sctk_cpu_relax();

	return 1;
}

static inline int sctk_mcslock_test_ticket( sctk_mcslock_ticket_t *ticket )
{
	return OPA_load_int( &( ticket->lock ) );
}

static inline int sctk_mcslock_wait_ticket( sctk_mcslock_ticket_t *ticket )
{
	while ( !OPA_load_int( &( ticket->lock ) ) )
		sctk_cpu_relax();
	return 1;
}

static inline int sctk_mcslock_cancel_ticket( sctk_mcslock_ticket_t *ticket )
{
	if ( OPA_cas_int( &( ticket->lock ), 0, 2 ) == 0 )
		return 1;

	/* Lock was already acquire */
	return 0;
}

static inline void sctk_mcslock_trash_ticket( sctk_mcslock_t *lock,
					      sctk_mcslock_ticket_t *ticket )
{
	sctk_mcslock_ticket_t *succ;
	succ = (sctk_mcslock_ticket_t *) OPA_load_ptr( &( ticket->next ) );

	if ( !succ )
	{
		if ( OPA_cas_ptr( lock, ticket, NULL ) == ticket )
			return;

		while ( !( succ = (sctk_mcslock_ticket_t *) OPA_load_ptr( &( ticket->next ) ) ) )
			sctk_cpu_relax();
	}

	if ( OPA_cas_int( &( succ->lock ), 0, 1 ) == 0 )
		return;

	/* cur thread become next ticket */
	sctk_mcslock_trash_ticket( lock, succ );
}

static inline void sctk_mcslock_init( sctk_mcslock_t *lock )
{
	OPA_store_ptr( lock, NULL );
}

#ifdef __cplusplus
} /* end of extern "C" */
#endif

#endif /* MPC_COMMON_INCLUDE_SPINLOCK_H_ */
