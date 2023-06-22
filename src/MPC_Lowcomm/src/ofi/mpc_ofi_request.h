#ifndef MPC_OFI_REQUEST
#define MPC_OFI_REQUEST

#include <mpc_common_spinlock.h>

/*********************
 * THE REQUEST CACHE *
 *********************/

#define MPC_OFI_REQUEST_PAD 64

struct mpc_ofi_request_cache_t;

struct mpc_ofi_request_t
{
   /* State and lock */
   volatile int done;
   volatile int free;
   mpc_common_spinlock_t lock;
   char _pad_[MPC_OFI_REQUEST_PAD];
   /* Internal completion callback */
   void * arg;
   int (*comptetion_cb)(void * arg);
};

static int mpc_ofi_request_test(struct mpc_ofi_request_t*req)
{
   return req->done;
}

#define MPC_OFI_REQUEST_CACHE_SIZE 512

struct mpc_ofi_request_cache_t
{
   struct mpc_ofi_request_t requests[MPC_OFI_REQUEST_CACHE_SIZE];
   mpc_common_spinlock_t lock;
};

int mpc_ofi_request_cache_init(struct mpc_ofi_request_cache_t *cache);


struct mpc_ofi_request_t * mpc_ofi_request_acquire(struct mpc_ofi_request_cache_t *cache,
                                                           int (*comptetion_cb)(void *),
                                                           void *arg);


static inline int mpc_ofi_request_done(struct mpc_ofi_request_t *request)
{
   int ret = 0;

   /* Call request completion CB if present */
   if(request->comptetion_cb)
   {
      ret = (request->comptetion_cb)(request->arg);
   }

   mpc_common_spinlock_lock_yield(&request->lock);
   request->done = 1;
   request->free = 1;
   mpc_common_spinlock_unlock(&request->lock);

   return ret;
}


int mpc_ofi_request_cache_release(struct mpc_ofi_request_cache_t *cache);


#endif /* MPC_OFI_REQUEST */