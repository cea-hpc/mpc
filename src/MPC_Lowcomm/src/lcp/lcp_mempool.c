#include "lcp_mempool.h"
#include "mpc_common_debug.h"
#include "mpc_common_spinlock.h"
#include "sctk_alloc.h"
#include <stddef.h>

#define LCP_MP_LOG

mpc_common_spinlock_t mp_lock;

FILE *logfile;

void _lcp_mempool_add(lcp_mempool *mp, lcp_mp_buffer *buf){
    buf->next = mp->head;
    buf->canary = 'c';
    mp->head = buf;
    mp->available++;
}

lcp_mp_buffer *_lcp_mempool_malloc(lcp_mempool *mp){
    return (lcp_mp_buffer *)malloc(mp->size + sizeof(lcp_mp_buffer));
}

lcp_mp_buffer *_lcp_mempool_pop(lcp_mempool *mp){
    lcp_mp_buffer *buf = mp->head;
    mp->head = mp->head->next;
    mp->available--;
    return buf;
}

int _lcp_mempool_is_below_minimum(lcp_mempool *mp){
    return mp->allocated + mp->available <= mp->min;
}

int _lcp_mempool_has_inertia(lcp_mempool *mp){
    return mp->inertia > 0;
}

int _lcp_mempool_malloc_phase(lcp_mempool *mp){
    return mp->inertia > mp->max_inertia / 2;
}

int _lcp_mempool_free_phase(lcp_mempool *mp){
    return mp->inertia < -mp->max_inertia / 2;
}

int _lcp_mempool_is_full(lcp_mempool *mp){
    return mp->available>= mp->max;
}

lcp_mp_buffer *_lcp_mempool_buffer_shift_back(void *buf){
    return (lcp_mp_buffer *)(buf - sizeof(char) - sizeof(lcp_mp_buffer *));
}

int lcp_mempool_empty(lcp_mempool *mp){
    lcp_mp_buffer *head;
    fclose(logfile);
    while(mp->head){
            head = _lcp_mempool_pop(mp);
            free(head);
    }
    return 0;
}

int lcp_mempool_init(lcp_mempool *mp, int min, int max, int size, int max_inertia){
    
#ifdef LCP_MP_LOG
    mpc_common_spinlock_lock(&mp_lock);
    char title[50];
    sprintf(title, "log_m%d_M%d_i%d.csv", min, max, max_inertia);
    logfile = fopen(title, "w+");
    char *header = "type,allocated,available,inertia\n";
    fprintf(logfile, "%s", header);
    mpc_common_spinlock_unlock(&mp_lock);
#endif

    int i;
    mp->size = size;
    mp->allocated = 0;
    mp->max_inertia = max_inertia;
    mp->inertia = 0;
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
    int effective_malloc = 0;
    mpc_common_spinlock_lock(&mp_lock);
    if(mp->inertia < mp->max_inertia && !(rand() % 3)){
        mp->inertia++;
    }
    if(mp->available <= 0){
        effective_malloc = 1;
        _lcp_mempool_add(mp, _lcp_mempool_malloc(mp));
    }
#ifdef LCP_MP_LOG
    fprintf(logfile, "alloc,%d,%d,%d,%d\n", mp->allocated, mp->available, mp->inertia, effective_malloc);
#endif
    lcp_mp_buffer *ret = _lcp_mempool_pop(mp);
    if(!ret) printf("error allocating\n");
    mp->allocated++;
    mpc_common_spinlock_unlock(&mp_lock);
    return ret->buffer;
}

void lcp_mempool_free(lcp_mempool *mp, void *buffer){
    lcp_mp_buffer *buf = _lcp_mempool_buffer_shift_back(buffer);
    int effective_free = 0;
    if ( buf->canary != 'c' ){
        return;
    }
    mpc_common_spinlock_lock(&mp_lock);
    if(mp->inertia > -mp->max_inertia && !(rand() % 3)) mp->inertia--;
    // if mp does not have enough buffers, restack
    // if mp does not have to many buffers and has inertia, restack
    if( _lcp_mempool_is_below_minimum(mp) || (!_lcp_mempool_free_phase(mp) && !_lcp_mempool_is_full(mp) )){
        _lcp_mempool_add(mp, buf);
    }
    else{
	    // printf("freeing\n");
        free(buf);
        effective_free = 1;
        if(!_lcp_mempool_is_below_minimum(mp) && _lcp_mempool_free_phase(mp) && mp->available){
            lcp_mp_buffer *head = _lcp_mempool_pop(mp);
            free(head);
        }
    }
#ifdef LCP_MP_LOG
    fprintf(logfile, "free,%d,%d,%d,%d\n", mp->allocated, mp->available, mp->inertia, effective_free);
#endif
    mpc_common_spinlock_unlock(&mp_lock);
    mp->allocated--;
}
