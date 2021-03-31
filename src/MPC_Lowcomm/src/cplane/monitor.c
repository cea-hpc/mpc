#include "monitor.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <ctype.h>
#include <stdio.h>
#include <time.h>
#include <errno.h>
#include <string.h>
#include <mpc_launch_pmi.h>
#include <mpc_common_rank.h>

#include <sctk_alloc.h>

#include <mpc_common_debug.h>
#include <mpc_common_helper.h>
#include <mpc_lowcomm.h>

#include "uid.h"
#include "monitor.h"


/***************
* GLOBAL VARS *
***************/

static struct _mpc_lowcomm_monitor_s __monitor = { 0 };

/***************
* ERROR CODES *
***************/

void mpc_lowcomm_monitor_retcode_print(mpc_lowcomm_monitor_retcode_t code, const char *ctx)
{
	char *desc = NULL;

	switch(code)
	{
		case MPC_LAUNCH_MONITOR_RET_SUCCESS:
			desc = "SUCCESS";
			break;

		case MPC_LAUNCH_MONITOR_RET_ERROR:
			desc = "Generic Error";
			break;

		case MPC_LAUNCH_MONITOR_RET_DUPLICATE_KEY:
			desc = "Key is duplicated";
			break;

		case MPC_LAUNCH_MONITOR_RET_NOT_REACHABLE:
			desc = "Provided UID was not reachable";
			break;

		case MPC_LAUNCH_MONITOR_RET_INVALID_UID:
			desc = "Provided UID was not known";
			break;

		case MPC_LAUNCH_MONITOR_RET_REFUSED:
			desc = "Connection refused";
			break;

		default:
			bad_parameter("No such errcode %d", code);
	}

	mpc_common_debug_error("%s : %s", ctx, desc);
}

/**********************
* SETUP AND TEARDOWN *
**********************/

static char *__server_key_name(char *buf, int size, int process_rank)
{
	snprintf(buf, size, "cplane_monitor_server_%d", process_rank);
	return buf;
}

static char *__server_uid_name(char *buf, int size)
{
	snprintf(buf, size, "cplane_monitor_uid");
	return buf;
}

static inline mpc_lowcomm_monitor_retcode_t __bootstrap_ring(void)
{
	if(mpc_common_get_local_task_rank() != 0)
	{
		/* We only want one bootstrap per process */
		return MPC_LAUNCH_MONITOR_RET_SUCCESS;
	}

	/* Bootstrap Kademlia like connections on WORLD */

	uint32_t target = 0;
	assume(__monitor.process_set != NULL);

	uint32_t i       = 1;
	int      my_rank = mpc_common_get_process_rank();

	int cnt = 0;

	for(i = 1; i < __monitor.process_set->peer_count; i <<= 1)
	{
		target = (my_rank + i) % __monitor.process_set->peer_count;

		/* Getting the client does create it if not present already */
		mpc_lowcomm_monitor_retcode_t retcode;
		_mpc_lowcomm_monitor_get_client(&__monitor,
		                                __monitor.process_set->peers[target]->infos.uid,
		                                _MPC_LOWCOMM_MONITOR_GET_CLIENT_DIRECT,
		                                &retcode);
		if(retcode != MPC_LAUNCH_MONITOR_RET_SUCCESS)
		{
			mpc_lowcomm_monitor_retcode_print(retcode, "Bootstraping ring");
		}

		if(_MPC_LOWCOMM_MONITOR_MAX_CLIENTS_ <= ++cnt)
		{
			break;
		}
	}

	return MPC_LAUNCH_MONITOR_RET_SUCCESS;
}




static inline int __exchange_peer_info(void)
{
	/* Here each rank != 0 in the process set requests infos from 0
	 * this has the effect of registering in 0 and pre retrieving 0's infos */
	mpc_lowcomm_monitor_retcode_t ret;
	mpc_lowcomm_peer_uid_t        root_process_uid = _mpc_lowcomm_set_root(__monitor.process_set->uid);

	if(root_process_uid != __monitor.process_uid)
	{
		mpc_lowcomm_monitor_command_get_peer_info(root_process_uid,
		                                          root_process_uid,
		                                          &ret);
	}

	return 0;
}

static inline int __register_process_set(void)
{
	/* Declare own job */
	uint32_t job_id = __monitor.monitor_gid;

	int task_rank = mpc_common_get_task_rank();

	if(mpc_common_get_local_task_rank() == 0)
	{
		/* No need to register several times world in this proc */

		/* Now create all peers */
		int i;
		int proc_count   = mpc_common_get_process_count();
		int my_proc_rank = mpc_common_get_process_rank();

		uint64_t *peers_ids = sctk_malloc(sizeof(uint64_t) * proc_count);
		assume(peers_ids != NULL);

		for(i = 0; i < proc_count; i++)
		{
			mpc_lowcomm_peer_uid_t pid = mpc_lowcomm_monitor_uid_of(job_id, i);
			char server_key[128];
			/* They all should be PMI reachable */
			__server_key_name(server_key, 128, i);
			char uri[155];
			snprintf(uri, 155, "pmi://%s", server_key);
			_mpc_lowcomm_peer_register(pid,
			                           0,
			                           uri,
			                           (my_proc_rank == i) );
			peers_ids[i] = pid;
		}

		mpc_lowcomm_peer_uid_t local_peer_id = mpc_lowcomm_monitor_uid_of(job_id, mpc_common_get_process_rank() );

		/* Create the set atop the world set */
		__monitor.process_set = _mpc_lowcomm_set_init(job_id,
		                                                   "mpc://process/leaders",
		                                                   mpc_common_get_task_count(),
		                                                   peers_ids,
		                                                   proc_count,
		                                                   (task_rank == 0) /* is_lead */,
		                                                   local_peer_id);

		sctk_free(peers_ids);
	}


	/* Just to make sure world set is known before continuing */
	mpc_lowcomm_barrier(MPC_COMM_WORLD);

	assume(__monitor.process_uid != 0);

	return 0;
}

int _mpc_lowcomm_monitor_setup()
{
	mpc_lowcomm_monitor_retcode_t ret;

	ret = _mpc_lowcomm_monitor_init(&__monitor);

	if(ret != MPC_LAUNCH_MONITOR_RET_SUCCESS)
	{
		mpc_lowcomm_monitor_retcode_print(ret, __func__);
		return 1;
	}

	/* Time to provide generic sets */
	if(_mpc_lowcomm_set_setup() )
	{
		return 1;
	}

	/* Time to EXCHANGE UID infos */

	char buff[64];

	__server_uid_name(buff, 64);
	char data[32];

	if(mpc_common_get_process_rank() == 0)
	{
		/* Now compute set ID for this comm */
		__monitor.monitor_gid = _mpc_lowcomm_uid_new(__monitor.monitor_uri);

		snprintf(data, 32, "%u", __monitor.monitor_gid);

		if(mpc_launch_pmi_is_initialized() )
		{
			mpc_launch_pmi_put(data, buff, 0 /* non local */);
		}
	}

	/* Sync the job to ensure PMI exchange */
	mpc_launch_pmi_barrier();

	if(mpc_common_get_process_rank() != 0)
	{
		mpc_launch_pmi_get(data, 32, buff, 1);
		sscanf(data,"%u", &__monitor.monitor_gid);
	}

	/* Now we can set our peer ID (in process realm at is is used for network setup) */
	__monitor.process_uid = mpc_lowcomm_monitor_uid_of(__monitor.monitor_gid, mpc_common_get_process_rank());


	/* At this point all processes in the group share the same GID without comm */

	_mpc_lowcomm_monitor_command_engine_init();
	_mpc_lowcomm_monitor_on_demand_callbacks_init();

	return 0;
}

static inline int __remove_uid(_mpc_lowcomm_set_t *set, void *arg)
{
	mpc_common_debug_error("JOBID : %u is_lead %d", set->uid, set->is_lead);

	/* Only the lead process proceeds to clear */
	if(set->is_lead)
	{
		mpc_common_debug_error("CLEAR");
		_mpc_lowcomm_uid_clear(set->uid);
	}

	return 0;
}

int _mpc_lowcomm_monitor_teardown()
{
	mpc_lowcomm_monitor_retcode_t ret;

	ret = _mpc_lowcomm_monitor_release(&__monitor);

	if(ret != MPC_LAUNCH_MONITOR_RET_SUCCESS)
	{
		mpc_lowcomm_monitor_retcode_print(ret, __func__);
		return 1;
	}

	if(_mpc_lowcomm_set_teardown() )
	{
		return 1;
	}

	_mpc_lowcomm_monitor_command_engine_teardown();
	_mpc_lowcomm_monitor_on_demand_callbacks_teardown();


	return 0;
}

int _mpc_lowcomm_monitor_setup_per_task()
{
	__register_process_set();
	__bootstrap_ring();

	if(mpc_common_get_process_rank() == 0)
	{
		__exchange_peer_info();
		/* Now load other sets (requires commands) */
		_mpc_lowcomm_set_load_from_system();
	}

	return 0;
}

int _mpc_lowcomm_monitor_teardown_per_task()
{
	if(mpc_common_get_local_task_rank() == 0)
	{
		/* Remove local sets from UID store */
		_mpc_lowcomm_set_iterate(__remove_uid, NULL);
	}
	return 0;
}

/**************************
* MONITOR HUB DEFINITION *
**************************/

_mpc_lowcomm_client_ctx_t *_mpc_lowcomm_client_ctx_new(uint64_t uid, int fd)
{
	_mpc_lowcomm_client_ctx_t *ret = sctk_malloc(sizeof(_mpc_lowcomm_client_ctx_t) );

	assume(ret != NULL);

	ret->uid       = uid;
	ret->client_fd = fd;
	time(&ret->last_used);

	return ret;
}

int _mpc_lowcomm_client_ctx_release(_mpc_lowcomm_client_ctx_t **client)
{
	if(!client)
	{
		return 1;
	}

	if(*client)
	{
		shutdown( (*client)->client_fd, SHUT_RDWR);
		close( (*client)->client_fd);
		sctk_free(*client);
	}

	return 0;
}

_mpc_lowcomm_client_ctx_t *__accept_incoming(struct _mpc_lowcomm_monitor_s *monitor)
{
	struct sockaddr_in client_addr;
	socklen_t          len = 0;

	int new_fd = accept(monitor->server_socket, (struct sockaddr *)&client_addr, &len);

	if(new_fd < 0)
	{
		//perror ("accept");
		return NULL;
	}

	/* First of all read UID from socket */
	uint64_t uid = 0;

	int ret = mpc_common_io_safe_read(new_fd, &uid, sizeof(uint64_t) );

	if(ret == 0)
	{
		close(new_fd);
		return NULL;
	}

	int refused = 0;

	/* Set UID is non-zero if we receive 0 we skip the
	 * already connected set check as we have a temporary
	 * client */
	if(uid)
	{
		/* Now make sure this UID is not already known */
		if(_mpc_lowcomm_monitor_client_known(monitor, uid) != NULL)
		{
			mpc_common_debug("UID %lu is alreary connected closing", uid);

			/* Notify remote end of our refusal */

			refused = 1;
			shutdown(new_fd, SHUT_RDWR);
			close(new_fd);
			return NULL;
		}
	}


	/* Notify the client of acceptance or refusal */
	mpc_common_io_safe_write(new_fd, &refused, sizeof(int) );

	if(!refused)
	{
		_mpc_lowcomm_client_ctx_t *new = _mpc_lowcomm_client_ctx_new(uid, new_fd);
		_mpc_lowcomm_monitor_client_add(monitor, new);
		return new;
	}

	return NULL;
}

static inline int __handle_query(_mpc_lowcomm_client_ctx_t *ctx, _mpc_lowcomm_monitor_wrap_t *data)
{
	assume(data != NULL);
	assume(ctx != NULL);


	mpc_lowcomm_monitor_command_t cmd      = data->command;
	mpc_lowcomm_monitor_args_t *  cmd_data = data->content;
	_mpc_lowcomm_monitor_wrap_t * resp     = NULL;


	if(data->is_response)
	{
		/* Send back to the calling context */
		assume(data->dest == __monitor.process_uid);
		_mpc_lowcomm_monitor_command_engine_push_response(data);
		return 0;
	}



	switch(cmd)
	{
		case MPC_LAUNCH_MONITOR_COMMAND_REQUEST_SET_INFO:
		{
			/* Here we register the incoming set */
			_mpc_lowcomm_monitor_command_register_set_info(cmd_data);

			mpc_lowcomm_set_uid_t target_set = mpc_lowcomm_peer_get_set(data->dest);
			resp = _mpc_lowcomm_monitor_command_return_set_info(data->from,
			                                                    data->match_key,
			                                                    target_set);
		}
		break;

		case MPC_LAUNCH_MONITOR_COMMAND_REQUEST_PEER_INFO:
			/* In all cases register the incoming peer */
			_mpc_lowcomm_monitor_command_register_peer_info(cmd_data);

			resp = _mpc_lowcomm_monitor_command_return_peer_info(data->from,
			                                                     data->match_key,
			                                                     cmd_data->peer.requested_peer);
			break;

		case MPC_LAUNCH_MONITOR_PING:
			resp = _mpc_lowcomm_monitor_command_return_ping_info(data->from, 1);
			break;

		case MPC_LAUNCH_MONITOR_ON_DEMAND:
			resp = _mpc_lowcomm_monitor_command_process_ondemand(data->from, data->match_key, cmd_data);
			break;

		case MPC_LAUNCH_MONITOR_COMMAND_NONE:
			return -1;
	}

	mpc_common_debug_error("RESP == %p", resp);

	if(resp != NULL)
	{
		/* We need to route */
		mpc_lowcomm_monitor_retcode_t send_ret;
		if(_mpc_lowcomm_monitor_command_send(resp, &send_ret) < 0)
		{
			mpc_lowcomm_monitor_retcode_print(send_ret, "Sending response command");
			return -1;
		}
	}

	return 0;
}

static inline void __notify_new_client(void)
{
	char buff[1];
	mpc_common_io_safe_write(__monitor.clientfd[1], buff, 1);
}



static void *__server_loop(void *pmonitor)
{
	struct _mpc_lowcomm_monitor_s *monitor = (struct _mpc_lowcomm_monitor_s *)pmonitor;

	/* This is the CTX to FD table */
	_mpc_lowcomm_client_ctx_t *all_ctx[FD_SETSIZE] = { 0 };


	while(1)
	{
		/* Now time to setup FDSETS */
		FD_ZERO(&monitor->read_fds);

		/* Add server socket to read_fds */
		FD_SET(monitor->server_socket, &monitor->read_fds);

		/* Add the notification pipe */
		FD_SET(monitor->notiffd[0], &monitor->read_fds);

		/* Add the notification pipe */
		FD_SET(monitor->clientfd[0], &monitor->read_fds);


		/* Now add all client sockets */
		_mpc_lowcomm_client_ctx_t *ctx;

		MPC_HT_ITER(&monitor->client_contexts, ctx)
		{
			if(!ctx)
			{
				continue;
			}

			all_ctx[ctx->client_fd] = ctx;

			FD_SET(ctx->client_fd, &monitor->read_fds);
		}
		MPC_HT_ITER_END

		struct timeval to;

		to.tv_sec = 10;
		to.tv_usec = 0;

		int ret = select(FD_SETSIZE,
		                 &monitor->read_fds,
		                 NULL,
		                 NULL,
		                 &to);

		if(ret < 0)
		{
			perror("select");
			if(errno == EINTR || errno == EAGAIN)
			{
				continue;
			}

			perror("select");
			break;
		}

		if(!monitor->running)
		{
			/* Done */
			break;
		}

		int i;

		for(i = 0; i < FD_SETSIZE; i++)
		{
			if(FD_ISSET(i, &monitor->read_fds) )
			{
				if(i == monitor->clientfd[0])
				{
					continue;
				}

				if(i == monitor->server_socket)
				{
					mpc_common_debug_error("Server EVENT");


					_mpc_lowcomm_client_ctx_t *new_ctx = NULL;
					/* Accept incoming connection */
					if( (new_ctx = __accept_incoming(monitor) ) != NULL)
					{
						/* new client */
						all_ctx[new_ctx->client_fd] = new_ctx;
					}
					else
					{
						/* Was refused or error */
					}
					break;
				}

				/* Process incoming message */
				_mpc_lowcomm_monitor_wrap_t *query = _mpc_lowcomm_monitor_recv(i);


				if(!query)
				{
					mpc_common_debug_error("Client %lu left", all_ctx[i]->uid);
					_mpc_lowcomm_monitor_client_remove(monitor, all_ctx[i]->uid);
					all_ctx[i] = NULL;
					break;
				}

				mpc_common_debug_warning("Handle message for %llu from %llu", query->dest, query->from);
				/* Check if we are the dest or if we are routing */
				if(_mpc_lowcomm_peer_is_local(query->dest) )
				{
					__handle_query(all_ctx[i], query);
				}
				else
				{
					/* We need to route */
					mpc_common_debug_error("ROUTING");
					mpc_lowcomm_monitor_retcode_t route_ret;
					if(_mpc_lowcomm_monitor_command_send(query, &route_ret) < 0)
					{
					}
				}
			}
		}
	}



	return NULL;
}

static inline int __get_port_from_socket(int socket)
{
	struct sockaddr_in sin;
	socklen_t          len = sizeof(struct sockaddr);

	int ret = getsockname(socket, (struct sockaddr *)&sin, &len);

	if(ret < 0)
	{
		perror("getsockname");
		return -1;
	}

	return ntohs(sin.sin_port);
}

static mpc_lowcomm_monitor_retcode_t __start_server_socket(struct _mpc_lowcomm_monitor_s *monitor)
{
	struct addrinfo *res = NULL;
	struct addrinfo  hints;

	memset(&hints, 0, sizeof(struct addrinfo) );

	hints.ai_family   = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags    = AI_PASSIVE;

	/* Note we bind to port 0 as we do not care much of where we are */
	int ret = getaddrinfo(NULL, "0", &hints, &res);

	if(ret < 0)
	{
		if(ret == EAI_SYSTEM)
		{
			fprintf(stderr, "Monitor getaddrinfo error: %s\n", strerror(errno) );
		}
		else
		{
			fprintf(stderr, "Monitor getaddrinfo error: %s\n", gai_strerror(ret) );
		}
		return MPC_LAUNCH_MONITOR_RET_ERROR;
	}

	struct addrinfo *tmp = NULL;

	for(tmp = res; tmp != NULL; tmp = tmp->ai_next)
	{
		/* Now try the various configurations */

		/* Socket creation */
		monitor->server_socket = socket(tmp->ai_family, tmp->ai_socktype, tmp->ai_protocol);

		if(monitor->server_socket < 0)
		{
			/* Not working */
			monitor->server_socket = -1;
			continue;
		}

		/* Try to bind */
		ret = bind(monitor->server_socket, tmp->ai_addr, tmp->ai_addrlen);

		if(ret < 0)
		{
			/* Not working */
			perror("Failed to bind monitor server");
			close(monitor->server_socket);
			monitor->server_socket = -1;
			continue;
		}
	}

	if(monitor->server_socket < 0)
	{
		/* We failed creating the monitor server */
		mpc_common_debug_error("Failed binding monitor server socket");
		return MPC_LAUNCH_MONITOR_RET_ERROR;
	}

	ret = listen(monitor->server_socket, 32);

	if(ret < 0)
	{
		perror("listen");
		mpc_common_debug_error("Failed listen on monitor server socket");
		return MPC_LAUNCH_MONITOR_RET_ERROR;
	}

	/* If we are here the server is ready to start its thread
	 * but first lets setup our address info */
	int port = __get_port_from_socket(monitor->server_socket);

	if(port < 0)
	{
		mpc_common_debug_error("Failed retrieving port on monitor server socket");
		return MPC_LAUNCH_MONITOR_RET_ERROR;
	}

	char hostname[512];
	ret = gethostname(hostname, 512);

	if(ret < 0)
	{
		mpc_common_debug_error("Failed retrieving hostname on monitor server socket");
		return MPC_LAUNCH_MONITOR_RET_ERROR;
	}

	ret = snprintf(monitor->monitor_uri, MPC_LOWCOMM_PEER_URI_SIZE, "%s:%d", hostname, port);

	if(MPC_LOWCOMM_PEER_URI_SIZE <= ret)
	{
		/* URI was mpc_common_debug_error */
		mpc_common_debug_fatal("Monitor server URI was too long to be stored");
		return MPC_LAUNCH_MONITOR_RET_ERROR;
	}

	return MPC_LAUNCH_MONITOR_RET_SUCCESS;
}

static inline void __monitor_publish_over_pmi(struct _mpc_lowcomm_monitor_s *monitor)
{
	int  process_rank = mpc_common_get_process_rank();
	char buff[64];

	__server_key_name(buff, 64, process_rank);

	if(mpc_launch_pmi_is_initialized() )
	{
		mpc_launch_pmi_put(monitor->monitor_uri, buff, 0 /* non local */);
	}

	/* Sync the job to ensure PMI exchange */
	mpc_launch_pmi_barrier();
}

mpc_lowcomm_monitor_retcode_t _mpc_lowcomm_monitor_init(struct _mpc_lowcomm_monitor_s *monitor)
{
	/* Clear sets */
	FD_ZERO(&monitor->read_fds);


	monitor->process_set   = NULL;
	monitor->process_uid = 0;

	/* Prepare client list */
	pthread_mutex_init(&monitor->client_lock, NULL);
	monitor->client_count = 0;
	mpc_common_hashtable_init(&monitor->client_contexts, _MPC_LOWCOMM_MONITOR_MAX_CLIENTS_);

	/* First step listen on 0.0.0.0 */
	monitor->server_socket = -1;

	mpc_lowcomm_monitor_retcode_t ret = __start_server_socket(monitor);

	if(ret != MPC_LAUNCH_MONITOR_RET_SUCCESS)
	{
		mpc_common_debug_fatal("Failed to start the cplane monitor server");
	}

	monitor->running = 1;
	pipe(monitor->notiffd);
	pipe(monitor->clientfd);



	/* Now start the server thread */
	int rc = pthread_create(&monitor->server_thread, NULL, __server_loop, (void *)monitor);

	if(rc < 0)
	{
		perror("pthread_create");
		mpc_common_debug_fatal("Failed creating cplane server threads");
	}

	mpc_common_debug_warning("SERVER running @ %s", monitor->monitor_uri);

	__monitor_publish_over_pmi(monitor);

	return MPC_LAUNCH_MONITOR_RET_SUCCESS;
}

mpc_lowcomm_monitor_retcode_t _mpc_lowcomm_monitor_release(struct _mpc_lowcomm_monitor_s *monitor)
{
	monitor->running = 0;

	/* Notify end to select thread */
	int dummy = 0;
	mpc_common_io_safe_write(monitor->notiffd[1], &dummy, sizeof(int) );

	/* Disconnect all */
	_mpc_lowcomm_monitor_disconnect(monitor);

	shutdown(monitor->server_thread, SHUT_RDWR);
	close(monitor->server_thread);

	pthread_join(monitor->server_thread, NULL);

	close(monitor->notiffd[0]);
	close(monitor->notiffd[1]);


	close(monitor->clientfd[0]);
	close(monitor->clientfd[1]);

	mpc_common_hashtable_release(&monitor->client_contexts);
	pthread_mutex_destroy(&monitor->client_lock);

	return 0;
}

_mpc_lowcomm_client_ctx_t *_mpc_lowcomm_monitor_client_known(struct _mpc_lowcomm_monitor_s *monitor,
                                                             uint64_t uid)
{
	pthread_mutex_lock(&monitor->client_lock);
	/* Is the peer already connected ? */
	_mpc_lowcomm_client_ctx_t *ctx = mpc_common_hashtable_get(&monitor->client_contexts, uid);
	pthread_mutex_unlock(&monitor->client_lock);

	return ctx;
}

int _mpc_lowcomm_monitor_client_add(struct _mpc_lowcomm_monitor_s *monitor,
                                    _mpc_lowcomm_client_ctx_t *client)
{
	pthread_mutex_lock(&monitor->client_lock);
	mpc_common_hashtable_set(&monitor->client_contexts, client->uid, client);
	pthread_mutex_unlock(&monitor->client_lock);

	__notify_new_client();

	return 0;
}

int _mpc_lowcomm_monitor_client_remove(struct _mpc_lowcomm_monitor_s *monitor,
                                       uint64_t uid)
{
	pthread_mutex_lock(&monitor->client_lock);
	/* Is the peer already connected ? */
	_mpc_lowcomm_client_ctx_t *ctx = mpc_common_hashtable_get(&monitor->client_contexts, uid);

	if(!ctx)
	{
		pthread_mutex_unlock(&monitor->client_lock);
		return -1;
	}

	mpc_common_hashtable_delete(&monitor->client_contexts, uid);
	_mpc_lowcomm_client_ctx_release(&ctx);
	monitor->client_count--;

	pthread_mutex_unlock(&monitor->client_lock);

	return 0;
}

static inline _mpc_lowcomm_client_ctx_t *__connect_client(struct _mpc_lowcomm_monitor_s *monitor,
                                                          char *uri,
                                                          uint64_t uid,
                                                          mpc_lowcomm_monitor_retcode_t *retcode)
{
	if(strstr(uri, "pmi://") )
	{
		mpc_common_debug_error("Cannot connect to an URI not resolved through PMI");
		return NULL;
	}


	char localuri[MPC_LOWCOMM_PEER_URI_SIZE];
	snprintf(localuri, MPC_LOWCOMM_PEER_URI_SIZE, uri);

	/*mpc_common_debug_warning("(%u, %u) connecting to (%u, %u)", mpc_lowcomm_peer_get_set(monitor->process_uid),
	 *                       mpc_lowcomm_peer_get_rank(monitor->process_uid), mpc_lowcomm_peer_get_set(uid), mpc_lowcomm_peer_get_rank(uid) );*/

	/*mpc_common_debug_warning("%lu -> %lu", monitor->process_uid, uid);
	 * mpc_common_debug_warning("%lu [label='(%u, %u)']",monitor->process_uid , mpc_lowcomm_peer_get_set(monitor->process_uid),
	 *                       mpc_lowcomm_peer_get_rank(monitor->process_uid));*/

	char *hostname;
	char *port;

	char *sep = strchr(localuri, ':');

	if(!sep)
	{
		mpc_common_debug_error("Bad peer URI %s", uri);
		*retcode = MPC_LAUNCH_MONITOR_RET_ERROR;
		return NULL;
	}

	*sep     = '\0';
	hostname = localuri;
	port     = sep + 1;


	int len = strlen(port);
	int i;

	for(i = 0; i < len; i++)
	{
		if(!isdigit(port[i]) )
		{
			mpc_common_debug_error("%s is not a numeric port", port);
			*retcode = MPC_LAUNCH_MONITOR_RET_ERROR;
			return NULL;
		}
	}


	/* Now time to connect */

	struct addrinfo *res = NULL;
	struct addrinfo  hints;

	memset(&hints, 0, sizeof(struct addrinfo) );

	hints.ai_family   = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	/* Note we bind to port 0 as we do not care much of where we are */
	int ret = getaddrinfo(hostname, port, &hints, &res);

	if(ret < 0)
	{
		if(ret == EAI_SYSTEM)
		{
			fprintf(stderr, "Failed resolving peer: %s\n", strerror(errno) );
		}
		else
		{
			fprintf(stderr, "Failed resolving peer: %s\n", gai_strerror(ret) );
		}

		*retcode = MPC_LAUNCH_MONITOR_RET_NOT_REACHABLE;
		return NULL;
	}

	struct addrinfo *tmp = NULL;
	int client_socket    = -1;

	for(tmp = res; tmp != NULL; tmp = tmp->ai_next)
	{
		/* Now try the various configurations */

		/* Socket creation */
		client_socket = socket(tmp->ai_family, tmp->ai_socktype, tmp->ai_protocol);

		if(client_socket < 0)
		{
			/* Not working */
			client_socket = -1;
			continue;
		}

		if(connect(client_socket, tmp->ai_addr, tmp->ai_addrlen) < 0)
		{
			close(client_socket);
			client_socket = -1;
			continue;
		}
	}

	if(client_socket < 0)
	{
		*retcode = MPC_LAUNCH_MONITOR_RET_NOT_REACHABLE;
		return NULL;
	}

	/* If we have a socket the first step is to write our UID */
	mpc_common_io_safe_write(client_socket, &__monitor.process_uid, sizeof(uint64_t) );

	/* Now wait to see if we were refused (already connected) */
	int refused = 0;
	mpc_common_io_safe_read(client_socket, &refused, sizeof(int) );

	if(refused)
	{
		close(client_socket);
		*retcode = MPC_LAUNCH_MONITOR_RET_REFUSED;
		return NULL;
	}

	/* If we are here we can create the client */
	_mpc_lowcomm_client_ctx_t *new = _mpc_lowcomm_client_ctx_new(uid, client_socket);

	/* And we register it */
	_mpc_lowcomm_monitor_client_add(monitor, new);


	*retcode = MPC_LAUNCH_MONITOR_RET_SUCCESS;

	return new;
}

static inline _mpc_lowcomm_client_ctx_t *__get_client(struct _mpc_lowcomm_monitor_s *monitor,
                                                      mpc_lowcomm_peer_uid_t client_uid,
                                                      mpc_lowcomm_monitor_retcode_t *retcode)
{
	_mpc_lowcomm_peer_t *peer = _mpc_lowcomm_peer_get(client_uid);

	if(!peer)
	{
		*retcode = MPC_LAUNCH_MONITOR_RET_INVALID_UID;
		return NULL;
	}

	if(strstr(peer->infos.uri, "pmi://") )
	{
		/* We need to PMI resolve */
		char *key = peer->infos.uri + 6;
		char  value[MPC_LOWCOMM_PEER_URI_SIZE];
		if(mpc_launch_pmi_get(value, MPC_LOWCOMM_PEER_URI_SIZE, key, 1) != MPC_LAUNCH_PMI_SUCCESS)
		{
			mpc_common_debug_fatal("Failed retrieving remote monitor key %s", key);
		}

		mpc_common_debug("Resolved %s to %s", peer->infos.uri, value);
		snprintf(peer->infos.uri, MPC_LOWCOMM_PEER_URI_SIZE, value);
	}


	/* Now time to connect */
	_mpc_lowcomm_client_ctx_t *ctx = __connect_client(monitor, peer->infos.uri, client_uid, retcode);

	if(ctx != NULL)
	{
		/* All done seems OK */
		*retcode = MPC_LAUNCH_MONITOR_RET_SUCCESS;
		monitor->client_count++;
	}

	return ctx;
}

static inline _mpc_lowcomm_client_ctx_t *__get_closest_in_set(struct _mpc_lowcomm_monitor_s *monitor,
                                                              mpc_lowcomm_peer_uid_t dest,
                                                              mpc_lowcomm_set_uid_t set_uid,
                                                              mpc_lowcomm_monitor_retcode_t *retcode)
{
	/* First approach client scan */
	_mpc_lowcomm_client_ctx_t *ctx          = NULL;
	_mpc_lowcomm_client_ctx_t *ellected_ctx = NULL;

	mpc_lowcomm_peer_uid_t current_route_uid = 0;

	MPC_HT_ITER(&monitor->client_contexts, ctx)
	{
		if(!ctx)
		{
			continue;
		}

		if(mpc_lowcomm_peer_closer(dest, current_route_uid, ctx->uid) )
		{
			/* We have one pick it if closer */
			current_route_uid = ctx->uid;
			ellected_ctx      = ctx;
		}
	}
	MPC_HT_ITER_END

	if(ellected_ctx != NULL)
	{
		*retcode = MPC_LAUNCH_MONITOR_RET_SUCCESS;
		/* We found a set member to route to */
		mpc_common_debug_warning("ROUTE to %u through %u", dest, ctx->uid);
		return ctx;
	}

	/* Here we did not find a route if we are the root process we do direct. If not we do route to the root process */
	//if(mpc_lowcomm_peer_get_rank(monitor->process_uid) == 0)
	if(1)
	{
		/* If we are here we need to connect directly to the set master as
		* we did not find any set member in our viscinity */
		mpc_lowcomm_peer_uid_t master_s_uid = mpc_lowcomm_monitor_uid_of(set_uid, 0);
		return _mpc_lowcomm_monitor_get_client(monitor, master_s_uid, _MPC_LOWCOMM_MONITOR_GET_CLIENT_DIRECT, retcode);
	}
	else
	{
		/* Here as we are a subprocess we route to 0 which should already be connected */
		mpc_lowcomm_peer_uid_t master_s_uid = mpc_lowcomm_monitor_uid_of(monitor->process_set->uid, 0);
		return _mpc_lowcomm_monitor_get_client(monitor, master_s_uid, _MPC_LOWCOMM_MONITOR_GET_CLIENT_CAN_ROUTE, retcode);
	}


	return NULL;
}

static inline _mpc_lowcomm_client_ctx_t *__route_client(struct _mpc_lowcomm_monitor_s *monitor,
                                                        mpc_lowcomm_peer_uid_t client_uid,
                                                        mpc_lowcomm_monitor_retcode_t *retcode)
{
	/* First lets check if the set UID is possibly known */
	mpc_lowcomm_set_uid_t set_uid = mpc_lowcomm_peer_get_set(client_uid);

	/* Make sure the set is known */
	_mpc_lowcomm_set_t *tset = _mpc_lowcomm_set_get(set_uid);

	if(!tset)
	{
		/* Give us a chance to get the set by reloading
		 * sets from the system */
		_mpc_lowcomm_set_load_from_system();
		tset = _mpc_lowcomm_set_get(set_uid);
	}

	if(!tset)
	{
		/* No set with such UID */
		*retcode = MPC_LAUNCH_MONITOR_RET_INVALID_UID;
		return NULL;
	}

	/* If we have a set make sure the target UID is known */
	if(!_mpc_lowcomm_set_contains(tset, client_uid) )
	{
		*retcode = MPC_LAUNCH_MONITOR_RET_INVALID_UID;
		return NULL;
	}

	/* Now we are sure destination is valid now return the closest next
	 * hop we know about */
	_mpc_lowcomm_client_ctx_t *ret = __get_closest_in_set(monitor, client_uid, set_uid, retcode); 
	
	if(ret)
	{
		mpc_common_debug_warning("ROUTE [%lu through %lu]", mpc_lowcomm_monitor_get_uid(), ret->uid);
	}

	return ret;
}

_mpc_lowcomm_client_ctx_t *_mpc_lowcomm_monitor_get_client(struct _mpc_lowcomm_monitor_s *monitor,
                                                           mpc_lowcomm_peer_uid_t client_uid,
                                                           _mpc_lowcomm_monitor_get_client_policy_t policy,
                                                           mpc_lowcomm_monitor_retcode_t *retcode)
{
	/* Is the peer already connected ? */
	_mpc_lowcomm_client_ctx_t *ctx = _mpc_lowcomm_monitor_client_known(monitor, client_uid);

	if(ctx)
	{
		/* In all cases if we are already connected: do direct */
		*retcode = MPC_LAUNCH_MONITOR_RET_SUCCESS;
		return ctx;
	}

	if(policy == _MPC_LOWCOMM_MONITOR_GET_CLIENT_DIRECT)
	{
		/* We were requested to direct connect */
		return __get_client(monitor, client_uid, retcode);
	}
	else if(policy == _MPC_LOWCOMM_MONITOR_GET_CLIENT_CAN_ROUTE)
	{
		/* Peer is not directly connected we then attempt to rely on routing */
		return __route_client(monitor, client_uid, retcode);
	}

	not_reachable();

	return NULL;
}

mpc_lowcomm_monitor_retcode_t _mpc_lowcomm_monitor_disconnect(struct _mpc_lowcomm_monitor_s *monitor)
{
	_mpc_lowcomm_client_ctx_t *client = NULL;

	int did_delete = 0;

	do
	{
		did_delete = 0;
		MPC_HT_ITER(&monitor->client_contexts, client)
		{
			if(client)
			{
				did_delete = 1;
				mpc_common_hashtable_delete(&monitor->client_contexts, client->uid);
				_mpc_lowcomm_client_ctx_release(&client);
				break;
			}
		}
		MPC_HT_ITER_END
	} while(did_delete);

	return MPC_LAUNCH_MONITOR_RET_SUCCESS;
}

int _mpc_lowcomm_peer_is_local(mpc_lowcomm_peer_uid_t uid)
{
	mpc_lowcomm_set_uid_t set_id = mpc_lowcomm_peer_get_set(uid);

	_mpc_lowcomm_set_t *set = _mpc_lowcomm_set_get(set_id);

	if(!set)
	{
		return 0;
	}

	return set->local_peer == uid;
}

/*********************
* COMMAND GENERATOR *
*********************/

static inline uint64_t __get_new_command_index(void)
{
	static uint64_t __command_index = 0;
	static mpc_common_spinlock_t __command_index_lock = SCTK_SPINLOCK_INITIALIZER;

	uint64_t ret = 0;

	mpc_common_spinlock_lock(&__command_index_lock);
	ret = ++__command_index;
	mpc_common_spinlock_unlock(&__command_index_lock);


	return ret;
}

static struct mpc_common_hashtable __response_table;

int _mpc_lowcomm_monitor_command_engine_init(void)
{
	mpc_common_hashtable_init(&__response_table, 32);
	return 0;
}

int _mpc_lowcomm_monitor_command_engine_teardown(void)
{
	mpc_common_hashtable_release(&__response_table);
	return 0;
}

static inline _mpc_lowcomm_monitor_wrap_t *__command_engine_get_response(uint64_t match_key)
{
	return mpc_common_hashtable_get(&__response_table, match_key);
}

int _mpc_lowcomm_monitor_command_engine_push_response(_mpc_lowcomm_monitor_wrap_t *response)
{
	assume(response != NULL);
	assume(response->is_response);

	/* Make sure it is not already processed */
	_mpc_lowcomm_monitor_wrap_t *existing_resp = __command_engine_get_response(response->match_key);
	assume(existing_resp == NULL);

	/* Now push it in */
	mpc_common_hashtable_set(&__response_table, response->match_key, response);

	return 0;
}

#define AWAIT_RESPONSE_USLEEP_UNIT    200


_mpc_lowcomm_monitor_wrap_t *_mpc_lowcomm_monitor_command_engine_await_response(uint64_t match_key, int max_second_wait)
{
	uint64_t total_time_awaited = 0;

	_mpc_lowcomm_monitor_wrap_t *resp = NULL;

	do
	{
		resp = __command_engine_get_response(match_key);

		if(!resp)
		{
			if(max_second_wait * 1e6 <= total_time_awaited)
			{
				/* We did timeout */
				break;
			}

			usleep(AWAIT_RESPONSE_USLEEP_UNIT);
			total_time_awaited += AWAIT_RESPONSE_USLEEP_UNIT;
		}
	} while(resp == NULL);

	/* If we have a response we remove it from the table as it is about to be consumed */
	if(resp)
	{
		mpc_common_hashtable_delete(&__response_table, resp->match_key);
	}

	return resp;
}

mpc_lowcomm_monitor_args_t *mpc_lowcomm_monitor_response_get_content(mpc_lowcomm_monitor_response_t response)
{
	if(!response)
	{
		return NULL;
	}

	_mpc_lowcomm_monitor_wrap_t *wr = (_mpc_lowcomm_monitor_wrap_t *)response;

	return wr->content;
}

int mpc_lowcomm_monitor_response_free(mpc_lowcomm_monitor_response_t response)
{
	if(!response)
	{
		return 1;
	}

	_mpc_lowcomm_monitor_wrap_t *wr = (_mpc_lowcomm_monitor_wrap_t *)response;

	_mpc_lowcomm_monitor_wrap_free(wr);

	return 0;
}

int _mpc_lowcomm_monitor_command_send(_mpc_lowcomm_monitor_wrap_t *cmd, mpc_lowcomm_monitor_retcode_t *ret)
{
	mpc_common_debug_warning("Command from (%u, %u) %llu to (%u, %u) %llu", mpc_lowcomm_peer_get_set(cmd->from),
	                         mpc_lowcomm_peer_get_rank(cmd->from), cmd->from, mpc_lowcomm_peer_get_set(cmd->dest), mpc_lowcomm_peer_get_rank(cmd->dest), cmd->dest );

	/* First of all get the peer */
	_mpc_lowcomm_client_ctx_t *peer_info = _mpc_lowcomm_monitor_get_client(&__monitor,
	                                                                       cmd->dest,
	                                                                       _MPC_LOWCOMM_MONITOR_GET_CLIENT_CAN_ROUTE,
	                                                                       ret);

	if(!peer_info)
	{
		return -1;
	}

	cmd->ttl--;

	if(cmd->ttl == 0)
	{
		mpc_common_debug_error("Message's TTL reached 0 there probably was a routing problem");
		mpc_common_debug_error("FROM %d TO %d", cmd->dest);
	}

		mpc_common_debug_error("WRITING to == %llu", peer_info->uid);


	if(_mpc_lowcomm_monitor_wrap_send(peer_info->client_fd, cmd) < 0)
	{
		return -1;
	}

	return 0;
}

int _mpc_lowcomm_monitor_command_register_set_info(mpc_lowcomm_monitor_args_t *cmd)
{
	/*first of all make sure the set is not already known */
	if(_mpc_lowcomm_set_get(cmd->set_info.uid) )
	{
		return -1;
	}

	/* Now register all not known peers */
	uint64_t i;


	uint64_t *peers_uids = sctk_malloc(cmd->set_info.peer_count * sizeof(uint64_t) );
	assume(peers_uids != NULL);

	for(i = 0; i < cmd->set_info.peer_count; i++)
	{
		mpc_lowcomm_monitor_peer_info_t *pinfo = &cmd->set_info.peers[i];

		/* Only register unknown peers we may already know root */
		if(!_mpc_lowcomm_peer_get(pinfo->uid) )
		{
			_mpc_lowcomm_peer_register(pinfo->uid,
			                           pinfo->local_task_count,
			                           pinfo->uri,
			                           0);
		}
		peers_uids[i] = pinfo->uid;
	}



	/* and eventually create the set */
	_mpc_lowcomm_set_init(cmd->set_info.uid,
	                      cmd->set_info.name,
	                      cmd->set_info.total_task_count,
	                      peers_uids,
	                      cmd->set_info.peer_count,
	                      0,
	                      0);


	sctk_free(peers_uids);

	return 0;
}

int mpc_lowcomm_monitor_peer_reachable_directly(mpc_lowcomm_peer_uid_t target_peer)
{
	mpc_lowcomm_monitor_retcode_t rc;
	_mpc_lowcomm_client_ctx_t *   ret = _mpc_lowcomm_monitor_get_client(&__monitor,
	                                                                    target_peer,
	                                                                    _MPC_LOWCOMM_MONITOR_GET_CLIENT_DIRECT,
	                                                                    &rc);

	return ret != NULL;
}
/**************************
 * COMMON WRAP GENERATION *
 **************************/


static inline _mpc_lowcomm_peer_t * __gen_wrap_for_peer(mpc_lowcomm_peer_uid_t dest,
													    mpc_lowcomm_monitor_command_t cmd,
														int is_response,
														_mpc_lowcomm_monitor_wrap_t **outcmd)
{
	_mpc_lowcomm_peer_t *rpeer = _mpc_lowcomm_peer_get(dest);

	/* If response this is set in the calling function */
	uint64_t response_index = 0;

	if(!is_response)
	{
		response_index = __get_new_command_index();
	}

	/* This prepares a request for set info this contains no data */
	*outcmd = _mpc_lowcomm_monitor_wrap_new(cmd,
											is_response,
											dest,
											__monitor.process_uid,
											response_index,
											sizeof(mpc_lowcomm_monitor_args_t));

	return rpeer;
}



/****************
* SET EXCHANGE *
****************/

static inline _mpc_lowcomm_monitor_wrap_t *__generate_set_decription(mpc_lowcomm_peer_uid_t dest,
                                                                     mpc_lowcomm_set_uid_t requested_set,
                                                                     int is_response)
{
	_mpc_lowcomm_set_t *set        = _mpc_lowcomm_set_get(requested_set);
	uint64_t            peer_count = 0;
	int no_such_uid = 0;

	if(!set)
	{
		no_such_uid = 1;
	}
	else
	{
		peer_count = set->peer_count;
	}

	/* If response this is set in the calling function */
	uint64_t response_index = 0;

	if(!is_response)
	{
		response_index = __get_new_command_index();
	}

	/* This prepares a request for set info this contains no data */
	_mpc_lowcomm_monitor_wrap_t *cmd = _mpc_lowcomm_monitor_wrap_new(MPC_LAUNCH_MONITOR_COMMAND_REQUEST_SET_INFO,
	                                                                 is_response,
	                                                                 dest,
	                                                                 __monitor.process_uid,
	                                                                 response_index,
	                                                                 sizeof(mpc_lowcomm_monitor_args_t)
	                                                                 + peer_count * sizeof(mpc_lowcomm_monitor_peer_info_t) );



	cmd->content->set_info.uid        = requested_set;
	cmd->content->set_info.peer_count = peer_count;

	if(no_such_uid)
	{
		cmd->content->set_info.retcode          = MPC_LAUNCH_MONITOR_RET_INVALID_UID;
		cmd->content->set_info.total_task_count = 0;
	}
	else
	{
		cmd->content->set_info.retcode = MPC_LAUNCH_MONITOR_RET_SUCCESS;
		snprintf(cmd->content->set_info.name, MPC_LOWCOMM_SET_NAME_LEN, set->name);
		cmd->content->set_info.total_task_count = set->total_task_count;

		uint64_t i;

		for(i = 0; i < peer_count; i++)
		{
			memcpy(&cmd->content->set_info.peers[i], &set->peers[i]->infos, sizeof(mpc_lowcomm_monitor_peer_info_t) );
		}
	}

	return cmd;
}

_mpc_lowcomm_monitor_wrap_t *_mpc_lowcomm_monitor_command_request_set_info(mpc_lowcomm_peer_uid_t dest)
{
	/* This is a request for remote set info also sending our own data */
	return __generate_set_decription(dest, __monitor.process_set->uid, 0);
}

mpc_lowcomm_monitor_response_t mpc_lowcomm_monitor_get_set_info(mpc_lowcomm_peer_uid_t target_peer,
                                                                mpc_lowcomm_monitor_retcode_t *ret)
{
	_mpc_lowcomm_monitor_wrap_t *out_cmd = _mpc_lowcomm_monitor_command_request_set_info(target_peer);


	if(_mpc_lowcomm_monitor_command_send(out_cmd, ret) < 0)
	{
		_mpc_lowcomm_monitor_wrap_free(out_cmd);
		return NULL;
	}


	_mpc_lowcomm_monitor_wrap_t *resp = _mpc_lowcomm_monitor_command_engine_await_response(out_cmd->match_key,
	                                                                                       60);


	if(!resp)
	{
		mpc_common_debug_warning("mpc_lowcomm_monitor_get_set_info timed out when targetting %u", target_peer);
	}
	else
	{
		if(resp->content->set_info.retcode == MPC_LAUNCH_MONITOR_RET_SUCCESS)
		{
			_mpc_lowcomm_monitor_command_register_set_info(resp->content);
		}
		else
		{
			mpc_lowcomm_monitor_retcode_print(resp->content->set_info.retcode, __FUNCTION__);
		}
	}

	return (mpc_lowcomm_monitor_response_t)resp;
}

_mpc_lowcomm_monitor_wrap_t *_mpc_lowcomm_monitor_command_return_set_info(mpc_lowcomm_peer_uid_t dest,
                                                                          uint64_t response_index,
                                                                          mpc_lowcomm_set_uid_t requested_set)
{
	_mpc_lowcomm_monitor_wrap_t *cmd = __generate_set_decription(dest, requested_set, 1);

	cmd->match_key = response_index;

	return cmd;
}

/****************
 * PING COMMAND *
 ****************/

static inline _mpc_lowcomm_monitor_wrap_t * __generate_ping_cmd(mpc_lowcomm_peer_uid_t dest, int is_response)
{
	_mpc_lowcomm_monitor_wrap_t * cmd = NULL;
	_mpc_lowcomm_peer_t * rpeer = __gen_wrap_for_peer(dest, MPC_LAUNCH_MONITOR_PING, is_response, &cmd);

	if(!rpeer)
	{
		cmd->content->ping.retcode = MPC_LAUNCH_MONITOR_RET_INVALID_UID;
	}
	else
	{
		cmd->content->ping.ping = time(NULL);
	}

	return cmd;
}


mpc_lowcomm_monitor_response_t mpc_lowcomm_monitor_ping(mpc_lowcomm_peer_uid_t dest, mpc_lowcomm_monitor_retcode_t *ret)
{
	_mpc_lowcomm_monitor_wrap_t * cmd = __generate_ping_cmd(dest, 0);

	if(_mpc_lowcomm_monitor_command_send(cmd, ret) < 0)
	{
		_mpc_lowcomm_monitor_wrap_free(cmd);
		return NULL;
	}

	_mpc_lowcomm_monitor_wrap_t *resp = _mpc_lowcomm_monitor_command_engine_await_response(cmd->match_key,
	                                                                                       60);


	if(!resp)
	{
		mpc_common_debug_warning("_mpc_lowcomm_monitor_command_request_peer_info timed out when targetting %u", dest);
	}

	return (mpc_lowcomm_monitor_response_t)resp;
}

_mpc_lowcomm_monitor_wrap_t *_mpc_lowcomm_monitor_command_return_ping_info(mpc_lowcomm_peer_uid_t dest,
                                                                           uint64_t response_index)
{
	_mpc_lowcomm_monitor_wrap_t *resp = __generate_ping_cmd(dest, 1);

	resp->match_key = response_index;

	return resp;
}
/**********************
 * ON DEMMAND COMMAND *
 **********************/

static inline _mpc_lowcomm_monitor_wrap_t * __generate_ondemand_cmd(mpc_lowcomm_peer_uid_t dest,
									   char *target,
									   char * data,
									   int is_response)
{
	_mpc_lowcomm_monitor_wrap_t * cmd = NULL;
	_mpc_lowcomm_peer_t * rpeer = __gen_wrap_for_peer(dest, MPC_LAUNCH_MONITOR_ON_DEMAND, is_response, &cmd);

	if(!rpeer)
	{
		cmd->content->on_demand.retcode = MPC_LAUNCH_MONITOR_RET_INVALID_UID;
	}
	else
	{
		snprintf(cmd->content->on_demand.target, MPC_LOWCOMM_ONDEMAND_TARGET_LEN, target);
		snprintf(cmd->content->on_demand.data, MPC_LOWCOMM_ONDEMAND_DATA_LEN, data);

	}

	return cmd;
}


mpc_lowcomm_monitor_response_t mpc_lowcomm_monitor_ondemand(mpc_lowcomm_peer_uid_t dest,
															char *target,
															char *data,
															mpc_lowcomm_monitor_retcode_t *ret)
{
	_mpc_lowcomm_monitor_wrap_t * cmd = __generate_ondemand_cmd(dest, target, data, 0);

	if(_mpc_lowcomm_monitor_command_send(cmd, ret) < 0)
	{
		_mpc_lowcomm_monitor_wrap_free(cmd);
		return NULL;
	}

	mpc_common_debug_error("BEFORE RESP (%ld)", cmd->match_key);

	_mpc_lowcomm_monitor_wrap_t *resp = _mpc_lowcomm_monitor_command_engine_await_response(cmd->match_key,
	                                                                                       60);

	mpc_common_debug_error("AFTER RESP (%ld)", cmd->match_key);


	if(!resp)
	{
		mpc_common_debug_warning("_mpc_lowcomm_monitor_command_request_peer_info timed out when targetting %u", dest);
	}

	return (mpc_lowcomm_monitor_response_t)resp;
}

_mpc_lowcomm_monitor_wrap_t *_mpc_lowcomm_monitor_command_process_ondemand(mpc_lowcomm_peer_uid_t dest,
																		   uint64_t response_index,
                                                                           mpc_lowcomm_monitor_args_t *ondemand)
{
	/* Try to call */
	void * ctx = NULL;
	mpc_lowcomm_on_demand_callback_t cb = mpc_lowcomm_monitor_get_on_demand_callback(ondemand->on_demand.target, &ctx);

	_mpc_lowcomm_monitor_wrap_t *resp = __generate_ondemand_cmd(dest, ondemand->on_demand.target, "", 1);
	resp->match_key = response_index;

	if(!cb)
	{
		resp->content->on_demand.retcode = MPC_LAUNCH_MONITOR_RET_ERROR;
	}
	else
	{
		int ret = (cb)(dest, ondemand->on_demand.data, resp->content->on_demand.data, MPC_LOWCOMM_ONDEMAND_DATA_LEN, ctx);

		if(ret != 0)
		{
			resp->content->on_demand.retcode = MPC_LAUNCH_MONITOR_RET_ERROR;
		}
		else
		{
			resp->content->on_demand.retcode = MPC_LAUNCH_MONITOR_RET_SUCCESS;
		}
	}

	mpc_common_debug_error("SENDING RESP MATCH %lu", response_index);

	return resp;
}



/*****************
* PEER EXCHANGE *
*****************/

int _mpc_lowcomm_monitor_command_register_peer_info(mpc_lowcomm_monitor_args_t *cmd)
{
	mpc_lowcomm_monitor_peer_info_t *pinfo = &cmd->peer.info;

	if(!_mpc_lowcomm_peer_get(pinfo->uid) )
	{
		_mpc_lowcomm_peer_register(pinfo->uid,
		                           pinfo->local_task_count,
		                           pinfo->uri,
		                           0);
	}

	return 0;
}

static inline _mpc_lowcomm_monitor_wrap_t *__generate_peer_decription(mpc_lowcomm_peer_uid_t dest,
                                                                      mpc_lowcomm_peer_uid_t requested_peer,
                                                                      int is_response)
{
	_mpc_lowcomm_monitor_wrap_t * cmd = NULL;
	_mpc_lowcomm_peer_t * rpeer = __gen_wrap_for_peer(dest, MPC_LAUNCH_MONITOR_COMMAND_REQUEST_PEER_INFO, is_response, &cmd);

	if(!rpeer)
	{
		cmd->content->peer.retcode = MPC_LAUNCH_MONITOR_RET_INVALID_UID;
	}
	else
	{
		cmd->content->peer.retcode = MPC_LAUNCH_MONITOR_RET_SUCCESS;
		memcpy(&cmd->content->peer.info, &rpeer->infos, sizeof(mpc_lowcomm_monitor_peer_info_t) );
	}

	return cmd;
}

_mpc_lowcomm_monitor_wrap_t *_mpc_lowcomm_monitor_command_return_peer_info(mpc_lowcomm_peer_uid_t dest,
                                                                           uint64_t response_index,
                                                                           mpc_lowcomm_peer_uid_t requested_peer)
{
	_mpc_lowcomm_monitor_wrap_t *resp = __generate_peer_decription(dest,
	                                                               requested_peer,
	                                                               1);

	resp->match_key = response_index;

	return resp;
}

mpc_lowcomm_monitor_response_t mpc_lowcomm_monitor_command_get_peer_info(mpc_lowcomm_peer_uid_t dest,
                                                                         mpc_lowcomm_peer_uid_t requested_peer,
                                                                         mpc_lowcomm_monitor_retcode_t *ret)
{
	/* Add ourselves in peer content */
	_mpc_lowcomm_monitor_wrap_t *peer_cmd = __generate_peer_decription(dest,
	                                                                   __monitor.process_uid,
	                                                                   0);

	/* Request for remote */
	peer_cmd->content->peer.requested_peer = requested_peer;


	if(_mpc_lowcomm_monitor_command_send(peer_cmd, ret) < 0)
	{
		_mpc_lowcomm_monitor_wrap_free(peer_cmd);
		return NULL;
	}

	_mpc_lowcomm_monitor_wrap_t *resp = _mpc_lowcomm_monitor_command_engine_await_response(peer_cmd->match_key,
	                                                                                       60);


	if(!resp)
	{
		mpc_common_debug_warning("_mpc_lowcomm_monitor_command_request_peer_info timed out when targetting %u", dest);
	}
	else
	{
		if(resp->content->set_info.retcode == MPC_LAUNCH_MONITOR_RET_SUCCESS)
		{
			_mpc_lowcomm_monitor_command_register_peer_info(resp->content);
		}
		else
		{
			mpc_lowcomm_monitor_retcode_print(resp->content->set_info.retcode, __FUNCTION__);
		}
	}

	return (mpc_lowcomm_monitor_response_t)resp;
}

mpc_lowcomm_set_uid_t mpc_lowcomm_monitor_get_gid()
{
	assert(__monitor.monitor_gid != 0);
	return __monitor.monitor_gid;
}

mpc_lowcomm_peer_uid_t mpc_lowcomm_monitor_get_uid()
{
	assert(__monitor.process_uid != 0);
	return __monitor.process_uid;
}

/***********************
 * ON DEMAND CALLBACKS *
 ***********************/

struct __on_demand_callback
{
	char target[MPC_LOWCOMM_ONDEMAND_TARGET_LEN];
	mpc_lowcomm_on_demand_callback_t callback;
	struct __on_demand_callback * next;
	void * ctx;
};

mpc_common_spinlock_t __on_demand_callbacks_table_lock;
struct __on_demand_callback *__on_demand_callbacks_table = NULL;

int _mpc_lowcomm_monitor_on_demand_callbacks_init(void)
{
	__on_demand_callbacks_table = NULL;
	mpc_common_spinlock_init(&__on_demand_callbacks_table_lock, 0);
	return 0;
}

int _mpc_lowcomm_monitor_on_demand_callbacks_teardown(void)
{
	mpc_common_spinlock_lock(&__on_demand_callbacks_table_lock);
	struct __on_demand_callback * head = __on_demand_callbacks_table;

	while(head)
	{
		head = head->next;
		sctk_free(head);
	}

	return 0;
}

mpc_lowcomm_on_demand_callback_t mpc_lowcomm_monitor_get_on_demand_callback(char *target, void **ctx)
{
	struct __on_demand_callback * head = __on_demand_callbacks_table;

	while(head)
	{
		if(!strcmp(target, head->target))
		{
			*ctx = head->ctx;
			return head->callback;
		}
	}

	return NULL;
}


int mpc_lowcomm_monitor_register_on_demand_callback(char *target,
                                                    mpc_lowcomm_on_demand_callback_t callback,
													void * ctx)
{
	assume(callback != NULL);

	struct __on_demand_callback * new = sctk_malloc(sizeof(struct __on_demand_callback ));
	assume(new != NULL);

	snprintf(new->target, MPC_LOWCOMM_ONDEMAND_TARGET_LEN, target);
	new->callback = callback;
	new->ctx = ctx;

	mpc_common_spinlock_lock(&__on_demand_callbacks_table_lock);
	new->next = __on_demand_callbacks_table;
	__on_demand_callbacks_table = new;
	mpc_common_spinlock_unlock(&__on_demand_callbacks_table_lock);

	return 0;
}

int mpc_lowcomm_monitor_unregister_on_demand_callback(char * target)
{
	int did_handle = 0;
	mpc_common_spinlock_lock(&__on_demand_callbacks_table_lock);
	if(__on_demand_callbacks_table)
	{
		if(!strcmp(__on_demand_callbacks_table->target, target))
		{
			/* The target is the first one just pop it */
			struct __on_demand_callback * next = __on_demand_callbacks_table->next;
			sctk_free(__on_demand_callbacks_table);
			__on_demand_callbacks_table = next;
			did_handle = 1;
		}
	}
	mpc_common_spinlock_unlock(&__on_demand_callbacks_table_lock);

	if(did_handle)
	{
		return 0;
	}

	mpc_common_spinlock_lock(&__on_demand_callbacks_table_lock);

	struct __on_demand_callback * head = __on_demand_callbacks_table;

	while(head)
	{
		if(head->next)
		{
			if(!strcmp(target, head->next->target))
			{
				head->next = head->next->next;
				sctk_free(head->next);
				break;
			}
		}
	}

	mpc_common_spinlock_unlock(&__on_demand_callbacks_table_lock);

	return 0;
}