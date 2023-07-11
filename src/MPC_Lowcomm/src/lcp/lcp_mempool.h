#include <stdio.h>
#include <sctk_alloc.h>
#include <stdlib.h>
#include <mpc_common_spinlock.h>

typedef struct lcp_mp_buffer_s{
    struct lcp_mp_buffer_s *next;
    char canary;
    char buffer[0];
} lcp_mp_buffer;

typedef struct {
    lcp_mp_buffer *head;
    int min, max, allocated, available;
    size_t size;
} lcp_mempool;

void _lcp_mempool_stack(void * buf);

int lcp_mempool_init(lcp_mempool *mp, int min, int max, int size);

void *lcp_mempool_alloc(lcp_mempool *mempool);

void lcp_mempool_free(lcp_mempool *mempool, void *buffer);