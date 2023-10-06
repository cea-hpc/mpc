/* ############################# MPC License ############################## */
/* # Fri Oct  6 12:44:33 CEST 2023                                        # */
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
#ifndef MPC_OFI_REQUEST
#define MPC_OFI_REQUEST

#include <mpc_common_debug.h>
#include <mpc_common_spinlock.h>
#include <sctk_alloc.h>

/*********************
 * THE REQUEST CACHE *
 *********************/

#define MPC_OFI_IOVEC_SIZE 16
#define MPC_OFI_REQUEST_PAD 64

struct _mpc_ofi_request_cache_t;
struct _mpc_ofi_domain_t;


struct _mpc_ofi_request_t
{
   /* State and lock */
   volatile int done;
   volatile int free;
   mpc_common_spinlock_t lock;
   char _pad_[MPC_OFI_REQUEST_PAD];
   /* Internal completion callback */
   void * arg;
   int (*comptetion_cb)(struct _mpc_ofi_request_t * req, void * arg);
   /* External completion callback */
   void * arg_ext;
   int (*comptetion_cb_ext)(struct _mpc_ofi_request_t * req, void * arg_ext);
   /* MR to be freed */
   struct fid_mr *mr[MPC_OFI_IOVEC_SIZE];
   unsigned int mr_count;
   struct _mpc_ofi_domain_t * domain;
   short was_allocated;
};

__UNUSED__ static int _mpc_ofi_request_test(struct _mpc_ofi_request_t*req)
{
   return req->done;
}

struct _mpc_ofi_request_cache_t
{
   unsigned int request_count;
   struct _mpc_ofi_request_t *requests;
   struct _mpc_ofi_domain_t *domain;
   mpc_common_spinlock_t lock;
};


int _mpc_ofi_request_cache_init(struct _mpc_ofi_request_cache_t *cache, struct _mpc_ofi_domain_t *domain);


struct _mpc_ofi_request_t * _mpc_ofi_request_acquire(struct _mpc_ofi_request_cache_t *cache,
                                                   int (*comptetion_cb)(struct _mpc_ofi_request_t *, void *),
                                                   void *arg,
                                                   int (*comptetion_cb_ext)(struct _mpc_ofi_request_t *, void *),
                                                   void *arg_ext);


__UNUSED__ static int _mpc_ofi_request_done(struct _mpc_ofi_request_t *request)
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

   if(request->was_allocated)
   {
      sctk_free(request);
   }

   return ret;
}


int _mpc_ofi_request_cache_release(struct _mpc_ofi_request_cache_t *cache);


#endif /* MPC_OFI_REQUEST */
