#ifndef __SCTK_SHM_RAW_QUEUES_API_H__
#define __SCTK_SHM_RAW_QUEUES_API_H__

#include <stdint.h>
#include "stdlib.h"

#define VCLI_CELLS_SIZE 16*1024
#define CACHELINE_SIZE 64

#include "sctk_raw_freelist_queue.h"

void sctk_shm_init_regions_infos(int);
void sctk_shm_add_region_infos(char*,size_t,int,int);
void sctk_shm_reset_process_queues(int);

sctk_shm_cell_t* sctk_shm_pop_cell_dest(sctk_shm_list_type_t,int);
sctk_shm_cell_t* sctk_shm_pop_cell_recv(void);
sctk_shm_cell_t* sctk_shm_pop_cell_free(int);
void sctk_shm_push_cell_dest(sctk_shm_list_type_t,sctk_shm_cell_t*,int); 
void sctk_shm_push_cell_origin(sctk_shm_list_type_t,sctk_shm_cell_t*);
int sctk_shm_isempty_process_queue(sctk_shm_list_type_t,int);

#endif /* __SCTK_SHM_RAW_QUEUES_API_H__ */
