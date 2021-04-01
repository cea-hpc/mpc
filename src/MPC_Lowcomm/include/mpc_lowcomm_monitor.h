#ifndef MPC_LAUNCH_MONITOR_H_
#define MPC_LAUNCH_MONITOR_H_

#include <stdlib.h>
#include <mpc_common_types.h>

/***************
* ERROR CODES *
***************/

typedef enum
{
	MPC_LAUNCH_MONITOR_RET_SUCCESS,
	MPC_LAUNCH_MONITOR_RET_ERROR,
	MPC_LAUNCH_MONITOR_RET_DUPLICATE_KEY,
	MPC_LAUNCH_MONITOR_RET_NOT_REACHABLE,
	MPC_LAUNCH_MONITOR_RET_INVALID_UID,
	MPC_LAUNCH_MONITOR_RET_REFUSED
}mpc_lowcomm_monitor_retcode_t;

void mpc_lowcomm_monitor_retcode_print(mpc_lowcomm_monitor_retcode_t code, const char *ctx);

/*******************
* ID MANIPULATION *
*******************/

typedef uint32_t   mpc_lowcomm_set_uid_t;
typedef uint64_t   mpc_lowcomm_peer_uid_t;

mpc_lowcomm_peer_uid_t mpc_lowcomm_monitor_get_uid();
mpc_lowcomm_set_uid_t mpc_lowcomm_monitor_get_gid();

static inline mpc_lowcomm_set_uid_t mpc_lowcomm_peer_get_set(mpc_lowcomm_peer_uid_t uid)
{
	return uid >> 32;
}

static inline int mpc_lowcomm_peer_get_rank(mpc_lowcomm_peer_uid_t uid)
{
	return (int)uid;
}

static inline mpc_lowcomm_peer_uid_t mpc_lowcomm_monitor_uid_of(uint32_t set_uid, int peer_rank)
{
	uint64_t ret = 0;
	uint64_t lset_uid = set_uid;

	ret |= lset_uid << 32;
	ret |= peer_rank;

	return ret;
}

static inline mpc_lowcomm_peer_uid_t mpc_lowcomm_monitor_local_uid_of(int peer_rank)
{
	return mpc_lowcomm_monitor_uid_of(mpc_lowcomm_monitor_get_gid(), peer_rank);
}

/******************
 * PEER FUNCTIONS *
 ******************/

char * mpc_lowcomm_peer_format_r(mpc_lowcomm_peer_uid_t uid, char * buff, int len);
char * mpc_lowcomm_peer_format(mpc_lowcomm_peer_uid_t uid);
int mpc_lowcomm_peer_closer(mpc_lowcomm_peer_uid_t dest, mpc_lowcomm_peer_uid_t current, mpc_lowcomm_peer_uid_t candidate);
int mpc_lowcomm_monitor_peer_reachable_directly(mpc_lowcomm_peer_uid_t target_peer);

/*****************
 * SET INTERFACE *
 *****************/

typedef void * mpc_lowcomm_monitor_set_t;

int mpc_lowcomm_monitor_set_iterate(int (*callback)(mpc_lowcomm_monitor_set_t set, void * arg), void * arg);

char * mpc_lowcomm_monitor_set_name(mpc_lowcomm_monitor_set_t set);
mpc_lowcomm_set_uid_t mpc_lowcomm_monitor_set_gid(mpc_lowcomm_monitor_set_t set);

#define MPC_LOWCOMM_PEER_URI_SIZE     256

typedef struct mpc_lowcomm_monitor_peer_info_s
{
	mpc_lowcomm_peer_uid_t uid;
	uint64_t               local_task_count;
	char                   uri[MPC_LOWCOMM_PEER_URI_SIZE];
}mpc_lowcomm_monitor_peer_info_t;

uint64_t mpc_lowcomm_monitor_set_peer_count(mpc_lowcomm_monitor_set_t set);
int mpc_lowcomm_monitor_set_peers(mpc_lowcomm_monitor_set_t set, mpc_lowcomm_monitor_peer_info_t * peers, uint64_t peers_len);

/************
* COMMANDS *
************/

typedef enum
{
	MPC_LAUNCH_MONITOR_COMMAND_NONE,
	MPC_LAUNCH_MONITOR_COMMAND_REQUEST_PEER_INFO,
	MPC_LAUNCH_MONITOR_COMMAND_REQUEST_SET_INFO,
	MPC_LAUNCH_MONITOR_PING,
	MPC_LAUNCH_MONITOR_ON_DEMAND,
	MPC_LAUNCH_MONITOR_CONNECTIVITY
}mpc_lowcomm_monitor_command_t;

const char * mpc_lowcomm_monitor_command_tostring(mpc_lowcomm_monitor_command_t cmd);

#define MPC_LAUNCH_MONITOR_KEY_LEN    128
#define MPC_LOWCOMM_SET_NAME_LEN      256
#define MPC_LOWCOMM_ONDEMAND_TARGET_LEN 32
#define MPC_LOWCOMM_ONDEMAND_DATA_LEN MPC_LOWCOMM_PEER_URI_SIZE
#define MPC_LOWCOMM_MONITOR_MAX_CLIENTS    16

typedef union
{
	struct
	{
		mpc_lowcomm_monitor_peer_info_t info;
		mpc_lowcomm_peer_uid_t          requested_peer;
		mpc_lowcomm_monitor_retcode_t   retcode;
	}peer;

	struct
	{
		mpc_lowcomm_set_uid_t           uid;
		mpc_lowcomm_monitor_retcode_t   retcode;
		char                            name[MPC_LOWCOMM_SET_NAME_LEN];
		uint64_t                        total_task_count;
		uint64_t                        peer_count;
		mpc_lowcomm_monitor_peer_info_t peers[];
	}set_info;

	struct
	{
		uint64_t                      ping;
		mpc_lowcomm_monitor_retcode_t retcode;
	}ping;

	struct
	{
		char target[MPC_LOWCOMM_ONDEMAND_TARGET_LEN];
		char data[MPC_LOWCOMM_ONDEMAND_DATA_LEN];
		mpc_lowcomm_monitor_retcode_t retcode;
	}on_demand;

	struct
	{
		mpc_lowcomm_peer_uid_t peers[MPC_LOWCOMM_MONITOR_MAX_CLIENTS];
		uint64_t peers_count;
	}connectivity;

}mpc_lowcomm_monitor_args_t;

typedef void *mpc_lowcomm_monitor_response_t;

mpc_lowcomm_monitor_args_t *mpc_lowcomm_monitor_response_get_content(mpc_lowcomm_monitor_response_t response);

int mpc_lowcomm_monitor_response_free(mpc_lowcomm_monitor_response_t response);

/****************
 * GET SET INFO *
 ****************/

mpc_lowcomm_monitor_response_t mpc_lowcomm_monitor_get_set_info(mpc_lowcomm_peer_uid_t target_peer,
                                                                mpc_lowcomm_monitor_retcode_t *ret);

/*****************
 * GET PEER INFO *
 *****************/

mpc_lowcomm_monitor_response_t mpc_lowcomm_monitor_command_get_peer_info(mpc_lowcomm_peer_uid_t dest,
                                                                         mpc_lowcomm_peer_uid_t requested_peer,
                                                                         mpc_lowcomm_monitor_retcode_t *ret);

/***************
 * PING REMOTE *
 ***************/

mpc_lowcomm_monitor_response_t mpc_lowcomm_monitor_ping(mpc_lowcomm_peer_uid_t dest, mpc_lowcomm_monitor_retcode_t *ret);

/*********************
 * ON DEMAND SUPPORT *
 *********************/

typedef int (*mpc_lowcomm_on_demand_callback_t)(mpc_lowcomm_peer_uid_t from,
												char *data,
												char * return_data,
												int return_data_len,
												void *ctx);

int mpc_lowcomm_monitor_register_on_demand_callback(char *target,
                                                    mpc_lowcomm_on_demand_callback_t callback,
													void * ctx);

mpc_lowcomm_on_demand_callback_t mpc_lowcomm_monitor_get_on_demand_callback(char *target, void **ctx);

int mpc_lowcomm_monitor_unregister_on_demand_callback(char * target);

mpc_lowcomm_monitor_response_t mpc_lowcomm_monitor_ondemand(mpc_lowcomm_peer_uid_t dest,
															char *target,
															char *data,
															mpc_lowcomm_monitor_retcode_t *ret);

/*************************
 * GET CONNECTIVITY INFO *
 *************************/

mpc_lowcomm_monitor_response_t mpc_lowcomm_monitor_connectivity(mpc_lowcomm_peer_uid_t dest, mpc_lowcomm_monitor_retcode_t *ret);

void mpc_lowcomm_monitor_synchronous_connectivity_dump(void);

#endif /* MPC_LAUNCH_MONITOR_H_ */
