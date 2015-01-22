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
#ifndef __SCTK__NET_TOOLS_H_
#define __SCTK__NET_TOOLS_H_
#ifdef __cplusplus
extern "C"
{
#endif

#include "sctk_low_level_comm.h"
#include "sctk_io_helper.h"
void sctk_net_copy_in_buffer ( sctk_thread_ptp_message_t *msg,
                               char *buffer );

void *sctk_net_if_one_msg_in_buffer ( sctk_thread_ptp_message_t *msg );

size_t sctk_net_determine_message_size ( sctk_thread_ptp_message_t *msg );

int sctk_net_copy_frag_msg (
    const sctk_thread_ptp_message_t *msg,
    char *buffer,
    const size_t curr_copy,
    const size_t max_copy );
void sctk_net_write_in_fd ( sctk_thread_ptp_message_t *msg,
                            int fd );
void sctk_net_read_in_fd ( sctk_thread_ptp_message_t *msg,
                           int fd );
void sctk_net_message_copy ( sctk_message_to_copy_t *tmp );

void sctk_net_message_copy_from_buffer ( char *body,
                                         sctk_message_to_copy_t *tmp, char free_headers );
#ifdef __cplusplus
}
#endif
#endif
