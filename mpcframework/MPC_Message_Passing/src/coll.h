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
#ifndef __SCTK_COLLECTIVE_COMMUNICATIONS_H_
#define __SCTK_COLLECTIVE_COMMUNICATIONS_H_

#include <mpc_mp_coll.h>


#include "sctk_inter_thread_comm.h"


struct mpc_mp_coll_s;

/************************************************************************/
/*collective communication implementation                               */
/************************************************************************/

/************************************
 * SIMPLE COLLECTIVE IMPLEMENTATION *
 ************************************/

/* Barrier */

typedef struct
{
	volatile int done /* = 0 */;
	sctk_thread_mutex_t lock/*  = SCTK_THREAD_MUTEX_INITIALIZER */;
	sctk_thread_cond_t cond/* = SCTK_THREAD_COND_INITIALIZER */;
} _mpc_coll_barrier_simple_t;

void _mpc_coll_barrier_simple_init ( struct mpc_mp_coll_s *tmp, mpc_mp_communicator_t id );

/* Broadcast */

typedef struct
{
	volatile int done /* = 0 */;
	sctk_thread_mutex_t lock/*  = SCTK_THREAD_MUTEX_INITIALIZER */;
	sctk_thread_cond_t cond/* = SCTK_THREAD_COND_INITIALIZER */;
	void *buffer;
	size_t size;
} _mpc_coll_bcast_simple_t;

void _mpc_coll_bcast_simple_init ( struct mpc_mp_coll_s *tmp, mpc_mp_communicator_t id );

/* Allreduce */

typedef struct
{
	volatile int done /* = 0 */;
	sctk_thread_mutex_t lock/*  = SCTK_THREAD_MUTEX_INITIALIZER */;
	sctk_thread_cond_t cond/* = SCTK_THREAD_COND_INITIALIZER */;
	void *buffer;
	size_t size;
} _mpc_coll_allreduce_simple_t;

void _mpc_coll_allreduce_simple_init ( struct mpc_mp_coll_s *tmp, mpc_mp_communicator_t id );

/* Init */

void mpc_mp_coll_init_simple ( mpc_mp_communicator_t id );

/*********************************
 * OPT COLLECTIVE IMPLEMENTATION *
 *********************************/

/* Barrier */

typedef struct
{
	int dummy;
} _mpc_coll_opt_barrier_t;

void _mpc_coll_opt_barrier_init ( struct mpc_mp_coll_s *tmp, mpc_mp_communicator_t id );

/* Broadcast */

typedef struct
{
	int dummy;
} _mpc_coll_opt_bcast_t;

void _mpc_coll_opt_bcast_init ( struct mpc_mp_coll_s *tmp, mpc_mp_communicator_t id );

/* Allreduce */

typedef struct
{
	int dummy;
} _mpc_coll_opt_allreduce_t;

void _mpc_coll_opt_allreduce_init ( struct mpc_mp_coll_s *tmp, mpc_mp_communicator_t id );

/* Init */

void mpc_mp_coll_init_opt ( mpc_mp_communicator_t id );

/************************************
 * HETERO COLLECTIVE IMPLEMENTATION *
 ************************************/

/* Barrier */

typedef struct
{
	OPA_int_t tasks_entered_in_node;
	volatile unsigned int generation;
} _mpc_coll_hetero_barrier_t;

void _mpc_coll_hetero_barrier_init ( struct mpc_mp_coll_s *tmp, mpc_mp_communicator_t id );

/* Broadcast */

typedef struct
{
	OPA_int_t tasks_entered_in_node;
	OPA_int_t tasks_exited_in_node;
	volatile unsigned int generation;
	OPA_ptr_t buff_root;
} _mpc_coll_hetero_bcast_t;

void _mpc_coll_hetero_bcast_init ( struct mpc_mp_coll_s *tmp, mpc_mp_communicator_t id );

/* Allreduce */

typedef struct
{
	/* TODO: initialization */
	OPA_int_t tasks_entered_in_node;
	volatile unsigned int generation;
	volatile void *volatile *buff_in;
	volatile void *volatile *buff_out;
} _mpc_coll_hetero_allreduce_t;

void _mpc_coll_hetero_allreduce_init ( struct mpc_mp_coll_s *tmp, mpc_mp_communicator_t id );

/* Init */

void mpc_mp_coll_init_hetero ( mpc_mp_communicator_t id );

/******************************************
 * OPT NO ALLOC COLLECTIBE IMPLEMENTATION *
 ******************************************/


/* Barrier */

typedef struct
{
	int dummy;
} _mpc_coll_noalloc_barrier_t;

void _mpc_coll_noalloc_barrier_init ( struct mpc_mp_coll_s *tmp, mpc_mp_communicator_t id );

/* Broadcast */

typedef struct
{
	int dummy;
} _mpc_coll_noalloc_bcast_t;

void _mpc_coll_noalloc_bcast_init ( struct mpc_mp_coll_s *tmp, mpc_mp_communicator_t id );

/* Allreduce */

typedef struct
{
	int dummy;
} _mpc_coll_noalloc_allreduce_t;

void _mpc_coll_noalloc_allreduce_init ( struct mpc_mp_coll_s *tmp, mpc_mp_communicator_t id );

/* Init */

void mpc_mp_coll_init_noalloc ( mpc_mp_communicator_t id );


/**************************
 * INTERNAL INIT FUNCTION *
 **************************/

struct mpc_mp_coll_s;

void _mpc_coll_init ( mpc_mp_communicator_t id,
			void ( *barrier ) ( struct mpc_mp_coll_s *, mpc_mp_communicator_t id ),
			void ( *broadcast ) ( struct mpc_mp_coll_s *, mpc_mp_communicator_t id ),
			void ( *allreduce ) ( struct mpc_mp_coll_s *, mpc_mp_communicator_t id ) );



/**********************************************
 * GENERIC COLLECTIVE COMMUNICATION INTERFACE *
 **********************************************/

/* Init */

struct mpc_mp_coll_s;

void mpc_mp_coll_init ( mpc_mp_communicator_t id,
			void ( *barrier ) ( struct mpc_mp_coll_s *, mpc_mp_communicator_t id ),
			void ( *broadcast ) ( struct mpc_mp_coll_s *, mpc_mp_communicator_t id ),
			void ( *allreduce ) ( struct mpc_mp_coll_s *, mpc_mp_communicator_t id ) );


/* Barrier */

typedef union
{
	_mpc_coll_barrier_simple_t barrier_simple;
	_mpc_coll_opt_barrier_t barrier_opt_messages;
	_mpc_coll_noalloc_barrier_t barrier_opt_noalloc_split_messages;
	_mpc_coll_hetero_barrier_t barrier_hetero_messages;
} mpc_mp_barrier_t;


/* Broadcast */

typedef union
{
	_mpc_coll_bcast_simple_t broadcast_simple;
	_mpc_coll_opt_bcast_t broadcast_opt_messages;
	_mpc_coll_noalloc_bcast_t broadcast_opt_noalloc_split_messages;
	_mpc_coll_hetero_bcast_t broadcast_hetero_messages;
} mpc_mp_bcast_t;


/* Allreduce */

typedef union
{
	_mpc_coll_allreduce_simple_t allreduce_simple;
	_mpc_coll_opt_allreduce_t allreduce_opt_messages;
	_mpc_coll_noalloc_allreduce_t allreduce_opt_noalloc_split_messages;
	_mpc_coll_hetero_allreduce_t allreduce_hetero_messages;
} sctk_allreduce_t;

/* Collective Structure */

struct mpc_mp_coll_s{
	mpc_mp_barrier_t barrier;
	void ( *barrier_func ) ( const mpc_mp_communicator_t ,
	                         struct mpc_mp_coll_s * );

	mpc_mp_bcast_t broadcast;
	void ( *broadcast_func ) ( void *, const size_t ,
	                           const int , const mpc_mp_communicator_t,
	                           struct mpc_mp_coll_s * );

	sctk_allreduce_t allreduce;
	void ( *allreduce_func ) ( const void *, void *,
	                           const size_t ,
	                           const size_t ,
	                           sctk_Op_f func,
	                           const mpc_mp_communicator_t ,
	                           const mpc_mp_datatype_t ,
	                           struct mpc_mp_coll_s * );
};

#endif
