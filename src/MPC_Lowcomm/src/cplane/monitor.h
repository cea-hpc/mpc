#ifndef _MPC_LOWCOMM_MONITOR_H_
#define _MPC_LOWCOMM_MONITOR_H_

#include <mpc_lowcomm_monitor.h>

#include <mpc_common_datastructure.h>

#include <time.h>
#include <pthread.h>
#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>


#include "set.h"
#include "proto.h"

/**********************
* SETUP AND TEARDOWN *
**********************/

int _mpc_lowcomm_monitor_setup();
int _mpc_lowcomm_monitor_teardown();

int _mpc_lowcomm_monitor_setup_per_task();
int _mpc_lowcomm_monitor_teardown_per_task();

/**************************
* MONITOR HUB DEFINITION *
**************************/

typedef struct _mpc_lowcomm_client_ctx_s
{
	uint64_t uid;
	int      client_fd;
	time_t   last_used;
	pthread_mutex_t write_lock;
}_mpc_lowcomm_client_ctx_t;

_mpc_lowcomm_client_ctx_t *_mpc_lowcomm_client_ctx_new(uint64_t uid, int fd);
int _mpc_lowcomm_client_ctx_release(_mpc_lowcomm_client_ctx_t **client);


#define _MPC_LOWCOMM_MONITOR_MAX_CLIENTS_    16

struct _mpc_lowcomm_monitor_s
{
	/* Client context list to handle per UID monitor sockets */
	pthread_mutex_t             client_lock;
	struct mpc_common_hashtable client_contexts;
	uint32_t                    client_count;


	/* This is the server information */
	int                         running;
	pthread_t                   server_thread;
	int                         server_socket;
	fd_set                      read_fds;
	int                         notiffd[2];
	char                        monitor_uri[MPC_LOWCOMM_PEER_URI_SIZE];

	/* This is the process set lead group info (entry point to routing) */
	mpc_lowcomm_set_uid_t       monitor_gid;
	_mpc_lowcomm_set_t *        process_set;
	mpc_lowcomm_peer_uid_t      process_uid;
	
};

mpc_lowcomm_monitor_retcode_t _mpc_lowcomm_monitor_init(struct _mpc_lowcomm_monitor_s *monitor);

mpc_lowcomm_monitor_retcode_t _mpc_lowcomm_monitor_release(struct _mpc_lowcomm_monitor_s *monitor);

_mpc_lowcomm_client_ctx_t *_mpc_lowcomm_monitor_client_known(struct _mpc_lowcomm_monitor_s *monitor,
                                                             mpc_lowcomm_peer_uid_t uid);

int _mpc_lowcomm_monitor_client_add(struct _mpc_lowcomm_monitor_s *monitor,
                                    _mpc_lowcomm_client_ctx_t *client);

int _mpc_lowcomm_monitor_client_remove(struct _mpc_lowcomm_monitor_s *monitor,
                                       mpc_lowcomm_peer_uid_t uid);

typedef enum
{
	_MPC_LOWCOMM_MONITOR_GET_CLIENT_DIRECT,
	_MPC_LOWCOMM_MONITOR_GET_CLIENT_CAN_ROUTE
}_mpc_lowcomm_monitor_get_client_policy_t;


_mpc_lowcomm_client_ctx_t *_mpc_lowcomm_monitor_get_client(struct _mpc_lowcomm_monitor_s *monitor,
                                                           mpc_lowcomm_peer_uid_t uid,
                                                           _mpc_lowcomm_monitor_get_client_policy_t policy,
                                                           mpc_lowcomm_monitor_retcode_t *retcode);

mpc_lowcomm_monitor_retcode_t _mpc_lowcomm_monitor_disconnect(struct _mpc_lowcomm_monitor_s *monitor);


int _mpc_lowcomm_peer_is_local(mpc_lowcomm_peer_uid_t uid);


/*********************
* COMMAND GENERATOR *
*********************/

int _mpc_lowcomm_monitor_command_engine_init(void);
int _mpc_lowcomm_monitor_command_engine_teardown(void);


int _mpc_lowcomm_monitor_command_engine_push_response(_mpc_lowcomm_monitor_wrap_t *response);
int _mpc_lowcomm_monitor_command_send(_mpc_lowcomm_monitor_wrap_t *cmd, mpc_lowcomm_monitor_retcode_t *ret);


/****************
* SET EXCHANGE *
****************/

int _mpc_lowcomm_monitor_command_register_set_info(mpc_lowcomm_monitor_args_t *cmd);

_mpc_lowcomm_monitor_wrap_t *_mpc_lowcomm_monitor_command_request_set_info(mpc_lowcomm_peer_uid_t dest);

_mpc_lowcomm_monitor_wrap_t *_mpc_lowcomm_monitor_command_return_set_info(mpc_lowcomm_peer_uid_t dest,
                                                                          uint64_t response_index,
                                                                          mpc_lowcomm_set_uid_t requested_set);

/*****************
* PEER EXCHANGE *
*****************/

int _mpc_lowcomm_monitor_command_register_peer_info(mpc_lowcomm_monitor_args_t *cmd);

_mpc_lowcomm_monitor_wrap_t *_mpc_lowcomm_monitor_command_request_peer_info(mpc_lowcomm_peer_uid_t dest);

_mpc_lowcomm_monitor_wrap_t *_mpc_lowcomm_monitor_command_return_peer_info(mpc_lowcomm_peer_uid_t dest,
                                                                           uint64_t response_index,
                                                                           mpc_lowcomm_peer_uid_t requested_peer);

/********
 * PING *
 ********/

_mpc_lowcomm_monitor_wrap_t *_mpc_lowcomm_monitor_command_return_ping_info(mpc_lowcomm_peer_uid_t dest,
                                                                           uint64_t response_index);

/***********************
 * ON DEMAND CALLBACKS *
 ***********************/

int _mpc_lowcomm_monitor_on_demand_callbacks_init(void);
int _mpc_lowcomm_monitor_on_demand_callbacks_teardown(void);

_mpc_lowcomm_monitor_wrap_t *_mpc_lowcomm_monitor_command_process_ondemand(mpc_lowcomm_peer_uid_t dest,
																		   uint64_t response_index,
                                                                           mpc_lowcomm_monitor_args_t *ondemand);

#endif /* _MPC_LOWCOMM_MONITOR_H_ */
