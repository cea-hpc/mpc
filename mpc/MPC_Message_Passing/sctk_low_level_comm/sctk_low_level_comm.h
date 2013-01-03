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
#ifndef __SCTK_LOW_LEVEL_COMM_H_
#define __SCTK_LOW_LEVEL_COMM_H_

#include <sctk_inter_thread_comm.h>
void sctk_net_init_driver (char *name);
void sctk_net_init_pmi();
int sctk_is_net_migration_available();

void sctk_network_send_message (sctk_thread_ptp_message_t * msg);
void sctk_network_send_message_set(void (*sctk_network_send_message_val) (sctk_thread_ptp_message_t *));

void sctk_network_notify_recv_message (sctk_thread_ptp_message_t * msg);
void sctk_network_notify_recv_message_set(void (*sctk_network_notify_recv_message_val) (sctk_thread_ptp_message_t *));

void sctk_network_notify_matching_message (sctk_thread_ptp_message_t * msg);
void sctk_network_notify_matching_message_set(void (*sctk_network_notify_matching_message_val) (sctk_thread_ptp_message_t *));

void sctk_network_notify_perform_message (int remote_process, int remote_task_id);
void sctk_network_notify_perform_message_set(void (*sctk_network_notify_perform_message_val) (int,int));

void sctk_network_notify_idle_message ();
void sctk_network_notify_idle_message_set(void (*sctk_network_notify_idle_message_val) ());

void sctk_network_notify_any_source_message ();
void sctk_network_notify_any_source_message_set(void (*sctk_network_notify_perform_message_val) ());
size_t sctk_net_memory_allocation_hook(size_t size_origin);

#endif
