#ifndef __SCTK_RAW_FREELIST_QUEUE_H__
#define __SCTK_RAW_FREELIST_QUEUE_H__

#include "stdlib.h"
#include <mpc_config.h>
#include "mpc_common_spinlock.h"
#include "sctk_shm_raw_queues_archdefs.h"

#define SCTK_SHM_CELL_SIZE 4096

/**
 * ENUM
 */
typedef enum {
    SCTK_SHM_MTHREADS,
    SCTK_SHM_ATOMIC
} sctk_shm_context_queue_type_t;

typedef enum {
    SCTK_SHM_EAGER, 
#ifdef MPC_USE_CMA
    SCTK_SHM_RDMA, 
#endif /* MPC_USE_CMA */
    SCTK_SHM_FIRST_FRAG, 
    SCTK_SHM_NEXT_FRAG, 
    SCTK_SHM_CMPL, 
    SCTK_SHM_ACK, 
} sctk_shm_msg_type_t;

typedef enum {
  SCTK_SHM_CELLS_QUEUE_RECV = 0,
  SCTK_SHM_CELLS_QUEUE_FREE = 1,
  SCTK_SHM_CELLS_QUEUE_CTRL = 2,
  SCTK_SHM_CELLS_LIST_COUNT = 3,
} sctk_shm_list_type_t;

/**
 * STRUCTS
 */
typedef struct sctk_shm_cell_s{
    mpc_common_spinlock_t lock;
    sctk_shm_msg_type_t msg_type;       /* Cell msg type                        */ 
    size_t size_to_copy;
    int src;
    int dest;
    int frag_hkey;
    char data[SCTK_SHM_CELL_SIZE];      /* Actual data transferred              */
} sctk_shm_cell_t;
typedef struct sctk_shm_cell_s sctk_shm_cell_t;

struct sctk_shm_item_s
{
    unsigned int src;
    struct sctk_shm_item_s *next;
    sctk_shm_cell_t cell;
};
typedef struct sctk_shm_item_s sctk_shm_item_t;

struct sctk_shm_list_s
{
    volatile sctk_shm_item_t *head;
    sctk_shm_item_t *tail;
    char pad[128];
    mpc_common_spinlock_t lock;
#ifdef SHM_USE_ATOMIC_QUEUE
    char cache_pad[CACHELINE_SIZE];
    sctk_shm_item_t *shadow_head;
#endif /* SHM_USE_ATOMIC_QUEUE */
};
// __attribute__ ((aligned(CACHELINE_SIZE)));
typedef struct sctk_shm_list_s sctk_shm_list_t;

static inline sctk_shm_item_t *
sctk_shm_abs_to_rel(char *base_addr, sctk_shm_item_t *abs_addr)
{
    return (sctk_shm_item_t *)((size_t)abs_addr - (size_t)base_addr);
}

static inline sctk_shm_item_t *
sctk_shm_rel_to_abs(char *base_addr, sctk_shm_item_t *rel_addr)
{
    return (sctk_shm_item_t *)((size_t)base_addr + (size_t)rel_addr) ;
}

static inline sctk_shm_cell_t *
sctk_shm_item_to_cell(sctk_shm_item_t *item)
{
    return (sctk_shm_cell_t *)((item)?&(item->cell):NULL);
}

static inline sctk_shm_item_t *
sctk_shm_cell_to_item(sctk_shm_cell_t * cell)
{
    return (sctk_shm_item_t *)((cell)?sctk_shm_container_of(cell,sctk_shm_item_t,cell):NULL);
}

static inline int 
sctk_shm_queue_isempty(volatile sctk_shm_list_t *queue)
{

#ifdef SHM_USE_ATOMIC_QUEUE
    return (queue->shadow_head == 0 && queue->head == 0);
#else /* SHM_USE_ATOMIC_QUEUE */
    return (queue->head == 0);
#endif /* SHM_USE_ATOMIC_QUEUE */

}

#endif /* __SCTK_RAW_FREELIST_QUEUE_H__ */
