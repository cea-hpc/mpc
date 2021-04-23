#include "lowcomm_thread.h"

int _mpc_lowcomm_kernel_thread_create(_mpc_lowcomm_kernel_thread_t *th,
                                     void * (*func)(void *),
                                     void * arg)
{
    struct _mpc_lowcomm_kernel_thread_s dummy;

    if(!th)
    {
        th = &dummy;
    }

#ifdef MPC_Threads
	mpc_thread_attr_t attr;

	/* Launch the polling thread */
	mpc_thread_attr_init(&attr);
	mpc_thread_attr_setscope(&attr, SCTK_THREAD_SCOPE_SYSTEM);
	mpc_thread_core_thread_create(&th->th, &attr, (void * (*)(void *) )func, arg);
#else
	pthread_create(&th->th, NULL, (void * (*)(void *) )func, arg);
#endif	
	return 0;
}


int _mpc_lowcomm_kernel_thread_join(_mpc_lowcomm_kernel_thread_t *th)
{
#ifdef MPC_Threads
	mpc_thread_join(th->th, NULL);
#else
	pthread_join(th->th, NULL);
#endif

    return 0;
}
