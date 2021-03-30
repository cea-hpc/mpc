#ifndef PEER_H_
#define PEER_H_

#include <mpc_common_types.h>
#include <mpc_lowcomm_monitor.h>


typedef struct _mpc_lowcomm_peer_s
{
	mpc_lowcomm_monitor_peer_info_t infos;
	int is_local;
}_mpc_lowcomm_peer_t;

/********************
* INIT AND RELEASE *
********************/

int _mpc_lowcomm_peer_setup();
int _mpc_lowcomm_peer_teardown();

/****************
* REGISTRATION *
****************/

static inline mpc_lowcomm_peer_uid_t _mpc_lowcomm_set_root(uint32_t set_uid)
{
	return mpc_lowcomm_monitor_uid_of(set_uid, 0);
}

_mpc_lowcomm_peer_t *_mpc_lowcomm_peer_register(mpc_lowcomm_peer_uid_t uid,
												uint64_t local_task_count,
												char *uri,
												int is_local);
int _mpc_lowcomm_peer_free(_mpc_lowcomm_peer_t *peer);
int _mpc_lowcomm_peer_remove(uint64_t uid);

/*******************
* QUERY INTERFACE *
*******************/

_mpc_lowcomm_peer_t *_mpc_lowcomm_peer_get(mpc_lowcomm_peer_uid_t uid);
int _mpc_lowcomm_peer_alive(mpc_lowcomm_peer_uid_t uid);

#endif /* PEER_H_ */
