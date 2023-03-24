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
#include "monitor.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <ctype.h>
#include <stdio.h>
#include <time.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>
#include <semaphore.h>
#include <mpc_launch_pmi.h>
#include <mpc_common_rank.h>
#include <netinet/tcp.h>
#include <sctk_alloc.h>

#include <mpc_common_debug.h>
#include <mpc_common_helper.h>
#include <mpc_lowcomm.h>

#include "mpc_common_spinlock.h"

#include "communicator.h"
#include "lowcomm_thread.h"

#include "uid.h"
#include "monitor.h"
#include "name.h"

//#define MONITOR_DEBUG


static volatile int __monitor_running = 0;


const char * mpc_lowcomm_monitor_command_tostring(mpc_lowcomm_monitor_command_t cmd)
{
	switch(cmd)
	{
		case MPC_LOWCOMM_MONITOR_COMMAND_NONE:
			return "MPC_LOWCOMM_MONITOR_COMMAND_NONE";
		case MPC_LOWCOMM_MONITOR_COMMAND_REQUEST_PEER_INFO:
			return "MPC_LOWCOMM_MONITOR_COMMAND_REQUEST_PEER_INFO";
		case MPC_LOWCOMM_MONITOR_COMMAND_REQUEST_SET_INFO:
			return "MPC_LOWCOMM_MONITOR_COMMAND_REQUEST_SET_INFO";
		case MPC_LOWCOMM_MONITOR_PING:
			return "MPC_LOWCOMM_MONITOR_PING";
		case MPC_LOWCOMM_MONITOR_ON_DEMAND:
			return "MPC_LOWCOMM_MONITOR_ON_DEMAND";
		case MPC_LOWCOMM_MONITOR_CONNECTIVITY:
			return "MPC_LOWCOMM_MONITOR_CONNECTIVITY";
		case MPC_LOWCOMM_MONITOR_COMM_INFO:
			return "MPC_LOWCOMM_MONITOR_COMM_INFO";
		case MPC_LOWCOMM_MONITOR_NAMING:
			return "MPC_LOWCOMM_MONITOR_NAMING";
	}

	return "NO SUCH COMMAND";
}


/***************
* GLOBAL VARS *
***************/

static struct _mpc_lowcomm_monitor_s __monitor = { 0 };

/******************
 * WORKER THREADS *
 ******************/

typedef struct __monitor_work_context_s
{
	/* Params for event loop callbacks */
	mpc_lowcomm_monitor_worker_callback_t callback;
	void * ctx;
	/* Params for query handling */
	_mpc_lowcomm_client_ctx_t * client;
	_mpc_lowcomm_monitor_wrap_t * data;
	struct __monitor_work_context_s * next;
}__monitor_work_context_t;


struct __monitor_worker_s
{
	volatile int done;
	_mpc_lowcomm_kernel_thread_t worker_thread;
	__monitor_work_context_t * work_list;
	pthread_mutex_t lock;
	sem_t semaphore;
};

struct __monitor_worker_s __worker;

static inline __monitor_work_context_t * __monitor_worker_pop(void)
{
	__monitor_work_context_t * ret = NULL;
	pthread_mutex_lock(&__worker.lock);
	if(__worker.work_list)
	{
		ret = __worker.work_list;
		__worker.work_list = __worker.work_list->next;
	}
	pthread_mutex_unlock(&__worker.lock);

	return ret;
}



static inline __monitor_work_context_t * __monitor_work_context_new(_mpc_lowcomm_client_ctx_t * ctx, _mpc_lowcomm_monitor_wrap_t *data)
{
	__monitor_work_context_t * ret = sctk_malloc(sizeof(__monitor_work_context_t));

	assume(ret != NULL);

	ret->client = ctx;
	ret->data = data;
	ret->callback = NULL;
	ret->ctx = NULL;

	return ret;
}

static inline __monitor_work_context_t * __monitor_work_context_new_callback(mpc_lowcomm_monitor_worker_callback_t callback, void * ctx)
{
	__monitor_work_context_t * ret = sctk_malloc(sizeof(__monitor_work_context_t));

	assume(ret != NULL);

	ret->client = NULL;
	ret->data = NULL;
	ret->callback = callback;
	ret->ctx = ctx;

	return ret;
}

static inline void __monitor_worker_push(__monitor_work_context_t * work)
{
	pthread_mutex_lock(&__worker.lock);
	work->next = __worker.work_list;
	__worker.work_list = work;
	pthread_mutex_unlock(&__worker.lock);
}


int __monitor_worker_work_push(_mpc_lowcomm_client_ctx_t * ctx, _mpc_lowcomm_monitor_wrap_t *data)
{
	__monitor_work_context_t * new = __monitor_work_context_new(ctx, data);
	__monitor_worker_push(new);
	sem_post(&__worker.semaphore);

	return 0;
}

int __monitor_worker_work_push_callback(mpc_lowcomm_monitor_worker_callback_t callback, void * ctx)
{
	__monitor_work_context_t * new = __monitor_work_context_new_callback(callback, ctx);
	__monitor_worker_push(new);
	sem_post(&__worker.semaphore);

	return 0;
}

int mpc_lowcomm_monitor_event_loop_push(mpc_lowcomm_monitor_worker_callback_t callback, void *arg)
{
	return __monitor_worker_work_push_callback(callback, arg);
}

int __monitor_work_context_free(__monitor_work_context_t * wc)
{
	sctk_free(wc);
	return 0;
}


static inline int __handle_query(_mpc_lowcomm_client_ctx_t *ctx, _mpc_lowcomm_monitor_wrap_t *data);

static void * __worker_loop(void *pwork)
{
	struct __monitor_worker_s * worker = (struct __monitor_worker_s *)pwork;

	while(1)
	{
		sem_wait(&worker->semaphore);

		if( worker->done && (worker->work_list == NULL) )
		{
			/* Done and all done */
			return NULL;
		}

		__monitor_work_context_t * work = __monitor_worker_pop();

		if(work)
		{
			if(work->callback)
			{
				(work->callback)(work->ctx);
			}
			else
			{
				//_mpc_lowcomm_monitor_wrap_print(work->data, "PROCESSING");
				__handle_query(work->client, work->data);
				__monitor_work_context_free(work);
			}
		}

	}
}


static inline int __monitor_worker_init(void)
{
	__worker.done = 0;
	__worker.work_list = NULL;
	pthread_mutex_init(&__worker.lock, NULL);
	sem_init(&__worker.semaphore, 0, 0);
	_mpc_lowcomm_kernel_thread_create(&__worker.worker_thread, __worker_loop, (void*)&__worker);

	return 0;
}

static inline int __monitor_worker_release(void)
{
	__worker.done = 1;

	sem_post(&__worker.semaphore);

	_mpc_lowcomm_kernel_thread_join(&__worker.worker_thread);

	__worker.work_list = NULL;
	pthread_mutex_init(&__worker.lock, NULL);
	sem_init(&__worker.semaphore, 0, 0);

	return 0;
}



/***************
* ERROR CODES *
***************/

void mpc_lowcomm_monitor_retcode_print(mpc_lowcomm_monitor_retcode_t code, const char *ctx)
{
	char *desc = NULL;

	switch(code)
	{
		case MPC_LOWCOMM_MONITOR_RET_SUCCESS:
			desc = "SUCCESS";
			break;

		case MPC_LOWCOMM_MONITOR_RET_ERROR:
			desc = "Generic Error";
			break;

		case MPC_LOWCOMM_MONITOR_RET_DUPLICATE_KEY:
			desc = "Key is duplicated";
			break;

		case MPC_LOWCOMM_MONITOR_RET_NOT_REACHABLE:
			desc = "Provided UID was not reachable";
			break;

		case MPC_LOWCOMM_MONITOR_RET_INVALID_UID:
			desc = "Provided UID was not known";
			break;

		case MPC_LOWCOMM_MONITOR_RET_REFUSED:
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
	snprintf(buf, size, "%s", "cplane_monitor_uid");
	return buf;
}

static inline mpc_lowcomm_monitor_retcode_t __bootstrap_ring(void)
{
    int process_count = mpc_common_get_process_count(); 
	if( process_count == 1)
	{
		return MPC_LOWCOMM_MONITOR_RET_SUCCESS;
	}

	/* Bootstrap Kademlia like connections on WORLD */

	uint32_t target = 0;
	assume(__monitor.process_set != NULL);

	uint32_t i       = 1;
	int      my_rank = mpc_common_get_process_rank();

	int cnt = 0;

	mpc_lowcomm_peer_uid_t me = mpc_lowcomm_monitor_get_uid();

    int shift = 2;

    if(512 < process_count)
    {
        shift += 2;
    }

    if(1024 <= process_count)
    {
        shift += 2;
    }

	for(i = 1; i < __monitor.process_set->peer_count; i <<= shift)
	{
		target = (my_rank + i) % __monitor.process_set->peer_count;

		mpc_lowcomm_peer_uid_t to =  __monitor.process_set->peers[target]->infos.uid;

		/* Getting the client does create it if not present already */
		mpc_lowcomm_monitor_retcode_t retcode;
		_mpc_lowcomm_monitor_get_client(&__monitor,
		                                to,
		                                _MPC_LOWCOMM_MONITOR_GET_CLIENT_DIRECT,
		                                &retcode);
		if(retcode != MPC_LOWCOMM_MONITOR_RET_SUCCESS)
		{
			mpc_lowcomm_monitor_retcode_print(retcode, "Bootstraping ring");
			mpc_common_debug_fatal("Could not bootstrap ring");
		}

		char b1[128], b2[128];

#ifdef MONITOR_DEBUG
		mpc_common_debug_error("RING == %lu %s to %lu %s", me, mpc_lowcomm_peer_format_r(me, b1, 128) , to, mpc_lowcomm_peer_format_r(to, b2, 128) );
#endif

		if(MPC_LOWCOMM_MONITOR_MAX_CLIENTS <= ++cnt)
		{
			break;
		}
	}

	return MPC_LOWCOMM_MONITOR_RET_SUCCESS;
}




static inline int __exchange_peer_info(void)
{
	/* Here each rank != 0 in the process set requests infos from 0
	 * this has the effect of registering in 0 and pre retrieving 0's infos */
	mpc_lowcomm_monitor_retcode_t ret;
	mpc_lowcomm_peer_uid_t        root_process_uid = _mpc_lowcomm_set_root(__monitor.process_set->gid);

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
		char uri[155];

		if(i == my_proc_rank)
		{
			snprintf(uri, 155, "%s", __monitor.monitor_uri);
		}
		else
		{
			/* They all should be PMI reachable */
			__server_key_name(server_key, 128, i);
			snprintf(uri, 155, "pmi://%s", server_key);
		}
		_mpc_lowcomm_peer_register(pid,
									0,
									uri,
									(my_proc_rank == i) );
		peers_ids[i] = pid;
	}

	mpc_lowcomm_peer_uid_t local_peer_id = mpc_lowcomm_monitor_uid_of(job_id, mpc_common_get_process_rank() );


	char command_line[MPC_LOWCOMM_SET_NAME_LEN];
	mpc_common_helper_command_line_pretty(0, command_line, MPC_LOWCOMM_SET_NAME_LEN);


	/* Create the set atop the world set */
	__monitor.process_set = _mpc_lowcomm_set_init(job_id,
												  command_line,
												  mpc_common_get_task_count(),
												  peers_ids,
												  proc_count,
												  (mpc_common_get_process_rank() == 0) /* is_lead */,
												  local_peer_id);

	sctk_free(peers_ids);

	return 0;
}

int _mpc_lowcomm_monitor_setup()
{
	mpc_lowcomm_monitor_retcode_t ret;

	/* Time to provide generic sets */
	if(_mpc_lowcomm_set_setup() )
	{
		return 1;
	}

	ret = _mpc_lowcomm_monitor_init(&__monitor);

	if(ret != MPC_LOWCOMM_MONITOR_RET_SUCCESS)
	{
		mpc_lowcomm_monitor_retcode_print(ret, __func__);
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

	if(mpc_launch_pmi_is_initialized() )
	{
		/* Sync the job to ensure PMI exchange */
		mpc_launch_pmi_barrier();
	}

	if(mpc_common_get_process_rank() != 0)
	{
		mpc_launch_pmi_get(data, 32, buff, 0);
		sscanf(data,"%u", &__monitor.monitor_gid);
	}

	/* Now we can set our peer ID (in process realm at is is used for network setup) */
	__monitor.process_uid = mpc_lowcomm_monitor_uid_of(__monitor.monitor_gid, mpc_common_get_process_rank());

	if(mpc_launch_pmi_is_initialized() )
	{
		mpc_launch_pmi_barrier();
	}
	/* At this point all processes in the group share the same GID without comm */


	__register_process_set();
	__bootstrap_ring();
	_mpc_lowcomm_monitor_name_init();
	_mpc_lowcomm_monitor_on_demand_callbacks_init();
    _mpc_lowcomm_monitor_command_engine_init();

	if(mpc_launch_pmi_is_initialized() )
	{
		mpc_launch_pmi_barrier();
	}


	return 0;
}

static inline int __remove_uid(mpc_lowcomm_monitor_set_t pset, void *arg)
{
	_mpc_lowcomm_set_t *set = (_mpc_lowcomm_set_t*)pset;

	/* Only the lead process proceeds to clear */
	if(set->is_lead)
	{
		_mpc_lowcomm_uid_clear(set->gid);
	}

	return 0;
}

int _mpc_lowcomm_monitor_teardown()
{
	mpc_lowcomm_monitor_retcode_t ret;

	ret = _mpc_lowcomm_monitor_release(&__monitor);

	if(ret != MPC_LOWCOMM_MONITOR_RET_SUCCESS)
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
	_mpc_lowcomm_monitor_name_release();

	return 0;
}

int _mpc_lowcomm_monitor_setup_per_task()
{
	if(mpc_common_get_process_rank() == 0)
	{
		//__exchange_peer_info();
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

_mpc_lowcomm_client_ctx_t *_mpc_lowcomm_client_ctx_new(uint64_t uid, int fd, struct _mpc_lowcomm_monitor_s * monitor)
{
	_mpc_lowcomm_client_ctx_t *ret = sctk_malloc(sizeof(_mpc_lowcomm_client_ctx_t) );

	assume(ret != NULL);

	ret->uid       = uid;
	ret->client_fd = fd;
	ret->monitor = monitor;
	time(&ret->last_used);
	pthread_mutex_init(&ret->write_lock, NULL);

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

		/* There is a race between a client leaving
           and the monitor command completion */
		//pthread_mutex_destroy(&(*client)->write_lock);
		//sctk_free(*client);
	}

	return 0;
}

int _mpc_lowcomm_client_ctx_send(_mpc_lowcomm_client_ctx_t * peer_info, _mpc_lowcomm_monitor_wrap_t * cmd)
{
	pthread_mutex_lock(&peer_info->write_lock);

	if(_mpc_lowcomm_monitor_wrap_send(peer_info->client_fd, cmd) < 0)
	{
		pthread_mutex_unlock(&peer_info->write_lock);
		return -1;
	}

	pthread_mutex_unlock(&peer_info->write_lock);

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

	mpc_common_spinlock_lock_yield(&monitor->connect_accept_lock);

	int flag = 1;
	setsockopt(new_fd, IPPROTO_TCP, TCP_NODELAY, (char *) &flag, sizeof(int));

	/* First of all read UID from socket */
	uint64_t uid = 0;

	int ret = mpc_common_io_safe_read(new_fd, &uid, sizeof(uint64_t) );
	if(ret == 0)
	{
		mpc_common_spinlock_unlock(&monitor->connect_accept_lock);
		close(new_fd);
		return NULL;
	}

#ifdef MONITOR_DEBUG
	char bx[128];
	mpc_common_debug_error("[IN] %s INCOMING from %s", mpc_lowcomm_peer_format_r(mpc_lowcomm_monitor_get_uid(), bx, 128), mpc_lowcomm_peer_format(uid));
#endif

	int already_present = 0;

	/* Set UID is non-zero if we receive 0 we skip the
	 * already connected set check as we have a temporary
	 * client */
	if(uid)
	{
		/* Now make sure this UID is not already known */
		if(_mpc_lowcomm_monitor_client_known(monitor, uid) != NULL)
		{
			/* Now the process with the highest UID aborts the connection
			   this is how we advocate when doing synchronous cross-connections */

			if(uid < mpc_lowcomm_monitor_get_uid())
			{
#ifdef MONITOR_DEBUG
				char b1[128];
				mpc_common_debug_error("[REJECT] %s UID %s is alreary connected closing",mpc_lowcomm_peer_format_r(mpc_lowcomm_monitor_get_uid(), b1, 128), mpc_lowcomm_peer_format(uid));
#endif
				/* Notify remote end of our refusal */
				already_present = 1;
				shutdown(new_fd, SHUT_RDWR);
				close(new_fd);
				mpc_common_spinlock_unlock(&monitor->connect_accept_lock);
				return NULL;
			}
		}
	}

	_mpc_lowcomm_client_ctx_t *new = _mpc_lowcomm_client_ctx_new(uid, new_fd, monitor);
	_mpc_lowcomm_monitor_client_add(monitor, new);
#ifdef MONITOR_DEBUG
	char meb[128];
	mpc_common_debug_error("%s UID %s now connected", mpc_lowcomm_peer_format_r(mpc_lowcomm_monitor_get_uid(), meb, 128), mpc_lowcomm_peer_format(uid));
#endif
	mpc_common_spinlock_unlock(&monitor->connect_accept_lock);
	return new;
}

static inline int __handle_query(_mpc_lowcomm_client_ctx_t *ctx, _mpc_lowcomm_monitor_wrap_t *data)
{
	assume(data != NULL);
	//assume(ctx != NULL);


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
		case MPC_LOWCOMM_MONITOR_COMMAND_REQUEST_SET_INFO:
		{
			/* Here we register the incoming set */
			_mpc_lowcomm_monitor_command_register_set_info(cmd_data);

			mpc_lowcomm_set_uid_t target_set = mpc_lowcomm_peer_get_set(data->dest);
			resp = _mpc_lowcomm_monitor_command_return_set_info(data->from,
			                                                    data->match_key,
			                                                    target_set);
		}
		break;

		case MPC_LOWCOMM_MONITOR_COMMAND_REQUEST_PEER_INFO:
			/* In all cases register the incoming peer */
			_mpc_lowcomm_monitor_command_register_peer_info(cmd_data);

			resp = _mpc_lowcomm_monitor_command_return_peer_info(data->from,
			                                                     data->match_key,
			                                                     cmd_data->peer.requested_peer);
			break;

		case MPC_LOWCOMM_MONITOR_PING:
			resp = _mpc_lowcomm_monitor_command_return_ping_info(data->from, data->match_key);
			break;

		case MPC_LOWCOMM_MONITOR_ON_DEMAND:
			resp = _mpc_lowcomm_monitor_command_process_ondemand(data->from, data->match_key, cmd_data);
			break;

		case MPC_LOWCOMM_MONITOR_CONNECTIVITY:
			resp = _mpc_lowcomm_monitor_command_return_connectivity_info(data->from, data->match_key);
			break;

		case MPC_LOWCOMM_MONITOR_COMM_INFO:
			resp = _mpc_lowcomm_monitor_command_return_comm_info(data->from,
																 cmd_data,
                                                                 data->match_key);
			break;
		case MPC_LOWCOMM_MONITOR_NAMING:
			resp = _mpc_lowcomm_monitor_command_return_naming_info(data->from,
																   cmd_data,
                                                                   data->match_key);
			break;
		case MPC_LOWCOMM_MONITOR_COMMAND_NONE:
			return -1;
	}

	if(resp != NULL)
	{
#ifdef MONITOR_DEBUG
		char fromb[128], tob[128];
		mpc_common_debug_error("IN RESPONSE from %s to %s", mpc_lowcomm_peer_format_r(resp->from, fromb, 128), mpc_lowcomm_peer_format_r(resp->dest, tob, 128) );
#endif
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

}

static inline void __ack_new_client(void)
{

}

static void *__per_client_loop(void *pctx)
{
	_mpc_lowcomm_client_ctx_t *ctx = (_mpc_lowcomm_client_ctx_t*)pctx;

	while(1)
	{

		/* Process incoming message */
		_mpc_lowcomm_monitor_wrap_t *query = _mpc_lowcomm_monitor_recv(ctx->client_fd);

		if(!query)
		{
			mpc_common_nodebug("Client %lu left", ctx->uid);
			if(__monitor_running)
			{
				_mpc_lowcomm_monitor_client_remove(ctx->monitor, ctx->uid);
			}
			break;
		}


		/* Check if we are the dest or if we are routing */
		if(_mpc_lowcomm_peer_is_local(query->dest) )
		{
#ifdef MONITOR_DEBUG
		_mpc_lowcomm_monitor_wrap_print(query, "INCOMING QUERY");
#endif
			__monitor_worker_work_push(ctx, query);
		}
		else
		{
#ifdef MONITOR_DEBUG
			_mpc_lowcomm_monitor_wrap_print(query, "ROUTED QUERY");
#endif

			/* We need to route */
			mpc_lowcomm_monitor_retcode_t route_ret;
			if(_mpc_lowcomm_monitor_command_send(query, &route_ret) < 0)
			{
			}
		}
	}

	return NULL;
}


static inline void __start_client_loop(_mpc_lowcomm_client_ctx_t *ctx)
{
	_mpc_lowcomm_kernel_thread_create(NULL, __per_client_loop, (void*)ctx);
}



static void *__server_loop(void *pmonitor)
{
	struct _mpc_lowcomm_monitor_s *monitor = (struct _mpc_lowcomm_monitor_s *)pmonitor;

	while(1)
	{

		_mpc_lowcomm_client_ctx_t *new_ctx = NULL;
		/* Accept incoming connection */
		if( (new_ctx = __accept_incoming(monitor) ) != NULL)
		{
			/* new client */
			__start_client_loop(new_ctx);
		}
		else
		{
			/* Was refused or error */
			if(!__monitor_running)
			{
				break;
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
		return MPC_LOWCOMM_MONITOR_RET_ERROR;
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

		/* All done */
		break;
	}

	freeaddrinfo(res);

	if(monitor->server_socket < 0)
	{
		/* We failed creating the monitor server */
		mpc_common_debug_error("Failed binding monitor server socket");
		return MPC_LOWCOMM_MONITOR_RET_ERROR;
	}

	ret = listen(monitor->server_socket, 32);

	if(ret < 0)
	{
		perror("listen");
		mpc_common_debug_error("Failed listen on monitor server socket");
		return MPC_LOWCOMM_MONITOR_RET_ERROR;
	}

	/* If we are here the server is ready to start its thread
	 * but first lets setup our address info */
	int port = __get_port_from_socket(monitor->server_socket);

	if(port < 0)
	{
		mpc_common_debug_error("Failed retrieving port on monitor server socket");
		return MPC_LOWCOMM_MONITOR_RET_ERROR;
	}

	char hostname[512];
	ret = gethostname(hostname, 512);

	if(ret < 0)
	{
		mpc_common_debug_error("Failed retrieving hostname on monitor server socket");
		return MPC_LOWCOMM_MONITOR_RET_ERROR;
	}

    /* As the DNS does not take the load we resolve once
     * per server instead of once per client as we
     * do not want to break anything ! */
 
    char resolved_ip[256];
  
    if( mpc_common_resolve_local_ip_for_iface(resolved_ip, 256, "ib") < 0 )
    {
        /* Only use hostname and hope for the best */
        snprintf(resolved_ip, 256, "%s", hostname);
    }

    /* Make sure we do not publish loopback (who knows :-/) */
    if(!strcmp(resolved_ip, "127.0.0.1"))
    {
        snprintf(resolved_ip, 256, "%s", hostname);
    }

	ret = snprintf(monitor->monitor_uri, MPC_LOWCOMM_PEER_URI_SIZE, "%s:%d@%s", resolved_ip, port, hostname);

	if(MPC_LOWCOMM_PEER_URI_SIZE <= ret)
	{
		/* URI was mpc_common_debug_error */
		mpc_common_debug_fatal("Monitor server URI was too long to be stored");
		return MPC_LOWCOMM_MONITOR_RET_ERROR;
	}

	mpc_common_debug_info("MON: bound to %s", monitor->monitor_uri);

	return MPC_LOWCOMM_MONITOR_RET_SUCCESS;
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

}

mpc_lowcomm_monitor_retcode_t _mpc_lowcomm_monitor_init(struct _mpc_lowcomm_monitor_s *monitor)
{
	__monitor_worker_init();

	/* Clear sets */
	FD_ZERO(&monitor->read_fds);


	monitor->process_set   = NULL;
	monitor->process_uid = 0;

	/* Prepare client list */
	pthread_mutex_init(&monitor->client_lock, NULL);
	mpc_common_spinlock_init(&monitor->connect_accept_lock, 0);
	monitor->client_count = 0;
	mpc_common_hashtable_init(&monitor->client_contexts, MPC_LOWCOMM_MONITOR_MAX_CLIENTS);

	/* First step listen on 0.0.0.0 */
	monitor->server_socket = -1;

	mpc_lowcomm_monitor_retcode_t ret = __start_server_socket(monitor);

	if(ret != MPC_LOWCOMM_MONITOR_RET_SUCCESS)
	{
		mpc_common_debug_fatal("Failed to start the cplane monitor server");
	}

	__monitor_running = 1;

	/* Now start the server thread */
	int rc = _mpc_lowcomm_kernel_thread_create(&monitor->server_thread, __server_loop, (void *)monitor);

	if(rc < 0)
	{
		perror("pthread_create");
		mpc_common_debug_fatal("Failed creating cplane server threads");
	}

	__monitor_publish_over_pmi(monitor);

	return MPC_LOWCOMM_MONITOR_RET_SUCCESS;
}

mpc_lowcomm_monitor_retcode_t _mpc_lowcomm_monitor_release(struct _mpc_lowcomm_monitor_s *monitor)
{
	__monitor_worker_release();


	__monitor_running = 0;


	/* Disconnect all */
	_mpc_lowcomm_monitor_disconnect(monitor);

	shutdown(monitor->server_socket, SHUT_RDWR);
	close(monitor->server_socket);

	_mpc_lowcomm_kernel_thread_join(&monitor->server_thread);


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

static inline _mpc_lowcomm_client_ctx_t *___connect_client(struct _mpc_lowcomm_monitor_s *monitor,
                                                           char *uri,
                                                           uint64_t uid,
                                                           mpc_lowcomm_monitor_retcode_t *retcode)
{
	if(strstr(uri, "pmi://") )
	{
		mpc_common_debug_error("Cannot connect to an URI not resolved through PMI");
		return NULL;
	}


	//char bx[128];
	//mpc_common_debug_error("[OUT] %s CONNECTING to %s", mpc_lowcomm_peer_format_r(mpc_lowcomm_monitor_get_uid(), bx, 128), mpc_lowcomm_peer_format(uid));


	char localuri[MPC_LOWCOMM_PEER_URI_SIZE];
	snprintf(localuri, MPC_LOWCOMM_PEER_URI_SIZE, "%s", uri);

	mpc_common_debug("(%u, %u) connecting to (%u, %u)", mpc_lowcomm_peer_get_set(monitor->process_uid),
	                         mpc_lowcomm_peer_get_rank(monitor->process_uid), mpc_lowcomm_peer_get_set(uid), mpc_lowcomm_peer_get_rank(uid) );

	/*mpc_common_debug_warning("%lu -> %lu", monitor->process_uid, uid);
	 * mpc_common_debug_warning("%lu [label='(%u, %u)']",monitor->process_uid , mpc_lowcomm_peer_get_set(monitor->process_uid),
	 *                       mpc_lowcomm_peer_get_rank(monitor->process_uid));*/

	char *ip = NULL;
	char *port = NULL;
	char *hostname = NULL;

	char *sep = strchr(localuri, ':');

	if(!sep)
	{
		mpc_common_debug_error("Bad peer URI %s", uri);
		*retcode = MPC_LOWCOMM_MONITOR_RET_ERROR;
		return NULL;
	}

	char *sep2 = strchr(localuri, '@');

	if(!sep2)
	{
		mpc_common_debug_error("Bad peer URI %s", uri);
		*retcode = MPC_LOWCOMM_MONITOR_RET_ERROR;
		return NULL;
	}

	*sep     = '\0';
	*sep2 = '\0';
	ip = localuri;
	port     = sep + 1;
	hostname = sep2 + 1;

	int len = strlen(port);
	int i;

	for(i = 0; i < len; i++)
	{
		if(!isdigit(port[i]) )
		{
			mpc_common_debug_error("%s is not a numeric port", port);
			*retcode = MPC_LOWCOMM_MONITOR_RET_ERROR;
			return NULL;
		}
	}

	char * trials[2] = { ip , hostname };

	int client_socket    = -1;

	for( i = 0 ; i < 2 ; i++)
	{
		/* Now time to connect */

		struct addrinfo *res = NULL;
		struct addrinfo  hints;

		memset(&hints, 0, sizeof(struct addrinfo) );

		hints.ai_family   = AF_UNSPEC;
		hints.ai_socktype = SOCK_STREAM;

		mpc_common_debug_info("MON: Trying to connect to %s : %s", trials[i], port);

		/* Note we bind to port 0 as we do not care much of where we are */
		int ret = mpc_common_getaddrinfo(trials[i], port, &hints, &res, "ib");

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


			*retcode = MPC_LOWCOMM_MONITOR_RET_NOT_REACHABLE;
			return NULL;
		}

		struct addrinfo *tmp = NULL;

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


            /* Set a lower connect timeout ~3 sec */
            int synRetries = 1;
            setsockopt(client_socket, IPPROTO_TCP, TCP_SYNCNT, &synRetries, sizeof(int));

			if(connect(client_socket, tmp->ai_addr, tmp->ai_addrlen) < 0)
			{
				close(client_socket);
				client_socket = -1;
				continue;
			}


			mpc_common_debug_info("MON: DONE trying to connect to %s : %s == %d", trials[i], port, client_socket);
			/* all done */
			break;
		}

		mpc_common_freeaddrinfo(res);

		if(0 < client_socket)
		{
			/* Stop if option worked */
			break;
		}
	}

	if(client_socket < 0)
	{
		mpc_common_debug("MON: FAILED to connect to %s / %s : %s", trials[0], trials[1], port);
		*retcode = MPC_LOWCOMM_MONITOR_RET_NOT_REACHABLE;
		return NULL;
	}

    int flag = 1;
    setsockopt(client_socket, IPPROTO_TCP, TCP_NODELAY, (char *) &flag, sizeof(int));

	/* If we have a socket the first step is to write our UID */
	mpc_common_io_safe_write(client_socket, &__monitor.process_uid, sizeof(uint64_t) );

	/* If we are here we can create the client */
	_mpc_lowcomm_client_ctx_t *new = _mpc_lowcomm_client_ctx_new(uid, client_socket, monitor);

	__start_client_loop(new);

	/* And we register it */
	_mpc_lowcomm_monitor_client_add(monitor, new);


	*retcode = MPC_LOWCOMM_MONITOR_RET_SUCCESS;

	return new;
}

static inline _mpc_lowcomm_client_ctx_t *__connect_client(struct _mpc_lowcomm_monitor_s *monitor,
                                                          char *uri,
                                                          uint64_t uid,
                                                          mpc_lowcomm_monitor_retcode_t *retcode)
{
	mpc_common_spinlock_lock_yield(&monitor->connect_accept_lock);

	_mpc_lowcomm_client_ctx_t * ret = _mpc_lowcomm_monitor_client_known(monitor, uid);

	if(!ret)
	{
		ret =___connect_client(monitor, uri, uid, retcode);
	}

	mpc_common_spinlock_unlock(&monitor->connect_accept_lock);

	return ret;
}

static inline _mpc_lowcomm_client_ctx_t *__get_client(struct _mpc_lowcomm_monitor_s *monitor,
                                                      mpc_lowcomm_peer_uid_t client_uid,
                                                      mpc_lowcomm_monitor_retcode_t *retcode)
{
	_mpc_lowcomm_peer_t *peer = _mpc_lowcomm_peer_get(client_uid);

	if(!peer)
	{
		*retcode = MPC_LOWCOMM_MONITOR_RET_INVALID_UID;
		return NULL;
	}

	if(strstr(peer->infos.uri, "pmi://") )
	{
		/* We need to PMI resolve */
		char *key = peer->infos.uri + 6;
		char  value[MPC_LOWCOMM_PEER_URI_SIZE];
		if(mpc_launch_pmi_get(value, MPC_LOWCOMM_PEER_URI_SIZE, key, mpc_lowcomm_peer_get_rank(peer->infos.uid)) != MPC_LAUNCH_PMI_SUCCESS)
		{
			mpc_common_debug_fatal("Failed retrieving remote monitor key %s", key);
		}

		mpc_common_debug("Resolved %s to %s", peer->infos.uri, value);
		snprintf(peer->infos.uri, MPC_LOWCOMM_PEER_URI_SIZE, "%s", value);
	}


	/* Now time to connect */
	_mpc_lowcomm_client_ctx_t *ctx = __connect_client(monitor, peer->infos.uri, client_uid, retcode);

	if(ctx != NULL)
	{
		/* All done seems OK */
		*retcode = MPC_LOWCOMM_MONITOR_RET_SUCCESS;
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

	int has_routes = 0;
	mpc_lowcomm_peer_uid_t current_route_uid = 0;

	/* Only do distance routing in same set */
	if(set_uid == mpc_lowcomm_monitor_get_gid())
	{

		pthread_mutex_lock(&monitor->client_lock);

		MPC_HT_ITER(&monitor->client_contexts, ctx)
		{
			//mpc_common_debug_error("CTX == %p", ctx);
			if(ctx)
			{
				if(mpc_lowcomm_peer_closer(dest, current_route_uid, ctx->uid) )
				{
					/* We have one pick it if closer */
					current_route_uid = ctx->uid;
					ellected_ctx      = ctx;
					has_routes = 1;
				}
			}
		}
		MPC_HT_ITER_END(&monitor->client_contexts)

		pthread_mutex_unlock(&monitor->client_lock);

		/* Not connected yet or disconnected */
		if(!has_routes)
		{
			/* Peer is in my set do direct (case where all connections are not set or closed) */
			ellected_ctx = _mpc_lowcomm_monitor_get_client(monitor, dest, _MPC_LOWCOMM_MONITOR_GET_CLIENT_DIRECT, retcode);
		}

	}


	/* If we are here we must be from another set to we send to root rank if we are not root
	   and if we are root we connect to remote root */
	if(!ellected_ctx)
	{
		if(mpc_lowcomm_peer_get_rank(mpc_lowcomm_monitor_get_uid()) == 0)
		{
			mpc_lowcomm_peer_uid_t remote_master_s_uid = mpc_lowcomm_monitor_uid_of(set_uid, 0);
			return _mpc_lowcomm_monitor_get_client(monitor, remote_master_s_uid, _MPC_LOWCOMM_MONITOR_GET_CLIENT_DIRECT, retcode);
		}
		else
		{
			/* If I'm not root lets route to it */

			/* Here as we are a subprocess we route to 0 which should already be connected */
			mpc_lowcomm_peer_uid_t master_s_uid = mpc_lowcomm_monitor_uid_of(mpc_lowcomm_monitor_get_gid(), 0);
			return _mpc_lowcomm_monitor_get_client(monitor, master_s_uid, _MPC_LOWCOMM_MONITOR_GET_CLIENT_CAN_ROUTE, retcode);
		}
	}

	if(ellected_ctx != NULL)
	{
		*retcode = MPC_LOWCOMM_MONITOR_RET_SUCCESS;
		/* We found a set member to route to */
		return ellected_ctx;
	}

	//mpc_common_debug_error("NO route");

	return NULL;
}

static inline _mpc_lowcomm_client_ctx_t *__route_client(struct _mpc_lowcomm_monitor_s *monitor,
                                                        mpc_lowcomm_peer_uid_t client_uid,
                                                        mpc_lowcomm_monitor_retcode_t *retcode)
{

	/* First lets check if the set UID is possibly known */
	mpc_lowcomm_set_uid_t set_uid = mpc_lowcomm_peer_get_set(client_uid);

	/* For scalability only lead inquires for sets */
	if(mpc_common_get_process_rank() == 0)
	{
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
			*retcode = MPC_LOWCOMM_MONITOR_RET_INVALID_UID;
			return NULL;
		}

		/* If we have a set make sure the target UID is known */
		if(!_mpc_lowcomm_set_contains(tset, client_uid) )
		{
			*retcode = MPC_LOWCOMM_MONITOR_RET_INVALID_UID;
			return NULL;
		}
	}


	/* Now we are sure destination is valid now return the closest next
	 * hop we know about */
	_mpc_lowcomm_client_ctx_t *ret = __get_closest_in_set(monitor, client_uid, set_uid, retcode);

	if(ret)
	{
		//mpc_common_debug_warning("ROUTE [%lu to %lu through %lu]", mpc_lowcomm_monitor_get_uid(), client_uid, ret->uid);
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
		*retcode = MPC_LOWCOMM_MONITOR_RET_SUCCESS;
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
	uint64_t to_delete = 0;

	do
	{
		did_delete = 0;
		MPC_HT_ITER(&monitor->client_contexts, client)
		{
			if(client)
			{
				did_delete = 1;

				MPC_HT_ITER_BREAK(&monitor->client_contexts);
			}
		}
		MPC_HT_ITER_END(&monitor->client_contexts)

		if(did_delete)
		{
			mpc_common_hashtable_delete(&monitor->client_contexts, client->uid);
			_mpc_lowcomm_client_ctx_release(&client);
		}

	} while(did_delete);

	return MPC_LOWCOMM_MONITOR_RET_SUCCESS;
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
	static mpc_common_spinlock_t __command_index_lock = MPC_COMMON_SPINLOCK_INITIALIZER;

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
	//_mpc_lowcomm_monitor_wrap_print(cmd, "SENDING");

	cmd->ttl--;

	if(cmd->ttl < 0)
	{
		mpc_common_debug_error("Message's TTL reached 0 there probably was a routing problem");
		return 1;
	}

	if(cmd->dest == mpc_lowcomm_monitor_get_uid())
	{
		*ret = MPC_LOWCOMM_MONITOR_RET_SUCCESS;
		return __handle_query(NULL, cmd);
	}

	/* First of all get the peer */
	_mpc_lowcomm_client_ctx_t *peer_info = _mpc_lowcomm_monitor_get_client(&__monitor,
	                                                                       cmd->dest,
	                                                                       _MPC_LOWCOMM_MONITOR_GET_CLIENT_CAN_ROUTE,
	                                                                       ret);

	if(!peer_info)
	{
		return -1;
	}
	else
	{
		mpc_common_nodebug("SENDING through %s", mpc_lowcomm_peer_format(peer_info->uid));
	}

	_mpc_lowcomm_client_ctx_send(peer_info, cmd);

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

int mpc_lowcomm_monitor_peer_exists(mpc_lowcomm_peer_uid_t peer)
{
	return (_mpc_lowcomm_peer_get(peer) != NULL);
}

/**************************
 * COMMON WRAP GENERATION *
 **************************/

static inline _mpc_lowcomm_peer_t * __gen_wrap_for_peer_sized(mpc_lowcomm_peer_uid_t dest,
															  mpc_lowcomm_monitor_command_t cmd,
															  size_t extra_payload_size,
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
											sizeof(mpc_lowcomm_monitor_args_t) + extra_payload_size);

	return rpeer;
}


static inline _mpc_lowcomm_peer_t * __gen_wrap_for_peer(mpc_lowcomm_peer_uid_t dest,
													    mpc_lowcomm_monitor_command_t cmd,
														int is_response,
														_mpc_lowcomm_monitor_wrap_t **outcmd)
{
	return __gen_wrap_for_peer_sized( dest, cmd, 0, is_response, outcmd);
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
	_mpc_lowcomm_monitor_wrap_t *cmd = _mpc_lowcomm_monitor_wrap_new(MPC_LOWCOMM_MONITOR_COMMAND_REQUEST_SET_INFO,
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
		cmd->content->set_info.retcode          = MPC_LOWCOMM_MONITOR_RET_INVALID_UID;
		cmd->content->set_info.total_task_count = 0;
	}
	else
	{
		cmd->content->set_info.retcode = MPC_LOWCOMM_MONITOR_RET_SUCCESS;
		snprintf(cmd->content->set_info.name, MPC_LOWCOMM_SET_NAME_LEN, "%s", set->name);
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
	return __generate_set_decription(dest, __monitor.process_set->gid, 0);
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
	                                                                                       5);


	if(!resp)
	{
        *ret = MPC_LOWCOMM_MONITOR_RET_NOT_REACHABLE;
		mpc_common_debug_warning("mpc_lowcomm_monitor_get_set_info timed out when targetting %llu", target_peer);
	}
	else
	{
		if(resp->content->set_info.retcode == MPC_LOWCOMM_MONITOR_RET_SUCCESS)
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
	_mpc_lowcomm_peer_t * rpeer = __gen_wrap_for_peer(dest, MPC_LOWCOMM_MONITOR_PING, is_response, &cmd);

	if(!rpeer)
	{
		cmd->content->ping.retcode = MPC_LOWCOMM_MONITOR_RET_INVALID_UID;
	}

	cmd->content->ping.ping = time(NULL);

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
        *ret = MPC_LOWCOMM_MONITOR_RET_NOT_REACHABLE;
		mpc_common_debug_warning("_mpc_lowcomm_monitor_ping timed out when targetting %llu", dest);
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



/******************
 * NAMING COMMAND *
 ******************/

static inline _mpc_lowcomm_monitor_wrap_t * __generate_naming_cmd(mpc_lowcomm_peer_uid_t dest, size_t extra_size, int is_response)
{
	_mpc_lowcomm_monitor_wrap_t * cmd = NULL;
	_mpc_lowcomm_peer_t * rpeer = __gen_wrap_for_peer(dest, MPC_LOWCOMM_MONITOR_NAMING, is_response, &cmd);

	if(!rpeer)
	{
		cmd->content->ping.retcode = MPC_LOWCOMM_MONITOR_RET_INVALID_UID;
	}

	return cmd;
}


mpc_lowcomm_monitor_response_t mpc_lowcomm_monitor_naming(mpc_lowcomm_peer_uid_t dest,
														  mpc_lowcomm_monitor_command_naming_t operation,
														  mpc_lowcomm_peer_uid_t hosting_peer,
														  const char * name,
														  const char * port_name,
														  mpc_lowcomm_monitor_retcode_t *ret)
{
	_mpc_lowcomm_monitor_wrap_t * cmd = __generate_naming_cmd(dest, 0, 0);

	cmd->content->naming.operation = operation;
	cmd->content->naming.hosting_peer = hosting_peer;
	snprintf(cmd->content->naming.name, MPC_LOWCOMM_ONDEMAND_TARGET_LEN, "%s", name);
	snprintf(cmd->content->naming.port_name, MPC_LOWCOMM_ONDEMAND_TARGET_LEN, "%s", port_name);
	cmd->content->naming.retcode = MPC_LOWCOMM_MONITOR_RET_SUCCESS;

	if(_mpc_lowcomm_monitor_command_send(cmd, ret) < 0)
	{
		_mpc_lowcomm_monitor_wrap_free(cmd);
		return NULL;
	}

	_mpc_lowcomm_monitor_wrap_t *resp = _mpc_lowcomm_monitor_command_engine_await_response(cmd->match_key,
	                                                                                       60);


	if(!resp)
	{
        *ret = MPC_LOWCOMM_MONITOR_RET_NOT_REACHABLE;
		mpc_common_debug_warning("mpc_lowcomm_monitor_naming timed out when targetting %llu", dest);
	}

	return (mpc_lowcomm_monitor_response_t)resp;
}

_mpc_lowcomm_monitor_wrap_t *_mpc_lowcomm_monitor_command_return_naming_info(mpc_lowcomm_peer_uid_t dest,
																		     mpc_lowcomm_monitor_args_t *content,
                                                                             uint64_t response_index)
{
	/* We need to handle name list first as it may need more space */
	char *name_csv_list = NULL;
	size_t extra_size = 0;

	if(content->naming.operation == MPC_LOWCOMM_MONITOR_NAMING_LIST)
	{
		name_csv_list = _mpc_lowcomm_monitor_name_get_csv_list();

		if(name_csv_list)
		{
			extra_size += strlen(name_csv_list) + 1;
		}
		else
		{
			name_csv_list = "";
			extra_size = 1;
		}
	}

	/* This prepares a request for set info this contains no data */
	_mpc_lowcomm_monitor_wrap_t *resp = _mpc_lowcomm_monitor_wrap_new(MPC_LOWCOMM_MONITOR_NAMING,
																	  1,
																	  dest,
																	  __monitor.process_uid,
																	  response_index,
																	  sizeof(mpc_lowcomm_monitor_args_t) + extra_size);

	int error = MPC_LOWCOMM_MONITOR_RET_SUCCESS;

	switch (content->naming.operation)
	{
		case MPC_LOWCOMM_MONITOR_NAMING_PUT:
			if( _mpc_lowcomm_monitor_name_publish(content->naming.name, content->naming.port_name, content->naming.hosting_peer))
			{
				snprintf(resp->content->naming.name, MPC_LOWCOMM_ONDEMAND_TARGET_LEN, "%s", "Name already present");
				error = MPC_LOWCOMM_MONITOR_RET_DUPLICATE_KEY;
			}
		break;
		case MPC_LOWCOMM_MONITOR_NAMING_GET:
		{

			char * port = _mpc_lowcomm_monitor_name_get_port(content->naming.name, &resp->content->naming.hosting_peer);

			if(port)
			{
				snprintf(resp->content->naming.name, MPC_LOWCOMM_ONDEMAND_TARGET_LEN, "%s", content->naming.name);
				snprintf(resp->content->naming.port_name, MPC_LOWCOMM_ONDEMAND_TARGET_LEN, "%s", port);
			}
			else
			{
				snprintf(resp->content->naming.name, MPC_LOWCOMM_ONDEMAND_TARGET_LEN, "%s", "No such name");
				error = MPC_LOWCOMM_MONITOR_RET_ERROR;
			}

			break;
		}
		case MPC_LOWCOMM_MONITOR_NAMING_DEL:
			if(_mpc_lowcomm_monitor_name_unpublish(content->naming.name))
			{
				snprintf(resp->content->naming.name, MPC_LOWCOMM_ONDEMAND_TARGET_LEN, "%s", "No such name");
				error = MPC_LOWCOMM_MONITOR_RET_ERROR;
			}
		break;
		case MPC_LOWCOMM_MONITOR_NAMING_LIST:
			assume(name_csv_list);
			memcpy(resp->content->naming.name_list, name_csv_list, strlen(name_csv_list) + 1);
			_mpc_lowcomm_monitor_name_free_cvs_list(name_csv_list);
			break;
		default:
			error = 1;
	}

	if(error != MPC_LOWCOMM_MONITOR_RET_SUCCESS)
	{
		resp->content->naming.retcode = error;
	}
	else
	{
		resp->content->naming.retcode = MPC_LOWCOMM_MONITOR_RET_SUCCESS;
	}

	return resp;
}



/*********************
 * COMM INFO COMMAND *
 *********************/

static inline _mpc_lowcomm_monitor_wrap_t * __generate_comm_info_cmd(mpc_lowcomm_peer_uid_t dest,
																	 mpc_lowcomm_communicator_id_t target_id,
																	 uint64_t size,
																	 int is_response)
{
	_mpc_lowcomm_monitor_wrap_t * cmd = NULL;
	_mpc_lowcomm_peer_t * rpeer = __gen_wrap_for_peer(dest, MPC_LOWCOMM_MONITOR_COMM_INFO, is_response, &cmd);

	/* We used uin64_t to avoid header mess make sure it matches */
	assume(sizeof(mpc_lowcomm_communicator_id_t) == sizeof(uint64_t));

	if(!rpeer)
	{
		cmd->content->comm_info.retcode = MPC_LOWCOMM_MONITOR_RET_INVALID_UID;
	}

	cmd->content->comm_info.id = target_id;
	cmd->content->comm_info.size = size;
	cmd->content->comm_info.retcode = MPC_LOWCOMM_MONITOR_RET_SUCCESS;

	return cmd;
}


mpc_lowcomm_monitor_response_t mpc_lowcomm_monitor_comm_info(mpc_lowcomm_peer_uid_t dest,
															 uint64_t target_id,
															 mpc_lowcomm_monitor_retcode_t *ret)
{
	_mpc_lowcomm_monitor_wrap_t * cmd = __generate_comm_info_cmd(dest, target_id, 0, 0);

	if(_mpc_lowcomm_monitor_command_send(cmd, ret) < 0)
	{
		_mpc_lowcomm_monitor_wrap_free(cmd);
		return NULL;
	}

	_mpc_lowcomm_monitor_wrap_t *resp = _mpc_lowcomm_monitor_command_engine_await_response(cmd->match_key,
	                                                                                       60);


	if(!resp)
	{
        *ret = MPC_LOWCOMM_MONITOR_RET_NOT_REACHABLE;
		mpc_common_debug_warning("mpc_lowcomm_monitor_comm_info timed out when targetting %llu", dest);
	}

	return (mpc_lowcomm_monitor_response_t)resp;
}

_mpc_lowcomm_monitor_wrap_t *_mpc_lowcomm_monitor_command_return_comm_info(mpc_lowcomm_peer_uid_t dest,
																		   mpc_lowcomm_monitor_args_t *content,
                                                                           uint64_t response_index)
{
	mpc_lowcomm_communicator_t comm = mpc_lowcomm_get_communicator_from_id(content->comm_info.id);

	int comm_size = 0;

	if(comm)
	{
		comm_size = mpc_lowcomm_communicator_size(comm);
	}

	_mpc_lowcomm_monitor_wrap_t * resp = _mpc_lowcomm_monitor_wrap_new(MPC_LOWCOMM_MONITOR_COMM_INFO,
																	1,
																	dest,
																	__monitor.process_uid,
																	response_index,
																	sizeof(mpc_lowcomm_monitor_args_t)
																	+ (comm_size * sizeof(mpc_lowcomm_monitor_rank_desc_t)));

	if(comm)
	{
		/* We need more time to think of what it means */
		assume(!mpc_lowcomm_communicator_is_intercomm(comm));
		resp->content->comm_info.id = content->comm_info.id;
		resp->content->comm_info.size = comm_size;
		resp->content->comm_info.retcode = MPC_LOWCOMM_MONITOR_RET_SUCCESS;

		int i;

		for(i = 0 ; i < comm_size; i++)
		{
			resp->content->comm_info.rds[i].uid = mpc_lowcomm_communicator_uid(comm, i);
			resp->content->comm_info.rds[i].comm_world_rank = mpc_lowcomm_communicator_world_rank_of(comm, i);
		}

	}
	else
	{
		resp->content->comm_info.retcode = MPC_LOWCOMM_MONITOR_RET_INVALID_UID;
	}

	return resp;
}

/**********************
 * ON DEMMAND COMMAND *
 **********************/

static inline _mpc_lowcomm_monitor_wrap_t * __generate_ondemand_cmd(mpc_lowcomm_peer_uid_t dest,
																	char *target,
																	char * data,
																	size_t data_size,
																	int is_response)
{
	_mpc_lowcomm_monitor_wrap_t * cmd = NULL;
	_mpc_lowcomm_peer_t * rpeer = __gen_wrap_for_peer_sized(dest, MPC_LOWCOMM_MONITOR_ON_DEMAND, data_size, is_response, &cmd);

	if(!rpeer)
	{
		/* Peer is not known locally inform of possibly invalid UID */
		cmd->content->on_demand.retcode = MPC_LOWCOMM_MONITOR_RET_INVALID_UID;
	}

	snprintf(cmd->content->on_demand.target, MPC_LOWCOMM_ONDEMAND_TARGET_LEN, "%s", target);
	
	if(data != NULL)
	{
		memcpy(cmd->content->on_demand.data, data, data_size);
	}

	return cmd;
}


mpc_lowcomm_monitor_response_t mpc_lowcomm_monitor_ondemand(mpc_lowcomm_peer_uid_t dest,
															char *target,
															char *data,
															size_t data_size,
															mpc_lowcomm_monitor_retcode_t *ret)
{
	assume( mpc_lowcomm_peer_get_set(dest) != 0);

	_mpc_lowcomm_monitor_wrap_t * cmd = __generate_ondemand_cmd(dest, target, data, data_size, 0);

#ifdef MONITOR_DEBUG
	mpc_common_debug_error("Sending OD on CPLANE for net %s for %s (IDX: %llu)", target, mpc_lowcomm_peer_format(dest), cmd->match_key);
#endif

	if(_mpc_lowcomm_monitor_command_send(cmd, ret) < 0)
	{
		_mpc_lowcomm_monitor_wrap_free(cmd);
        *ret = MPC_LOWCOMM_MONITOR_RET_NOT_REACHABLE;
		mpc_common_debug_error("Failed to send OD on cplane");
		return NULL;
	}


	_mpc_lowcomm_monitor_wrap_t *resp = _mpc_lowcomm_monitor_command_engine_await_response(cmd->match_key,
	                                                                                       60);


	if(!resp)
	{
        //*ret = MPC_LOWCOMM_MONITOR_RET_NOT_REACHABLE;
		mpc_common_debug_warning("_mpc_lowcomm_monitor_ondemand timed out when targetting %llu", dest);
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

	_mpc_lowcomm_monitor_wrap_t *resp = __generate_ondemand_cmd(dest, ondemand->on_demand.target, NULL, MPC_LOWCOMM_ONDEMAND_DATA_LEN, 1);
	resp->match_key = response_index;

	if(!cb)
	{
		mpc_common_debug_error("No CB for %s (IDX %llu)", ondemand->on_demand.target, response_index);
		resp->content->on_demand.retcode = MPC_LOWCOMM_MONITOR_RET_ERROR;
	}
	else
	{
		int ret = (cb)(dest, ondemand->on_demand.data, resp->content->on_demand.data, MPC_LOWCOMM_ONDEMAND_DATA_LEN, ctx);


		if(ret != 0)
		{
			resp->content->on_demand.retcode = MPC_LOWCOMM_MONITOR_RET_ERROR;
		}
		else
		{
			resp->content->on_demand.retcode = MPC_LOWCOMM_MONITOR_RET_SUCCESS;
		}
	}

	return resp;
}

/*************************
 * GET CONNECTIVITY INFO *
 *************************/

static inline _mpc_lowcomm_monitor_wrap_t * __generate_connectivity_cmd(mpc_lowcomm_peer_uid_t dest, int num_peers,  int is_response)
{
    _mpc_lowcomm_peer_t *rpeer = _mpc_lowcomm_peer_get(dest);

	/* If response this is set in the calling function */
	uint64_t response_index = 0;

	if(!is_response)
	{
		response_index = __get_new_command_index();
	}

	/* This prepares a request for set info this contains no data */
	_mpc_lowcomm_monitor_wrap_t * cmd = _mpc_lowcomm_monitor_wrap_new(MPC_LOWCOMM_MONITOR_CONNECTIVITY,
										                              is_response,
									                                  dest,
									                                  __monitor.process_uid,
									                                  response_index,
									                                  sizeof(mpc_lowcomm_monitor_args_t) + num_peers * sizeof(mpc_lowcomm_peer_uid_t));

	if(!rpeer)
	{
		cmd->content->on_demand.retcode = MPC_LOWCOMM_MONITOR_RET_INVALID_UID;
	}

	return cmd;
}

_mpc_lowcomm_monitor_wrap_t *_mpc_lowcomm_monitor_command_return_connectivity_info(mpc_lowcomm_peer_uid_t dest,
                                                                                  uint64_t response_index)
{
    size_t peer_count = 0;
	pthread_mutex_lock(&__monitor.client_lock);

	_mpc_lowcomm_client_ctx_t *ctx = NULL;

    {
	MPC_HT_ITER(&__monitor.client_contexts, ctx)
	{
		if(ctx)
		{
	    	peer_count++;
		}
	}
	MPC_HT_ITER_END(&__monitor.client_contexts)
    }

	pthread_mutex_unlock(&__monitor.client_lock);

	_mpc_lowcomm_monitor_wrap_t *resp = __generate_connectivity_cmd(dest, peer_count,  1);
	resp->match_key = response_index;

	resp->content->connectivity.peers_count = 0;

	pthread_mutex_lock(&__monitor.client_lock);

	MPC_HT_ITER(&__monitor.client_contexts, ctx)
	{
		if(ctx)
		{
			if( resp->content->connectivity.peers_count < peer_count )
			{
				resp->content->connectivity.peers[resp->content->connectivity.peers_count] = ctx->uid;
				resp->content->connectivity.peers_count++;
			}
		}
	}
	MPC_HT_ITER_END(&__monitor.client_contexts)

	pthread_mutex_unlock(&__monitor.client_lock);

	return resp;
}


mpc_lowcomm_monitor_response_t mpc_lowcomm_monitor_connectivity(mpc_lowcomm_peer_uid_t dest, mpc_lowcomm_monitor_retcode_t *ret)
{
	_mpc_lowcomm_monitor_wrap_t * cmd = __generate_connectivity_cmd(dest, 0,  0);

	if(_mpc_lowcomm_monitor_command_send(cmd, ret) < 0)
	{
		_mpc_lowcomm_monitor_wrap_free(cmd);
		return NULL;
	}

	_mpc_lowcomm_monitor_wrap_t *resp = _mpc_lowcomm_monitor_command_engine_await_response(cmd->match_key,
	                                                                                       60);

	if(!resp)
	{
		mpc_common_debug_warning("mpc_lowcomm_monitor_connectivity timed out when targetting %u", dest);
	}

	return (mpc_lowcomm_monitor_response_t)resp;
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
	_mpc_lowcomm_peer_t * rpeer = __gen_wrap_for_peer(dest, MPC_LOWCOMM_MONITOR_COMMAND_REQUEST_PEER_INFO, is_response, &cmd);

	if(!rpeer)
	{
		cmd->content->peer.retcode = MPC_LOWCOMM_MONITOR_RET_INVALID_UID;
	}

	cmd->content->peer.retcode = MPC_LOWCOMM_MONITOR_RET_SUCCESS;
	memcpy(&cmd->content->peer.info, &rpeer->infos, sizeof(mpc_lowcomm_monitor_peer_info_t) );

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
		if(resp->content->set_info.retcode == MPC_LOWCOMM_MONITOR_RET_SUCCESS)
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

        head = head->next;
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

	snprintf(new->target, MPC_LOWCOMM_ONDEMAND_TARGET_LEN, "%s", target);
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


/************************
 * PUBLIC SET INTERFACE *
 ************************/

int mpc_lowcomm_monitor_set_iterate(int (*callback)(mpc_lowcomm_monitor_set_t set, void * arg), void * arg)
{
	return _mpc_lowcomm_set_iterate(callback, arg);
}

mpc_lowcomm_monitor_set_t mpc_lowcomm_monitor_set_get(mpc_lowcomm_set_uid_t gid)
{
	return (mpc_lowcomm_monitor_set_t)_mpc_lowcomm_set_get(gid);
}

int mpc_lowcomm_monitor_set_contains(mpc_lowcomm_set_uid_t gid, mpc_lowcomm_peer_uid_t uid)
{
	_mpc_lowcomm_set_t * set = _mpc_lowcomm_set_get(gid);

	if(!set)
	{
		return 0;
	}

	return _mpc_lowcomm_set_contains(set, uid);
}


char * mpc_lowcomm_monitor_set_name(mpc_lowcomm_monitor_set_t pset)
{
	if(!pset)
	{
		return NULL;
	}

	_mpc_lowcomm_set_t * set = (_mpc_lowcomm_set_t*)pset;

	return set->name;
}


mpc_lowcomm_set_uid_t mpc_lowcomm_monitor_set_gid(mpc_lowcomm_monitor_set_t pset)
{
	if(!pset)
	{
		return 0;
	}

	_mpc_lowcomm_set_t * set = (_mpc_lowcomm_set_t*)pset;

	return set->gid;
}

uint64_t mpc_lowcomm_monitor_set_peer_count(mpc_lowcomm_monitor_set_t pset)
{
	if(!pset)
	{
		return 0;
	}

	_mpc_lowcomm_set_t * set = (_mpc_lowcomm_set_t*)pset;

	return set->peer_count;
}

int mpc_lowcomm_monitor_set_peers(mpc_lowcomm_monitor_set_t pset, mpc_lowcomm_monitor_peer_info_t * peers, uint64_t peers_len)
{
	if(!pset)
	{
		return 0;
	}

	_mpc_lowcomm_set_t * set = (_mpc_lowcomm_set_t*)pset;

	uint64_t i;

	for( i = 0 ; (i < set->peer_count) && (i < peers_len) ; i++)
	{
		memcpy( &peers[i], &set->peers[i]->infos, sizeof(mpc_lowcomm_monitor_peer_info_t));
	}

	return 0;
}

/*********************************
 * SYNCHRONOUS CONNECTIVITY DUMP *
 *********************************/


void mpc_lowcomm_monitor_synchronous_radix_dump(void)
{
	/* We need the network */
	int rank = mpc_common_get_task_rank();
	assume(0 <= rank);

	int size = mpc_common_get_task_count();


			mpc_lowcomm_peer_uid_t me = mpc_lowcomm_monitor_get_uid();
			pthread_mutex_lock(&__monitor.client_lock);

			_mpc_lowcomm_client_ctx_t *ctx = NULL;

			int cnt = 0;

			MPC_HT_ITER(&__monitor.client_contexts, ctx)
			{
				if(ctx)
				{
					cnt++;
				}
			}
			MPC_HT_ITER_END(&__monitor.client_contexts)

			pthread_mutex_unlock(&__monitor.client_lock);


			printf("%lu %d\n", me, cnt);
}


void mpc_lowcomm_monitor_synchronous_connectivity_dump(void)
{
	/* We need the network */
	int rank = mpc_common_get_task_rank();
	assume(0 <= rank);

	int size = mpc_common_get_task_count();

	int i;

	if(!rank)
	{
		printf("graph G\n{\ngraph[concentrate=true]\n");
	}

	for(i = 0 ; i < size; i++)
	{
		if(i == rank)
		{
			mpc_lowcomm_peer_uid_t me = mpc_lowcomm_monitor_get_uid();
			printf("%lu [label=\"%lu %s\"]\n", me, me, mpc_lowcomm_peer_format(me));
			pthread_mutex_lock(&__monitor.client_lock);

			_mpc_lowcomm_client_ctx_t *ctx = NULL;

			MPC_HT_ITER(&__monitor.client_contexts, ctx)
			{
				if(ctx)
				{
					printf("%lu -- %lu\n", mpc_lowcomm_monitor_get_uid(), ctx->uid);
				}
			}
			MPC_HT_ITER_END(&__monitor.client_contexts)

			pthread_mutex_unlock(&__monitor.client_lock);
		}

		mpc_lowcomm_barrier(MPC_COMM_WORLD);
	}


	mpc_lowcomm_barrier(MPC_COMM_WORLD);

	if(!rank)
	{
		printf("\n}\n");
	}

}
