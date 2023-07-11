#include "lcp_mempool.h"
#include "mpc_common_debug.h"
#include "mpc_common_spinlock.h"
#include "sctk_alloc.h"

mpc_common_spinlock_t mp_lock;

void _lcp_mempool_add(lcp_mempool *mp, lcp_mp_buffer *buf){
    buf->next = mp->head;
    buf->canary = 'c';
    mp->head = buf;
    mp->available++;
}

lcp_mp_buffer *_lcp_mempool_malloc(lcp_mempool *mp){
    return (lcp_mp_buffer *)sctk_malloc(mp->size + sizeof(lcp_mp_buffer));
}

void *_lcp_mempool_pop(lcp_mempool *mp){
    void *buf = mp->head->buffer;
    mp->head = mp->head->next;
    return buf;
}

int lcp_mempool_init(lcp_mempool *mp, int min, int max, int size){
    int i;
    mp->size = size;
    mp->available = 0;
    mp->head = NULL;
    mp->min = min;
    mp->max = max;

    for(i = 0 ; i < mp->min ; i ++){
        _lcp_mempool_add(mp, _lcp_mempool_malloc(mp));
    }
    return 0;
}

void *lcp_mempool_alloc(lcp_mempool *mp){
    mpc_common_spinlock_lock(&mp_lock);
    if(mp->available <= 0){
        _lcp_mempool_add(mp, _lcp_mempool_malloc(mp));
    }
    mp->available--;
    void *ret = _lcp_mempool_pop(mp);
    if(!ret) mpc_common_debug_error("error allocating\n");
    mpc_common_spinlock_unlock(&mp_lock);
    return ret;
}

void lcp_mempool_free(lcp_mempool *mp, void *buffer){
    mpc_common_spinlock_lock(&mp_lock);
    if ( !buffer || *(char *)(buffer - sizeof(char)) != 'c' ){
        mpc_common_debug_error("memory corrupted\n");
        mpc_common_spinlock_unlock(&mp_lock);
        return;
    }
    lcp_mp_buffer *buf = (lcp_mp_buffer *)(buffer - sizeof(lcp_mp_buffer));
    if(mp->available < mp->max){
        _lcp_mempool_add(mp, buf);
    }
    else{
        sctk_free(buf);
    }
    mpc_common_spinlock_unlock(&mp_lock);
}