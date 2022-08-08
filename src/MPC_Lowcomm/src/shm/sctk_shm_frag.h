#ifndef __SCTK_SHM_FRAG__
#define __SCTK_SHM_FRAG__

#include "mpc_common_datastructure.h"
#include "sctk_shm_raw_queues.h"
#include "comm.h"

#define SCTK_SHM_MAX_FRAG_MSG_PER_PROCESS 128
typedef enum {SCTK_SHM_MULTI_FRAG, SCTK_SHM_MONO_FRAG} sctk_shm_frag_type_t;
typedef enum {SCTK_SHM_SENDER_HT, SCTK_SHM_RECVER_HT} sctk_shm_table_t;

typedef struct
{
    size_t size_total;
    size_t size_copied;
    int msg_frag_key;
    int local_mpi_rank;
    int remote_mpi_rank;
    mpc_lowcomm_ptp_message_t* header;
    char *payload_ptr;
    mpc_common_spinlock_t is_ready;
}sctk_shm_proc_frag_info_t;

typedef struct
{
   size_t msg_frag_offset;
   size_t msg_frag_size;
   unsigned int msg_frag_num;
   unsigned int msg_frag_id; 
   unsigned int msg_frag_key;
}sctk_shm_cell_frag_info_t;

void sctk_network_frag_shm_interface_init(void);
void sctk_network_frag_shm_interface_free(void);
void sctk_network_frag_msg_shm_idle(int);
mpc_lowcomm_ptp_message_t *sctk_network_frag_msg_shm_recv(sctk_shm_cell_t*);
int sctk_network_frag_msg_shm_send(mpc_lowcomm_ptp_message_t*,sctk_shm_cell_t*);

#endif /* __SCTK_SHM_FRAG__ */
