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
struct sctk_thread_ptp_message_s;

/*NOT THREAD SAFE use to add a route at initialisation time*/
void sctk_add_static_reorder_buffer(int dest);

/*THREAD SAFE use to add a route at compute time*/
void sctk_add_dynamic_reorder_buffer(int dest);

int sctk_send_message_from_network_reorder (struct sctk_thread_ptp_message_s * msg);
int sctk_prepare_send_message_to_network_reorder (struct sctk_thread_ptp_message_s * msg);

void sctk_set_dynamic_reordering_buffer_creation();

typedef struct {
  int key;
  struct sctk_thread_ptp_message_s* msg;
  UT_hash_handle hh;
}sctk_reorder_buffer_t;

#endif
