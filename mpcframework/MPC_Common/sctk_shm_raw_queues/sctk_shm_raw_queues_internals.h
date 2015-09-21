#ifndef __SCTK_SHM_RAW_QUEUES_INTERNALS_H__
#define __SCTK_SHM_RAW_QUEUES_INTERNALS_H__

#include <inttypes.h>

#include "sctk.h"
#include "sctk_shm_raw_queues.h"
#include "sctk_shm_raw_queues_helpers.h"
#include "sctk_shm_raw_queues_archdefs.h"

typedef uint64_t lfq_ptr_t;

struct lf_queue_s
{
    lfq_ptr_t head;
    lfq_ptr_t tail;
    sctk_spinlock_t lock; 
    char cache_pad[CACHELINE_SIZE - 2 * sizeof(lfq_ptr_t) - sizeof(sctk_spinlock_t)];
    lfq_ptr_t shadow_head;
} __attribute__ ((aligned(CACHELINE_SIZE)));
typedef struct lf_queue_s lf_queue_t;

struct lfq_cell_s
{
    lfq_ptr_t next;
    vcli_cell_t _cell;
    uint16_t maxsize;                  
    vcli_queue_t queue;
} __attribute__ ((aligned(CACHELINE_SIZE)));
typedef struct lfq_cell_s lfq_cell_t;

struct vcli_raw_state_s
{
    int cells_num; 
    int cells_size; 
    volatile lf_queue_t *send_queue; 
    volatile lf_queue_t *recv_queue;
    volatile lf_queue_t *cmpl_queue;
    volatile lf_queue_t *free_queue;
    lfq_cell_t * cells_pool;
};
typedef struct vcli_raw_state_s vcli_raw_state_t;

/* FUNCS */


lfq_cell_t *lfq_dequeue(volatile lf_queue_t*,lfq_cell_t*);
void sctk_vcli_raw_lfq_enqueue(volatile lf_queue_t*,lfq_cell_t*,lfq_cell_t*);
vcli_raw_state_t * sctk_vcli_raw_get_state(void *,size_t,int);
void sctk_vcli_raw_queue_reset(vcli_raw_state_t*,vcli_queue_t);

static inline uint32_t 
sctk_shm_raw_queues_size(int vcli_cells_num)
{
    void *size;
    uint32_t tmp;
    size = 0;

    /* vcli_queue size */
    tmp = 4 * sizeof(lf_queue_t);
    size += tmp;
    /* vcli_cell_pool size */
    tmp = vcli_cells_num * sizeof(lfq_cell_t);
    size += tmp;

    return ( ( uintptr_t ) page_align( size ) );
}

static inline lfq_ptr_t
vcli_raw_cell_to_ptr(lfq_cell_t *pools_ptr, lfq_cell_t *elem)
{
    return ((lfq_ptr_t)(uintptr_t)elem - (lfq_ptr_t)(uintptr_t)pools_ptr);
}

static inline lfq_cell_t*
vcli_raw_ptr_to_cell(lfq_cell_t *pools_ptr, lfq_ptr_t ptr)
{
    return ptr + (void *) pools_ptr ;
}

static inline vcli_cell_t *
lfq_to_vcli(lfq_cell_t * cell)
{
    if(cell)
        return &(cell->_cell);
    return NULL;
}

static inline lfq_cell_t *
vcli_to_lfq(vcli_cell_t * _cell)
{
    if(_cell)
        return sctk_vcli_container_of(_cell, lfq_cell_t, _cell);
    return NULL;
}

static inline void * 
vcli_raw_queue_base(void *ptr,vcli_raw_shm_queue_type_t type)
{
    return ptr + (type*sizeof(lf_queue_t));
}

/* First offset after all vcli queues */
static inline void *
vcli_raw_cell_pool_queue_base(void* ptr)
{
    return ptr + (4 * sizeof(lf_queue_t));
}

static inline int 
lfq_empty(volatile lf_queue_t *lfq)
{
    return (lfq->shadow_head == 0 && lfq->head == 0);
}

#endif /* __SCTK_SHM_RAW_QUEUES_INTERNALS_H__ */
