#include <stdio.h>
#include <sctk_alloc.h>
#include <stdlib.h>
#include <mpc_common_spinlock.h>

#ifndef LCP_MEMPOOL_H
#define LCP_MEMPOOL_H

typedef struct lcp_mp_buffer_s{
    struct lcp_mp_buffer_s *next;
    char canary;
    char buffer[0];
} lcp_mp_buffer;

typedef struct {
    lcp_mp_buffer *head;
    int min, max, allocated, available, inertia, max_inertia;
    void *(*malloc_func)(size_t size);
    void (*free_func)(void * pointer);
    size_t size;
} mpc_mempool;

void _mpc_mempool_stack(void * buf);

int mpc_mempool_init(mpc_mempool *mp, 
    int min, 
    int max, 
    int size, 
    void *(*malloc_func)(size_t), 
    void (*free_func)(void *));

void *mpc_mempool_alloc(mpc_mempool *mempool);

void mpc_mempool_free(mpc_mempool *mempool, void *buffer);

int mpc_mempool_empty(mpc_mempool *mp);

#endif