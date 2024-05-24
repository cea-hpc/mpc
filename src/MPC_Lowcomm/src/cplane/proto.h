/* ############################# MPC License ############################## */
/* # Thu May  6 10:26:16 CEST 2021                                        # */
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
/* # Maintainers:                                                         # */
/* # - CARRIBAULT Patrick patrick.carribault@cea.fr                       # */
/* # - JAEGER Julien julien.jaeger@cea.fr                                 # */
/* # - PERACHE Marc marc.perache@cea.fr                                   # */
/* # - ROUSSEL Adrien adrien.roussel@cea.fr                               # */
/* # - TABOADA Hugo hugo.taboada@cea.fr                                   # */
/* #                                                                      # */
/* # Authors:                                                             # */
/* # - BESNARD Jean-Baptiste jbbesnard@paratools.fr                       # */
/* #                                                                      # */
/* ######################################################################## */
#ifndef _MPC_LOWCOMM_MONITOR_PROTO_H_
#define _MPC_LOWCOMM_MONITOR_PROTO_H_

#include <mpc_lowcomm_monitor.h>
#include <mpc_common_types.h>
#include <stdlib.h>

#include "set.h"

/*****************************************************************
*    THIS DEFINES THE PROTOCOL INSIDE THE MONITOR INTERFACE.    *
* MESSAGES ARE WRAPPED INSIDE THE WRAP STRUCTURE AND PEELED OUT *
*               TO RECOVER THE INTERNAL COMMAND.                *
*****************************************************************/

/***********
* WRAPPER *
***********/

typedef struct _mpc_lowcomm_monitor_wrap_s
{
	mpc_lowcomm_monitor_command_t command;
	int                           is_response;
	mpc_lowcomm_peer_uid_t        from;
	mpc_lowcomm_peer_uid_t        dest;
	uint64_t                      size;
	uint64_t                      match_key;
	uint64_t                      magick;
	int                           ttl;
	mpc_lowcomm_monitor_args_t    content[];
}_mpc_lowcomm_monitor_wrap_t;


_mpc_lowcomm_monitor_wrap_t *_mpc_lowcomm_monitor_wrap_new(mpc_lowcomm_monitor_command_t cmd,
                                                           int is_response,
														   mpc_lowcomm_peer_uid_t dest,
                                                           mpc_lowcomm_peer_uid_t from,
                                                           uint64_t match_key,
                                                           size_t size);

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-function"

static inline size_t _mpc_lowcomm_monitor_wrap_total_size(_mpc_lowcomm_monitor_wrap_t *wr)
{
	return sizeof(_mpc_lowcomm_monitor_wrap_t) + wr->size;
}

#pragma clang diagnostic pop

int _mpc_lowcomm_monitor_wrap_free(_mpc_lowcomm_monitor_wrap_t *wr);
char * _mpc_lowcomm_monitor_wrap_debug(_mpc_lowcomm_monitor_wrap_t * wrap, char *state, char * buffer, int len);
void _mpc_lowcomm_monitor_wrap_print(_mpc_lowcomm_monitor_wrap_t * cmd, char *state);
int _mpc_lowcomm_monitor_wrap_send(int socket, _mpc_lowcomm_monitor_wrap_t *wr);
_mpc_lowcomm_monitor_wrap_t *_mpc_lowcomm_monitor_recv(int socket);

#endif /* _MPC_LOWCOMM_MONITOR_PROTO_H_ */
