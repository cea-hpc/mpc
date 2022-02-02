#ifndef LOWCOMM_THREAD_H
#define LOWCOMM_THREAD_H

#include <mpc_config.h>

#ifdef MPC_Threads
#include <mpc_thread.h>
#else
#include <pthread.h>
#endif

struct _mpc_lowcomm_kernel_thread_s
{
#ifdef MPC_Threads
	mpc_thread_t      th;
#else
	pthread_t th;
#endif
};



typedef struct _mpc_lowcomm_kernel_thread_s _mpc_lowcomm_kernel_thread_t;

int _mpc_lowcomm_kernel_thread_create(_mpc_lowcomm_kernel_thread_t *th,
                                     void * (*func)(void *),
                                     void * arg);

int _mpc_lowcomm_kernel_thread_join(_mpc_lowcomm_kernel_thread_t *th);

#endif /* LOWCOMM_THREAD_H */
