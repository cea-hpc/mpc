#ifndef MPC_LAUNCH_MONITOR_H_
#define MPC_LAUNCH_MONITOR_H_

#include <stdlib.h>
#include <mpc_common_types.h>
#include <mpc_lowcomm_monitor.h>

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


static inline mpc_lowcomm_set_uid_t mpc_lowcomm_peer_get_set(mpc_lowcomm_peer_uid_t uid)
{
	return uid >> 32;
}

static inline mpc_lowcomm_peer_uid_t mpc_lowcomm_peer_get_rank(mpc_lowcomm_peer_uid_t uid)
{
	return (uint32_t)uid;
}

mpc_lowcomm_peer_uid_t mpc_lowcomm_monitor_get_uid();
mpc_lowcomm_set_uid_t mpc_lowcomm_monitor_get_gid();
mpc_lowcomm_peer_uid_t mpc_lowcomm_monitor_get_uid_of(mpc_lowcomm_set_uid_t set, int rank);


int mpc_lowcomm_peer_closer(mpc_lowcomm_peer_uid_t dest, mpc_lowcomm_peer_uid_t current, mpc_lowcomm_peer_uid_t candidate);


int mpc_lowcomm_monitor_peer_reachable_directly(mpc_lowcomm_peer_uid_t target_peer);

/************
* COMMANDS *
************/

typedef enum
{
	MPC_LAUNCH_MONITOR_COMMAND_NONE,
	MPC_LAUNCH_MONITOR_COMMAND_REQUEST_PEER_INFO,
	MPC_LAUNCH_MONITOR_COMMAND_REQUEST_SET_INFO,
	MPC_LAUNCH_MONITOR_PING
}mpc_lowcomm_monitor_command_t;

#define MPC_LAUNCH_MONITOR_KEY_LEN    128
#define MPC_LOWCOMM_SET_NAME_LEN      256
#define MPC_LOWCOMM_PEER_URI_SIZE     256
#define MPC_LOWCOMM_ONDEMAND_TARGET_LEN 32
#define MPC_LOWCOMM_ONDEMAND_DATA_LEN MPC_LOWCOMM_PEER_URI_SIZE



typedef struct mpc_lowcomm_monitor_peer_info_s
{
	mpc_lowcomm_peer_uid_t uid;
	uint64_t               local_task_count;
	char                   uri[MPC_LOWCOMM_PEER_URI_SIZE];
}mpc_lowcomm_monitor_peer_info_t;


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

}mpc_lowcomm_monitor_args_t;

typedef void *mpc_lowcomm_monitor_response_t;


mpc_lowcomm_monitor_response_t mpc_lowcomm_monitor_get_set_info(mpc_lowcomm_peer_uid_t target_peer,
                                                                mpc_lowcomm_monitor_retcode_t *ret);

mpc_lowcomm_monitor_response_t mpc_lowcomm_monitor_command_get_peer_info(mpc_lowcomm_peer_uid_t dest,
                                                                         mpc_lowcomm_peer_uid_t requested_peer,
                                                                         mpc_lowcomm_monitor_retcode_t *ret);


mpc_lowcomm_monitor_response_t mpc_lowcomm_monitor_ping(mpc_lowcomm_peer_uid_t dest, mpc_lowcomm_monitor_retcode_t *ret);

mpc_lowcomm_monitor_args_t *mpc_lowcomm_monitor_response_get_content(mpc_lowcomm_monitor_response_t response);

int mpc_lowcomm_monitor_response_free(mpc_lowcomm_monitor_response_t response);

/*********************
 * ON DEMAND SUPPORT *
 *********************/

typedef int (*mpc_lowcomm_on_demand_callback_t)(char *out_data,
												char * return_data,
												int return_data_len);

int mpc_lowcomm_monitor_register_on_demand_callback(char *target,
                                                    mpc_lowcomm_on_demand_callback_t callback);

mpc_lowcomm_on_demand_callback_t mpc_lowcomm_monitor_get_on_demand_callback(char *target);

int mpc_lowcomm_monitor_unregister_on_demand_callback(char * target);

int mpc_lowcomm_monitor_call_on_demand(mpc_lowcomm_peer_uid_t dest,
									   char target[8],
									   char *sent_data,
									   char * received_data,
									   int received_data_len);


#endif /* MPC_LAUNCH_MONITOR_H_ */
