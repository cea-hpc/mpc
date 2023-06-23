#include "mpc_ofi_context.h"

#include "mpc_lowcomm_monitor.h"
#include "mpc_ofi_dns.h"
#include "mpc_ofi_domain.h"
#include "mpc_ofi_helpers.h"
#include "mpc_ofi_request.h"

#include <mpc_common_debug.h>

#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <rdma/fabric.h>


int mpc_ofi_context_init(struct mpc_ofi_context_t *ctx,
                        uint32_t domain_count,
                        char * provider,
                        mpc_ofi_context_policy_t policy,
                        mpc_ofi_context_recv_callback_t recv_callback,
                        void * callback_arg)
{
   memset(ctx, 0, sizeof(struct mpc_ofi_context_t));

   /* General parameters initialization */

   if(provider)
   {
      ctx->provider = strdup(provider);
   }

   mpc_common_spinlock_init(&ctx->lock, 0);

   ctx->recv_callback = recv_callback;
   ctx->callback_arg = callback_arg;


   ctx->numa_count = 1;

   if(MPC_OFI_POLICY_COUNT <= policy)
   {
      mpc_common_errorpoint("No such policy");
      return 1;
   }


   ctx->ctx_policy = policy;

   if(mpc_ofi_dns_init(&ctx->dns, MPC_OFI_DNS_RESOLUTION_CPLANE))
   {
      mpc_common_errorpoint("Failed to initialize central DNS");
      return 1;
   }

   /* Allocate and set libfabric configuration */

   struct fi_info * hints = mpc_ofi_get_requested_hints(provider);
   mpc_ofi_decode_mr_mode(hints->domain_attr->mr_mode);


   if( fi_getinfo(FI_VERSION(1, 5), NULL, NULL, 0, hints, &ctx->config) < 0)
   {
      (void)fprintf(stderr, "OFI Comm Library failed to start provider '%s' with given constraints.", hints->fabric_attr->prov_name);
      mpc_common_errorpoint("Initialization failed.");
      return 1;
   }

   /* Flag for Memory Registration */
   ctx->config->domain_attr->mr_mode = FI_MR_VIRT_ADDR | FI_MR_PROV_KEY | FI_MR_ALLOCATED | FI_MR_RAW;
   ctx->config->domain_attr->mr_key_size = 0;

   fi_freeinfo(hints);

   /* Start Fabric */

   MPC_OFI_CHECK_RET(fi_fabric(ctx->config->fabric_attr, &ctx->fabric, NULL));

   /* Setup domains */

   ctx->domain_count = domain_count;

   ctx->domains = calloc(ctx->domain_count, sizeof(struct mpc_ofi_domain_t));

   if(ctx->domains == NULL)
   {
      perror("calloc");
      return 1;
   }

   unsigned int i = 0;

   for(i = 0 ; i < ctx->domain_count; i++)
   {
      if(mpc_ofi_domain_init(&ctx->domains[i], ctx))
      {
         mpc_common_errorpoint("Failed to initialize a domain");
         return 1;
      }
   }

  	mpc_common_debug_info(stderr, "Using driver relying on %s(%s)\n", ctx->config->fabric_attr->prov_name, ctx->config->fabric_attr->name);

   return 0;
}


int mpc_ofi_context_release(struct mpc_ofi_context_t *ctx)
{
   unsigned int i = 0;

   for( i = 0 ; i < ctx->domain_count; i++)
   {
      if(mpc_ofi_domain_release(&ctx->domains[i]))
      {
         mpc_common_errorpoint("Failed to release a domain");
      }
   }

   /* Close the fabric (needs all the sub objects to be released)*/
   MPC_OFI_CHECK_RET(fi_close(&ctx->fabric->fid));

   if(mpc_ofi_dns_release(&ctx->dns))
   {
      mpc_common_errorpoint("Failed to release central DNS");
      return 1;
   }

   if(ctx->provider)
   {
      free(ctx->provider);
   }

   return 0;
}

static inline struct mpc_ofi_domain_t * __mpc_ofi_context_get_next_domain_rr(struct mpc_ofi_context_t *ctx)
{
   int target = 0;
   mpc_common_spinlock_lock(&ctx->lock);
   target = ctx->current_domain;
   ctx->current_domain = (ctx->current_domain + 1)%ctx->domain_count;
   mpc_common_spinlock_unlock(&ctx->lock);

   return &ctx->domains[target];
}


static inline struct mpc_ofi_domain_t * __mpc_ofi_context_get_next_domain(struct mpc_ofi_context_t *ctx)
{
   switch (ctx->ctx_policy)
   {
      case MPC_OFI_POLICY_RR:
         return __mpc_ofi_context_get_next_domain_rr(ctx);
      default:
         mpc_common_errorpoint("Not implemented");
         return NULL;
   }

   return NULL;
}


/**************************
 * THE OFI CL VIEW *
 **************************/





int mpc_ofi_view_init(struct mpc_ofi_view_t *view, struct mpc_ofi_context_t *ctx, uint64_t rank)
{
   view->domain = __mpc_ofi_context_get_next_domain(ctx);

   if(!view->domain)
   {
      mpc_common_errorpoint("Could not acquire domain from CTX");
      return -1;
   }

   view->rank = rank;

   /* Now register my rank as bound to this address */
   if( mpc_ofi_dns_register(&ctx->dns, rank, view->domain->address, view->domain->address_len) < 0 )
   {
      mpc_common_errorpoint("Failed to register new view's address");
      return -1;
   }

   return 0;
}

int mpc_ofi_view_wait_for_rank_registration(struct mpc_ofi_view_t *view,
                                           uint64_t rank, uint32_t timeout_sec)
{
   char buff[MPC_OFI_ADDRESS_LEN];
   size_t len = MPC_OFI_ADDRESS_LEN;
   uint32_t cnt = 0;

   do
   {
      int ret = mpc_ofi_dns_resolve(view->domain->ddns.main_dns, rank, buff, &len);
      if(ret == 0)
      {
         return 0;
      }
      sleep(1);
      cnt++;
   }while(cnt < timeout_sec);

   return -1;
}

int mpc_ofi_view_test(struct mpc_ofi_view_t *view,  struct mpc_ofi_request_t *req, int *done)
{
   *done = mpc_ofi_request_test(req);

   //if(!*done)
   {
      if(mpc_ofi_domain_poll(view->domain, 0))
      {
         mpc_common_errorpoint("Error polling for send");
         return -1;
      }
   }

   return 0;
}

int mpc_ofi_view_wait(struct mpc_ofi_view_t *view,  struct mpc_ofi_request_t *req)
{
   int done = 0;
   do
   {
      if( mpc_ofi_view_test(view,  req, &done) )
      {
         mpc_common_errorpoint("Error waiting for request");
         return -1;
      }
   }while(!done);

   return 0;
}


int mpc_ofi_view_send(struct mpc_ofi_view_t *view, void * buffer, size_t size, uint64_t dest, struct mpc_ofi_request_t **req,
                        int (*comptetion_cb_ext)(struct mpc_ofi_request_t *, void *),
                        void *arg_ext)
{
   return mpc_ofi_domain_send(view->domain, dest, buffer, size, req, comptetion_cb_ext, arg_ext);
}

int mpc_ofi_view_sendv(struct mpc_ofi_view_t *view,
                        uint64_t dest,
                        const struct iovec *iov,
                        size_t iovcnt,
                        struct mpc_ofi_request_t **req,
                        int (*comptetion_cb_ext)(struct mpc_ofi_request_t *, void *),
                        void *arg_ext)
{
   return mpc_ofi_domain_sendv(view->domain, dest, iov, iovcnt, req, comptetion_cb_ext, arg_ext);
}


int mpc_ofi_view_release(struct mpc_ofi_view_t *view)
{
   view->rank = 0;

   return 0;
}
