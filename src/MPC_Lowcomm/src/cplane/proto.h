#ifndef _MPC_LOWCOMM_MONITOR_PROTO_H_
#define _MPC_LOWCOMM_MONITOR_PROTO_H_

#include <mpc_lowcomm_monitor.h>
#include <mpc_common_types.h>
#include <stdlib.h>

#include "set.h"

/*****************************************************************
*    THIS DEFINES THE PROTOCOL INSIDE THE MONITOR INTERFACE.    *
* MESSAGES ARE WRAPPED INSIDE THE WRAP STRCUTURE AND PEELED OUT *
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
	uint64_t                      ttl;
	mpc_lowcomm_monitor_args_t    content[];
}_mpc_lowcomm_monitor_wrap_t;


_mpc_lowcomm_monitor_wrap_t *_mpc_lowcomm_monitor_wrap_new(mpc_lowcomm_monitor_command_t cmd,
                                                           int is_response,
														   mpc_lowcomm_peer_uid_t from,
                                                           mpc_lowcomm_peer_uid_t dest,
                                                           uint64_t match_key,
                                                           size_t size);

static inline size_t _mpc_lowcomm_monitor_wrap_total_size(_mpc_lowcomm_monitor_wrap_t *wr)
{
	return sizeof(_mpc_lowcomm_monitor_wrap_t) + wr->size;
}

int _mpc_lowcomm_monitor_wrap_free(_mpc_lowcomm_monitor_wrap_t *wr);
int _mpc_lowcomm_monitor_wrap_send(int socket, _mpc_lowcomm_monitor_wrap_t *wr);
_mpc_lowcomm_monitor_wrap_t *_mpc_lowcomm_monitor_recv(int socket);

#endif /* _MPC_LOWCOMM_MONITOR_PROTO_H_ */