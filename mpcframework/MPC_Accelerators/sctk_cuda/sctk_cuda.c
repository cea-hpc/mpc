
#if defined(MPC_Accelerators) && defined(MPC_USE_CUDA)
#include <sctk_cuda.h>
#include <sctk_debug.h>
#include <sctk_device_topology.h>
#include <sctk_spinlock.h>
#include <sctk_alloc.h>
#include <sctk_topology.h>
static void sctk_accl_cuda_print_ctx_flag(){
	/*unsigned int currentFlag=7357;*/
	/*CUcontext currentContext;*/
	/*CUresult res;*/

	/*cuCtxGetFlags(&currentFlag);*/

	/*res=cuCtxGetCurrent(&currentContext);*/
	/*printf("sctk:currentContext address=%p - %d\n",&currentContext,res);*/
	/*fflush(stdout);*/

	/*switch(currentFlag){*/
		/*case CU_CTX_SCHED_SPIN:*/
			/*printf("sctk:CU_CTX_SCHED_SPIN\n");*/
			/*fflush(stdout);*/
			/*break;*/
		/*case CU_CTX_SCHED_YIELD:*/
			/*printf("sctk:CU_CTX_SCHED_YIELD\n");*/
			/*fflush(stdout);*/
			/*break;*/
		/*case CU_CTX_SCHED_BLOCKING_SYNC:*/
			/*printf("sctk:CU_CTX_SCHED_BLOCKING_SYNC\n");*/
			/*fflush(stdout);*/
			/*break;*/
		/*case CU_CTX_SCHED_AUTO:*/
			/*printf("sctk:CU_CTX_SCHED_AUTO\n");*/
			/*fflush(stdout);*/
			/*break;*/
		/*case CU_CTX_MAP_HOST:*/
			/*printf("sctk:CU_CTX_MAP_HOST\n");*/
			/*fflush(stdout);*/
			/*break;*/
		/*case CU_CTX_LMEM_RESIZE_TO_MAX:*/
			/*printf("sctk:CU_CTX_LMEM_RESIZE_TO_MAX\n");*/
			/*fflush(stdout);*/
			/*break;*/
		/*default:*/
			/*printf("sctk:NO VALID FLAG FOUND\n");*/
			/*fflush(stdout);*/
	/*}*/
}


int lock=SCTK_SPINLOCK_INITIALIZER;
int cpt=0;
static int sctk_accl_cuda_get_closest_device(int cpu_id){
	int num_devices = 0;

	safe_cudart(cudaGetDeviceCount(&num_devices));
	/* if CUDA support is loaded but the current configuration does not provide a GPU: stop */ 
	if(num_devices<=0) return 0;
	
	/* else, we try to find the closest device for the current thread */
	sctk_device_t* closest_device = NULL;
	int closest_device_cuda_id = -1;

	//TODO il faudrait aussi gere le cas ou il y a plusieurs gpu par noeud a egal
	//distance, et d'autre gpu sur un autre noeud a distance differente
	if(sctk_device_matrix_is_equidistant("card*"))
	{
		int count;
		sctk_device_t ** device_list = sctk_device_get_from_handle_regexp("card*",&count);

		sctk_spinlock_lock(&lock);
		closest_device_cuda_id=(cpt+count)%count;
		cpt++;
		sctk_spinlock_unlock(&lock);

		// printf("COUNT DEVICES=%d, CLOSEST_DEVICE_CUDA_ID=%d\n",count, closest_device_cuda_id);
		// fflush(stdout);

		sctk_free(device_list);
	}
	else
	{
		closest_device = sctk_device_matrix_get_closest_from_pu(sctk_get_cpu(),"card*");
		closest_device_cuda_id = closest_device->device_cuda_id;
	}
	sctk_debug("CUDA: (DETECTION) nearest device = %d\n", closest_device_cuda_id);
	return closest_device_cuda_id;
	//return (closest_device_cuda_id+1)%2;
}

/**
 * returns 0 if init was Ok, 1 otherwise
 */
int sctk_accl_cuda_init(void** ptls_cuda){
	int num_devices = 0;

	safe_cudart(cudaGetDeviceCount(&num_devices));

	/* if CUDA support is loaded but the current configuration does not provide a GPU: stop */ 
	if(num_devices<=0) return 1;
	
	/* Else, we init tls_cuda if not already done */
	tls_cuda_t* cuda=(tls_cuda_t*) *ptls_cuda;
	if(cuda == NULL)
	{
		cuInit(0);
		cuda = (tls_cuda_t*)sctk_malloc(sizeof(tls_cuda_t));
		cuda->is_ready = -1;
		cuda->cpu_id = -1;

		sctk_debug("CUDA: (MALLOC) is_ready=%d, cpu_id=%d, address=%p\n",cuda->is_ready,cuda->cpu_id,cuda);
	}

	if(cuda->is_ready==1)
	{
		cuda = (tls_cuda_t*) *ptls_cuda;

		/* TODO: Determine the flags to be forwarded */
		unsigned int flags = CU_CTX_SCHED_YIELD;

		CUdevice dev = sctk_accl_cuda_get_closest_device(cuda->cpu_id);

		safe_cudadv(cuCtxCreate (&cuda->context,flags,dev));

		sctk_debug("CUDA: (INIT) cuda->is_ready=%d, cpu_id=%d,tls_cpu_id=%d, dev=%d\n",
				cuda->is_ready,
				sctk_get_cpu(),
				cuda->cpu_id,
				dev);
	}

	/* Set the current pointer as default one for the current thread */
	*ptls_cuda = cuda;

	sctk_nodebug("init:cuda address=%p\n",cuda);
	sctk_nodebug("init:ptls_cuda address=%p\n",*ptls_cuda);

	return 0;
}

int sctk_accl_cuda_pop_context(void** ptls_cuda){
	int num_devices = 0;

	safe_cudart(cudaGetDeviceCount(&num_devices));
	/* if CUDA support is loaded but the current configuration does not provide a GPU: stop */ 
	if(num_devices>0) return 1;
	
	/* Else, we try to pop a ctx from the GPU queue */
	CUresult cuda_error;
	tls_cuda_t* cuda=NULL;

	/* Init at first touch */
	if(*ptls_cuda == NULL)
	{
		/* CUDA init error ! */
		if( sctk_accl_cuda_init(ptls_cuda) != 0)
		{
			return 1;
		};
	}
	cuda = (tls_cuda_t*) *ptls_cuda;

	if(cuda->is_ready==1)
	{
		int cuda_device_id;
		safe_cudart(cudaGetDevice(&cuda_device_id));
		cuda_error = safe_cudadv(cuCtxPopCurrent(&cuda->context));
		sctk_debug("CUDA: (POP) cuCtxPopCurrent=%d, cuda_device_id=%d\n", cuda_error, cuda_device_id);
	}

	return cuda_error;
}

int sctk_accl_cuda_push_context(void* tls_cuda){
	int num_devices = 0;

	safe_cudart(cudaGetDeviceCount(&num_devices));
	/* if CUDA support is loaded but the current configuration does not provide a GPU: stop */ 
	if(num_devices<=0) return 1;
	CUresult cuda_error;

	/* else, we try to push a ctx in GPU queue */
	tls_cuda_t* cuda=NULL;

	cuda = (tls_cuda_t*) tls_cuda;

	/* if first touch, we can't pop a ctx, as no ctx has been pushed ever from this thread */
	if(tls_cuda==NULL)
	{
		/* CUDA init error ! */
		return 1;
	}

	if(cuda->is_ready==1)
	{
		cuda_error = safe_cudadv(cuCtxPushCurrent(cuda->context));

		int cuda_device_id;
		safe_cudart(cudaGetDevice(&cuda_device_id));
		sctk_debug("CUDA (PUSH) cuCtxPushCurrent=%d, cuda->is_ready=%d, cuda_device_id=%d\n", cuda_error, cuda->is_ready, cuda_device_id);
	}

	return cuda_error;
}

#endif //MPC_Accelerators && USE_CUDA
