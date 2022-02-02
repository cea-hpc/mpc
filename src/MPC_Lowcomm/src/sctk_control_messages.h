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
/* #   - BESNARD Jean-Baptiste jbbesnard@paratools.fr                     # */
/* #                                                                      # */
/* ######################################################################## */

#ifndef SCTK_CONTROL_MESSAGE_H
#define SCTK_CONTROL_MESSAGE_H

#include <comm.h>

/************************************************************************/
/* Process Level Control messages                                       */
/************************************************************************/

typedef enum {
  SCTK_PROCESS_FENCE,
  SCTK_PROCESS_RDMA_WIN_MAPTO,
  SCTK_PROCESS_RDMA_WIN_RELAX,
  SCTK_PROCESS_RDMA_EMULATED_WRITE,
  SCTK_PROCESS_RDMA_EMULATED_READ,
  SCTK_PROCESS_RDMA_EMULATED_FETCH_AND_OP,
  SCTK_PROCESS_RDMA_EMULATED_CAS,
  SCTK_PROCESS_RDMA_CONTROL_MESSAGE
} sctk_control_msg_process_t;

/************************************************************************/
/* Control Messages Interface                                           */
/************************************************************************/

void sctk_control_messages_send_process(int dest_process, int subtype,
                                        char param, void *buffer, size_t size);

void sctk_control_messages_send_rail( int dest, int subtype, char param, void *buffer, size_t size, int  rail_id );
void control_message_submit(mpc_lowcomm_ptp_message_class_t class, int rail_id,
                            mpc_lowcomm_peer_uid_t source_process, int source_rank, int subtype,
                            int param, void *data, size_t msg_size);
void sctk_control_messages_incoming( mpc_lowcomm_ptp_message_t * msg );
void sctk_control_messages_perform(mpc_lowcomm_ptp_message_t *msg, int force);

struct sctk_control_message_fence_ctx
{
	int source;
	int remote;
  mpc_lowcomm_communicator_id_t comm;
};

void sctk_control_message_fence(int target_task, mpc_lowcomm_communicator_t comm);
void sctk_control_message_fence_req(int target_task, mpc_lowcomm_communicator_t comm,
                                    mpc_lowcomm_request_t *req);
void sctk_control_message_fence_handler( struct sctk_control_message_fence_ctx *ctx );

/************************************************************************/
/* Control Messages List                                                */
/************************************************************************/

void sctk_control_message_init();
void sctk_control_message_push( mpc_lowcomm_ptp_message_t * msg );
void sctk_control_message_process();
void sctk_control_message_process_all();
int sctk_control_message_process_local(int rank);

#endif /* SCTK_CONTROL_MESSAGE_H */
