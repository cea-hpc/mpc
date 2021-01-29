#ifndef __SCTK_SHM_H__
#define __SCTK_SHM_H__

#include "sctk_shm_eager.h"
#include "sctk_shm_frag.h"
#include "sctk_shm_cma.h"
#include "sctk_shm_raw_queues.h"


/** \brief ROUTE level data structure for SHM 
*
*   This structure is stored in each \ref _mpc_lowcomm_endpoint_s structure
*   using the \ref _mpc_lowcomm_endpoint_info_t union
*/

typedef volatile struct sctk_shm_msg_list_s
{
	struct mpc_lowcomm_ptp_message_s *msg;
	int sctk_shm_dest;
	volatile struct sctk_shm_msg_list_s *prev, *next;
} sctk_shm_msg_list_t;


typedef struct
{
    int dest;
} _mpc_lowcomm_endpoint_info_shm_t;

typedef struct
{
} sctk_shm_rail_info_t;

void sctk_network_init_shm ( sctk_rail_info_t *rail );
void sctk_network_finalize_shm(sctk_rail_info_t *rail);
//char *sctk_get_qemu_shm_process_filename(void);
//size_t sctk_get_qemu_shm_process_size(void);

#endif /* __SCTK_SHM_H__ */
