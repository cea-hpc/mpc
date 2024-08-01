/* ############################# MPC License ############################## */
/* # Fri Oct  6 12:44:31 CEST 2023                                        # */
/* # Copyright or (C) or Copr. Commissariat a l'Energie Atomique          # */
/* #                                                                      # */
/* # IDDN.FR.001.230040.000.S.P.2007.000.10000                            # */
/* # This file is part of the MPC Runtime.                                # */
/* #                                                                      # */
/* # This software is governed by the CeCILL-C license under French law   # */
/* # and abiding by the rules of distribution of free software.  You can  # */
/* # use, modify and/ or redistribute the software under the terms of     # */
/* # the CeCILL-C license as circulated by CEA, CNRS and INRIA at the     # */
/* # following URL http://www.cecill.info.                                # */
/* #                                                                      # */
/* # The fact that you are presently reading this means that you have     # */
/* # had knowledge of the CeCILL-C license and that you accept its        # */
/* # terms.                                                               # */
/* #                                                                      # */
/* # Maintainers:                                                         # */
/* # - CARRIBAULT Patrick patrick.carribault@cea.fr                       # */
/* # - JAEGER Julien julien.jaeger@cea.fr                                 # */
/* # - PERACHE Marc marc.perache@cea.fr                                   # */
/* # - ROUSSEL Adrien adrien.roussel@cea.fr                               # */
/* # - TABOADA Hugo hugo.taboada@cea.fr                                   # */
/* #                                                                      # */
/* # Authors:                                                             # */
/* # - Jean-Baptiste Besnard <jbbesnard@paratools.com>                    # */
/* #                                                                      # */
/* ######################################################################## */
#include "mpc_ofi_request.h"
#include "mpc_common_spinlock.h"

#include "lowcomm_config.h"
#include <mpc_common_debug.h>
#include <mpc_ofi_domain.h>

#include <sctk_alloc.h>

#include <string.h>

#define MPC_MODULE "Lowcomm/Ofi/Request"

/*****************
 * REQUEST CACHE *
 *****************/

int _mpc_ofi_request_cache_init(struct _mpc_ofi_request_cache_t *cache, struct _mpc_ofi_domain_t *domain)
{
   cache->request_count = domain->config->request_cache_size;
   cache->requests = sctk_malloc(cache->request_count * sizeof(struct _mpc_ofi_request_t));
   assume(cache->requests);
   memset(cache->requests, 0, sizeof(struct _mpc_ofi_request_t) * cache->request_count);

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

int _mpc_ofi_request_cache_release(struct _mpc_ofi_request_cache_t *cache)
{
#if 0
   unsigned int i = 0;

   for(i = 0 ; i < cache->request_count; i++)
   {
      /* All requests are free ? */
      assert(cache->requests[i].free);
   }
#endif

   sctk_free(cache->requests);

   return 0;
}

static inline void __request_clear(struct _mpc_ofi_request_t * req,
                                   struct _mpc_ofi_domain_t * domain,
                                   int (*comptetion_cb)(struct _mpc_ofi_request_t *, void *),
                                    void *arg,
                                    int (*comptetion_cb_ext)(struct _mpc_ofi_request_t *, void *),
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


struct _mpc_ofi_request_t * _mpc_ofi_request_acquire(struct _mpc_ofi_request_cache_t *cache,
                                                           int (*comptetion_cb)(struct _mpc_ofi_request_t *, void *),
                                                           void *arg,
                                                           int (*comptetion_cb_ext)(struct _mpc_ofi_request_t *, void *),
                                                           void *arg_ext)
{
   unsigned int i = 0;
   struct _mpc_ofi_request_t * req = NULL;

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
   req = sctk_malloc(sizeof(struct _mpc_ofi_request_t));
   assume(req);
   mpc_common_spinlock_init(&req->lock, 0);
   __request_clear(req, cache->domain, comptetion_cb, arg, comptetion_cb_ext, arg_ext);
   req->was_allocated = 1;

   return req;
}
