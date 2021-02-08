#ifndef SET_H_
#define SET_H_

#include <mpc_common_types.h>

#include "peer.h"



typedef struct _mpc_lowcomm_set_s
{
	int                    is_lead;
	mpc_lowcomm_set_uid_t  uid;
	mpc_lowcomm_peer_uid_t local_peer;
	char                   name[MPC_LOWCOMM_SET_NAME_LEN];
	uint64_t               total_task_count;

	_mpc_lowcomm_peer_t ** peers;
	uint64_t               peer_count;
}_mpc_lowcomm_set_t;

/*****************
* SETUP TEADOWN *
*****************/

int _mpc_lowcomm_set_setup();
int _mpc_lowcomm_set_teardown();

/**********************
* REGISTER INTERFACE *
**********************/

_mpc_lowcomm_set_t *_mpc_lowcomm_set_init(mpc_lowcomm_set_uid_t uid,
                                          char *name,
                                          uint64_t total_task_count,
                                          uint64_t *peers_uids,
                                          uint64_t peer_count,
                                          int is_lead,
										  mpc_lowcomm_peer_uid_t local_peer);

int _mpc_lowcomm_set_free(_mpc_lowcomm_set_t *set);

int _mpc_lowcomm_set_load_from_system(void);

/*******************
* QUERY INTERFACE *
*******************/

_mpc_lowcomm_set_t *_mpc_lowcomm_set_get(mpc_lowcomm_set_uid_t uid);

int _mpc_lowcomm_set_contains(_mpc_lowcomm_set_t * set, mpc_lowcomm_peer_uid_t peer_uid);
int _mpc_lowcomm_set_iterate(int (*set_cb)(_mpc_lowcomm_set_t *set, void *arg), void *arg);


#endif /* SET_H_ */
