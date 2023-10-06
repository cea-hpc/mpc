#ifndef _MPC_LOWCOMM_UID_H_
#define _MPC_LOWCOMM_UID_H_

#include <mpc_common_types.h>
#include <mpc_lowcomm_monitor.h>

struct _mpc_lowcomm_uid_descriptor_s
{
	uint64_t               cookie;
	mpc_lowcomm_set_uid_t set_uid;
	char                   leader_uri[MPC_LOWCOMM_PEER_URI_SIZE];
};

int _mpc_lowcomm_uid_descriptor_load(struct _mpc_lowcomm_uid_descriptor_s *sd, char *path);

int _mpc_lowcomm_uid_scan(int (*uid_cb)(uint32_t uid, char * path));

mpc_lowcomm_set_uid_t _mpc_lowcomm_uid_new(char * rank_zero_uri);
int _mpc_lowcomm_uid_clear(uint64_t uid);

#endif /* _MPC_LOWCOMM_UID_H_ */
