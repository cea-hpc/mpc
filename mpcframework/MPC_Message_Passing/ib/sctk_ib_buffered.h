/* ############################# MPC License ############################## */
/* # Wed Nov 19 15:19:19 CET 2008                                         # */
/* # Copyright or (C) or Copr. Commissariat a l'Energie Atomique          # */
/* # Copyright or (C) or Copr. 2010-2012 Université de Versailles         # */
/* # St-Quentin-en-Yvelines                                               # */
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
/* #   - DIDELOT Sylvain sylvain.didelot@exascale-computing.eu            # */
/* #                                                                      # */
/* ######################################################################## */

#ifdef MPC_USE_INFINIBAND
#ifndef __SCTK__IB_BUFFERED_H_
#define __SCTK__IB_BUFFERED_H_

#include <mpc_common_spinlock.h>
#include <uthash.h>
#include <comm.h>

/********************************************************************/
/* Structures                                                       */
/********************************************************************/
typedef struct sctk_ib_buffered_table_s
{
	struct sctk_ib_buffered_entry_s *entries;
	mpc_common_spinlock_t lock;

	OPA_int_t number;
} sctk_ib_buffered_table_t;

typedef struct sctk_ib_buffered_s
{
	mpc_mp_ptp_message_body_t msg;
	int number;
	int index;
	int nb;
	size_t payload_size;
	size_t copied;
}
__attribute__ ( ( aligned ( 16 ) ) ) sctk_ib_buffered_t;

typedef struct sctk_ib_buffered_entry_s
{
	struct sctk_thread_ptp_message_s msg;
	int key;
	UT_hash_handle hh;
	int total;
	void *payload;
	sctk_ib_rdma_status_t status;
	mpc_common_spinlock_t lock;
	char dummy[64];
	/* Current copied */
	mpc_common_spinlock_t current_copied_lock;
	size_t current_copied;

	struct mpc_mp_ptp_message_content_to_copy_s *copy_ptr;
} sctk_ib_buffered_entry_t;

struct sctk_ibuf_s;

/********************************************************************/
/* Functions                                                        */
/********************************************************************/

int sctk_ib_buffered_prepare_msg ( struct sctk_rail_info_s *rail,
                                   struct sctk_ib_qp_s *remote,
                                   struct sctk_thread_ptp_message_s *msg,
                                   size_t size );

void sctk_ib_buffered_poll_recv ( struct sctk_rail_info_s *rail, struct sctk_ibuf_s *ibuf );

#endif
#endif
