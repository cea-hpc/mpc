/* ############################# MPC License ############################## */
/* # Wed Nov 19 15:19:19 CET 2008                                         # */
/* # Copyright or (C) or Copr. Commissariat a l'Energie Atomique          # */
/* # Copyright or (C) or Copr. 2010-2012 Université de Versailles         # */
/* # St-Quentin-en-Yvelines                                               # */
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
/* # Authors:                                                             # */
/* #   - PERACHE Marc marc.perache@cea.fr                                 # */
/* #   - CARRIBAULT Patrick patrick.carribault@cea.fr                     # */
/* #   - TCHIBOUKDJIAN Marc marc.tchiboukdjian@exascale-computing.eu      # */
/* #   - TABOADA Hugo hugo.taboada.ocre@cea.fr                            # */
/* #   - JAEGER Julien julien.jaeger@cea.fr                               # */
/* #                                                                      # */
/* ######################################################################## */


#define _GNU_SOURCE
#include "sctk_config.h"
#include "sctk_alloc.h"
#include "sctk_topology.h"
#include "sctk_accessor.h"

#include "sctk_spinlock.h"
#include "sctk_atomics.h"
#include <unistd.h>
#include <sctk_tls.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "hwloc.h"
#include <utlist.h>
#include <dlfcn.h>

#if defined(MPC_Accelerators)
#include "sctk_device_topology.h"
//CUDA

//desable because doesn't works with older cuda runtimes
//void sctk_print_current_cuda_ctx_flag(){
//    unsigned int currentFlag=7357;
//    CUcontext currentContext;
//    CUresult res;
//
//    cuCtxGetFlags(&currentFlag);
//
//    res=cuCtxGetCurrent(&currentContext);
//    printf("sctk:currentContext address=%p - %d\n",&currentContext,res);
//    fflush(stdout);
//
//    switch(currentFlag){
//        case CU_CTX_SCHED_SPIN:
//            printf("sctk:CU_CTX_SCHED_SPIN\n");
//            fflush(stdout);
//            break;
//        case CU_CTX_SCHED_YIELD:
//            printf("sctk:CU_CTX_SCHED_YIELD\n");
//            fflush(stdout);
//            break;
//        case CU_CTX_SCHED_BLOCKING_SYNC:
//            printf("sctk:CU_CTX_SCHED_BLOCKING_SYNC\n");
//            fflush(stdout);
//            break;
//        case CU_CTX_SCHED_AUTO:
//            printf("sctk:CU_CTX_SCHED_AUTO\n");
//            fflush(stdout);
//            break;
//        case CU_CTX_MAP_HOST:
//            printf("sctk:CU_CTX_MAP_HOST\n");
//            fflush(stdout);
//            break;
//        case CU_CTX_LMEM_RESIZE_TO_MAX:
//            printf("sctk:CU_CTX_LMEM_RESIZE_TO_MAX\n");
//            fflush(stdout);
//            break;
//        default:
//            printf("sctk:NO VALID FLAG FOUND\n");
//            fflush(stdout);
//    }
//}


int lock=SCTK_SPINLOCK_INITIALIZER;
int cpt=0;
int sctk_accelerator_cuda__get_closest_device(int cpu_id){
    int num_devices = 0;
    cudaGetDeviceCount(&num_devices);
    if(num_devices>0)
    {

        sctk_device_t* closest_device = NULL;
        int closest_device_cuda_id = -1;

        //TODO il faudrait aussi gere le cas ou il y a plusieurs gpu par noeud a egal
        //distance, et d'autre gpu sur un autre noeud a distance differente
        if(sctk_device_matrix_is_equidistant("card*")){
            int count;
            sctk_device_t ** device_list = sctk_device_get_from_handle_regexp("card*",&count);

            sctk_spinlock_lock(&lock);
            closest_device_cuda_id=(cpt+count)%count;
            cpt++;
            sctk_spinlock_unlock(&lock);

            // printf("COUNT DEVICES=%d, CLOSEST_DEVICE_CUDA_ID=%d\n",count, closest_device_cuda_id);
            // fflush(stdout);

            sctk_free(device_list);
        }else{
            closest_device = sctk_device_matrix_get_closest_from_pu(sctk_get_cpu(),"card*");
            closest_device_cuda_id = closest_device->device_cuda_id;
        }

        return closest_device_cuda_id;
        //return (closest_device_cuda_id+1)%2;

    }
    return 0;
}

//int sctk_accelerators_cuda__allocate_tls(void** ptls_cuda){
//}

int sctk_accelerators_cuda__init(void** ptls_cuda){
    int num_devices = 0;
    cudaGetDeviceCount(&num_devices);
    if(num_devices>0)
    {




        tls_cuda_t* cuda=(tls_cuda_t*) *ptls_cuda;
        if(cuda == NULL){
            //initialize the CUDA driver API
            cuInit(0);
            cuda = (tls_cuda_t*)sctk_malloc(sizeof(tls_cuda_t));
            cuda->is_ready = -1;
            cuda->cpu_id = -1;

            // printf("INIT_MALLOC : is_ready=%d, cpu_id=%d, address=%p\n",cuda->is_ready,cuda->cpu_id,cuda);
            // fflush(stdout);
        }




        if(cuda->is_ready==1){
            // printf("INIT_IF:cuda->is_ready=%d\n",cuda->is_ready);
            // fflush(stdout);

            cuda = (tls_cuda_t*) *ptls_cuda;
            CUresult cuda_error;

            //TODO mettre les bon flags
            unsigned int flags = CU_CTX_SCHED_YIELD;

            //TODO trouver le bon device a use

            // fprintf(stdout,"CPU_ID=%d\n",cuda->cpu_id);
            // fflush(stdout);
            CUdevice dev = sctk_accelerator_cuda__get_closest_device(cuda->cpu_id);

            // fprintf(stdout,"BEFOREcuCtxCreate=%d, cuda->is_ready=%d, cpu_id=%d,tls_cpu_id=%d, dev=%d\n",
            //         cuda_error,
            //         cuda->is_ready,
            //         sctk_get_cpu(),
            //         cuda->cpu_id,
            //         dev);
            // fflush(stdout);

            cuda_error = cuCtxCreate (&cuda->context,flags,dev);

            // fprintf(stdout,"AFTER cuCtxCreate=%d, cuda->is_ready=%d, cpu_id=%d,tls_cpu_id=%d, dev=%d\n",
            //         cuda_error,
            //         cuda->is_ready,
            //         sctk_get_cpu(),
            //         cuda->cpu_id,
            //         dev);
            // fflush(stdout);
            //sctk_print_current_cuda_ctx_flag();

        }

        *ptls_cuda = cuda;



        // fprintf(stdout,"init:cuda_context address=%p\n",cuda_context);
        // fflush(stdout);
        // fprintf(stdout,"init:tls_cuda_context address=%p\n",*tls_cuda_context);
        // fflush(stdout);
        // fprintf(stdout,"after cuCtxCreate\n");
        // fflush(stdout);

        return 1;

    }
    return 1;
}

int sctk_accelerators_cuda__pop_context(void** ptls_cuda){
    int num_devices = 0;
    cudaGetDeviceCount(&num_devices);
    if(num_devices>0)
    {

        CUresult cuda_error;
        tls_cuda_t* cuda=NULL;

        // printf("pop  : %p - cpuid=%d\n",cuda, sctk_get_cpu());
        // fflush(stdout);

        if(*ptls_cuda == NULL){
            sctk_accelerators_cuda__init(ptls_cuda);
        }
        cuda = (tls_cuda_t*) *ptls_cuda;

        if(cuda->is_ready==1){

            int cuda_device_id;
            cudaGetDevice(&cuda_device_id);
            // printf("cuCtxPopCurrent=%d, cuda_device_id=%d\n",
            //         cuda_error,
            //         cuda_device_id);
            // fflush(stdout);

            cuda_error = cuCtxPopCurrent(&cuda->context);
        }

        //sctk_print_current_cuda_ctx_flag();

        return cuda_error;

    }
    return 0;
}

int sctk_accelerators_cuda__push_context(void* tls_cuda){
    int num_devices = 0;
    cudaGetDeviceCount(&num_devices);
    if(num_devices>0)
    {

        CUresult cuda_error;
        tls_cuda_t* cuda=NULL;

        cuda = (tls_cuda_t*) tls_cuda;
        // printf("push: %p - cpuid=%d\n",cuda,sctk_get_cpu());
        // fflush(stdout);

        if(tls_cuda==NULL){
            return 0;
        }



        if(cuda->is_ready==1){
            cuda_error = cuCtxPushCurrent(cuda->context);
            // int ndev;
            // cudaGetDevice(&ndev);
            // fprintf(stdout,"cuCtxPushCurrent=%d - ndev=\n",cuda_error,ndev);

            // int cuda_device_id;
            // cudaGetDevice(&cuda_device_id);
            // fprintf(stdout,"cuCtxPushCurrent=%d, cuda->is_ready=%d, cuda_device_id=%d\n",
            //         cuda_error,
            //         cuda->is_ready,
            //         cuda_device_id);
            // fflush(stdout);
        }


        //sctk_print_current_cuda_ctx_flag();

        return cuda_error;

    }

    return 0;
}

#endif //MPC_Accelerators

#if defined(SCTK_USE_TLS)
__thread void* sctk_extls_storage = NULL;


struct dyn_sym_s
{
	char * name;
    void * addr;
	struct dyn_sym_s * next;
};


struct dsos_s
{
	char * name;
	struct dsos_s * next;
};

static size_t page_size = 0;
static struct dyn_sym_s * __sctk_dyn_symbols = NULL;
static struct dsos_s * __sctk_dsos = NULL;
static void * __sctk_tls_locate_tls_dyn_initializer_handle = NULL;
sctk_spinlock_t __sctk_tls_locate_tls_dyn_lock = 0;

#if defined (SCTK_USE_OPTIMIZED_TLS)
int MPC_Config_Status_MPC_HAVE_OPTION_ETLS_OPTIMIZED(){
  return 1;
}
#else
int MPC_Config_Status_MPC_HAVE_OPTION_ETLS_OPTIMIZED(){
  return 0;
}
#endif

#endif

/* This function is called to discover if a given
 * variable comes with a dynamic initializer */
void sctk_tls_locate_tls_dyn_initializer( char * fname )
{
	sctk_spinlock_lock( &__sctk_tls_locate_tls_dyn_lock );
	
	if( !__sctk_tls_locate_tls_dyn_initializer_handle )
		 __sctk_tls_locate_tls_dyn_initializer_handle = dlopen( NULL, RTLD_LAZY );

	dlerror();    /* Clear any existing error */

	void * ret = dlsym( __sctk_tls_locate_tls_dyn_initializer_handle, fname );

	char * error = NULL;
	
	if ((error = dlerror()) != NULL)  {
		//fprintf(stderr, "%s\n", error);
		sctk_spinlock_unlock( &__sctk_tls_locate_tls_dyn_lock );
		return;
	}

	sctk_spinlock_unlock( &__sctk_tls_locate_tls_dyn_lock );
	
	/* If we found a dynamic initializer call it */
	if( ret )
	{
		sctk_info("Locating Dyn initalizer for %s ret %p\n", fname, ret );
		void (*fn)() = (void (*)())ret;
		(fn)();
	}
}

int sctk_load_proc_self_maps()
{
	char * maps = calloc( 4 * 1024 * 1024, 1 );
	
	if( !maps )
	{
		return 1;
	}
	
	FILE * self = fopen("/proc/self/maps" ,"r");
	
	size_t ret = fread(maps, 1, 4 * 1024 * 1024 , self);
	
	//printf("READ %d\n", ret );
	
	char * line = maps;
	char * next_line;
	
	int cont = 1;
	
	unsigned long begin, end, size, inode;
	char skip[50], perm[5], dev[50], dsoname[500];
	
	do
	{
		next_line = strstr( line, "\n" );
		
		if( next_line )
		{
			*next_line = '\0';
			next_line++;
		}
		else
		{
			cont = 0;
		}
		
		//printf("%s\n", line );
		
		sscanf(line, "%lx-%lx %4s %s %s %ld %s", &begin, &end, perm,
		skip, dev, &inode, dsoname);
		
		if( (perm[2] == 'x') && !(dsoname[0] == '[') )
		{
			
			//fprintf(stderr,"==>%s (%s)\n", dsoname , perm);
		
			struct dsos_s * new = sctk_malloc( sizeof( struct dsos_s ) );
			
			if( !new )
			{
				return 1;
			}
			
			new->name = strdup( dsoname );
			new->next = __sctk_dsos;
			__sctk_dsos = new;
		
		}
		
		line = next_line;
		
		
	}while( cont );
	
	fclose( self );
	
	free( maps );

	return 0;
}


int sctk_load_wrapper_symbols( char * dso , void * handle )
{
	char command[1000];

	snprintf(command, 1000, "nm  %s 2>&1 | grep \"___mpc_TLS_w\\|_ZTW\"", dso );
	
	
	FILE * wrapper_symbols = popen(command, "r");
	
	while(!feof(wrapper_symbols))
	{
		char buff[500];
		
		if(fgets(buff, 500, wrapper_symbols) == 0)
		{
			break;
		}
		
		//printf("%s\n", buff );
	
		char * wrapp = strstr( buff , "___mpc_TLS_w" );
		
		if( wrapp )
		{
			char *retl = strstr( buff, "\n");
			
			if( retl )
			{
				*retl = '\0';
				
				while( retl != wrapp )
				{
					if( *(retl - 1) == ' ')
					{
						retl--;
						*retl = '\0';
					}
					else
					{
						break;
					}
				}
				
			}
			
			
			struct dyn_sym_s * new = sctk_malloc( sizeof(struct dyn_sym_s) );
			
			if( !new )
			{
				return 1;
			}
			
			new->name = strdup( wrapp );
			new->addr = dlsym(handle, wrapp );
			new->next = __sctk_dyn_symbols;
			__sctk_dyn_symbols = new;
			
			
			sctk_debug("Located dyn initializer : %s", wrapp );
		}
	}
	
	pclose( wrapper_symbols );
	
	return 0;
}

int sctk_locate_dynamic_initializers()
{
	if( sctk_load_proc_self_maps() )
	{
		fprintf(stderr, "Failled to load /proc/self/maps");
		return 1;
	}
	
	struct dsos_s * current = __sctk_dsos;
	
	void * lib_handle = dlopen( NULL, RTLD_LAZY );
	
	while( current )
	{
		//printf("CHECKING %s\n", current->name );
		if( sctk_load_wrapper_symbols(current->name, lib_handle) )
		{
			return 1;
		}
	
		current = current->next;
	}
	
	dlclose( lib_handle );
	
	return 0;
}

int sctk_call_dynamic_initializers()
{
	struct dyn_sym_s * current = __sctk_dyn_symbols;
	
	while( current )
	{
		if( current->addr )
		{
			sctk_debug("TLS : Calling dynamic init %s", current->name);
			((void (*)())current->addr)();
		}
		
		current = current->next;
	}
	
	return 0;
}

#if defined(SCTK_USE_TLS) && defined(Linux_SYS)


inline void* extls_get_context_storage_addr(void)
{
	return (void*)&sctk_extls_storage;
}

#define SCTK_COW_TLS

#if defined(SCTK_COW_TLS)
int MPC_Config_Status_MPC_HAVE_OPTION_ETLS_COW(){
  return 1;
}
#else
int MPC_Config_Status_MPC_HAVE_OPTION_ETLS_COW(){
  return 0;
}
#endif

void* sctk_align_ptr_to_page(void* ptr){
  if(!page_size)
	  page_size = getpagesize ();
  ptr = (char*)ptr - ((size_t) ptr % page_size);
  return ptr;
}

size_t sctk_align_size_to_page(size_t size){
  if(!page_size)
	  page_size = getpagesize ();
  if(size % page_size == 0){
    return size;
  }
  size += page_size - (size % page_size);
  return size;
}

size_t sctk_extls_size()
{
  size_t size;
  size = extls_get_sz_static_tls_segments();
  return size;
}

void sctk_tls_dtors_init(struct sctk_tls_dtors_s ** head)
{
	*head = NULL;
}

void sctk_tls_dtors_add(struct sctk_tls_dtors_s ** head, void * obj, void (*func)(void *))
{
	struct sctk_tls_dtors_s * elt = sctk_malloc(sizeof(struct sctk_tls_dtors_s));
	elt->dtor = func;
	elt->obj = obj;
	
	LL_PREPEND(*head, elt);
}

void sctk_tls_dtors_free(struct sctk_tls_dtors_s ** head)
{
	struct sctk_tls_dtors_s* elt = NULL, *tmp = NULL;
	int count = 0;
	
	LL_FOREACH_SAFE(*head, elt, tmp)
	{
		elt->dtor(elt->obj);
		LL_DELETE(*head, elt);
		sctk_free(elt);
	}
}

#ifdef MPC_DISABLE_HLS
int MPC_Config_Status_MPC_HAVE_OPTION_HLS(){
  return 0;
}
#else
int MPC_Config_Status_MPC_HAVE_OPTION_HLS(){
  return 1;
}
#endif

#else
#warning "Experimental Ex-TLS support desactivated"

int MPC_Config_Status_MPC_HAVE_OPTION_HLS(){
  return 0;
}
int MPC_Config_Status_MPC_HAVE_OPTION_ETLS_COW(){
  return 0;
}
int MPC_Config_Status_MPC_HAVE_OPTION_ETLS_OPTIMIZED(){
  return 0;
}
#endif
