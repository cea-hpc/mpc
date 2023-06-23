#include "mpc_ofi_request.h"
#include "mpc_common_spinlock.h"

#include <mpc_common_debug.h>
#include <string.h>

/*****************
 * REQUEST CACHE *
 *****************/

int mpc_ofi_request_cache_init(struct mpc_ofi_request_cache_t *cache)
{
   memset(cache->requests, 0, sizeof(struct mpc_ofi_request_t) * MPC_OFI_REQUEST_CACHE_SIZE);

   int i = 0;

   for(i = 0 ; i < MPC_OFI_REQUEST_CACHE_SIZE; i++)
   {
      /* All requests are free */
      cache->requests[i].free = 1;
      mpc_common_spinlock_init(&cache->requests[i].lock, 0);
   }

   return 0;
}

int mpc_ofi_request_cache_release(struct mpc_ofi_request_cache_t *cache)
{
   int i = 0;

   for(i = 0 ; i < MPC_OFI_REQUEST_CACHE_SIZE; i++)
   {
      /* All requests are free ? */
      assert(cache->requests[i].free);
   }

   return 0;
}

struct mpc_ofi_request_t * mpc_ofi_request_acquire(struct mpc_ofi_request_cache_t *cache,
                                                           int (*comptetion_cb)(struct mpc_ofi_request_t *, void *),
                                                           void *arg,
                                                           int (*comptetion_cb_ext)(struct mpc_ofi_request_t *, void *),
                                                           void *arg_ext)
{
   int i = 0;

   for(i = 0 ; i < MPC_OFI_REQUEST_CACHE_SIZE; i++)
   {
      struct mpc_ofi_request_t * req = &cache->requests[i];

      //mpc_common_debug_warning("%d == %d == %d", i, req->done, req->free);

      if(req->free)
      {
         mpc_common_spinlock_lock(&req->lock);

         if(!req->free)
         {
            mpc_common_spinlock_unlock(&req->lock);
            continue;
         }

         req->done = 0;
         req->free = 0;
         req->arg = arg;
         req->comptetion_cb = comptetion_cb;
         req->comptetion_cb_ext = comptetion_cb_ext;
         req->arg_ext = arg_ext;
         req->mr_count = 0;

         mpc_common_spinlock_unlock(&req->lock);

         return req;
      }
   }

   return NULL;
}

