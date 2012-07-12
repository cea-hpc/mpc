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
//#include <sctk_communicator.h>
#include <sctk.h>
#include <sctk_spinlock.h>
#include <sctk_thread.h>

typedef unsigned int sctk_datatype_t;
#define sctk_null_data_type ((sctk_datatype_t)(-1))
struct sctk_internal_collectives_struct_s;

/************************************************************************/
/*collective communication implementation                               */
/************************************************************************/
#include <sctk_simple_collective_communications.h>
#include <sctk_messages_opt_collective_communications.h>
#include <sctk_messages_hetero_collective_communications.h>

/************************************************************************/
/*Barrier                                                               */
/************************************************************************/

typedef union {
  sctk_barrier_simple_t barrier_simple;
  sctk_barrier_opt_messages_t barrier_opt_messages;
  sctk_barrier_hetero_messages_t barrier_hetero_messages;
} sctk_barrier_t;

void sctk_barrier(const sctk_communicator_t communicator);

/************************************************************************/
/*Broadcast                                                             */
/************************************************************************/

typedef union {
  sctk_broadcast_simple_t broadcast_simple;
  sctk_broadcast_opt_messages_t broadcast_opt_messages;
  sctk_broadcast_hetero_messages_t broadcast_hetero_messages;
} sctk_broadcast_t;

void sctk_broadcast (void *buffer, const size_t size,
		     const int root, const sctk_communicator_t com_id);

/************************************************************************/
/*Allreduce                                                             */
/************************************************************************/

typedef union {
  sctk_allreduce_simple_t allreduce_simple;
  sctk_allreduce_opt_messages_t allreduce_opt_messages;
  sctk_allreduce_hetero_messages_t allreduce_hetero_messages;
} sctk_allreduce_t;

void sctk_all_reduce (const void *buffer_in, void *buffer_out,
		      const size_t elem_size,
		      const size_t elem_number,
		      void (*func) (const sctk_communicator_t *, sctk_communicator_t *, size_t,
				    sctk_datatype_t),
		      const sctk_communicator_t com_id,
		      const sctk_datatype_t data_type);

/************************************************************************/
/*Collectives struct                                                    */
/************************************************************************/

typedef struct sctk_internal_collectives_struct_s{
  sctk_barrier_t barrier;
  void (*barrier_func)(const sctk_communicator_t ,
		       struct sctk_internal_collectives_struct_s *);

  sctk_broadcast_t broadcast;
  void (*broadcast_func) (void *, const size_t ,
			  const int , const sctk_communicator_t,
			  struct sctk_internal_collectives_struct_s *);

  sctk_allreduce_t allreduce;
  void (*allreduce_func) (const void *, void *,
			  const size_t ,
			  const size_t ,
			  void (*) (const void *, 
              void *, size_t,
				    sctk_datatype_t),
			  const sctk_communicator_t ,
			  const sctk_datatype_t ,
			  struct sctk_internal_collectives_struct_s *);
} sctk_internal_collectives_struct_t;

void sctk_collectives_init (sctk_communicator_t id,
			    void (*barrier)(sctk_internal_collectives_struct_t *, sctk_communicator_t id),
			    void (*broadcast)(sctk_internal_collectives_struct_t *, sctk_communicator_t id),
			    void (*allreduce)(sctk_internal_collectives_struct_t *, sctk_communicator_t id));

extern void (*sctk_collectives_init_hook)(sctk_communicator_t id);

void
sctk_terminaison_barrier (const int id);

#endif
