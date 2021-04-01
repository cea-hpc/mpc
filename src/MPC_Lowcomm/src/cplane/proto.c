#include "proto.h"

#include <sctk_alloc.h>
#include <mpc_common_debug.h>
#include <mpc_common_helper.h>
#include <mpc_common_spinlock.h>
#include <string.h>

_mpc_lowcomm_monitor_wrap_t *_mpc_lowcomm_monitor_wrap_new(mpc_lowcomm_monitor_command_t cmd,
                                                           int is_response,
                                                           mpc_lowcomm_peer_uid_t dest,
														   mpc_lowcomm_peer_uid_t from,
                                                           uint64_t match_key,
                                                           size_t size)
{
	_mpc_lowcomm_monitor_wrap_t *ret = sctk_malloc(sizeof(_mpc_lowcomm_monitor_wrap_t) + size);

	assume(ret != NULL);

	ret->command     = cmd;
	ret->is_response = is_response;
	ret->dest        = dest;
	ret->from        = from;
	ret->match_key   = match_key;
	ret->size        = size;
	ret->magick      = 1337;
	ret->ttl         = 1024;

	return ret;
}

int _mpc_lowcomm_monitor_wrap_free(_mpc_lowcomm_monitor_wrap_t *wr)
{
	sctk_free(wr);
	return 0;
}

char * _mpc_lowcomm_monitor_wrap_debug(_mpc_lowcomm_monitor_wrap_t * cmd, char *state, char * buffer, int len)
{
	mpc_common_debug_error("CMD == %p", cmd);
	char meb[32], fromb[32], tob[32];
	snprintf(buffer, len,"%s [%s] COMMAND %s from %s to %s",mpc_lowcomm_peer_format_r(mpc_lowcomm_monitor_get_uid(), meb, 32),
																				state,
																				mpc_lowcomm_monitor_command_tostring(cmd->command),
																				mpc_lowcomm_peer_format_r(cmd->from, fromb, 32),
																				mpc_lowcomm_peer_format_r(cmd->dest, tob, 32) );

	return buffer;
}

void _mpc_lowcomm_monitor_wrap_print(_mpc_lowcomm_monitor_wrap_t * cmd, char *state)
{
	char buff[128];
	mpc_common_debug_warning("%s", _mpc_lowcomm_monitor_wrap_debug(cmd, state, buff, 128));
}

int _mpc_lowcomm_monitor_wrap_send(int socket, _mpc_lowcomm_monitor_wrap_t *wr)
{
	ssize_t ret = mpc_common_io_safe_write(socket, wr, _mpc_lowcomm_monitor_wrap_total_size(wr) );

	if(ret < 0)
	{
		return -1;
	}
	else
	{
		return 0;
	}
}

_mpc_lowcomm_monitor_wrap_t *_mpc_lowcomm_monitor_recv(int socket)
{
	_mpc_lowcomm_monitor_wrap_t incoming_wr;

	if(mpc_common_io_safe_read(socket, &incoming_wr, sizeof(_mpc_lowcomm_monitor_wrap_t) ) <= 0)
	{
		return NULL;
	}

	assume(incoming_wr.magick == 1337);

	/* Reallocate piggyback */

	size_t full_size = _mpc_lowcomm_monitor_wrap_total_size(&incoming_wr);
	_mpc_lowcomm_monitor_wrap_t *ret = sctk_malloc(full_size);
	assume(ret != NULL);
	memcpy(ret, &incoming_wr, sizeof(_mpc_lowcomm_monitor_wrap_t) );

	/* Receive payload */
	if(mpc_common_io_safe_read(socket, ret->content, incoming_wr.size) < 0)
	{
		sctk_free(ret);
		return NULL;
	}

	return ret;
}

