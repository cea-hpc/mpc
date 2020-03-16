#ifndef __SCTK_SHM_EAGER_MSG_H__
#define __SCTK_SHM_EAGER_MSG_H__

#include "sctk_shm_raw_queues.h"
#include "comm.h"

#define SCTK_SHM_PTP_ALIGN ((sizeof(mpc_lowcomm_ptp_message_t)+63) & (~63))
//#define SCTK_SHM_PTP_ALIGN sizeof(mpc_lowcomm_ptp_message_t)

mpc_lowcomm_ptp_message_t * sctk_network_eager_msg_shm_recv(sctk_shm_cell_t*,int);
int sctk_network_eager_msg_shm_send(mpc_lowcomm_ptp_message_t*,sctk_shm_cell_t*);

#endif /* __SCTK_SHM_EAGER_MSG_H__ */
