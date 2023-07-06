#include "mpc_ofi_request.h"
#include "mpc_common_spinlock.h"

#include "lowcomm_config.h"
#include <mpc_common_debug.h>
#include <mpc_ofi_domain.h>

#include <sctk_alloc.h>

#include <string.h>

/*****************
 * REQUEST CACHE *
 *****************/

int mpc_ofi_request_cache_init(struct mpc_ofi_request_cache_t *cache, struct mpc_ofi_domain_t *domain)
{
   cache->request_count = domain->config->request_cache_size;
   cache->requests = sctk_malloc(cache->request_count * sizeof(struct mpc_ofi_request_t));
   assume(cache->requests);
   memset(cache->requests, 0, sizeof(struct mpc_ofi_request_t) * cache->request_count);

   cache->domain = domain;

   unsigned int i = 0;

   for(i = 0 ; i < cache->request_count; i++)
   {
      /* All requests are free */
      cache->requests[i].free = 1;
      cache->requests[i].was_allocated = 0;
      mpc_common_spinlock_init(&cache->requests[i].lock, 0);
   }

   return 0;
}

int mpc_ofi_request_cache_release(struct mpc_ofi_request_cache_t *cache)
{
   unsigned int i = 0;

#if 0
   for(i = 0 ; i < cache->request_count; i++)
   {
      /* All requests are free ? */
      assert(cache->requests[i].free);
   }
#endif

   free(cache->requests);

   return 0;
}

static inline void __request_clear(struct mpc_ofi_request_t * req,
                                   struct mpc_ofi_domain_t * domain,
                                   int (*comptetion_cb)(struct mpc_ofi_request_t *, void *),
                                    void *arg,
                                    int (*comptetion_cb_ext)(struct mpc_ofi_request_t *, void *),
                                    void *arg_ext)
{
   req->done = 0;
   req->free = 0;
   req->domain = domain;
   req->arg = arg;
   req->comptetion_cb = comptetion_cb;
   req->comptetion_cb_ext = comptetion_cb_ext;
   req->arg_ext = arg_ext;
   req->mr_count = 0;
   req->was_allocated = 0;
}


struct mpc_ofi_request_t * mpc_ofi_request_acquire(struct mpc_ofi_request_cache_t *cache,
                                                           int (*comptetion_cb)(struct mpc_ofi_request_t *, void *),
                                                           void *arg,
                                                           int (*comptetion_cb_ext)(struct mpc_ofi_request_t *, void *),
                                                           void *arg_ext)
{
   unsigned int i = 0;
   struct mpc_ofi_request_t * req = NULL;

   for(i = 0 ; i < cache->request_count; i++)
   {
      req = &cache->requests[i];

      //mpc_common_debug_warning("%d == %d == %d", i, req->done, req->free);

      if(req->free)
      {
         mpc_common_spinlock_lock(&req->lock);

         if(!req->free)
         {
            mpc_common_spinlock_unlock(&req->lock);
            continue;
         }

         __request_clear(req, cache->domain, comptetion_cb, arg, comptetion_cb_ext, arg_ext);

         mpc_common_spinlock_unlock(&req->lock);

         return req;
      }
   }

   /* If we are here we exhausted request cache allocate a request */
   req = sctk_malloc(sizeof(struct mpc_ofi_request_t));
   assume(req);
   mpc_common_spinlock_init(&req->lock, 0);
   __request_clear(req, cache->domain, comptetion_cb, arg, comptetion_cb_ext, arg_ext);
   req->was_allocated = 1;

   return req;
}

