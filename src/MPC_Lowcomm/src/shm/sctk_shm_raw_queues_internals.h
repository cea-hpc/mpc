#ifndef __SCTK_SHM_RAW_QUEUES_INTERNALS_H__
#define __SCTK_SHM_RAW_QUEUES_INTERNALS_H__

#include <stdlib.h>


#include "sctk_shm_raw_queues.h"
#include "sctk_shm_raw_queues_helpers.h"
#include "sctk_shm_raw_queues_archdefs.h"
#include "sctk_raw_freelist_queue.h"
#include "sctk_raw_freelist_queue.h"

struct sctk_shm_region_infos_s
{
    int cells_num; 
    int cells_size; 
    char* shm_base;
    char* all_shm_base;
    char* max_addr;
    mpc_common_spinlock_t global_lock;
    sctk_shm_list_t *recv_queue;
    sctk_shm_list_t *free_queue;
    sctk_shm_list_t *ctrl_queue;
    char *sctk_shm_asymm_addr;
};
typedef struct sctk_shm_region_infos_s sctk_shm_region_infos_t;

/* FUNCS */
sctk_shm_region_infos_t *sctk_shm_set_region_infos(void*,size_t,int);
void sctk_shm_reset_region_queues(sctk_shm_region_infos_t*,int);
size_t sctk_shm_get_region_size(int);



#endif /* __SCTK_SHM_RAW_QUEUES_INTERNALS_H__ */
