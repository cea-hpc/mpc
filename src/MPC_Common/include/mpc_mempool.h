#include <stdio.h>
#include <sctk_alloc.h>
#include <ck_stack.h>
#include <stdlib.h>
#include <mpc_common_spinlock.h>

#ifndef LCP_MEMPOOL_H
#define LCP_MEMPOOL_H

typedef struct mpc_mp_buffer_s{
    struct mpc_mp_buffer_s *next;
    char canary;
    struct mpc_mempool_s *mp;
    char buffer[0];
} mpc_mp_buffer;

typedef struct mpc_mempool_s {
    mpc_mp_buffer *head;
    int initialized;
    FILE *logfile;
    int min, max, allocated, available, inertia, max_inertia;
    void *(*malloc_func)(size_t size);
    void (*free_func)(void * pointer);
    size_t size;
    mpc_common_spinlock_t lock;
} mpc_mempool;


/**
 * @brief Add a buffer to the available buffers stack.
 * 
 * @param buf buffer to stack
 */
void _mpc_mempool_stack(void * buf);

/**
 * @brief Initialize a mempool.
 * 
 * @param mp pointer to mempool to be initialized. Must be allocated to `sizeof(mpc_mempool)`
 * @param min minimum number of used or available buffers (do not free under this number)
 * @param max maximum number of available buffers (systematically free over this number)
 * @param size buffers size
 * @param malloc_func function used to malloc buffers. Must be `void *func(int size)`
 * @param free_func function used to free buffers. Must be `int func(void *buffer)`
 * @return int MPC_SUCCESS in case of success
 */
int mpc_mempool_init(mpc_mempool *mp, 
    int min, 
    int max, 
    size_t size, 
    void *(*malloc_func)(size_t), 
    void (*free_func)(void *));

/**
 * @brief Allocate a fixed size buffer.
 * 
 * @param mempool mempool used to allocate the buffer
 * @return void* allocated buffer
 */
void *mpc_mempool_alloc(mpc_mempool *mempool);

/**
 * @brief Free a buffer allocated with a mpc_mempool.
 * 
 * @param mempool mempool used to allocate the buffer
 * @param buffer buffer to be freed
 */
void mpc_mempool_free(mpc_mempool *mempool, void *buffer);

/**
 * @brief Empty a mempool.
 * 
 * @param mp mempool to be emptied
 * @return int MPC_SUCCESS in case of success
 */
int mpc_mempool_empty(mpc_mempool *mp);

/**
 * @brief Allocate a buffer. Initialize the mempool if it is not initialized.
 * 
 * @param mp mempool used to allocate the buffer
 * @param min minimum number of used or available buffers (do not free under this number)
 * @param max maximum number of available buffers (systematically free over this number)
 * @param size buffers size
 * @param malloc_func function used to malloc buffers. Must be `void *func(int size)`
 * @param free_func function used to free buffers. Must be `int func(void *buffer)`
 * @return void* allocated buffer
 */
void *mpc_mempool_alloc_and_init(mpc_mempool *mp, 
                            int min, 
                                int max, 
                                int size, 
                                void *(*malloc_func)(size_t), 
                                void (*free_func)(void *)); 

typedef struct mpc_mpool_elem  mpc_mpool_elem_t;
typedef struct mpc_mpool_chunk mpc_mpool_chunk_t;
typedef struct mpc_mpool_param mpc_mpool_param_t;
typedef struct mpc_mpool_data  mpc_mpool_data_t;
typedef struct mpc_mpool       mpc_mpool_t;

/* Forward declaration for concurrency kit stack */
typedef struct ck_stack        ck_stack_t;
typedef struct ck_stack_entry  ck_stack_entry_t;

struct mpc_mpool_elem {
    ck_stack_entry_t *next;
    mpc_mpool_t *mp;
};

struct mpc_mpool_chunk {
        struct mpc_mpool_chunk *next;
        mpc_mpool_elem_t *elems;
        unsigned num_elems;
};

struct mpc_mpool_param {
    size_t alignment;
    size_t   elem_size;
    int      elem_per_chunk;
    int      max_elems;
    void *(*malloc_func)(size_t size);
    void (*free_func)(void * pointer);
};

struct mpc_mpool_data {
    size_t   alignment;
    size_t   elem_size;
    int      elem_per_chunk;
    int      max_elems;
    int      num_elems;
    int      num_chunks;
    mpc_mpool_chunk_t *chunks;
    void *(*malloc_func)(size_t size);
    void (*free_func)(void * pointer);
    mpc_common_spinlock_t lock;
};

struct mpc_mpool {
        ck_stack_t        free_list;
        mpc_mpool_data_t *data;
};

//FIXME: redefine scope for MPC return code 
int mpc_mpool_init(mpc_mpool_t *mp, mpc_mpool_param_t *params);
void mpc_mpool_fini(mpc_mpool_t *mp);
void mpc_mpool_push(void *obj);
void *mpc_mpool_pop(mpc_mpool_t *mp);
#endif
