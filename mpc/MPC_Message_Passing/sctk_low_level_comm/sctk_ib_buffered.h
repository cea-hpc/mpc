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
/* #   - DIDELOT Sylvain didelot.sylvain@gmail.com                        # */
/* #                                                                      # */
/* ######################################################################## */

#ifdef MPC_USE_INFINIBAND
#ifndef __SCTK__IB_BUFFERED_H_
#define __SCTK__IB_BUFFERED_H_

#include <sctk_spinlock.h>
#include <uthash.h>
#include <sctk_inter_thread_comm.h>

/*-----------------------------------------------------------
 *  Structures
 *----------------------------------------------------------*/

typedef struct sctk_ib_buffered_s {
  struct sctk_thread_ptp_message_s msg;
  int index;
  int nb;
  size_t payload_size;
  size_t copied;
} sctk_ib_buffered_t;

typedef struct sctk_ib_buffered_entry_s {
  struct sctk_thread_ptp_message_s msg;
  int key;
  UT_hash_handle hh;
  OPA_int_t current;
  int total;
  void* payload;
  sctk_ib_rdma_status_t status;
  sctk_spinlock_t lock;
  struct sctk_message_to_copy_s *copy_ptr;
} sctk_ib_buffered_entry_t;

struct sctk_ibuf_s;

/*-----------------------------------------------------------
 *  FUNCTIONS
 *----------------------------------------------------------*/

int sctk_ib_buffered_prepare_msg(struct sctk_rail_info_s *rail,
    struct sctk_route_table_s* route_table, struct sctk_thread_ptp_message_s * msg, size_t size);

#endif
#endif
