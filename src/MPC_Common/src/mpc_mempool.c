#include "mpc_mempool.h"
#include "../../MPC_Lowcomm/include/lcp_common.h"
#include "mpc_common_debug.h"
#include "mpc_common_spinlock.h"
#include <stddef.h>
#include <stdint.h>

int count_inertia;
#ifdef MPC_MP_LOG
int mempool_number;
#endif

void _mpc_mempool_add(mpc_mempool *mp, mpc_mp_buffer *buf)
{
	buf->next   = mp->head;
	buf->mp     = mp;
	buf->canary = 'c';
	mp->head    = buf;
	mp->available++;
}

mpc_mp_buffer *_mpc_mempool_malloc(mpc_mempool *mp)
{
	return (mpc_mp_buffer *)mp->malloc_func(mp->size + sizeof(mpc_mp_buffer) );
}

mpc_mp_buffer *_mpc_mempool_pop(mpc_mempool *mp)
{
	mpc_mp_buffer *buf = mp->head;

	mp->head = mp->head->next;
	mp->available--;
	return buf;
}

int _mpc_mempool_is_below_minimum(mpc_mempool *mp)
{
	return mp->allocated + mp->available <= mp->min;
}

int _mpc_mempool_has_inertia(mpc_mempool *mp)
{
	return mp->inertia > 0;
}

int _mpc_mempool_malloc_phase(mpc_mempool *mp)
{
	return mp->inertia > mp->max_inertia / 2;
}

int _mpc_mempool_free_phase(mpc_mempool *mp)
{
	return mp->inertia < -mp->max_inertia / 2;
}

int _mpc_mempool_is_full(mpc_mempool *mp)
{
	return mp->available >= mp->max;
}

mpc_mp_buffer *_mpc_mempool_buffer_shift_back(void *buf)
{
	return (mpc_mp_buffer *)(buf - sizeof(mpc_mp_buffer) );
}

int mpc_mempool_empty(mpc_mempool *mp)
{
	mpc_mp_buffer *head;

	while(mp->head)
	{
		head = _mpc_mempool_pop(mp);
		mp->free_func(head);
	}
#ifdef MPC_MP_LOG
	fclose(mp->logfile);
#endif
	return 0;
}

int mpc_mempool_init(mpc_mempool *mp,
                     int min,
                     int max,
                     size_t size,
                     void *(*malloc_func)(size_t),
                     void (*free_func)(void *) )
{
	int i;

	mp->initialized = 1;
	mp->size        = size;
	mp->allocated   = 0;
	mp->max_inertia = max / 5;
	mp->inertia     = 0;
	mp->available   = 0;
	mp->head        = NULL;
	mp->min         = min;
	mp->max         = max;
	mp->malloc_func = malloc_func;
	mp->free_func   = free_func;
    mpc_common_spinlock_init(&mp->lock, 0);
#ifdef MPC_MP_LOG
	mpc_common_spinlock_lock(&mp_lock);
	char title[50];
	sprintf(title, "log_m%d_M%d_i%d_%d.csv", min, max, mp->max_inertia, mempool_number);
	printf("initializing mempool %s file descriptor %p\n", title, mp->logfile);
	mempool_number++;
	mp->logfile = fopen(title, "w+");
	char *header = "type,allocated,available,inertia\n";
	fprintf(mp->logfile, "%s", header);
	mpc_common_spinlock_unlock(&mp_lock);
#endif

	for(i = 0; i < mp->min; i++)
	{
		_mpc_mempool_add(mp, _mpc_mempool_malloc(mp) );
	}
	return 0;
}

void *mpc_mempool_alloc(mpc_mempool *mp)
{
	int effective_malloc = 0;

	mpc_common_spinlock_lock(&mp->lock);
	count_inertia++;
	if(mp->inertia < mp->max_inertia && !(count_inertia % 3 == 0) )
	{
		mp->inertia++;
	}
	if(mp->available <= 0)
	{
		effective_malloc = 1;
		_mpc_mempool_add(mp, _mpc_mempool_malloc(mp) );
	}
#ifdef MPC_MP_LOG
	fprintf(mp->logfile, "alloc,%d,%d,%d,%d\n", mp->allocated, mp->available, mp->inertia, effective_malloc);
#endif
	mpc_mp_buffer *ret = _mpc_mempool_pop(mp);
    assume(ret != NULL);
	mp->allocated++;
	mpc_common_spinlock_unlock(&mp->lock);
	return ret->buffer;
}

void mpc_mempool_free(mpc_mempool *mp, void *buffer)
{
	mpc_mp_buffer *buf            = _mpc_mempool_buffer_shift_back(buffer);
	int            effective_free = 0;

	if(buf->canary != 'c')
	{
		mpc_common_debug_fatal("mempool memory corrupted");
	}
	if(!mp)
	{
		mp = buf->mp;
	}
	mpc_common_spinlock_lock(&mp->lock);
	count_inertia++;
	if(mp->inertia > -mp->max_inertia && !(count_inertia % 3 == 0) )
	{
		mp->inertia--;
	}
	// if mp does not have enough buffers, restack
	// if mp does not have to many buffers and has inertia, restack
	if(_mpc_mempool_is_below_minimum(mp) || (!_mpc_mempool_free_phase(mp) && !_mpc_mempool_is_full(mp) ) )
	{
		_mpc_mempool_add(mp, buf);
	}
	else
	{
		mp->free_func(buf);
		effective_free = 1;
		if(!_mpc_mempool_is_below_minimum(mp) && _mpc_mempool_free_phase(mp) && mp->available)
		{
			mpc_mp_buffer *head = _mpc_mempool_pop(mp);
			mp->free_func(head);
		}
	}
#ifdef MPC_MP_LOG
	fprintf(mp->logfile, "free,%d,%d,%d,%d\n", mp->allocated, mp->available, mp->inertia, effective_free);
#endif
	mp->allocated--;
	mpc_common_spinlock_unlock(&mp->lock);
}

void *mpc_mempool_alloc_and_init(mpc_mempool *mp,
                                 int min,
                                 int max,
                                 int size,
                                 void *(*malloc_func)(size_t),
                                 void (*free_func)(void *) )
{
	if(!mp->initialized)
	{
		mpc_mempool_init(mp, min, max, size, malloc_func, free_func);
	}
	return mpc_mempool_alloc(mp);
}

mpc_mpool_elem_t *mpc_mempool_elem_shift_back(void *buf)
{
	return (mpc_mpool_elem_t *)buf - 1; 
}

static size_t mpc_mempool_elem_real_size(mpc_mpool_data_t *data) {
        return mpc_align_up_pow2(data->elem_size, data->alignment);
}

static void *mpc_mempool_chunk_elems(mpc_mpool_data_t *data, mpc_mpool_chunk_t *chunk) {
        size_t chunk_header_padding;
        uintptr_t chunk_header = (uintptr_t)chunk + sizeof(mpc_mpool_chunk_t) + 
                sizeof(mpc_mpool_elem_t);

        chunk_header_padding = mpc_padding(chunk_header, data->alignment);
        mpc_common_debug("MEMPOOL: padding=%d", chunk_header_padding);
        return (void *)(chunk_header + (intptr_t)chunk_header_padding -
                        sizeof(mpc_mpool_elem_t));
}

static mpc_mpool_elem_t *mpc_mempool_chunk_elem(mpc_mpool_data_t *mp, 
                                             mpc_mpool_chunk_t *chunk, 
                                             int index) 
{
        return (mpc_mpool_elem_t *)((uintptr_t)chunk->elems + (intptr_t)(index * 
                                    mpc_mempool_elem_real_size(mp)));
}

void mpc_mpool_grow(mpc_mpool_t *mp) 
{
        mpc_mpool_data_t *data = mp->data;
        mpc_mpool_chunk_t *chunk;
        int i;
        int chunk_size;
        void *ptr;

        if (data->num_elems >= data->max_elems) {
                return;
        }

        mpc_common_spinlock_lock(&data->lock);

        chunk_size = sizeof(mpc_mpool_chunk_t) + data->elem_per_chunk *
                mpc_mempool_elem_real_size(data);

        ptr   = data->malloc_func(chunk_size); 
        if (ptr == NULL) {
                mpc_common_debug_error("MEMPOOL: could not grow");
                return;
        }
        chunk = (mpc_mpool_chunk_t *)ptr;
        mpc_common_debug("MEMPOOL: grow chunk=%p, elems=%p, size=%d, elem_size=%d",
                         chunk, chunk->elems, chunk_size,
                         mpc_mempool_elem_real_size(data));
        chunk->elems = mpc_mempool_chunk_elems(data, chunk); 
        chunk->num_elems = data->elem_per_chunk; 

        for (i = 0; i < data->elem_per_chunk; i++) {
                mpc_mpool_elem_t *elem = mpc_mempool_chunk_elem(data, chunk, i);
                /* Push elem with no lock */
                ck_stack_push_spnc(&mp->free_list, (ck_stack_entry_t *)elem);
        }

        /* Add chunk to list */
        chunk->next = data->chunks;
        data->chunks = chunk;

        mpc_common_spinlock_unlock(&data->lock);
}

void *mpc_mpool_get_grow(mpc_mpool_t *mp) 
{
        mpc_mpool_elem_t *elem;
        mpc_mpool_grow(mp);

        if ((elem = (mpc_mpool_elem_t *)ck_stack_pop_mpmc(&mp->free_list)) == NULL) {
                return NULL;
        }

        return (void *)(elem + 1);
}

void *mpc_mpool_pop(mpc_mpool_t *mp) 
{ 
        mpc_mpool_elem_t *elem;

        elem = (mpc_mpool_elem_t *)ck_stack_pop_mpmc(&mp->free_list);
        if (elem == NULL) {
                return mpc_mpool_get_grow(mp);
        }

        elem->mp = mp;
        return (void *)(elem + 1);
}

void mpc_mpool_push(void *obj)
{
        mpc_mpool_elem_t *elem = mpc_mempool_elem_shift_back(obj);

        ck_stack_push_upmc(&elem->mp->free_list, (ck_stack_entry_t *)elem);
}

int mpc_mpool_init(mpc_mpool_t *mp, mpc_mpool_param_t *params) 
{
        int rc = 0;

        if (params->alignment == 0 || !mpc_is_pow2(params->alignment) ||
            params->elem_per_chunk == 0 || params->max_elems < params->elem_per_chunk) 
        {
                mpc_common_debug_error("COMMON: wrong parameter, could not "
                                       "create mpool");
                return 1;
        }

        ck_stack_init(&mp->free_list);

        mp->data = sctk_malloc(sizeof(mpc_mpool_data_t));
        if (mp->data == NULL) {
                mpc_common_debug("MEMPOOL: could not allocate data");
                rc = 1;
                goto err;
        }
        mp->data->alignment        = params->alignment;
        mp->data->num_elems        = 0;
        mp->data->num_chunks       = 0;
        mp->data->chunks           = NULL;
        mp->data->elem_per_chunk   = params->elem_per_chunk;
        mp->data->elem_size        = sizeof(mpc_mpool_elem_t) + params->elem_size;
        mp->data->max_elems        = params->max_elems;
        mp->data->free_func        = params->free_func;
        mp->data->malloc_func      = params->malloc_func;
        mpc_common_spinlock_init(&(mp->data->lock), 0);

        mpc_common_debug("MEMPOOL: user elem size=%d, alignment=%d, struct chunk size=%d", 
                         params->elem_size, params->alignment, sizeof(mpc_mpool_chunk_t));

        mpc_mpool_grow(mp);

err:
        return rc;
}

void mpc_mpool_fini(mpc_mpool_t *mp) 
{
        mpc_mpool_chunk_t *chunk, *next_chunk;

        /* Release the chunks */
        next_chunk = mp->data->chunks;
        while (next_chunk != NULL) {
               chunk = next_chunk;
               next_chunk = next_chunk->next;
               mp->data->free_func(chunk);
        }

        mpc_common_debug("MEMPOOL: fini mempool");

        sctk_free(mp->data);
}
