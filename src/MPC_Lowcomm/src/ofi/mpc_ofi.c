#include "mpc_ofi.h"

#include <lcr/lcr_component.h>
#include <stdio.h>
#include <string.h>
#include <sctk_alloc.h>

#include "mpc_common_debug.h"
#include "mpc_lowcomm_monitor.h"
#include "mpc_ofi_helpers.h"
#include "ofi/mpc_ofi_context.h"
#include "ofi/mpc_ofi_dns.h"
#include "ofi/mpc_ofi_domain.h"
#include "rail.h"
#include "rdma/fabric.h"


static inline char *__gen_rail_target_name(sctk_rail_info_t *rail, char *buff, int bufflen)
{
	snprintf(buff, bufflen, "ofi-%d", rail->rail_number);
	return buff;
}

static void __add_route(mpc_lowcomm_peer_uid_t dest_uid, sctk_rail_info_t *rail)
{
	_mpc_lowcomm_endpoint_t *new_route = sctk_malloc(sizeof(_mpc_lowcomm_endpoint_t) );
	assume(new_route != NULL);
	_mpc_lowcomm_endpoint_init(new_route, dest_uid, rail, _MPC_LOWCOMM_ENDPOINT_DYNAMIC);

	/* Make sure the route is flagged connected */
	_mpc_lowcomm_endpoint_set_state(new_route, _MPC_LOWCOMM_ENDPOINT_CONNECTED);

   sctk_rail_add_dynamic_route(rail, new_route);
}



struct mpc_ofi_net_infos
{
   char addr[512];
   size_t size;
};

void mpc_ofi_connect_on_demand(struct sctk_rail_info_s *rail, mpc_lowcomm_peer_uid_t dest)
{

   struct mpc_ofi_net_infos my_infos = { 0 };
   my_infos.size = 512;

   struct mpc_ofi_context_t * ctx = &rail->network.ofi.ctx;

   if( mpc_ofi_dns_resolve(&ctx->dns, mpc_lowcomm_monitor_get_uid(), my_infos.addr, &my_infos.size) )
   {
      mpc_common_errorpoint_fmt("Failed to resolve OFI address for local UID %d", mpc_lowcomm_monitor_get_uid());
   }


	mpc_lowcomm_monitor_retcode_t ret = MPC_LOWCOMM_MONITOR_RET_SUCCESS;

   char my_net_name[128];
	mpc_lowcomm_monitor_response_t resp = mpc_lowcomm_monitor_ondemand(dest,
	                                                                   __gen_rail_target_name(rail, my_net_name, 128),
	                                                                   &my_infos,
	                                                                   sizeof(struct mpc_ofi_net_infos),
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

   struct mpc_ofi_net_infos * remote_info = (struct mpc_ofi_net_infos *)content->on_demand.data;

   /* Now add the response to the DNS */
   if( mpc_ofi_dns_register(&ctx->dns, dest, remote_info->addr, remote_info->size) )
   {
      mpc_common_errorpoint_fmt("Failed to register OFI address for remote UID %d", dest);
   }

	mpc_lowcomm_monitor_response_free(resp);

   /* Now add route as remote is in the DNS */
   __add_route(dest, rail);
}



static int __ofi_on_demand_callback(mpc_lowcomm_peer_uid_t from,
                                    char *data,
                                    char *return_data,
                                    int return_data_len,
                                    void *prail)
{
	sctk_rail_info_t *rail = prail;
   struct mpc_ofi_context_t * ctx = &rail->network.ofi.ctx;

   struct mpc_ofi_net_infos *infos = (struct mpc_ofi_net_infos*) data;

   /* Register remote info */
   if( mpc_ofi_dns_register(&ctx->dns, from, infos->addr, infos->size) )
   {
      mpc_common_errorpoint_fmt("CALLBACK Failed to register OFI address for remote UID %d", from);
      return 1;
   }

   /* Now send local info */
   assume(sizeof(struct mpc_ofi_net_infos) < return_data_len);
   struct mpc_ofi_net_infos *my_infos = return_data;
   my_infos->size = 512;


   if( mpc_ofi_dns_resolve(&ctx->dns, mpc_lowcomm_monitor_get_uid(), my_infos->addr, &my_infos->size) )
   {
      mpc_common_errorpoint_fmt("Failed to resolve OFI address for local UID %d", mpc_lowcomm_monitor_get_uid());
   }

   /* Now add route as remote is in the DNS */
   __add_route(from, rail);

	return 0;
}


static inline int __free_bsend_buffer(__UNUSED__ struct mpc_ofi_request_t *req, void *pbuffer)
{
   sctk_free(pbuffer);
   return 0;
}

#define MPC_OFI_BSEND_TRSH 1024

ssize_t mpc_ofi_send_am_bcopy(_mpc_lowcomm_endpoint_t *ep,
                              uint8_t id,
                              lcr_pack_callback_t pack,
                              void *arg,
                              unsigned flags)
{
   sctk_rail_info_t *rail = ep->rail;
	uint32_t payload_length;
	ssize_t sent;

   TODO("Use the right size from config an provider caps");
	lcr_ofi_am_hdr_t *hdr = sctk_malloc(MPC_OFI_BSEND_TRSH);

	if (hdr == NULL) {
	       mpc_common_debug_error("Could not allocate buffer.");
	       payload_length = -1;
	       goto err;
	}

	hdr->am_id = id;
	hdr->length = payload_length = pack(hdr->data, arg);
   payload_length = hdr->length;

   mpc_common_debug("OUT CB %d LEN %lld", hdr->am_id, hdr->length);

   if( mpc_ofi_view_send(&rail->network.ofi.view, hdr, hdr->length + sizeof(lcr_ofi_am_hdr_t), ep->dest, NULL, __free_bsend_buffer, hdr) )
   {
      mpc_common_errorpoint("Failed to send a message");
      return -1;
   }

err:
	return payload_length;
}

struct mpc_ofi_deffered_completion_s
{
   lcr_ofi_am_hdr_t hdr;
   lcr_completion_t *comp;
};


static inline int __zcopy_completion_cb(struct mpc_ofi_request_t *req, void *pcomp)
{
   struct mpc_ofi_deffered_completion_s *comp = (struct mpc_ofi_deffered_completion_s *)pcomp;
   comp->comp->comp_cb(comp->comp);
   sctk_free(comp);
   return 0;
}


int mpc_ofi_send_am_zcopy(_mpc_lowcomm_endpoint_t *ep,
                                    uint8_t id,
                                    const void *header,
                                    unsigned header_length,
                                    const struct iovec *iov,
                                    size_t iovcnt,
                                    unsigned flags,
                                    lcr_completion_t *comp)
{
   struct iovec full_iov[MPC_OFI_IOVEC_SIZE + 2];

	struct mpc_ofi_deffered_completion_s *completion = sctk_malloc(sizeof(struct mpc_ofi_deffered_completion_s));

   completion->hdr.am_id = id;
   completion->comp = comp;

   assume((iovcnt + 2) < MPC_OFI_IOVEC_SIZE);

   size_t total_size = 0;
   int total_iov_size = 1;

   full_iov[0].iov_base = &completion->hdr;
   full_iov[0].iov_len = sizeof(lcr_ofi_am_hdr_t);

   int iov_offset = 1;

   if(header_length)
   {
      full_iov[iov_offset].iov_base = (void*)header;
      full_iov[iov_offset].iov_len = header_length;
      total_size += full_iov[iov_offset].iov_len;
      iov_offset++;
      total_iov_size++;
   }

   unsigned int i = 0;
   for(i = 0 ; i < iovcnt; i++)
   {
      full_iov[iov_offset].iov_base = iov[i].iov_base;
      full_iov[iov_offset].iov_len = iov[i].iov_len;
      total_size += full_iov[iov_offset].iov_len;
      iov_offset++;
      total_iov_size++;
   }

   completion->hdr.length = total_size;

   if(mpc_ofi_view_sendv(&ep->rail->network.ofi.view,
                         ep->dest,
                         full_iov,
                         total_iov_size,
                         NULL,
                         __zcopy_completion_cb,
                         (void*)completion))
   {
      mpc_common_errorpoint("Failed to sendv a message");
      return -1;
   }

   return MPC_LOWCOMM_SUCCESS;
}


int mpc_ofi_get_attr(sctk_rail_info_t *rail,
                     lcr_rail_attr_t *attr)
{

	attr->iface.cap.am.max_iovecs = MPC_OFI_IOVEC_SIZE; //FIXME: arbitrary value...
	attr->iface.cap.am.max_bcopy  = MPC_OFI_BSEND_TRSH;
	attr->iface.cap.am.max_zcopy  = MPC_OFI_DOMAIN_EAGER_SIZE;

	attr->iface.cap.tag.max_bcopy = 0;
	attr->iface.cap.tag.max_zcopy = 0;

	attr->iface.cap.rndv.max_put_zcopy = 0;
	attr->iface.cap.rndv.max_get_zcopy = 0;

	return MPC_LOWCOMM_SUCCESS;
}


int mpc_ofi_query_devices(__UNUSED__ lcr_component_t *component,
                          lcr_device_t **devices_p,
                          unsigned int *num_devices_p)
{
   struct fi_info * hints = mpc_ofi_get_requested_hints("tcp");

   struct fi_info *config = NULL;

   if( fi_getinfo(FI_VERSION(1, 5), NULL, NULL, 0, hints, &config) < 0)
   {
      mpc_common_debug_error("OFI Comm Library failed to start provider '%s' with given constraints.", hints->fabric_attr->prov_name);
      mpc_common_errorpoint("Initialization failed.");
      return 1;
   }

   struct fi_info * tmp = config;

   int device_count = 0;

   while(tmp)
   {
      device_count++;
      TODO("We only keep the first device for now -- trusting OFI");
      break;
      tmp = tmp->next;
   }

   *devices_p = malloc(sizeof(lcr_device_t) * device_count);
   *num_devices_p = device_count;

   device_count = 0;
   tmp = config;
   while(tmp)
   {
      snprintf((*devices_p)[device_count].name, LCR_DEVICE_NAME_MAX, "%s @ %s", tmp->fabric_attr->name, tmp->domain_attr->name);
      device_count++;
      TODO("We only keep the first device for now -- trusting OFI");
      break;
      tmp = tmp->next;
   }

   fi_freeinfo(config);
   fi_freeinfo(hints);

}

int mpc_ofi_progress(sctk_rail_info_t *rail)
{
   mpc_ofi_domain_poll(rail->network.ofi.view.domain, 0);
}

static int __mpc_ofi_context_recv_callback_t(void *buffer, size_t len, struct mpc_ofi_request_t * req, void * prail)
{
   sctk_rail_info_t *rail = (sctk_rail_info_t *)prail;

	int rc = MPC_LOWCOMM_SUCCESS;

   lcr_ofi_am_hdr_t *hdr = (lcr_ofi_am_hdr_t*)buffer;

   mpc_common_debug("IN CB %d LEN %lld", hdr->am_id, hdr->length);

	lcr_am_handler_t handler = rail->am[hdr->am_id];
	if (handler.cb == NULL) {
		mpc_common_debug_fatal("LCP: handler id %d not supported.", hdr->am_id);
	}

	rc = handler.cb(handler.arg, hdr->data, hdr->length, 0);

	if (rc != MPC_LOWCOMM_SUCCESS) {
		mpc_common_debug_error("LCP: handler id %d failed.", hdr->am_id);
	}

   /* Complete the recv request */
   TODO("Is it safe to assume it is complete ?");
   mpc_ofi_request_done(req);


	return rc;
}


int mpc_ofi_iface_open(char *device_name, int id,
                       lcr_rail_config_t *rail_config,
                       lcr_driver_config_t *driver_config,
                       sctk_rail_info_t **iface_p)
{
	int rc = MPC_LOWCOMM_SUCCESS;
	sctk_rail_info_t *rail = NULL;

	UNUSED(device_name);

	lcr_rail_init(rail_config, driver_config, &rail);
	if(rail == NULL)
	{
		mpc_common_debug_error("LCR: could not allocate tcp rail");
		rc = MPC_LOWCOMM_ERROR;
		goto err;
	}

	strcpy(rail->device_name, "default");
	rail->rail_number = id;

	//FIXME: to pass the assert in sctk_network_init_tcp_all
	rail->connect_on_demand         = mpc_ofi_connect_on_demand;

	/* New API */
	rail->send_am_bcopy  = mpc_ofi_send_am_bcopy;
	rail->send_am_zcopy  = mpc_ofi_send_am_zcopy;
	rail->iface_get_attr = mpc_ofi_get_attr;
   rail->iface_progress = mpc_ofi_progress;

   /* Init capabilities */
   rail->cap = LCR_IFACE_CAP_REMOTE;


	/* Register our connection handler */
	char my_net_name[128];
	mpc_lowcomm_monitor_register_on_demand_callback(__gen_rail_target_name(rail, my_net_name, 128),
	                                                __ofi_on_demand_callback,
	                                                (void *)rail);

   if(mpc_ofi_context_init(&rail->network.ofi.ctx, 4, driver_config->driver.value.ofi.provider, MPC_OFI_POLICY_RR, __mpc_ofi_context_recv_callback_t, (void*)rail))
   {
      mpc_common_errorpoint("Failed to start OFI context");
      return 1;
   }

   /* Now initialize a view for each local rank*/
   if(mpc_ofi_view_init(&rail->network.ofi.view, &rail->network.ofi.ctx, mpc_lowcomm_monitor_get_uid()))
   {
      mpc_common_errorpoint("Failed to start OFI view");
      return 1;
   }

	*iface_p = rail;
err:
	return rc;
}

lcr_component_t ofi_component =
{
	.name          = { "ofi"    },
	.rail_name     = { "ofimpi" },
	.query_devices = mpc_ofi_query_devices,
	.iface_open    = mpc_ofi_iface_open,
	.devices       = NULL,
	.num_devices   = 0,
	.flags         = 0,
	.next          = NULL
};
LCR_COMPONENT_REGISTER(&ofi_component)