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
#ifndef __SCTK_REORDER_H_
#define __SCTK_REORDER_H_

#include <uthash.h>
#include <mpc_common_spinlock.h>
#include <mpc_lowcomm_monitor.h>


struct mpc_lowcomm_ptp_message_s;
struct sctk_ib_buffered_table_s;

/*NOT THREAD SAFE use to add a route at initialisation time*/
void sctk_add_static_reorder_buffer ( int dest );

/*THREAD SAFE use to add a route at compute time*/
void sctk_add_dynamic_reorder_buffer ( int dest );

/*
 * Return values for sctk_send_message_from_network_reorder
 */
/* Undefined message */
#define REORDER_UNDEFINED -1
/* Message with the correct sequence number found */
#define REORDER_FOUND_EXPECTED 0
/* Message without numbering */
#define REORDER_NO_NUMBERING 1
/* Message with an incorrect sequence number found */
#define REORDER_FOUND_NOT_EXPECTED 2
/* Message not found */
#define REORDER_NOT_FOUND 3

int sctk_send_message_from_network_reorder ( struct mpc_lowcomm_ptp_message_s *msg );
int sctk_prepare_send_message_to_network_reorder ( struct mpc_lowcomm_ptp_message_s *msg );

typedef struct
{
	int key;
	struct mpc_lowcomm_ptp_message_s *msg;
	UT_hash_handle hh;
} mpc_lowcomm_reorder_buffer_t;

typedef struct
{
	mpc_lowcomm_peer_uid_t uid;
	int destination;
} sctk_reorder_key_t;

typedef struct sctk_reorder_table_s
{
	sctk_reorder_key_t key;

	OPA_int_t message_number_src;
	OPA_int_t message_number_dest;

	mpc_common_spinlock_t lock;
	mpc_lowcomm_reorder_buffer_t *buffer;

	UT_hash_handle hh;
} sctk_reorder_table_t;

typedef struct sctk_reorder_task_s
{
	sctk_reorder_table_t *table;
	mpc_common_spinlock_t lock;
} sctk_reorder_list_t;

sctk_reorder_table_t *sctk_get_reorder ( int dest );

sctk_reorder_table_t *sctk_get_reorder_to_process ( int dest );

void sctk_reorder_list_init ( sctk_reorder_list_t *reorder );

#endif
