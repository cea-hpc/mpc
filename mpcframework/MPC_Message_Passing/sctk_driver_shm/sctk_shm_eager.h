#ifndef __SCTK_SHM_EAGER_MSG_H__
#define __SCTK_SHM_EAGER_MSG_H__

#include "sctk_shm_raw_queues.h"
#include "sctk_inter_thread_comm.h"

#define SCTK_SHM_PTP_ALIGN ((sizeof(sctk_thread_ptp_message_t)+63) & (~63))
//#define SCTK_SHM_PTP_ALIGN sizeof(sctk_thread_ptp_message_t)

sctk_thread_ptp_message_t * sctk_network_eager_msg_shm_recv(sctk_shm_cell_t*,int);
int sctk_network_eager_msg_shm_send(sctk_thread_ptp_message_t*,int);

#endif /* __SCTK_SHM_EAGER_MSG_H__ */
