#include "tbsm.h"

#include "tbsm_types.h"

#include <sctk_rail.h>
#include <sctk_alloc.h>

#include <utlist.h>

#include <lcr/lcr_component.h>

static ssize_t lcr_tbsm_send_am_bcopy(_mpc_lowcomm_endpoint_t *ep, uint8_t id, 
                                      lcr_pack_callback_t pack, void *arg,
                                      __UNUSED__ unsigned flags)
{
        sctk_rail_info_t *rail           = ep->rail;
	lcr_tbsm_iface_info_t *tbsm_info = ep->rail->network.tbsm.info;
	uint32_t payload_length          = -1;

        /* Allocate TBSM package */
        lcr_tbsm_pkg_t *tbsm_pkg = sctk_malloc(sizeof(lcr_tbsm_pkg_t)); 
        if (tbsm_pkg == NULL) {
                mpc_common_debug_error("TBSM: could not allocate bcopy tbsm "
                                       "pkg.");
                payload_length = -1;
                goto err;
        }
        memset(tbsm_pkg, 0, sizeof(lcr_tbsm_pkg_t));

        /* Allocate bcopy buffer */
        tbsm_pkg->buf = sctk_malloc(rail->network.tbsm.bcopy_buf_size);
	if (tbsm_pkg->buf == NULL) {
	       mpc_common_debug_error("Could not allocate bcopy tbsm buffer.");
	       payload_length = -1;
	       goto err;
	}
	memset(tbsm_pkg->buf, 0, rail->network.tbsm.bcopy_buf_size);

	tbsm_pkg->am_id = id;
        tbsm_pkg->size  = payload_length = pack(tbsm_pkg->buf, arg);

        /* Insert package in list */
        mpc_common_spinlock_lock(&(tbsm_info->list->lock));
        DL_APPEND(tbsm_info->list->list, tbsm_pkg);
        tbsm_info->list->size++;
        mpc_common_spinlock_unlock(&(tbsm_info->list->lock));
err:
        return payload_length;
}

int lcr_tbsm_send_put_zcopy(_mpc_lowcomm_endpoint_t *ep,
                            uint64_t local_addr,
                            uint64_t remote_offset,
                            lcr_memp_t *remote_key,
                            size_t size,
                            lcr_completion_t *comp) 
{
        UNUSED(ep);
        void *dest = remote_key->pin.sm.buf;
        memcpy(dest, (void *)local_addr + remote_offset, size);

        comp->sent = size;
        comp->comp_cb(comp);
        return MPC_LOWCOMM_SUCCESS;
}

int lcr_tbsm_send_get_zcopy(_mpc_lowcomm_endpoint_t *ep,
                            uint64_t local_addr,
                            uint64_t remote_offset,
                            lcr_memp_t *remote_key,
                            size_t size,
                            lcr_completion_t *comp) 
{
        UNUSED(ep);
        void *src = remote_key->pin.sm.buf;
        memcpy((void *)local_addr, src + remote_offset, size);

        comp->sent = size;
        comp->comp_cb(comp);
        return MPC_LOWCOMM_SUCCESS;
}

void lcr_tbsm_mem_register(struct sctk_rail_info_s *rail, 
                           lcr_memp_t *list, 
                           void *addr, size_t size)
{
        UNUSED(rail);
        list->pin.sm.buf  = addr; 
        list->pin.sm.size = size;
}

void lcr_tbsm_mem_unregister(struct sctk_rail_info_s *rail, 
                             lcr_memp_t *list)
{
        UNUSED(rail);
        UNUSED(list);
        return;
}

int lcr_tbsm_pack_rkey(sctk_rail_info_t *rail,
                       lcr_memp_t *memp, void *dest)
{
        UNUSED(rail);
        int packed_size = 0;
        void *p = dest;

        *(uint64_t *)p = (uint64_t)memp->pin.sm.buf; packed_size += sizeof(uint64_t);
        p += sizeof(uint64_t);
        *(size_t *)p   = memp->pin.sm.size; packed_size += sizeof(size_t);
        
        return packed_size;
}

int lcr_tbsm_unpack_rkey(sctk_rail_info_t *rail,
                        lcr_memp_t *memp, void *dest)
{
        UNUSED(rail);
        int unpacked_size = 0;
        void *p = dest;	

        memp->pin.sm.buf  = *(uint64_t *)p; unpacked_size += sizeof(uint64_t);
        p += sizeof(uint64_t);
        memp->pin.sm.size = *(size_t *)p; unpacked_size += sizeof(size_t);

        return unpacked_size;
}

//FIXME: this function could be factorize between all components
static inline int lcr_tbsm_invoke_am(sctk_rail_info_t *rail, 
                                     uint8_t am_id,
                                     size_t length,
                                     void *data)
{
	int rc = MPC_LOWCOMM_SUCCESS;

	lcr_am_handler_t handler = rail->am[am_id];
	if (handler.cb == NULL) {
		mpc_common_debug_fatal("LCP: handler id %d not supported.", am_id);
	}

	rc = handler.cb(handler.arg, data, length, LCR_IFACE_SM_REQUEST);
	if (rc != MPC_LOWCOMM_SUCCESS) {
		mpc_common_debug_error("LCP: handler id %d failed.", am_id);
	}

	return rc;
}

void lcr_tbsm_connect_on_demand(sctk_rail_info_t *rail, uint64_t uid)
{
	lcr_tbsm_iface_info_t *tbsm_info = rail->network.tbsm.info;

        mpc_common_spinlock_lock(&(tbsm_info->list->lock));
        if (sctk_rail_get_any_route_to_process(rail, uid) == NULL) {

                _mpc_lowcomm_endpoint_t *ep;

                //NOTE: there is no special data to be associated with a tbsm
                //      endpoint since there is only one queue.
                ep = sctk_malloc(sizeof(_mpc_lowcomm_endpoint_t));
                if (ep == NULL) {
                        mpc_common_debug_error("TBSM: could not allocate ep");
                        return;
                }
                _mpc_lowcomm_endpoint_init(ep, uid, rail, 
                                           _MPC_LOWCOMM_ENDPOINT_DYNAMIC);

                sctk_rail_add_dynamic_route(rail, ep);

                /* Make sure the route is flagged connected */
                _mpc_lowcomm_endpoint_set_state(ep, _MPC_LOWCOMM_ENDPOINT_CONNECTED);
        }
        mpc_common_spinlock_unlock(&(tbsm_info->list->lock));
}

int lcr_tbsm_iface_progress(sctk_rail_info_t *rail)
{
        int rc = MPC_LOWCOMM_SUCCESS;
        lcr_tbsm_pkg_t *pkg = NULL;
	lcr_tbsm_iface_info_t *tbsm_info = rail->network.tbsm.info;

        /* Pop first element */
        mpc_common_spinlock_lock(&(tbsm_info->list->lock));
        /* Fast path if list is empty */
        if (!tbsm_info->list->size) {
                mpc_common_spinlock_unlock(&(tbsm_info->list->lock));
                return MPC_LOWCOMM_SUCCESS;
        }

        pkg = tbsm_info->list->list;
        assert(pkg);
        DL_DELETE(tbsm_info->list->list, pkg);
        
        /* Decrement list size */
        tbsm_info->list->size--;
        mpc_common_debug("TBSM: size list %d", tbsm_info->list->size);
        mpc_common_spinlock_unlock(&(tbsm_info->list->lock));

        lcr_tbsm_invoke_am(rail, pkg->am_id, pkg->size, pkg->buf);
        
        //NOTE: pkg must be a bcopy, so free buffer. This would have to change
        //      if zcopy is implemented.
        sctk_free(pkg->buf);
        sctk_free(pkg);

        return rc;
}

int lcr_tbsm_get_attr(sctk_rail_info_t *rail,
                      lcr_rail_attr_t *attr)
{
        _mpc_lowcomm_tbsm_rail_info_t *tbsm_iface = &(rail->network.tbsm);

        attr->iface.cap.am.max_iovecs = 6; //FIXME: arbitrary value...
        attr->iface.cap.am.max_bcopy  = tbsm_iface->eager_limit;
        //FIXME: no support for zcopy since data has to be passed as a 
        //       single buffer through the am callback. For zero copy, we 
        //       use rndv.
        attr->iface.cap.am.max_zcopy  = 0;

        attr->iface.cap.tag.max_bcopy  = 0; /* No tag-matching capabilities */
        attr->iface.cap.tag.max_zcopy  = 0; /* No tag-matching capabilities */
        attr->iface.cap.tag.max_iovecs = 0; /* No tag-matching capabilities */

        attr->iface.cap.rndv.max_put_zcopy = tbsm_iface->max_msg_size;
        attr->iface.cap.rndv.max_get_zcopy = tbsm_iface->max_msg_size;
        attr->iface.cap.rndv.min_frag_size = tbsm_iface->max_msg_size;

        return MPC_LOWCOMM_SUCCESS;
}


int lcr_tbsm_iface_init(sctk_rail_info_t *iface)
{
        int rc = MPC_LOWCOMM_SUCCESS;
        _mpc_lowcomm_tbsm_rail_info_t *tbsm_iface = &(iface->network.tbsm);
        struct _mpc_lowcomm_config_struct_net_driver_tbsm tbsm_info =
                iface->runtime_config_driver_config->driver.value.tbsm;
         
        tbsm_iface->info = sctk_malloc(sizeof(lcr_tbsm_iface_info_t));
        if (tbsm_iface->info == NULL) {
                mpc_common_debug_error("LCP: could not allocate sm info.");
                rc = MPC_LOWCOMM_ERROR;
                goto err;
        }

        tbsm_iface->info->list = sctk_malloc(sizeof(lcr_tbsm_pkg_list_t));
        if (tbsm_iface->info->list == NULL) {
                mpc_common_debug_error("LCP: could not allocate sm pkg list.");
                rc = MPC_LOWCOMM_ERROR;
                //FIXME: free tbsm_iface.info.
                goto err;
        }
        memset(tbsm_iface->info->list, 0, sizeof(lcr_tbsm_pkg_list_t));
        mpc_common_spinlock_init(&(tbsm_iface->info->list->lock), 0);

        /* Init capabilities */
        iface->cap = LCR_IFACE_CAP_SELF | LCR_IFACE_CAP_RMA;

        tbsm_iface->bcopy_buf_size = tbsm_info.bcopy_buf_size;
        tbsm_iface->eager_limit    = tbsm_info.eager_limit;
        tbsm_iface->max_msg_size   = tbsm_info.max_msg_size;

        /* Endpoint functions */
        iface->send_am_bcopy = lcr_tbsm_send_am_bcopy;
        iface->put_zcopy     = lcr_tbsm_send_put_zcopy;
        iface->get_zcopy     = lcr_tbsm_send_get_zcopy;

        /* Interface functions */
        iface->iface_pack_memp   = lcr_tbsm_pack_rkey; 
        iface->iface_unpack_memp = lcr_tbsm_unpack_rkey; 
        iface->rail_pin_region   = lcr_tbsm_mem_register;
        iface->rail_unpin_region = lcr_tbsm_mem_unregister;
        iface->iface_get_attr    = lcr_tbsm_get_attr;
        iface->iface_progress    = lcr_tbsm_iface_progress;
        iface->connect_on_demand = lcr_tbsm_connect_on_demand;

err:
        return rc;
}

int lcr_tbsm_query_devices(lcr_component_t *component,
                           lcr_device_t **devices_p,
                           unsigned int *num_devices_p)
{
        UNUSED(component);
        int rc                = MPC_LOWCOMM_SUCCESS;
        int num_devices       = 1;
        lcr_device_t *devices = NULL;

        /* There can be only one tbsm device */
        devices = sctk_malloc(num_devices * sizeof(*devices));
        if (devices == NULL) {
                mpc_common_debug_error("LCP: could not allocate sm devices");
                rc = MPC_LOWCOMM_ERROR;
                goto err;
        }

        strcpy(devices[0].name, "tbsm"); /* not used */

        *devices_p     = devices;
        *num_devices_p = num_devices;

err:
        return rc;
}

int lcr_tbsm_iface_open(char *device_name, int id,
                        lcr_rail_config_t *rail_config, 
                        lcr_driver_config_t *driver_config,
                        sctk_rail_info_t **iface_p)
{
        int rc = MPC_LOWCOMM_SUCCESS;
        sctk_rail_info_t *iface = NULL;
        UNUSED(device_name);

        lcr_rail_init(rail_config, driver_config, &iface);
        if (iface == NULL) {
                mpc_common_debug_error("LCR: could not allocate sm rail");
                rc = MPC_LOWCOMM_ERROR;
                goto err;
        }
        strcpy(iface->device_name, "tbsm");
	iface->rail_number = id;

        lcr_tbsm_iface_init(iface);

        *iface_p = iface;
err:
        return rc;
}

lcr_component_t tbsm_component = {
        .name = { "tbsm" },
        .rail_name = { "tbsmmpi" },
        .query_devices = lcr_tbsm_query_devices,
        .iface_open = lcr_tbsm_iface_open,
        .devices = NULL,
        .num_devices = 0,
        .flags = 0,
        .next = NULL
};
LCR_COMPONENT_REGISTER(&tbsm_component)
