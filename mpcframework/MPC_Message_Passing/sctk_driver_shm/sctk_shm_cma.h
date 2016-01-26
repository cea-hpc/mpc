#ifndef __SCTK_SHM_CMA_H__
#define __SCTK_SHM_CMA_H__

#include "sctk_shm_raw_queues.h"
#include "sctk_inter_thread_comm.h"

#define MPC_USE_CMA

typedef struct
{
  sctk_thread_ptp_message_t * msg;
  int iovec_len;
  pid_t pid;
} sctk_shm_iovec_info_t;

int sctk_network_cma_shm_interface_init(void*);
sctk_thread_ptp_message_t *sctk_network_cma_cmpl_msg_shm_recv(sctk_shm_cell_t*);
sctk_thread_ptp_message_t *sctk_network_cma_msg_shm_recv(sctk_shm_cell_t *,int);
int sctk_network_cma_msg_shm_send(sctk_thread_ptp_message_t*,sctk_shm_cell_t*);

#endif /* __SCTK_SHM_CMA_H__ */
