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

#include <sctk_inter_thread_comm.h>

#include <mpc_common_spinlock.h>
#include <sctk_thread.h>
#include <sctk_types.h>

struct sctk_internal_collectives_struct_s;

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

void _mpc_coll_barrier_simple_init ( struct sctk_internal_collectives_struct_s *tmp, sctk_communicator_t id );

/* Broadcast */

typedef struct
{
	volatile int done /* = 0 */;
	sctk_thread_mutex_t lock/*  = SCTK_THREAD_MUTEX_INITIALIZER */;
	sctk_thread_cond_t cond/* = SCTK_THREAD_COND_INITIALIZER */;
	void *buffer;
	size_t size;
} _mpc_coll_bcast_simple_t;

void _mpc_coll_bcast_simple_init ( struct sctk_internal_collectives_struct_s *tmp, sctk_communicator_t id );

/* Allreduce */

typedef struct
{
	volatile int done /* = 0 */;
	sctk_thread_mutex_t lock/*  = SCTK_THREAD_MUTEX_INITIALIZER */;
	sctk_thread_cond_t cond/* = SCTK_THREAD_COND_INITIALIZER */;
	void *buffer;
	size_t size;
} _mpc_coll_allreduce_simple_t;

void _mpc_coll_allreduce_simple_init ( struct sctk_internal_collectives_struct_s *tmp, sctk_communicator_t id );

/* Init */

void _mpc_coll_init_simple ( sctk_communicator_t id );

/*********************************
 * OPT COLLECTIVE IMPLEMENTATION *
 *********************************/

/* Barrier */

typedef struct
{
	int dummy;
} _mpc_coll_opt_barrier_t;

void _mpc_coll_opt_barrier_init ( struct sctk_internal_collectives_struct_s *tmp, sctk_communicator_t id );

/* Broadcast */

typedef struct
{
	int dummy;
} _mpc_coll_opt_bcast_t;

void _mpc_coll_opt_bcast_init ( struct sctk_internal_collectives_struct_s *tmp, sctk_communicator_t id );

/* Allreduce */

typedef struct
{
	int dummy;
} _mpc_coll_opt_allreduce_t;

void _mpc_coll_opt_allreduce_init ( struct sctk_internal_collectives_struct_s *tmp, sctk_communicator_t id );

/* Init */

void _mpc_coll_init_opt ( sctk_communicator_t id );

/************************************
 * HETERO COLLECTIVE IMPLEMENTATION *
 ************************************/

/* Barrier */

typedef struct
{
	OPA_int_t tasks_entered_in_node;
	volatile unsigned int generation;
} _mpc_coll_hetero_barrier_t;

void _mpc_coll_hetero_barrier_init ( struct sctk_internal_collectives_struct_s *tmp, sctk_communicator_t id );

/* Broadcast */

typedef struct
{
	OPA_int_t tasks_entered_in_node;
	OPA_int_t tasks_exited_in_node;
	volatile unsigned int generation;
	OPA_ptr_t buff_root;
} _mpc_coll_hetero_bcast_t;

void _mpc_coll_hetero_bcast_init ( struct sctk_internal_collectives_struct_s *tmp, sctk_communicator_t id );

/* Allreduce */

typedef struct
{
	/* TODO: initialization */
	OPA_int_t tasks_entered_in_node;
	volatile unsigned int generation;
	volatile void *volatile *buff_in;
	volatile void *volatile *buff_out;
} _mpc_coll_hetero_allreduce_t;

void _mpc_coll_hetero_allreduce_init ( struct sctk_internal_collectives_struct_s *tmp, sctk_communicator_t id );

/* Init */

void _mpc_coll_init_hetero ( sctk_communicator_t id );

/******************************************
 * OPT NO ALLOC COLLECTIBE IMPLEMENTATION *
 ******************************************/


/* Barrier */

typedef struct
{
	int dummy;
} _mpc_coll_noalloc_barrier_t;

void _mpc_coll_noalloc_barrier_init ( struct sctk_internal_collectives_struct_s *tmp, sctk_communicator_t id );

/* Broadcast */

typedef struct
{
	int dummy;
} _mpc_coll_noalloc_bcast_t;

void _mpc_coll_noalloc_bcast_init ( struct sctk_internal_collectives_struct_s *tmp, sctk_communicator_t id );

/* Allreduce */

typedef struct
{
	int dummy;
} _mpc_coll_noalloc_allreduce_t;

void _mpc_coll_noalloc_allreduce_init ( struct sctk_internal_collectives_struct_s *tmp, sctk_communicator_t id );

/* Init */

void _mpc_coll_init_noalloc ( sctk_communicator_t id );

/**********************************************
 * GENERIC COLLECTIVE COMMUNICATION INTERFACE *
 **********************************************/

/* Barrier */

typedef union
{
	_mpc_coll_barrier_simple_t barrier_simple;
	_mpc_coll_opt_barrier_t barrier_opt_messages;
	_mpc_coll_noalloc_barrier_t barrier_opt_noalloc_split_messages;
	_mpc_coll_hetero_barrier_t barrier_hetero_messages;
} sctk_barrier_t;

void sctk_barrier ( const sctk_communicator_t communicator );

/* Broadcast */

typedef union
{
	_mpc_coll_bcast_simple_t broadcast_simple;
	_mpc_coll_opt_bcast_t broadcast_opt_messages;
	_mpc_coll_noalloc_bcast_t broadcast_opt_noalloc_split_messages;
	_mpc_coll_hetero_bcast_t broadcast_hetero_messages;
} sctk_broadcast_t;

void sctk_broadcast ( void *buffer, const size_t size,
                      const int root, const sctk_communicator_t com_id );

/* Allreduce */

typedef union
{
	_mpc_coll_allreduce_simple_t allreduce_simple;
	_mpc_coll_opt_allreduce_t allreduce_opt_messages;
	_mpc_coll_noalloc_allreduce_t allreduce_opt_noalloc_split_messages;
	_mpc_coll_hetero_allreduce_t allreduce_hetero_messages;
} sctk_allreduce_t;

void sctk_all_reduce ( const void *buffer_in, void *buffer_out,
                       const size_t elem_size,
                       const size_t elem_number,
                       sctk_Op_f func,
                       const sctk_communicator_t communicator,
                       const sctk_datatype_t data_type );

/* Collective Structure */

typedef struct sctk_internal_collectives_struct_s
{
	sctk_barrier_t barrier;
	void ( *barrier_func ) ( const sctk_communicator_t ,
	                         struct sctk_internal_collectives_struct_s * );

	sctk_broadcast_t broadcast;
	void ( *broadcast_func ) ( void *, const size_t ,
	                           const int , const sctk_communicator_t,
	                           struct sctk_internal_collectives_struct_s * );

	sctk_allreduce_t allreduce;
	void ( *allreduce_func ) ( const void *, void *,
	                           const size_t ,
	                           const size_t ,
	                           sctk_Op_f func,
	                           const sctk_communicator_t ,
	                           const sctk_datatype_t ,
	                           struct sctk_internal_collectives_struct_s * );
} sctk_internal_collectives_struct_t;

/* Init */

void sctk_collectives_init ( sctk_communicator_t id,
                             void ( *barrier ) ( sctk_internal_collectives_struct_t *, sctk_communicator_t id ),
                             void ( *broadcast ) ( sctk_internal_collectives_struct_t *, sctk_communicator_t id ),
                             void ( *allreduce ) ( sctk_internal_collectives_struct_t *, sctk_communicator_t id ) );

extern void ( *sctk_collectives_init_hook ) ( sctk_communicator_t id );

void
sctk_terminaison_barrier (void);

#endif
