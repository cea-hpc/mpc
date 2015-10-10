#ifndef __SCTK_SHM_RAW_QUEUES_ALLOC_H__ 
#define __SCTK_SHM_RAW_QUEUES_ALLOC_H__ 

//#define ATOMIC_QUEUE
//#define QEMU_SHM_USE_MPC

#ifndef QEMU_SHM_USE_MPC

    #include <sctk_alloc.h>
    #define sctk_qemu_shm_vcli_alloc sctk_malloc;

#else /* QEMU_SHM_USE_MPC */

    #include <stdlib.h>
    #define sctk_qemu_shm_vcli_alloc malloc;
    /** Assume stay present independently of NDEBUG/NO_INTERNAL_ASSERT **/
#endif /* QEMU_SHM_USE_MPC */

#endif /* SCTK_SHM_RAW_QUEUES_ALLOC_H__ */
