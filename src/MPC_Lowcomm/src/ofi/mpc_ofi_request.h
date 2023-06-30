#ifndef MPC_OFI_REQUEST
#define MPC_OFI_REQUEST

#include <mpc_common_spinlock.h>
#include <mpc_common_debug.h>

/*********************
 * THE REQUEST CACHE *
 *********************/

#define MPC_OFI_IOVEC_SIZE 16
#define MPC_OFI_REQUEST_PAD 64

struct mpc_ofi_request_cache_t;
struct mpc_ofi_domain_t;


struct mpc_ofi_request_t
{
   /* State and lock */
   volatile int done;
   volatile int free;
   mpc_common_spinlock_t lock;
   char _pad_[MPC_OFI_REQUEST_PAD];
   /* Internal completion callback */
   void * arg;
   int (*comptetion_cb)(struct mpc_ofi_request_t * req, void * arg);
   /* External completion callback */
   void * arg_ext;
   int (*comptetion_cb_ext)(struct mpc_ofi_request_t * req, void * arg_ext);
   /* MR to be freed */
   struct fid_mr *mr[MPC_OFI_IOVEC_SIZE];
   unsigned int mr_count;
   struct mpc_ofi_domain_t * domain;
};

__UNUSED__ static int mpc_ofi_request_test(struct mpc_ofi_request_t*req)
{
   return req->done;
}

#define MPC_OFI_REQUEST_CACHE_SIZE 16

struct mpc_ofi_request_cache_t
{
   struct mpc_ofi_request_t requests[MPC_OFI_REQUEST_CACHE_SIZE];
   struct mpc_ofi_domain_t *domain;
   mpc_common_spinlock_t lock;
};




int mpc_ofi_request_cache_init(struct mpc_ofi_request_cache_t *cache, struct mpc_ofi_domain_t *domain);


struct mpc_ofi_request_t * mpc_ofi_request_acquire(struct mpc_ofi_request_cache_t *cache,
                                                   int (*comptetion_cb)(struct mpc_ofi_request_t *, void *),
                                                   void *arg,
                                                   int (*comptetion_cb_ext)(struct mpc_ofi_request_t *, void *),
                                                   void *arg_ext);


__UNUSED__ static int mpc_ofi_request_done(struct mpc_ofi_request_t *request)
{
   int ret = 0;

   mpc_common_spinlock_lock_yield(&request->lock);

   assume(!request->free);

   /* Call request completion CB if present */
   if(request->comptetion_cb)
   {
      ret = (request->comptetion_cb)(request, request->arg);
   }

   if(request->comptetion_cb_ext)
   {
      ret = (request->comptetion_cb_ext)(request, request->arg_ext);
   }

   request->done = 1;
   request->free = 1;

   mpc_common_spinlock_unlock(&request->lock);

   return ret;
}


int mpc_ofi_request_cache_release(struct mpc_ofi_request_cache_t *cache);


#endif /* MPC_OFI_REQUEST */