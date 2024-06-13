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
#ifndef MPC_COMMON_MCS_LOCK_H_
#define MPC_COMMON_MCS_LOCK_H_

#include "sctk_alloc_posix.h"
#ifdef __cplusplus
extern "C" {
#endif

#include "mpc_common_spinlock.h"

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

// NOLINTBEGIN(clang-diagnostic-unused-function)

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

// NOLINTEND(clang-diagnostic-unused-function)


#ifdef __cplusplus
} /* end of extern "C" */
#endif

#endif /* MPC_COMMON_MCS_LOCK_H_ */
