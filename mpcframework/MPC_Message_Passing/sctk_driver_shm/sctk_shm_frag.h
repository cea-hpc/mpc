#ifndef __SCTK_SHM_FRAG__
#define __SCTK_SHM_FRAG__

#include "sctk_ht.h"
#include "sctk_shm_raw_queues.h"
#include "sctk_inter_thread_comm.h"

typedef struct
{
    void *addr;
    size_t size_total;
    size_t size_compute;
    int key;
    void *msg;
    int dest;
    sctk_thread_ptp_message_t* header;
}sctk_shm_rdv_info_t;

sctk_thread_ptp_message_t *sctk_network_frag_msg_shm_resend(sctk_shm_cell_t*,int);
sctk_thread_ptp_message_t *sctk_network_frag_msg_shm_recv(sctk_shm_cell_t*,int);
int sctk_network_frag_msg_shm_send(sctk_thread_ptp_message_t*,int);
void sctk_network_frag_msg_shm_idle(int);
void sctk_network_frag_shm_interface_init(void);

#endif /* __SCTK_SHM_FRAG__ */
