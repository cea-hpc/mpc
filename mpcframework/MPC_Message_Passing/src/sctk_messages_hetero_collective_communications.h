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
#ifndef __SCTK_MESSAGES_HETERO_COLLECTIVE_COMMUNICATIONS_H_
#define __SCTK_MESSAGES_HETERO_COLLECTIVE_COMMUNICATIONS_H_


/************************************************************************/
/*Barrier                                                               */
/************************************************************************/

typedef struct
{
	OPA_int_t tasks_entered_in_node;
	volatile unsigned int generation;
} sctk_barrier_hetero_messages_t;

void sctk_barrier_hetero_messages_init ( struct sctk_internal_collectives_struct_s *tmp, sctk_communicator_t id );

/************************************************************************/
/*Broadcast                                                             */
/************************************************************************/

typedef struct
{
	OPA_int_t tasks_entered_in_node;
	OPA_int_t tasks_exited_in_node;
	volatile unsigned int generation;
	OPA_ptr_t buff_root;
} sctk_broadcast_hetero_messages_t;

void sctk_broadcast_hetero_messages_init ( struct sctk_internal_collectives_struct_s *tmp, sctk_communicator_t id );

/************************************************************************/
/*Allreduce                                                             */
/************************************************************************/

typedef struct
{
	/* TODO: initialization */
	OPA_int_t tasks_entered_in_node;
	volatile unsigned int generation;
	volatile void *volatile *buff_in;
	volatile void *volatile *buff_out;
} sctk_allreduce_hetero_messages_t;

void sctk_allreduce_hetero_messages_init ( struct sctk_internal_collectives_struct_s *tmp, sctk_communicator_t id );

/************************************************************************/
/*Init                                                                  */
/************************************************************************/
void sctk_collectives_init_hetero_messages ( sctk_communicator_t id );

#endif
