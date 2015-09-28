#ifndef __SCTK_SHM_RAW_QUEUES_API_H__
#define __SCTK_SHM_RAW_QUEUES_API_H__

#include <stdint.h>

#define VCLI_CELLS_SIZE 16*1024
#define CACHELINE_SIZE 64

typedef uint16_t vcli_queue_t;
typedef uint16_t vcli_port_t;

typedef enum {SCTK_SHM_EAGER, SCTK_SHM_RDMA, SCTK_SHM_CMPL} vcli_shm_type_t;

enum vcli_raw_shm_queue_type_e
{
    SCTK_QEMU_SHM_SEND_QUEUE_ID = 0, 
    SCTK_QEMU_SHM_RECV_QUEUE_ID = 1,
    SCTK_QEMU_SHM_CMPL_QUEUE_ID = 2,
    SCTK_QEMU_SHM_FREE_QUEUE_ID = 3,
    SCTK_QEMU_SHM_CELL_POOLS_ID = 4, 
};
typedef enum vcli_raw_shm_queue_type_e vcli_raw_shm_queue_type_t;

struct vcli_cell_s{
    uint16_t size;                  /* Amount of data packed in a cell */
    vcli_port_t from_port;          /* Port from which the cell was sent */
    vcli_port_t to_port;            /* Port to which the cell is adressed */
    vcli_shm_type_t msg_type;
    void *opaque;                   /* Opaque data used by the sender */
    char data[VCLI_CELLS_SIZE];     /* Actual data transferred */
    char pad[CACHELINE_SIZE];       /* Prevent false sharing with container */
};
typedef struct vcli_cell_s vcli_cell_t;

void sctk_vcli_raw_infos_init(int);
void sctk_vcli_raw_infos_add(void*,size_t,int,int);

void vcli_raw_reset_infos_reset(vcli_queue_t);
void vcli_raw_reset_infos_init(void);
int sctk_vcli_raw_infos_get_total_queues(void);
vcli_cell_t *vcli_raw_pop_cell(vcli_raw_shm_queue_type_t,vcli_queue_t);
void vcli_raw_push_cell(vcli_raw_shm_queue_type_t,vcli_cell_t *);
//void *sctk_vcli_get_raw_infos( void );
uint32_t sctk_shm_get_raw_queues_size(int);

int vcli_raw_empty_queue(vcli_raw_shm_queue_type_t,vcli_queue_t);
vcli_cell_t *vcli_raw_pop_free_cell(vcli_queue_t);
vcli_cell_t *vcli_raw_pop_recv_cell(vcli_queue_t);
void vcli_raw_push_send_cell(vcli_cell_t *);

#endif /* __SCTK_SHM_RAW_QUEUES_API_H__ */
