#if defined(MPC_Accelerators) && defined(MPC_USE_CUDA)
#include <sctk_accelerators.h>
/*#include <sctk_cuda.h>*/
#include <sctk_debug.h>
#include <sctk_device_topology.h>
#include <sctk_spinlock.h>
#include <sctk_alloc.h>
#include <sctk_topology.h>

extern __thread void* tls_cuda;

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
	int num_devices = sctk_accl_get_nb_devices(), nb_check;

	/* if CUDA support is loaded but the current configuration does not provide a GPU: stop */ 
	if(num_devices<=0) return 0;
	
	safe_cudart(cudaGetDeviceCount(&nb_check));
	assert(num_devices == nb_check);
	
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

		sctk_free(device_list);
	}
	else
	{
		closest_device = sctk_device_matrix_get_closest_from_pu(sctk_get_cpu(),"card*");
		closest_device_cuda_id = closest_device->device_cuda_id;
	}
	sctk_nodebug("CUDA: (DETECTION) nearest device = %d", closest_device_cuda_id);
	return closest_device_cuda_id;
	//return (closest_device_cuda_id+1)%2;
}

/**
 * returns 0 if init was Ok, 1 otherwise
 */
int sctk_accl_cuda_init(){
	int num_devices = sctk_accl_get_nb_devices(), check_nb;

	/* if CUDA support is loaded but the current configuration does not provide a GPU: stop */ 
	if(num_devices<=0)
	{
		sctk_debug("CUDA: No GPU found ! No init !");
		return 1;
	}

	/* optional: sanity check */
	safe_cudart(cudaGetDeviceCount(&check_nb));
	assert(check_nb == num_devices);

	/* we init tls_cuda if not already done */
	tls_cuda_t* cuda=(tls_cuda_t*) tls_cuda;
	cuda = (tls_cuda_t*)sctk_malloc(sizeof(tls_cuda_t));
	cuda->pushed = 0;
	cuda->cpu_id = sctk_get_cpu();

	sctk_nodebug("CUDA: (MALLOC) pushed?%d, cpu_id=%d, address=%p",cuda->pushed,cuda->cpu_id,cuda);
	/* TODO: Determine the flags to be forwarded */
	unsigned int flags = CU_CTX_SCHED_AUTO;

	CUdevice dev = sctk_accl_cuda_get_closest_device(cuda->cpu_id);

	safe_cudadv(cuCtxCreate (&cuda->context,flags,dev));
	cuda->pushed = 1;
	sctk_nodebug("CUDA: (INIT) cpu_id=%d,tls_cpu_id=%d, dev=%d",
			sctk_get_cpu(),
			cuda->cpu_id,
			dev);

	/* Set the current pointer as default one for the current thread */
	tls_cuda = cuda;

	sctk_nodebug("init:cuda address=%p",cuda);
	sctk_nodebug("init:ptls_cuda address=%p",&tls_cuda);

	return 0;
}

/**
 * Popping a context means we request CUDA to detach the current ctx from the
 * calling thread.
 *
 * This is done when saving the thread, before yielding it with another one (save())
 */
int sctk_accl_cuda_pop_context(){
	int num_devices = sctk_accl_get_nb_devices(), check_nb;

	/* if CUDA support is loaded but the current configuration does not provide a GPU: stop */ 
	if(num_devices<=0) return 1;

	/* sanity check */
	safe_cudart(cudaGetDeviceCount(&check_nb));
	assert(num_devices == check_nb);
	
	/* we try to pop a ctx from the GPU queue */
	tls_cuda_t* cuda= (tls_cuda_t*)tls_cuda;

	/* Init at first touch */
	if(cuda == NULL)
	{
		/* This ctx does not handles a CUDA ctx */
		return 1;
	}
	cuda = (tls_cuda_t*) tls_cuda;

	if(cuda->pushed)
	{
		int cuda_device_id;
		safe_cudart(cudaGetDevice(&cuda_device_id));
		safe_cudadv(cuCtxPopCurrent(&cuda->context));
		sctk_nodebug("CUDA: (POP) device_id=%d", cuda_device_id);
		cuda->pushed = 0;
	}

	return 0;
}

/**
 * Pushing a context means attaching the calling thread context to CUDA.
 *
 * This is done when a threads is restored, its context is pushed-front
 */
int sctk_accl_cuda_push_context()
{
	int num_devices = sctk_accl_get_nb_devices(), nb_check;

	/* if CUDA support is loaded but the current configuration does not provide a GPU: stop */ 
	if(num_devices<=0) return 1;
	
	/* sanity checks */
	safe_cudart(cudaGetDeviceCount(&nb_check));
	assert(nb_check == num_devices);

	/* else, we try to push a ctx in GPU queue */
	tls_cuda_t* cuda = (tls_cuda_t*) tls_cuda;

	/* if first touch, we can't pop a ctx, as no ctx has been pushed ever from this thread */
	if(cuda==NULL)
	{
		/*if( sctk_accl_cuda_init() != 0)*/
		/*{*/
			return 1;
		/*};*/
	}

	if(!cuda->pushed)
	{
		safe_cudadv(cuCtxPushCurrent(cuda->context));

		int cuda_device_id;
		safe_cudart(cudaGetDevice(&cuda_device_id));
		sctk_nodebug("CUDA (PUSH) device_id=%d", cuda_device_id);
		cuda->pushed = 1;
	}
}

#endif //MPC_Accelerators && USE_CUDA
