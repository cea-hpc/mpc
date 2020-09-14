#ifndef MPC_LOWCOMM_AM_INTERNAL_H_
#define MPC_LOWCOMM_AM_INTERNAL_H_

#include <mpc_config.h>
#include <mpc_lowcomm_am.h>

#ifdef MPC_Threads
/* This is needed to rewrite pthread primitives
   to MPC ones when MPC_Threads is enabled */
#include <mpc_pthread.h>
#else
	#include <pthread.h>
#endif

#define MPC_LOWCOMM_AM_MAX_FN_LEN    32
#define MPC_LOWCOMM_AM_MAX_SEND_CNT     8

struct mpc_lowcomm_am_ctx_s
{
    pthread_t acc_progress_thread;
    mpc_lowcomm_communicator_t active_message_comm;
};

#endif /* MPC_LOWCOMM_AM_INTERNAL_H_ */
