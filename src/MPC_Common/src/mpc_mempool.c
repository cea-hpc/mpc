#include "mpc_mempool.h"
#include "mpc_common_debug.h"
#include "mpc_common_spinlock.h"
#include "sctk_alloc.h"
#include <stddef.h>

#define LCP_MP_LOG

mpc_common_spinlock_t mp_lock;
int count_inertia;

FILE *logfile;

void _mpc_mempool_add(mpc_mempool *mp, lcp_mp_buffer *buf){
    buf->next = mp->head;
    buf->canary = 'c';
    mp->head = buf;
    mp->available++;
}

lcp_mp_buffer *_mpc_mempool_malloc(mpc_mempool *mp){
    return (lcp_mp_buffer *)mp->malloc_func(mp->size + sizeof(lcp_mp_buffer));
}

lcp_mp_buffer *_mpc_mempool_pop(mpc_mempool *mp){
    lcp_mp_buffer *buf = mp->head;
    mp->head = mp->head->next;
    mp->available--;
    return buf;
}

int _mpc_mempool_is_below_minimum(mpc_mempool *mp){
    return mp->allocated + mp->available <= mp->min;
}

int _mpc_mempool_has_inertia(mpc_mempool *mp){
    return mp->inertia > 0;
}

int _mpc_mempool_malloc_phase(mpc_mempool *mp){
    return mp->inertia > mp->max_inertia / 2;
}

int _mpc_mempool_free_phase(mpc_mempool *mp){
    return mp->inertia < -mp->max_inertia / 2;
}

int _mpc_mempool_is_full(mpc_mempool *mp){
    return mp->available>= mp->max;
}

lcp_mp_buffer *_mpc_mempool_buffer_shift_back(void *buf){
    return (lcp_mp_buffer *)(buf - sizeof(char) - sizeof(lcp_mp_buffer *));
}

int mpc_mempool_empty(mpc_mempool *mp){
    lcp_mp_buffer *head;
    fclose(logfile);
    while(mp->head){
            head = _mpc_mempool_pop(mp);
            free(head);
    }
    return 0;
}

int mpc_mempool_init(mpc_mempool *mp, 
    int min, 
    int max, 
    int size, 
    void *(*malloc_func)(size_t), 
    void (*free_func)(void *)){

    int i;
    mp->size = size;
    mp->allocated = 0;
    mp->max_inertia = max / 5;
    mp->inertia = 0;
    mp->available = 0;
    mp->head = NULL;
    mp->min = min;
    mp->max = max;
    mp->malloc_func = malloc_func;
    mp->free_func = free_func;

#ifdef LCP_MP_LOG
    mpc_common_spinlock_lock(&mp_lock);
    char title[50];
    sprintf(title, "log_m%d_M%d_i%d.csv", min, max, mp->max_inertia);
    logfile = fopen(title, "w+");
    char *header = "type,allocated,available,inertia\n";
    fprintf(logfile, "%s", header);
    mpc_common_spinlock_unlock(&mp_lock);
#endif

    for(i = 0 ; i < mp->min ; i ++){
        _mpc_mempool_add(mp, _mpc_mempool_malloc(mp));
    }
    return 0;
}

void *mpc_mempool_alloc(mpc_mempool *mp){
    int effective_malloc = 0;
    mpc_common_spinlock_lock(&mp_lock);
    count_inertia++;
    if(mp->inertia < mp->max_inertia && !(count_inertia % 3 == 0)) mp->inertia++;
    if(mp->available <= 0){
        effective_malloc = 1;
        _mpc_mempool_add(mp, _mpc_mempool_malloc(mp));
    }
#ifdef LCP_MP_LOG
    fprintf(logfile, "alloc,%d,%d,%d,%d\n", mp->allocated, mp->available, mp->inertia, effective_malloc);
#endif
    lcp_mp_buffer *ret = _mpc_mempool_pop(mp);
    if(!ret) printf("error allocating\n");
    mp->allocated++;
    mpc_common_spinlock_unlock(&mp_lock);
    return ret->buffer;
}

void mpc_mempool_free(mpc_mempool *mp, void *buffer){
    lcp_mp_buffer *buf = _mpc_mempool_buffer_shift_back(buffer);
    int effective_free = 0;
    if ( buf->canary != 'c' ){
        return;
    }
    mpc_common_spinlock_lock(&mp_lock);
    count_inertia++;
    if(mp->inertia > -mp->max_inertia && !(count_inertia % 3 == 0)) mp->inertia--;
    // if mp does not have enough buffers, restack
    // if mp does not have to many buffers and has inertia, restack
    if( _mpc_mempool_is_below_minimum(mp) || (!_mpc_mempool_free_phase(mp) && !_mpc_mempool_is_full(mp) )){
        _mpc_mempool_add(mp, buf);
    }
    else{
        mp->free_func(buf);
        effective_free = 1;
        if(!_mpc_mempool_is_below_minimum(mp) && _mpc_mempool_free_phase(mp) && mp->available){
            lcp_mp_buffer *head = _mpc_mempool_pop(mp);
            mp->free_func(head);
        }
    }
#ifdef LCP_MP_LOG
    fprintf(logfile, "free,%d,%d,%d,%d\n", mp->allocated, mp->available, mp->inertia, effective_free);
#endif
    mpc_common_spinlock_unlock(&mp_lock);
    mp->allocated--;
}
