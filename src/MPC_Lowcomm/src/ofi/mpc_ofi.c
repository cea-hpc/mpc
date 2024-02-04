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
/* # - Gilles Moreau <gilles.moreau@cea.fr>                               # */
/* # - Jean-Baptiste Besnard <jbbesnard@paratools.com>                    # */
/* # - Paul Canat <pcanat@paratools.com>                                  # */
/* #                                                                      # */
/* ######################################################################## */
#include "mpc_ofi.h"


#include <limits.h>
#include <sctk_alloc.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "endpoint.h"
#include "lcr/lcr_component.h"
#include "mpc_common_debug.h"
#include "mpc_lowcomm_monitor.h"
#include "mpc_ofi_context.h"
#include "mpc_ofi_dns.h"
#include "mpc_ofi_domain.h"
#include "mpc_ofi_helpers.h"
#include "rail.h"
#include "rdma/fabric.h"
#include "rdma/fi_domain.h"
#include "rdma/fi_errno.h"


static inline char *__gen_rail_target_name(sctk_rail_info_t *rail, char *buff, int bufflen)
{
	(void)snprintf(buff, bufflen, "ofi-%d", rail->rail_number);
	return buff;
}

struct _mpc_ofi_deffered_completion_s
{
   lcr_ofi_am_hdr_t hdr;
   lcr_completion_t *comp;
   struct iovec full_iov[MPC_OFI_IOVEC_SIZE + 2];
};


static inline void __init_ofi_endpoint(sctk_rail_info_t *rail, _mpc_lowcomm_ofi_endpoint_info_t *ofi_data)
{
   mpc_mempool_init(&ofi_data->bsend, MPC_OFI_EP_MEMPOOL_MIN, MPC_OFI_EP_MEMPOOL_MAX, rail->runtime_config_driver_config->driver.value.ofi.bcopy_size + sizeof(lcr_ofi_am_hdr_t), sctk_malloc, sctk_free);
   mpc_mempool_init(&ofi_data->deffered, MPC_OFI_EP_MEMPOOL_MIN, MPC_OFI_EP_MEMPOOL_MAX, sizeof(struct _mpc_ofi_deffered_completion_s), sctk_malloc, sctk_free);
}

static void __add_route(mpc_lowcomm_peer_uid_t dest_uid, sctk_rail_info_t *rail, _mpc_lowcomm_endpoint_state_t state)
{
	static mpc_common_spinlock_t add_route_lock = MPC_COMMON_SPINLOCK_INITIALIZER;

	mpc_common_spinlock_lock(&add_route_lock);


	_mpc_lowcomm_endpoint_t * previous_ep = sctk_rail_get_any_route_to_process(rail, dest_uid);

	if(!previous_ep)
	{
		_mpc_lowcomm_endpoint_t *new_route = sctk_malloc(sizeof(_mpc_lowcomm_endpoint_t) );
		assume(new_route != NULL);
		_mpc_lowcomm_endpoint_init(new_route, dest_uid, rail, _MPC_LOWCOMM_ENDPOINT_DYNAMIC);

		__init_ofi_endpoint(rail, &new_route->data.ofi);

		/* Make sure the route is flagged connected */
		_mpc_lowcomm_endpoint_set_state(new_route, state);

		sctk_rail_add_dynamic_route(rail, new_route);

	}
	else
	{
		mpc_common_tracepoint_fmt("Set state %d", state);
		/* Just update the state */
		_mpc_lowcomm_endpoint_set_state(previous_ep, state);
	}

	mpc_common_spinlock_unlock(&add_route_lock);
}

struct _mpc_ofi_net_infos
{
   char addr[MPC_OFI_ADDRESS_LEN];
   size_t size;
   mpc_lowcomm_peer_uid_t my_uid;
   int can_initiate_connection;
};

void _mpc_ofi_wait_for_connection(struct sctk_rail_info_s *rail, struct _mpc_ofi_domain_conn * cstate)
{
	mpc_common_tracepoint_fmt("wait start to %s", mpc_lowcomm_peer_format(cstate->remote_uid));

	/* First create a non-connected route */
	__add_route(cstate->remote_uid, rail, _MPC_LOWCOMM_ENDPOINT_CONNECTING);

   while(_mpc_ofi_domain_conn_get(cstate) != MPC_OFI_DOMAIN_ENDPOINT_CONNECTED)
   {
      _mpc_ofi_domain_poll(rail->network.ofi.ctx.domain, -1);
   }

	/* The route is now connected */
	__add_route(cstate->remote_uid, rail, _MPC_LOWCOMM_ENDPOINT_CONNECTED);

	mpc_common_tracepoint_fmt("wait DONE to %s", mpc_lowcomm_peer_format(cstate->remote_uid));


	/* The route will be added when moving to connected in the EQ polling */
}

#define RAIL_NAME_LEN 128

void _mpc_ofi_connect_on_demand_passive(struct sctk_rail_info_s *rail, mpc_lowcomm_peer_uid_t dest)
{
   struct _mpc_ofi_context_t * ctx = &rail->network.ofi.ctx;


   struct _mpc_ofi_net_infos my_infos = { 0 };
   my_infos.size = MPC_OFI_ADDRESS_LEN;
   my_infos.my_uid =mpc_lowcomm_monitor_get_uid();
   my_infos.can_initiate_connection = 0;


	mpc_common_tracepoint_fmt("connectionless connect to %s", mpc_lowcomm_peer_format(dest));

   int addr_found = 0;

   _mpc_ofi_dns_resolve(&ctx->dns, my_infos.my_uid, my_infos.addr, &my_infos.size, &addr_found);

   if( !addr_found )
   {
      mpc_common_debug_fatal("Failed to resolve OFI address for local UID %d", mpc_lowcomm_monitor_get_uid());
   }

	mpc_lowcomm_monitor_retcode_t ret = MPC_LOWCOMM_MONITOR_RET_SUCCESS;

	char rail_name[RAIL_NAME_LEN];
	mpc_lowcomm_monitor_response_t resp = mpc_lowcomm_monitor_ondemand(dest,
	                                                                   __gen_rail_target_name(rail, rail_name, RAIL_NAME_LEN),
	                                                                   (char*)&my_infos,
	                                                                   sizeof(struct _mpc_ofi_net_infos),
	                                                                   &ret);

	if(!resp)
	{
		mpc_common_debug_fatal("Could not connect to UID %lu (timeout)", dest);
	}

	mpc_lowcomm_monitor_args_t *content = mpc_lowcomm_monitor_response_get_content(resp);

	if(content->on_demand.retcode != MPC_LOWCOMM_MONITOR_RET_SUCCESS)
	{
		mpc_common_debug_fatal("Could not connect to UID %lu", dest);
	}

   struct _mpc_ofi_net_infos * remote_info = (struct _mpc_ofi_net_infos *)content->on_demand.data;


	/* Now add the response to the DNS */
	if( _mpc_ofi_dns_register(&ctx->dns, dest, remote_info->addr, remote_info->size, ctx->domain->ep /* The Connectionless EP*/) )
	{
		mpc_common_errorpoint_fmt("Failed to register OFI address for remote UID %d", dest);
	}

	mpc_lowcomm_monitor_response_free(resp);

   /* Now add route as remote is in the DNS */
   __add_route(dest, rail, _MPC_LOWCOMM_ENDPOINT_CONNECTED);
}



void _mpc_ofi_connect_on_demand(struct sctk_rail_info_s *rail, mpc_lowcomm_peer_uid_t dest)
{
   struct _mpc_ofi_context_t * ctx = &rail->network.ofi.ctx;

	if(!ctx->domain->is_passive_endpoint)
	{
		/* Here we just want to share addresses and add them in  the DNS
		   No connect/accept and connection mitigation*/
		_mpc_ofi_connect_on_demand_passive(rail, dest);
		return;
	}

	/* Here we start the passive endpoint case */
	mpc_common_tracepoint_fmt("passive endpoint connect to %s", mpc_lowcomm_peer_format(dest));


	int is_first_to_connect = 0;

	/* First make sure we are not already being connected to */
	struct _mpc_ofi_domain_conn * cstate = _mpc_ofi_domain_conntrack_add(&ctx->domain->conntrack,
                                                                              				 0,
                                                                              				 dest,
                                                                              				 MPC_OFI_DOMAIN_ENDPOINT_BOOTSTRAP,
																														 &is_first_to_connect);
	if(!is_first_to_connect)
	{
		_mpc_ofi_wait_for_connection(rail, cstate);
		return;
	}

	/* I attempt to bootstrap the connection */

   struct _mpc_ofi_net_infos my_infos = { 0 };
   my_infos.size = 0;
   my_infos.my_uid =mpc_lowcomm_monitor_get_uid();
   my_infos.can_initiate_connection = 0;

	mpc_lowcomm_monitor_retcode_t ret = MPC_LOWCOMM_MONITOR_RET_SUCCESS;

   char rail_name[RAIL_NAME_LEN];
	mpc_lowcomm_monitor_response_t resp = mpc_lowcomm_monitor_ondemand(dest,
	                                                                   __gen_rail_target_name(rail, rail_name, RAIL_NAME_LEN),
	                                                                   (char*)&my_infos,
	                                                                   sizeof(struct _mpc_ofi_net_infos),
	                                                                   &ret);

	if(!resp)
	{
		mpc_common_debug_fatal("Could not connect to UID %lu (timeout)", dest);
	}

	mpc_lowcomm_monitor_args_t *content = mpc_lowcomm_monitor_response_get_content(resp);

	if(content->on_demand.retcode != MPC_LOWCOMM_MONITOR_RET_SUCCESS)
	{
		mpc_common_debug_fatal("Could not connect to UID %lu", dest);
	}

	struct _mpc_ofi_net_infos * remote_info = (struct _mpc_ofi_net_infos *)content->on_demand.data;

	if(remote_info->can_initiate_connection)
	{
		mpc_common_tracepoint_fmt("Initiating the connection to %s", mpc_lowcomm_peer_format(dest));

		_mpc_ofi_domain_conn_set(cstate, MPC_OFI_DOMAIN_ENDPOINT_CONNECTING);

		/* Now add route as remote connecting */
		__add_route(dest, rail, _MPC_LOWCOMM_ENDPOINT_CONNECTING);

		/* We were elected as the initiator */
		/* May create a new endpoint if the endpoint is passive (returns the connectionless endpoint otherwise) */
		_mpc_ofi_domain_connect(rail->network.ofi.ctx.domain, dest, remote_info->addr, remote_info->size);
		_mpc_ofi_wait_for_connection(rail, cstate);
	}
	else
	{
		mpc_common_tracepoint_fmt("Inhibiting the connection to %s", mpc_lowcomm_peer_format(dest));

		/* We are waiting for the remote endpoint to join us */
		_mpc_ofi_wait_for_connection(rail, cstate);
	}

	mpc_lowcomm_monitor_response_free(resp);
}

static int __ofi_on_demand_callback_connectionless(mpc_lowcomm_peer_uid_t from,
																	struct _mpc_ofi_net_infos *infos,
																	char *return_data,
																	int return_data_len,
																	sctk_rail_info_t * rail)
{
   struct _mpc_ofi_context_t * ctx = &rail->network.ofi.ctx;

   /* Register remote info */
   if( _mpc_ofi_dns_register(&ctx->dns, from, infos->addr, infos->size, ctx->domain->ep) )
   {
      mpc_common_errorpoint_fmt("CALLBACK Failed to register OFI address for remote UID %d", from);
      return 1;
   }

   /* Now send local info */
   assume(sizeof(struct _mpc_ofi_net_infos) < (unsigned int)return_data_len);
   struct _mpc_ofi_net_infos *my_infos = (struct _mpc_ofi_net_infos *)return_data;
   my_infos->size = MPC_OFI_ADDRESS_LEN;

   int addr_found = 0;
   _mpc_ofi_dns_resolve(&ctx->dns, mpc_lowcomm_monitor_get_uid(), my_infos->addr, &my_infos->size, &addr_found);

   if( !addr_found )
   {
      mpc_common_debug_fatal("Failed to resolve OFI address for local UID %d", mpc_lowcomm_monitor_get_uid());
   }

	/* Now add route as remote is in the DNS and we have a related endpoint
		at current state, meaning the endpoint is a connectionless one */
	__add_route(from, rail, _MPC_LOWCOMM_ENDPOINT_CONNECTED);

	return 0;
}


static int __ofi_on_demand_callback(mpc_lowcomm_peer_uid_t from,
                                    char *data,
                                    char *return_data,
                                    int return_data_len,
                                    void *prail)
{
	sctk_rail_info_t *rail = prail;
   struct _mpc_ofi_context_t * ctx = &rail->network.ofi.ctx;
   struct _mpc_ofi_net_infos *infos = (struct _mpc_ofi_net_infos*) data;

	if(!ctx->domain->is_passive_endpoint)
	{
		/* This is the connectionless mode */
		return __ofi_on_demand_callback_connectionless(from, infos, return_data, return_data_len, prail);
	}

	int is_first = 0;
	int is_connecting = 0;

	/* If we are here our main concern in to acquire the lock to ourselves */
	struct _mpc_ofi_domain_conn * cstate = _mpc_ofi_domain_conntrack_add(&ctx->domain->conntrack,
                                                                              				 0,
                                                                              				 from,
                                                                              				 MPC_OFI_DOMAIN_ENDPOINT_ACCEPTING,
																														 &is_first);
   struct _mpc_ofi_net_infos *my_infos = (struct _mpc_ofi_net_infos *)return_data;
   /* Now send local info */
   assume(sizeof(struct _mpc_ofi_net_infos) < (unsigned int)return_data_len);



	mpc_common_tracepoint_fmt("Incoming on-demand from %s is_first: %d", mpc_lowcomm_peer_format(cstate->remote_uid), is_first);

	if(!is_first)
	{

		if(_mpc_ofi_domain_conn_get(cstate) == MPC_OFI_DOMAIN_ENDPOINT_BOOTSTRAP )
		{
			/* Here both endpoints are connecting concurrently we only keep the largest UID one */
			if(mpc_lowcomm_monitor_get_uid() < from)
			{
				is_connecting = 0;
			}
			else
			{
				is_connecting = 1;
			}
			mpc_common_tracepoint_fmt("Incoming on-demand from %s already BOOTSTRAPING decision is %d (%lu < %lu)", mpc_lowcomm_peer_format(cstate->remote_uid), is_connecting, mpc_lowcomm_monitor_get_uid(), from);

		}
		else
		{
			mpc_common_debug_fatal("Non coherent endpoint state");
		}

	}
	else
	{
		is_connecting = 1;
	}



   my_infos->size = MPC_OFI_ADDRESS_LEN;
	my_infos->can_initiate_connection = is_connecting;

   int addr_found = 0;
   _mpc_ofi_dns_resolve(&ctx->dns, mpc_lowcomm_monitor_get_uid(), my_infos->addr, &my_infos->size, &addr_found);

   if( !addr_found )
   {
      mpc_common_debug_fatal("Failed to resolve OFI address for local UID %d", mpc_lowcomm_monitor_get_uid());
   }

		mpc_common_tracepoint_fmt("Returning to %s can_connect: %d", mpc_lowcomm_peer_format(cstate->remote_uid), is_connecting);


	return 0;
}

static inline int __free_bsend_buffer_mempool(__UNUSED__ struct _mpc_ofi_request_t *req, void *pbuffer){
   mpc_mempool_free(NULL, pbuffer);
   return 0;
}


ssize_t _mpc_ofi_send_am_bcopy(_mpc_lowcomm_endpoint_t *ep,
                              uint8_t id,
                              lcr_pack_callback_t pack,
                              void *arg,
                              __UNUSED__ unsigned flags)
{

   sctk_rail_info_t *rail = ep->rail;
	uint32_t payload_length = 0;

   lcr_ofi_am_hdr_t *hdr = mpc_mempool_alloc(&ep->data.ofi.bsend);

	if (hdr == NULL) {
	       mpc_common_debug_error("Could not allocate buffer.");
	       payload_length = -1;
	       goto err;
	}

   memset(hdr, 0, sizeof(lcr_ofi_am_hdr_t));

	hdr->am_id = id;
	hdr->length = pack(hdr->data, arg);
   payload_length = hdr->length;

   if( _mpc_ofi_domain_send(rail->network.ofi.ctx.domain, ep->dest, hdr, hdr->length + sizeof(lcr_ofi_am_hdr_t), NULL, __free_bsend_buffer_mempool, hdr) )
   {
      mpc_common_errorpoint("Failed to send a message");
      return -1;
   }

err:
	return payload_length;
}

static inline int __deffered_completion_cb(__UNUSED__ struct _mpc_ofi_request_t *req, void *pcomp)
{
   struct _mpc_ofi_deffered_completion_s *comp = (struct _mpc_ofi_deffered_completion_s *)pcomp;
   comp->comp->comp_cb(comp->comp);
   mpc_mempool_free(NULL, comp);
   return 0;
}


int _mpc_ofi_send_am_zcopy(_mpc_lowcomm_endpoint_t *ep,
                                    uint8_t id,
                                    void *header,
                                    unsigned header_length,
                                    const struct iovec *iov,
                                    size_t iovcnt,
                                    __UNUSED__  unsigned flags,
                                    lcr_completion_t *comp)
{
	struct _mpc_ofi_deffered_completion_s *completion =  mpc_mempool_alloc(&ep->data.ofi.deffered);
   assume(completion != NULL);

   completion->hdr.am_id = id;
   completion->comp = comp;

   assume((iovcnt + 2) < MPC_OFI_IOVEC_SIZE);

   size_t total_size = 0;
   int total_iov_size = 1;

   completion->full_iov[0].iov_base = &completion->hdr;
   completion->full_iov[0].iov_len = sizeof(lcr_ofi_am_hdr_t);

   int iov_offset = 1;

   if(header_length)
   {
      completion->full_iov[iov_offset].iov_base = (void*)header;
      completion->full_iov[iov_offset].iov_len = header_length;
      total_size += completion->full_iov[iov_offset].iov_len;
      iov_offset++;
      total_iov_size++;
   }

   unsigned int i = 0;
   for(i = 0 ; i < iovcnt; i++)
   {
      completion->full_iov[iov_offset].iov_base = iov[i].iov_base;
      completion->full_iov[iov_offset].iov_len = iov[i].iov_len;
      total_size += completion->full_iov[iov_offset].iov_len;
      iov_offset++;
      total_iov_size++;
   }

   completion->hdr.length = total_size;

   if(_mpc_ofi_domain_sendv(ep->rail->network.ofi.ctx.domain,
                         ep->dest,
                         completion->full_iov,
                         total_iov_size,
                         NULL,
                         __deffered_completion_cb,
                         (void*)completion))
   {
      mpc_common_errorpoint("Failed to sendv a message");
      return -1;
   }

   return MPC_LOWCOMM_SUCCESS;
}

int  _mpc_ofi_pin(struct sctk_rail_info_s *rail, struct sctk_rail_pin_ctx_list *list, 
                  const void *addr, size_t size,
                   unsigned flags)
{
        UNUSED(flags);
   if(_mpc_ofi_domain_memory_register(rail->network.ofi.ctx.domain,
                                     (void *)addr,
                                     size,
                                     FI_REMOTE_READ | FI_REMOTE_WRITE,
                                     &list->pin.ofipin.ofi))
   {
      mpc_common_errorpoint("Failed to register memory for RDMA");
      mpc_common_debug_abort();
   }

   /* And get the key */
   list->pin.ofipin.shared.ofi_remote_mr_key = fi_mr_key(list->pin.ofipin.ofi);
   list->pin.ofipin.shared.addr = (void *)addr;
   list->pin.ofipin.shared.size = size;

   return 0;
}

int _mpc_ofi_unpin(struct sctk_rail_info_s *rail, struct sctk_rail_pin_ctx_list *list)
{

   if(_mpc_ofi_domain_memory_unregister(rail->network.ofi.ctx.domain, list->pin.ofipin.ofi))
   {
      mpc_common_errorpoint("Failed to register memory for RDMA");
      mpc_common_debug_abort();
   }

   return 0;
}


int _mpc_ofi_pack_rkey(sctk_rail_info_t *rail,
                      lcr_memp_t *memp, void *dest)
{
   UNUSED(rail);
   memcpy(dest, &memp->pin.ofipin.shared, sizeof(struct _mpc_ofi_shared_pinning_context));
   return sizeof(struct _mpc_ofi_shared_pinning_context);
}

int _mpc_ofi_unpack_rkey(sctk_rail_info_t *rail,
                        lcr_memp_t *memp, void *dest)
{
        UNUSED(rail);
        memcpy(&memp->pin.ofipin.shared, dest, sizeof(struct _mpc_ofi_shared_pinning_context));
        return sizeof(struct _mpc_ofi_shared_pinning_context);
}

uint64_t __convert_rma_offset(struct _mpc_ofi_domain_t *domain, void * buff_base_addr, uint64_t remote_offset)
{
   //_mpc_ofi_decode_mr_mode(domain->ctx->config->domain_attr->mr_mode);

   if(domain->ctx->config->domain_attr->mr_mode & FI_MR_VIRT_ADDR)
   {
      mpc_common_tracepoint_fmt("returning converted %lu %lu %lu", buff_base_addr, remote_offset, buff_base_addr + remote_offset);
      return (uint64_t)buff_base_addr + remote_offset;
   }

   mpc_common_tracepoint_fmt("returning offset %lu", remote_offset);
   return remote_offset;
}


int _mpc_ofi_get_zcopy(_mpc_lowcomm_endpoint_t *ep,
                           uint64_t local_addr,
                           uint64_t remote_offset,
                           lcr_memp_t *local_key,
                           lcr_memp_t *remote_key,
                           size_t size,
                           lcr_completion_t *comp)
{
        UNUSED(local_key);
	struct _mpc_ofi_deffered_completion_s *completion =  mpc_mempool_alloc(&ep->data.ofi.deffered);
   assume(completion != NULL);

   assert(size);

   comp->sent = size;
   completion->comp = comp;

   uint64_t network_offset = __convert_rma_offset(ep->rail->network.ofi.ctx.domain, remote_key->pin.ofipin.shared.addr, remote_offset);

   if(_mpc_ofi_domain_get(ep->rail->network.ofi.ctx.domain,
                     (void *)local_addr,
                     size,
                     ep->dest,
                     network_offset,
                     remote_key->pin.ofipin.shared.ofi_remote_mr_key,
                     NULL,
                     __deffered_completion_cb,
                     completion))
   {
      mpc_common_errorpoint("Failed to issue RDMA read");
      return -1;
   }

   _mpc_ofi_domain_poll(ep->rail->network.ofi.ctx.domain, FI_SEND);


   return MPC_LOWCOMM_SUCCESS;
}

int _mpc_ofi_send_put_zcopy(_mpc_lowcomm_endpoint_t *ep,
                           uint64_t local_addr,
                           uint64_t remote_offset,
                           lcr_memp_t *local_key,
                           lcr_memp_t *remote_key,
                           size_t size,
                           lcr_completion_t *comp)
{
        UNUSED(local_key);
	struct _mpc_ofi_deffered_completion_s *completion =  mpc_mempool_alloc(&ep->data.ofi.deffered);
   assume(completion != NULL);

   assert(size);

   comp->sent = size;
   completion->comp = comp;

   uint64_t network_offset = __convert_rma_offset(ep->rail->network.ofi.ctx.domain, remote_key->pin.ofipin.shared.addr, remote_offset);


   if(_mpc_ofi_domain_put(ep->rail->network.ofi.ctx.domain,
                     (void*)local_addr,
                     size,
                     ep->dest,
                     network_offset,
                     remote_key->pin.ofipin.shared.ofi_remote_mr_key,
                     NULL,
                     __deffered_completion_cb,
                     completion))
   {
      mpc_common_errorpoint("Failed to issue RDMA write");
      return -1;
   }

   _mpc_ofi_domain_poll(ep->rail->network.ofi.ctx.domain, FI_SEND);


   return MPC_LOWCOMM_SUCCESS;
}

int _mpc_ofi_iface_is_reachable(sctk_rail_info_t *rail, uint64_t uid) {
        //FIXME: check whether getting connection info should be done here. For
        //       now just return true.
        UNUSED(rail); UNUSED(uid);
        return 1;
}


int __passive_endpoint_did_accept_a_connection(mpc_lowcomm_peer_uid_t uid, void * prail)
{
   sctk_rail_info_t *rail = (sctk_rail_info_t *)prail;
	__add_route(uid, rail, _MPC_LOWCOMM_ENDPOINT_CONNECTED);
   mpc_common_tracepoint_fmt("[OFI] enabling incoming connection endpoint from %lu = %s", uid, mpc_lowcomm_peer_format(uid));

   return 0;
}


int _mpc_ofi_get_attr(sctk_rail_info_t *rail,
                     lcr_rail_attr_t *attr)
{
	attr->iface.cap.am.max_iovecs = MPC_OFI_IOVEC_SIZE;
	attr->iface.cap.am.max_bcopy  = rail->runtime_config_driver_config->driver.value.ofi.bcopy_size - sizeof(lcr_ofi_am_hdr_t);
	attr->iface.cap.am.max_zcopy  = rail->runtime_config_driver_config->driver.value.ofi.eager_size - sizeof(lcr_ofi_am_hdr_t);

	attr->iface.cap.tag.max_bcopy = 0;
	attr->iface.cap.tag.max_zcopy = 0;

	attr->iface.cap.rndv.max_put_zcopy = INT_MAX;
	attr->iface.cap.rndv.max_get_zcopy = INT_MAX;

        attr->mem.size_packed_mkey = sizeof(struct _mpc_ofi_shared_pinning_context);

	return MPC_LOWCOMM_SUCCESS;
}


int _mpc_ofi_query_devices(lcr_component_t *component,
                          lcr_device_t **devices_p,
                          unsigned int *num_devices_p)
{

   struct fi_info * hints = _mpc_ofi_get_requested_hints(component->driver_config->driver.value.ofi.provider, component->driver_config->driver.value.ofi.endpoint_type);

   struct fi_info *config = NULL;

   int requested_fi_major = 1, requested_fi_minor = 5;
   int err = fi_getinfo(FI_VERSION(requested_fi_major, requested_fi_minor), NULL, NULL, 0, hints, &config);

   if(err < 0)
   {
      mpc_common_debug_error("OFI Comm Library failed to start provider '%s' with given constraints.", hints->fabric_attr->prov_name);

      if (err == -FI_EBADFLAGS)
      {
        mpc_common_debug_error("The specified endpoint or domain capability or operation flags are invalid.");
      }
      else if (err == -FI_ENODATA)
      {
        mpc_common_debug_error("Could not find any provider that complies with the provided hints. "
            "Please check the provider name and the endpoint type.");
      }
      else if (err == -FI_ENOMEM)
      {
        mpc_common_debug_error("Insufficient memory to complete the operation");
      }
      else if (err == -FI_ENOSYS)
      {
        uint32_t provider_fi_version = fi_version();
        mpc_common_debug_error("The requested libfabric version (%d.%d) is newer than the libfabric version being used (%d.%d)",
            requested_fi_major, requested_fi_minor, FI_MAJOR(provider_fi_version), FI_MINOR(provider_fi_version));
      }
      else
      {
        mpc_common_debug_error("Unknown libfabric error: fi_getinfo returned error code %d", err);
      }

      mpc_common_errorpoint("Initialization failed.");
      return 1;
   }

   struct fi_info * tmp = config;

   int device_count = 0;

   while(tmp)
   {
      device_count++;
      //We only keep the first device for now -- trusting OFI
      break;
      tmp = tmp->next;
   }

   assume(device_count > 0);
   *devices_p = malloc(sizeof(lcr_device_t) * device_count);
   *num_devices_p = device_count;

   device_count = 0;
   tmp = config;
   while(tmp)
   {
      (void)snprintf((*devices_p)[device_count].name, LCR_DEVICE_NAME_MAX, "%s (%s) : %s @ %s", tmp->fabric_attr->prov_name,
                                                                                                                  _mpc_ofi_decode_endpoint_type(tmp->ep_attr->type),
                                                                                                                  tmp->fabric_attr->name,
                                                                                                                  tmp->domain_attr->name);
      device_count++;
      //We only keep the first device for now -- trusting OFI
      break;
      tmp = tmp->next;
   }

   fi_freeinfo(config);
   fi_freeinfo(hints);

   return 0;
}

int _mpc_ofi_progress(sctk_rail_info_t *rail)
{
   _mpc_ofi_domain_poll(rail->network.ofi.ctx.domain, 0);
   return 0;
}

static int ___mpc_ofi_context_recv_callback_t(void *buffer, __UNUSED__ size_t len, struct _mpc_ofi_request_t * req, void * prail)
{
   sctk_rail_info_t *rail = (sctk_rail_info_t *)prail;

	int rc = MPC_LOWCOMM_SUCCESS;

   lcr_ofi_am_hdr_t *hdr = (lcr_ofi_am_hdr_t*)buffer;

	lcr_am_handler_t handler = rail->am[hdr->am_id];
	if (handler.cb == NULL) {
		mpc_common_debug_fatal("LCP: handler id %d not supported.", hdr->am_id);
	}

   mpc_common_tracepoint_fmt("[OFI] callback for a payload of %lu", len);

	rc = handler.cb(handler.arg, hdr->data, hdr->length, 0);

	if (rc != MPC_LOWCOMM_SUCCESS) {
		mpc_common_debug_error("LCP: handler id %d failed.", hdr->am_id);
	}

   /* Complete the recv request */
   _mpc_ofi_request_done(req);

	return rc;
}

void _mpc_ofi_release(sctk_rail_info_t *rail)
{
   TODO("Check why we get fi_close(&domain->domain->fid) Device or resource busy(-16)");
  _mpc_ofi_context_release(&rail->network.ofi.ctx);
}


int _mpc_ofi_iface_open(__UNUSED__ const char *device_name, int id,
                       lcr_rail_config_t *rail_config,
                       lcr_driver_config_t *driver_config,
                       sctk_rail_info_t **iface_p,
                       unsigned features)
{
        UNUSED(features);
	int rc = MPC_LOWCOMM_SUCCESS;
	sctk_rail_info_t *rail = NULL;

	lcr_rail_init(rail_config, driver_config, &rail);
	if(rail == NULL)
	{
		mpc_common_debug_error("LCR: could not allocate tcp rail");
		rc = MPC_LOWCOMM_ERROR;
		goto err;
	}

	rail->rail_number = id;

	//FIXME: to pass the assert in sctk_network_init_tcp_all
	rail->connect_on_demand         = _mpc_ofi_connect_on_demand;

	/* New API */
	rail->send_am_bcopy  = _mpc_ofi_send_am_bcopy;
	rail->send_am_zcopy  = _mpc_ofi_send_am_zcopy;
	rail->iface_get_attr = _mpc_ofi_get_attr;
   rail->iface_progress = _mpc_ofi_progress;
   rail->driver_finalize = _mpc_ofi_release;

   rail->get_zcopy = _mpc_ofi_get_zcopy;
   rail->put_zcopy = _mpc_ofi_send_put_zcopy;
   rail->iface_pack_memp = _mpc_ofi_pack_rkey;
   rail->iface_unpack_memp = _mpc_ofi_unpack_rkey;
   rail->iface_is_reachable = _mpc_ofi_iface_is_reachable;
   rail->iface_register_mem = _mpc_ofi_pin;
   rail->iface_unregister_mem = _mpc_ofi_unpin;


   /* Init capabilities */
   rail->cap = LCR_IFACE_CAP_RMA | LCR_IFACE_CAP_REMOTE;

	/* Register our connection handler */
	char my_net_name[MPC_COMMON_MAX_STRING_SIZE];
	mpc_lowcomm_monitor_register_on_demand_callback(__gen_rail_target_name(rail, my_net_name, MPC_COMMON_MAX_STRING_SIZE),
	                                                __ofi_on_demand_callback,
	                                                (void *)rail);

   if(_mpc_ofi_context_init(&rail->network.ofi.ctx,
                           driver_config->driver.value.ofi.provider,
                           MPC_OFI_POLICY_RR,
                           ___mpc_ofi_context_recv_callback_t,
                           (void*)rail,
                           __passive_endpoint_did_accept_a_connection,
                           rail,
                           &driver_config->driver.value.ofi))
   {
      mpc_common_errorpoint("Failed to start OFI context");
      return 1;
   }


	*iface_p = rail;
err:
	return rc;
}

lcr_component_t tcpofi_component =
{
	.name          = { "tcpofirail"    },
	.query_devices = _mpc_ofi_query_devices,
	.iface_open    = _mpc_ofi_iface_open,
	.devices       = NULL,
	.num_devices   = 0,
	.flags         = 0,
	.next          = NULL
};
LCR_COMPONENT_REGISTER(&tcpofi_component)

lcr_component_t shmofi_component =
{
	.name          = { "shmofirail"    },
	.query_devices = _mpc_ofi_query_devices,
	.iface_open    = _mpc_ofi_iface_open,
	.devices       = NULL,
	.num_devices   = 0,
	.flags         = 0,
	.next          = NULL 
};
LCR_COMPONENT_REGISTER(&shmofi_component)

lcr_component_t verbsofi_component =
{
	.name          = { "verbsofirail"  },
	.query_devices = _mpc_ofi_query_devices,
	.iface_open    = _mpc_ofi_iface_open,
	.devices       = NULL,
	.num_devices   = 0,
	.flags         = 0,
	.next          = NULL 
};
LCR_COMPONENT_REGISTER(&verbsofi_component)
