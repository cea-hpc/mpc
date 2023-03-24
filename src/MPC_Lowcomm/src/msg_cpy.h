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

#include "lowcomm.h"
#include "mpc_common_helper.h"
#include <sys/uio.h>

/* IOVEC NET FUNCS */
/* Copy function for net driver copy */
typedef void (*_mpc_lowcomm_msg_cpy_iovec_t)(struct iovec *, int, mpc_lowcomm_ptp_message_t *);
/* Convert sctk_thread_ptp_msg_t to struct iovec */
struct iovec *_mpc_lowcomm_msg_cpy_to_iovec(mpc_lowcomm_ptp_message_t *, int *, size_t);

/* sctk_net_copy for iovec */
void _mpc_lowcomm_msg_cpy_from_iovec(mpc_lowcomm_ptp_message_content_to_copy_t *, _mpc_lowcomm_msg_cpy_iovec_t);

void _mpc_lowcomm_msg_cpy_in_buffer(mpc_lowcomm_ptp_message_t *msg,
                                    char *buffer);
void _mpc_lowcomm_msg_cpy_from_buffer(char *body,
                                      mpc_lowcomm_ptp_message_content_to_copy_t *tmp, char free_headers);

void mpc_lowcomm_request_pack(void *request, void *buffer);
void mpc_lowcomm_request_unpack(void *request, void *buffer);

size_t _mpc_lowcomm_msg_cpy_size(mpc_lowcomm_ptp_message_t *msg);

int _mpc_lowcomm_msg_cpy_frag(
	const mpc_lowcomm_ptp_message_t *msg,
	char *buffer,
	const size_t curr_copy,
	const size_t max_copy);


void _mpc_lowcomm_msg_cpy(mpc_lowcomm_ptp_message_content_to_copy_t *tmp);

void _mpc_lowcomm_msg_cpy_in_fd(mpc_lowcomm_ptp_message_t *msg,
                                int fd);
void _mpc_lowcomm_msg_cpy_from_fd(mpc_lowcomm_ptp_message_t *msg,
                                  int fd);


#ifdef __cplusplus
}
#endif
#endif
