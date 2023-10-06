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
#include "mpc_ofi_context.h"

#include "mpc_common_datastructure.h"
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


int _mpc_ofi_context_init(struct _mpc_ofi_context_t *ctx,
                        char * provider,
                        _mpc_ofi_context_policy_t policy,
                        _mpc_ofi_context_recv_callback_t recv_callback,
                        void * callback_arg,
                        _mpc_ofi_context_accept_callback_t accept_cb,
                        void *accept_cb_arg,
                        struct _mpc_lowcomm_config_struct_net_driver_ofi * config)
{
   memset(ctx, 0, sizeof(struct _mpc_ofi_context_t));

   ctx->rail_config = config;

   /* General parameters initialization */

   if(provider)
   {
      ctx->provider = strdup(provider);
   }

   mpc_common_spinlock_init(&ctx->lock, 0);

   ctx->recv_callback = recv_callback;
   ctx->callback_arg = callback_arg;

   ctx->accept_cb = accept_cb;
   ctx->accept_cb_arg = accept_cb_arg;

   ctx->numa_count = 1;

   if(MPC_OFI_POLICY_COUNT <= policy)
   {
      mpc_common_errorpoint("No such policy");
      return 1;
   }


   ctx->ctx_policy = policy;

   if(_mpc_ofi_dns_init(&ctx->dns))
   {
      mpc_common_errorpoint("Failed to initialize central DNS");
      return 1;
   }

   /* Allocate and set libfabric configuration */

   struct fi_info * hints = _mpc_ofi_get_requested_hints(provider, config->endpoint_type);


   if( fi_getinfo(FI_VERSION(1, 5), NULL, NULL, 0, hints, &ctx->config) < 0)
   {
      (void)fprintf(stderr, "OFI Comm Library failed to start provider '%s' with given constraints.", hints->fabric_attr->prov_name);
      mpc_common_errorpoint("Initialization failed.");
      return 1;
   }

   /* Flag for Memory Registration */

   if(!strcmp(provider, "tcp"))
   {
      /* TCP does not like VIRT address offsetting and needs to be reset here */
      ctx->config->domain_attr->mr_mode = FI_MR_LOCAL | FI_MR_PROV_KEY | FI_MR_ALLOCATED;
      ctx->config->domain_attr->mr_key_size = 0;
   }


   ctx->config->domain_attr->control_progress = FI_PROGRESS_MANUAL;
   ctx->config->domain_attr->data_progress = FI_PROGRESS_MANUAL;

   fi_freeinfo(hints);

   /* Start Fabric */

   MPC_OFI_CHECK_RET(fi_fabric(ctx->config->fabric_attr, &ctx->fabric, NULL));

   /* Setup domains */


   ctx->domain = calloc(1, sizeof(struct _mpc_ofi_domain_t));

   if(ctx->domain == NULL)
   {
      perror("calloc");
      return 1;
   }

	if(_mpc_ofi_domain_init(ctx->domain, ctx, ctx->rail_config))
	{
		mpc_common_errorpoint("Failed to initialize a domain");
		return 1;
	}


   struct fid_ep * ep = NULL;

   if(!ctx->domain->is_passive_endpoint)
   {
      ep = ctx->domain->ep;
   }

   /* Now register my rank as bound to this address (possibly with a NULL endpoint for passive case ) */
   if( _mpc_ofi_dns_register(&ctx->dns, mpc_lowcomm_monitor_get_uid(), ctx->domain->address, ctx->domain->address_len, ep) < 0 )
   {
      mpc_common_errorpoint("Failed to register new view's address");
      return -1;
   }


  	mpc_common_debug_info("Using driver relying on %s(%s)\n", ctx->config->fabric_attr->prov_name, ctx->config->fabric_attr->name);

   return 0;
}


int _mpc_ofi_context_release(struct _mpc_ofi_context_t *ctx)
{

	if(_mpc_ofi_domain_release(ctx->domain))
	{
		mpc_common_errorpoint("Failed to release a domain");
	}

   /* Close the fabric (needs all the sub objects to be released)*/
   TODO("Understand why we get -EBUSY");
   fi_close(&ctx->fabric->fid);

   if(_mpc_ofi_dns_release(&ctx->dns))
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
