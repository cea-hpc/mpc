#ifndef __SCTK_SHM_CMA_H__
#define __SCTK_SHM_CMA_H__

#include "sctk_shm_raw_queues.h"
#include "sctk_inter_thread_comm.h"

#define MPC_USE_CMA

typedef struct
{
  struct iovec * ptr;
  int mpi_src;
  int len;
  pid_t pid;
  sctk_thread_ptp_message_t * msg;
} sctk_shm_iovec_info_t;

sctk_thread_ptp_message_t *sctk_network_rdma_cmpl_msg_shm_recv(vcli_cell_t*);
sctk_thread_ptp_message_t *sctk_network_rdma_msg_shm_recv(vcli_cell_t *,int);
int sctk_network_rdma_msg_shm_send(sctk_thread_ptp_message_t*,vcli_cell_t*,int);

#endif /* __SCTK_SHM_CMA_H__ */
