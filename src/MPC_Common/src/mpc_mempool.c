#include "mpc_mempool.h"
#include "mpc_common_debug.h"
#include "mpc_common_spinlock.h"
#include "mpc_common_helper.h"
#include <stddef.h>
#include <stdint.h>

#if MPC_USE_CK
#include <ck_stack.h>
#endif

int count_inertia;
#ifdef MPC_MP_LOG
int mempool_number;
#endif

mpc_mempool_elem_t *mpc_mempool_elem_shift_back(void *buf)
{
	return (mpc_mempool_elem_t *)buf - 1;
}

void _mpc_mempool_add(mpc_mempool_t *mp, mpc_mempool_elem_t *elem)
{
	elem->next    = mp->free_list;
	elem->mp      = mp;
	elem->canary  = 'c';
	mp->free_list = elem;
	mp->data->available++;
}

mpc_mempool_elem_t *_mpc_mempool_malloc(mpc_mempool_t *mp)
{
	return (mpc_mempool_elem_t *)
                mp->data->malloc_func(mp->data->elem_size +
                                      sizeof(mpc_mempool_elem_t));
}

mpc_mempool_elem_t *_mpc_mempool_pop(mpc_mempool_t *mp)
{
	mpc_mempool_elem_t *elem = mp->free_list;

	if ( elem == NULL )
	{
		mpc_common_debug_warning("Current mempool element is already NULL");
		mp->data->available--;
		return NULL;
	}

	mp->free_list = mp->free_list->next;
	mp->data->available--;
	return elem;
}

int _mpc_mempool_is_below_minimum(mpc_mempool_t *mp)
{
	return mp->data->num_elems + mp->data->available <= mp->data->min_elems;
}

int _mpc_mempool_has_inertia(mpc_mempool_t *mp)
{
	return mp->data->inertia > 0;
}

int _mpc_mempool_malloc_phase(mpc_mempool_t *mp)
{
	return mp->data->inertia > mp->data->max_inertia / 2;
}

int _mpc_mempool_free_phase(mpc_mempool_t *mp)
{
	return mp->data->inertia < -mp->data->max_inertia / 2;
}

int _mpc_mempool_is_full(mpc_mempool_t *mp)
{
	return mp->data->available >= mp->data->max_elems;
}

int mpc_mempool_empty(mpc_mempool_t *mp)
{
	mpc_mempool_elem_t *head;

	while(mp->free_list)
	{
		head = _mpc_mempool_pop(mp);
		mp->data->free_func(head);
	}
#ifdef MPC_MP_LOG
	fclose(mp->logfile);
#endif
	return 0;
}

int mpc_mempool_init(mpc_mempool_t *mp,
                     int min,
                     int max,
                     size_t size,
                     void *(*malloc_func)(size_t),
                     void (*free_func)(void *) )
{
	int i;

	assert(min >= 0);
	assert(max >= min);

        mp->data = sctk_malloc(sizeof(mpc_mempool_data_t));
        if (mp->data == NULL) {
                mpc_common_debug_error("MEMPOOL: could not allocate "
                                       "memory pool data.");
                return 1;
        }

	mp->data->elem_size   = size;
	mp->data->num_elems   = 0;
	mp->data->max_inertia = max / 5;
	mp->data->inertia     = 0;
	mp->data->available   = 0;
	mp->data->min_elems   = min;
	mp->data->max_elems   = max;
	mp->data->malloc_func = malloc_func;
	mp->data->free_func   = free_func;

	mp->free_list   = NULL;

        mpc_common_spinlock_init(&mp->data->lock, 0);
#ifdef MPC_MP_LOG
	mpc_common_spinlock_lock(&mp_lock);
	char title[50];
	sprintf(title, "log_m%d_M%d_i%d_%d.csv", min, max, mp->data->max_inertia, mempool_number);
	printf("initializing mempool %s file descriptor %p\n", title, mp->data->logfile);
	mempool_number++;
	mp->data->logfile = fopen(title, "w+");
	char *header = "type,allocated,available,inertia\n";
	fprintf(mp->data->logfile, "%s", header);
	mpc_common_spinlock_unlock(&mp_lock);
#endif

	for(i = 0; i < mp->data->min_elems; i++)
	{
		_mpc_mempool_add(mp, _mpc_mempool_malloc(mp) );
	}

        mp->data->initialized = 1;
	return 0;
}

void *mpc_mempool_alloc(mpc_mempool_t *mp)
{
#ifdef MPC_MP_LOG
	int effective_malloc = 0;
#endif
        mpc_mempool_data_t *data = mp->data;

	mpc_common_spinlock_lock(&data->lock);
	count_inertia++;
	if(data->inertia < data->max_inertia && !(count_inertia % 3 == 0) )
	{
		data->inertia++;
	}
	if(data->available <= 0)
	{
#ifdef MPC_MP_LOG
		effective_malloc = 1;
#endif
		_mpc_mempool_add(mp, _mpc_mempool_malloc(mp) );
	}
#ifdef MPC_MP_LOG
	fprintf(data->logfile, "alloc,%d,%d,%d\n", data->available, data->inertia, effective_malloc);
#endif
	mpc_mempool_elem_t *ret = _mpc_mempool_pop(mp);
        assume(ret != NULL);
	mpc_common_spinlock_unlock(&data->lock);
	return ret + 1;
}

void mpc_mempool_free(mpc_mempool_t *mp, void *obj)
{
	mpc_mempool_elem_t *elem = mpc_mempool_elem_shift_back(obj);
#ifdef MPC_MP_LOG
	int            effective_free = 0;
#endif

	if(elem->canary != 'c')
	{
		mpc_common_debug_fatal("mempool memory corrupted");
	}
	if(!mp)
	{
		mp = elem->mp;
	}
	mpc_common_spinlock_lock(&mp->data->lock);

	count_inertia++;

	if(mp->data->inertia > -mp->data->max_inertia && !(count_inertia % 3 == 0) )
	{
		mp->data->inertia--;
	}
	// if mp does not have enough buffers, restack
	// if mp does not have to many buffers and has inertia, restack
	if(_mpc_mempool_is_below_minimum(mp) || (!_mpc_mempool_free_phase(mp) && !_mpc_mempool_is_full(mp) ) )
	{
		_mpc_mempool_add(mp, elem);
	}
	else
	{
		mp->data->free_func(elem);
#ifdef MPC_MP_LOG
		effective_free = 1;
#endif
		if(!_mpc_mempool_is_below_minimum(mp) && _mpc_mempool_free_phase(mp) && mp->data->available)
		{
			mpc_mempool_elem_t *head = _mpc_mempool_pop(mp);
			mp->data->free_func(head);
		}
	}
#ifdef MPC_MP_LOG
	fprintf(mp->logfile, "free,%d,%d,%d,%d\n", mp->data->allocated,
                mp->data->available, mp->data->inertia, effective_free);
#endif
	mpc_common_spinlock_unlock(&mp->data->lock);
}

void *mpc_mempool_alloc_and_init(mpc_mempool_t *mp,
                                 int min,
                                 int max,
                                 int size,
                                 void *(*malloc_func)(size_t),
                                 void (*free_func)(void *) )
{
	if(!mp->data->initialized)
	{
		mpc_mempool_init(mp, min, max, size, malloc_func, free_func);
	}
	return mpc_mempool_alloc(mp);
}

//TODO: do a ascii schematic of the memory.

size_t mpc_mpool_get_elem_size(mpc_mempool_t *mp)
{
        return mp->data->elem_size - sizeof(mpc_mempool_elem_t);
}

static size_t mpc_mempool_elem_real_size(mpc_mempool_data_t *data) {
        return mpc_common_align_up_pow2(data->elem_size, data->alignment);
}

static void *mpc_mempool_chunk_elems(mpc_mempool_data_t *data, mpc_mempool_chunk_t *chunk) {
        size_t chunk_header_padding;
        //FIXME: approximative use of intptr_t and uintptr_t
        uintptr_t chunk_header = (uintptr_t)chunk + sizeof(mpc_mempool_chunk_t) +
                sizeof(mpc_mempool_elem_t);

        chunk_header_padding = mpc_common_padding(chunk_header, data->alignment);
        return (void *)(chunk_header + (intptr_t)chunk_header_padding -
                        sizeof(mpc_mempool_elem_t));
}

static mpc_mempool_elem_t *mpc_mempool_chunk_elem(mpc_mempool_data_t *mp,
                                             mpc_mempool_chunk_t *chunk,
                                             int index)
{
        return (mpc_mempool_elem_t *)((uintptr_t)chunk->elems + (intptr_t)(index *
                                    mpc_mempool_elem_real_size(mp)));
}

void mpc_mpool_grow(mpc_mempool_t *mp)
{
        mpc_mempool_data_t *data = mp->data;
        mpc_mempool_chunk_t *chunk;
        int i;
        int chunk_size;
        void *ptr;

        if (data->num_elems >= data->max_elems) {
                return;
        }

#if MPC_USE_CK
        mpc_common_spinlock_lock(&data->lock);
        /* Grow only if free list empty. This prevents multiple producer from
         * growing the queue at the same time */
        if (!CK_STACK_ISEMPTY(mp->ck_free_list)) {
                return;
        }
#endif

        //FIXME: explain why data->alignment needs to be in size.
        chunk_size = sizeof(mpc_mempool_chunk_t) + data->alignment +
                data->elem_per_chunk * mpc_mempool_elem_real_size(data);

        ptr   = data->malloc_func(chunk_size);
        if (ptr == NULL) {
                mpc_common_debug_error("MEMPOOL: could not grow");
                return;
        }
        chunk = (mpc_mempool_chunk_t *)ptr;
        chunk->elems = mpc_mempool_chunk_elems(data, chunk);
        chunk->num_elems = data->elem_per_chunk;

        for (i = 0; i < data->elem_per_chunk; i++) {
                mpc_mempool_elem_t *elem = mpc_mempool_chunk_elem(data, chunk, i);
                /* Push elem with no lock */
#if MPC_USE_CK
                ck_stack_push_mpmc(mp->ck_free_list, (ck_stack_entry_t *)elem);
#else
                elem->next = mp->free_list;
                mp->free_list = elem;
#endif
        }

        /* Add chunk to list */
        chunk->next      = data->chunks;
        data->chunks     = chunk;
        data->num_elems += data->elem_per_chunk;

#if MPC_USE_CK
        mpc_common_spinlock_unlock(&data->lock);
#endif
}

#if MPC_USE_CK
static mpc_mempool_elem_t *mpc_mpool_get_grow(mpc_mempool_t *mp)
{
        mpc_mempool_elem_t *elem;
        mpc_mpool_grow(mp);

        if ((elem = (mpc_mempool_elem_t *)ck_stack_pop_mpmc(mp->ck_free_list)) == NULL) {
                return NULL;
        }

        elem->mp = mp;
        return elem;
}
#endif

void *mpc_mpool_pop(mpc_mempool_t *mp)
{
        mpc_mempool_elem_t *elem;

#if MPC_USE_CK
        elem = (mpc_mempool_elem_t *)ck_stack_pop_mpmc(mp->ck_free_list);
#else
        mpc_common_spinlock_lock(&mp->data->lock);
        elem = mp->free_list;
#endif
        if (elem == NULL) {
#if MPC_USE_CK
                elem = mpc_mpool_get_grow(mp);
#else
                mpc_mpool_grow(mp);
                elem = mp->free_list;
                if (elem == NULL) {
                        return NULL;
                }
#endif
        }

        elem->mp = mp;
#if !MPC_USE_CK
        mp->free_list = mp->free_list->next;
        mpc_common_spinlock_unlock(&mp->data->lock);
#endif

        return (void *)(elem + 1);
}

void mpc_mpool_push(void *obj)
{
        mpc_mempool_elem_t *elem = mpc_mempool_elem_shift_back(obj);

#if MPC_USE_CK
        ck_stack_push_mpmc(elem->mp->ck_free_list, (ck_stack_entry_t *)elem);
#else
        mpc_mempool_t *mp = elem->mp;
        mpc_common_spinlock_lock(&mp->data->lock);
        elem->next = mp->free_list;
        mp->free_list = elem;
        mpc_common_spinlock_unlock(&mp->data->lock);
#endif
}

int mpc_mpool_init(mpc_mempool_t *mp, mpc_mempool_param_t *params)
{
        int rc = 0;

        if (params->alignment == 0 || !mpc_common_is_powerof2(params->alignment) ||
            params->elem_per_chunk == 0 || params->max_elems < params->elem_per_chunk)
        {
                mpc_common_debug_error("COMMON: wrong parameter, could not "
                                       "create mpool.");
                return 1;
        }

#if MPC_USE_CK
        mp->ck_free_list = sctk_malloc(sizeof(ck_stack_t));
        ck_stack_init(mp->ck_free_list);
#else
        mp->free_list = NULL;
#endif

        mp->data = sctk_malloc(sizeof(mpc_mempool_data_t));
        if (mp->data == NULL) {
                mpc_common_debug_error("MEMPOOL: could not allocate data.");
                rc = 1;
                goto err;
        }
        mp->data->alignment        = params->alignment;
        mp->data->num_elems        = 0;
        mp->data->num_chunks       = 0;
        mp->data->chunks           = NULL;
        mp->data->elem_per_chunk   = params->elem_per_chunk;
        mp->data->elem_size        = sizeof(mpc_mempool_elem_t) + params->elem_size;
        mp->data->max_elems        = params->max_elems;
        mp->data->free_func        = params->free_func;
        mp->data->malloc_func      = params->malloc_func;
        mpc_common_spinlock_init(&(mp->data->lock), 0);

        mpc_common_debug("MEMPOOL: user elem size=%d, alignment=%d, struct chunk size=%d.",
                         params->elem_size, params->alignment, sizeof(mpc_mempool_chunk_t));

        mpc_mpool_grow(mp);

err:
        return rc;
}

void mpc_mpool_fini(mpc_mempool_t *mp)
{
        mpc_mempool_chunk_t *chunk, *next_chunk;

        /* Release the chunks */
        next_chunk = mp->data->chunks;
        while (next_chunk != NULL) {
               chunk = next_chunk;
               next_chunk = next_chunk->next;
               mp->data->free_func(chunk);
        }

        sctk_free(mp->data);

#if MPC_USE_CK
        sctk_free(mp->ck_free_list);
#endif
}
