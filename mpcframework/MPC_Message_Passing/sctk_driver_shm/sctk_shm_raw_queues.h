#ifndef __SCTK_SHM_RAW_QUEUES_API_H__
#define __SCTK_SHM_RAW_QUEUES_API_H__

#include <stdint.h>
#include "stdlib.h"

#define CACHELINE_SIZE 64
#include "sctk_raw_freelist_queue.h"

void sctk_shm_init_regions_infos(int);
void sctk_shm_free_regions_infos();
void sctk_shm_add_region_infos(char*,size_t,int,int);
void sctk_shm_reset_process_queues(int);

void sctk_shm_release_cell(sctk_shm_cell_t *);
void sctk_shm_send_cell(sctk_shm_cell_t *);
sctk_shm_cell_t* sctk_shm_recv_cell(void);
sctk_shm_cell_t *sctk_shm_get_cell(int, int);

sctk_shm_cell_t* sctk_shm_pop_cell_dest(sctk_shm_list_type_t,int);
void sctk_shm_push_cell_dest(sctk_shm_list_type_t,sctk_shm_cell_t*,int); 
void sctk_shm_push_cell_origin(sctk_shm_list_type_t,sctk_shm_cell_t*);
int sctk_shm_isempty_process_queue(sctk_shm_list_type_t,int);

#endif /* __SCTK_SHM_RAW_QUEUES_API_H__ */
