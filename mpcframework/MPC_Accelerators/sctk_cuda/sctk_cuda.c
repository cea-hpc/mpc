
#if defined(MPC_Accelerators) && defined(MPC_USE_CUDA)
#include <sctk_cuda.h>
#include <sctk_debug.h>
#include <sctk_device_topology.h>
#include <sctk_spinlock.h>
void sctk_accl_cuda_print_ctx_flag(){
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
int sctk_accl_cuda_get_closest_device(int cpu_id){
	int num_devices = 0;
	cudaGetDeviceCount(&num_devices);
	if(num_devices>0)
	{
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

		return closest_device_cuda_id;
		//return (closest_device_cuda_id+1)%2;
	}
	return 0;
}

int sctk_accl_cuda_init(void** ptls_cuda){
    int num_devices = 0;
    cudaGetDeviceCount(&num_devices);
    if(num_devices>0)
    {
        tls_cuda_t* cuda=(tls_cuda_t*) *ptls_cuda;
        if(cuda == NULL)
		{
            //initialize the CUDA driver API
            cuInit(0);
            cuda = (tls_cuda_t*)sctk_malloc(sizeof(tls_cuda_t));
            cuda->is_ready = -1;
            cuda->cpu_id = -1;

            sctk_nodebug("INIT_MALLOC : is_ready=%d, cpu_id=%d, address=%p\n",cuda->is_ready,cuda->cpu_id,cuda);
        }
        
		if(cuda->is_ready==1)
		{
            cuda = (tls_cuda_t*) *ptls_cuda;
            CUresult cuda_error;

            //TODO mettre les bon flags
            unsigned int flags = CU_CTX_SCHED_YIELD;

            //TODO trouver le bon device a use

            sctk_nodebug("CPU_ID=%d\n",cuda->cpu_id);
            CUdevice dev = sctk_accl_cuda_get_closest_device(cuda->cpu_id);

            sctk_nodebug("BEFOREcuCtxCreate=%d, cuda->is_ready=%d, cpu_id=%d,tls_cpu_id=%d, dev=%d\n",
                    cuda_error,
                    cuda->is_ready,
                    sctk_get_cpu(),
                    cuda->cpu_id,
                    dev);

            cuda_error = cuCtxCreate (&cuda->context,flags,dev);

            sctk_nodebug("AFTER cuCtxCreate=%d, cuda->is_ready=%d, cpu_id=%d,tls_cpu_id=%d, dev=%d\n",
                     cuda_error,
                     cuda->is_ready,
                     sctk_get_cpu(),
                     cuda->cpu_id,
                     dev);
        }

        *ptls_cuda = cuda;
        
		sctk_nodebug("init:cuda address=%p\n",cuda);
		sctk_nodebug("init:ptls_cuda address=%p\n",*ptls_cuda);

        return 1;
    }
    return 1;
}

int sctk_accl_cuda_pop_context(void** ptls_cuda){
    int num_devices = 0;
    cudaGetDeviceCount(&num_devices);
    if(num_devices>0)
    {
        CUresult cuda_error;
        tls_cuda_t* cuda=NULL;

        sctk_nodebug("pop  : %p - cpuid=%d\n",cuda, sctk_get_cpu());

        if(*ptls_cuda == NULL)
		{
            sctk_accl_cuda_init(ptls_cuda);
        }
        cuda = (tls_cuda_t*) *ptls_cuda;

        if(cuda->is_ready==1)
		{

            int cuda_device_id;
            cudaGetDevice(&cuda_device_id);
            sctk_nodebug("cuCtxPopCurrent=%d, cuda_device_id=%d\n", cuda_error, cuda_device_id);

            cuda_error = cuCtxPopCurrent(&cuda->context);
        }

        return cuda_error;
    }
    return 0;
}

int sctk_accl_cuda_push_context(void* tls_cuda){
    int num_devices = 0;
    cudaGetDeviceCount(&num_devices);
    if(num_devices>0)
    {
        CUresult cuda_error;
        tls_cuda_t* cuda=NULL;

        cuda = (tls_cuda_t*) tls_cuda;
        sctk_nodebug("push: %p - cpuid=%d\n",cuda,sctk_get_cpu());

        if(tls_cuda==NULL)
		{
            return 0;
        }
        
		if(cuda->is_ready==1)
		{
            cuda_error = cuCtxPushCurrent(cuda->context);

            int cuda_device_id;
            cudaGetDevice(&cuda_device_id);
            sctk_nodebug("cuCtxPushCurrent=%d, cuda->is_ready=%d, cuda_device_id=%d\n", cuda_error, cuda->is_ready, cuda_device_id);
        }
        
		return cuda_error;
    }
    return 0;
}

#endif //MPC_Accelerators && USE_CUDA
