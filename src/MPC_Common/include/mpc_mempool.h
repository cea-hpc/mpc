#ifndef LCP_MEMPOOL_H
#define LCP_MEMPOOL_H

#include <stdio.h>
#include <sctk_alloc.h>
#include <mpc_common_spinlock.h>

typedef struct mpc_mempool_elem  mpc_mempool_elem_t;
typedef struct mpc_mempool_chunk mpc_mempool_chunk_t;
typedef struct mpc_mempool_param mpc_mempool_param_t;
typedef struct mpc_mempool_data  mpc_mempool_data_t;
typedef struct mpc_mempool       mpc_mempool_t;

/* Forward declaration for concurrency kit stack */
typedef struct ck_stack        ck_stack_t;

//FIXME: CK implementation is based on the fact that ck_stack_entry_t has the
//       same structure has mpc_mempool_elem_t (first elem is a next pointer).
//       As a consequence, a simple cast works fine but it does not seems like
//       a good solution. 
//       On the plus side, this allows to have all implementations working with
//       the same data structures.
struct mpc_mempool_elem {
    struct mpc_mempool_elem *next;
    char         canary;
    mpc_mempool_t *mp;
};

struct mpc_mempool_chunk {
        struct mpc_mempool_chunk *next;
        mpc_mempool_elem_t       *elems;
        unsigned                  num_elems;
};

struct mpc_mempool_param {
    size_t   alignment;
    size_t   elem_size;
    int      elem_per_chunk;
    int      max_elems;
    int      min_elems;
    void *(*malloc_func)(size_t size);
    void (*free_func)(void * pointer);
};

struct mpc_mempool_data {
    size_t             alignment;      /* Data alignment */
    size_t             elem_size;      /* Total element size */
    int                elem_per_chunk; /* Allocated on each grow */
    int                min_elems;      
    int                max_elems;       
    int                num_elems;      /* Current number of allocated elements */
    int                num_chunks;
    mpc_mempool_chunk_t *chunks;       /* Pointer to first chunk */
#ifdef MPC_MP_LOG
    FILE              *logfile;
#endif
    int available, inertia, max_inertia;
    int initialized;
    void *(*malloc_func)(size_t size);
    void (*free_func)(void * pointer);
    mpc_common_spinlock_t lock;
};

struct mpc_mempool {
#if MPC_USE_CK
        ck_stack_t        *ck_free_list;
#endif
        mpc_mempool_elem_t *free_list;
        mpc_mempool_data_t *data;
};

/**
 * @brief Add a buffer to the available buffers stack.
 * 
 * @param buf buffer to stack
 */
void _mpc_mempool_t_stack(void * buf);

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
int mpc_mempool_init(mpc_mempool_t *mp, 
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
void *mpc_mempool_alloc(mpc_mempool_t *mempool);

/**
 * @brief Free a buffer allocated with a mpc_mempool.
 * 
 * @param mempool mempool used to allocate the buffer
 * @param buffer buffer to be freed
 */
void mpc_mempool_free(mpc_mempool_t *mempool, void *buffer);

/**
 * @brief Empty a mempool.
 * 
 * @param mp mempool to be emptied
 * @return int MPC_SUCCESS in case of success
 */
int mpc_mempool_empty(mpc_mempool_t *mp);

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
void *mpc_mempool_alloc_and_init(mpc_mempool_t *mp, 
                                 int min, 
                                 int max, 
                                 int size, 
                                 void *(*malloc_func)(size_t), 
                                 void (*free_func)(void *)); 

int mpc_mpool_init(mpc_mempool_t *mp, mpc_mempool_param_t *params);
size_t mpc_mpool_get_elem_size(mpc_mempool_t *mp);
void mpc_mpool_fini(mpc_mempool_t *mp);
void mpc_mpool_push(void *obj);
void *mpc_mpool_pop(mpc_mempool_t *mp);

#endif
