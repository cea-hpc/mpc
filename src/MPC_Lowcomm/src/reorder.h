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
#ifndef __SCTK__MPC_LOWCOMM_REORDER_H_
#define __SCTK__MPC_LOWCOMM_REORDER_H_

#include <uthash.h>
#include <mpc_common_spinlock.h>
#include <mpc_lowcomm_monitor.h>

struct mpc_lowcomm_ptp_message_s;

/*
 * Return values for _mpc_lowcomm_reorder_msg_check
 */
/* Undefined message */
#define _MPC_LOWCOMM_REORDER_UNDEFINED -1
/* Message with the correct sequence number found */
#define _MPC_LOWCOMM_REORDER_FOUND_EXPECTED 0
/* Message without numbering */
#define _MPC_LOWCOMM_REORDER_NO_NUMBERING 1
/* Message with an incorrect sequence number found */
#define _MPC_LOWCOMM_REORDER_FOUND_NOT_EXPECTED 2
/* Message not found */
#define _MPC_LOWCOMM_REORDER_NOT_FOUND 3

int _mpc_lowcomm_reorder_msg_check( struct mpc_lowcomm_ptp_message_s *msg );
int _mpc_lowcomm_reorder_msg_register( struct mpc_lowcomm_ptp_message_s *msg );

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
} _mpc_lowcomm_reorder_key_t;

typedef struct _mpc_lowcomm_reorder_table_s
{
	_mpc_lowcomm_reorder_key_t key;

	OPA_int_t message_number_src;
	OPA_int_t message_number_dest;

	mpc_common_spinlock_t lock;
	mpc_lowcomm_reorder_buffer_t *buffer;

	UT_hash_handle hh;
} _mpc_lowcomm_reorder_table_t;

typedef struct _mpc_lowcomm_reorder_list_s
{
	_mpc_lowcomm_reorder_table_t *table;
	mpc_common_spinlock_t lock;
} _mpc_lowcomm_reorder_list_t;

void _mpc_lowcomm_reorder_list_init( _mpc_lowcomm_reorder_list_t *reorder );

#endif
