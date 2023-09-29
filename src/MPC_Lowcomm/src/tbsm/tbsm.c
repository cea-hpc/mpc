#include "tbsm.h"

#include "mpc_common_spinlock.h"

#include <rail.h>
#include <sctk_alloc.h>

#include <utlist.h>

#include <lcr/lcr_component.h>

typedef struct lcr_tbsm_pkg {
        uint8_t             am_id; /* active message id */
        void               *buf;   /* addr of bcopy buf or send buf */
        size_t              size;
        mpc_queue_elem_t    elem;
        uint64_t            seqn;
} lcr_tbsm_pkg_t;

static ssize_t lcr_tbsm_send_am_bcopy(_mpc_lowcomm_endpoint_t *ep, uint8_t id, 
                                      lcr_pack_callback_t pack, void *arg,
                                      __UNUSED__ unsigned flags)
{
	_mpc_lowcomm_tbsm_rail_info_t *tbsm_info = &(ep->rail->network.tbsm);
	uint32_t payload_length          = -1;

        /* Allocate TBSM package */
        lcr_tbsm_pkg_t *tbsm_pkg = sctk_malloc(sizeof(lcr_tbsm_pkg_t)); 
        if (tbsm_pkg == NULL) {
                mpc_common_debug_error("TBSM: could not allocate bcopy tbsm "
                                       "pkg.");
                goto err;
        }
        memset(tbsm_pkg, 0, sizeof(lcr_tbsm_pkg_t));

        /* Allocate bcopy buffer */
        tbsm_pkg->buf = sctk_malloc(tbsm_info->bcopy_buf_size);
	if (tbsm_pkg->buf == NULL) {
	       mpc_common_debug_error("TBSM: could not allocate bcopy tbsm "
                                      "buffer.");
	       goto err;
	}
	memset(tbsm_pkg->buf, 0, tbsm_info->bcopy_buf_size);

	tbsm_pkg->am_id = id;
        tbsm_pkg->size  = payload_length = pack(tbsm_pkg->buf, arg);
        if (payload_length < 0) {
                mpc_common_debug_error("TBSM: could not pack data.");
                goto err;
        }

        /* Insert package in list */
        mpc_common_spinlock_lock(&(tbsm_info->tx_lock));
        tbsm_pkg->seqn = tbsm_info->seqn++;
        mpc_common_debug("LCR TBSM: pushing package. seqn=%llu", tbsm_pkg->seqn);
        mpc_queue_push(&tbsm_info->queue, &tbsm_pkg->elem);
        mpc_common_spinlock_unlock(&(tbsm_info->tx_lock));
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

	rc = handler.cb(handler.arg, data, length, 0);
	if (rc != MPC_LOWCOMM_SUCCESS) {
		mpc_common_debug_error("LCP: handler id %d failed.", am_id);
	}

	return rc;
}

void lcr_tbsm_connect_on_demand(sctk_rail_info_t *rail, uint64_t uid)
{
	_mpc_lowcomm_tbsm_rail_info_t *tbsm_info = &(rail->network.tbsm);

        mpc_common_spinlock_lock(&(tbsm_info->conn_lock));
        if (sctk_rail_get_any_route_to_process(rail, uid) == NULL) {

                _mpc_lowcomm_endpoint_t *ep;

                //NOTE: there is no special data to be associated with a tbsm
                //      endpoint since there is only one queue.
                ep = sctk_malloc(sizeof(_mpc_lowcomm_endpoint_t));
                if (ep == NULL) {
                        mpc_common_debug_error("TBSM: could not allocate ep");
                        mpc_common_spinlock_unlock(&(tbsm_info->conn_lock));
                        return;
                }
                _mpc_lowcomm_endpoint_init(ep, uid, rail, 
                                           _MPC_LOWCOMM_ENDPOINT_DYNAMIC);

                sctk_rail_add_dynamic_route(rail, ep);

                /* Make sure the route is flagged connected */
                _mpc_lowcomm_endpoint_set_state(ep, _MPC_LOWCOMM_ENDPOINT_CONNECTED);
        }
        mpc_common_spinlock_unlock(&(tbsm_info->conn_lock));
}

int lcr_tbsm_iface_progress(sctk_rail_info_t *rail)
{
        int rc = MPC_LOWCOMM_SUCCESS;
        lcr_tbsm_pkg_t *pkg = NULL;
        mpc_queue_elem_t *tx_elem;
        _mpc_lowcomm_tbsm_rail_info_t *tbsm_info = &(rail->network.tbsm);

        mpc_common_spinlock_lock(&(tbsm_info->tx_lock));

        /* Fast path if list is empty */
        if (mpc_queue_is_empty(&(tbsm_info->queue))) {
                mpc_common_spinlock_unlock(&(tbsm_info->tx_lock));
                goto err;
        }

        pkg = mpc_queue_pull_elem(&(tbsm_info->queue), lcr_tbsm_pkg_t, elem);

        mpc_common_debug("LCR TBSM: pulling package. seqn=%llu", pkg->seqn);
        mpc_common_spinlock_unlock(&(tbsm_info->tx_lock));

        while (pkg->seqn != tbsm_info->expected_seqn) {
                mpc_thread_yield();
        }

        mpc_common_spinlock_lock(&(tbsm_info->poll_lock));

        rc = lcr_tbsm_invoke_am(rail, pkg->am_id, pkg->size, pkg->buf);
        
        tbsm_info->expected_seqn++;
        mpc_common_spinlock_unlock(&(tbsm_info->poll_lock));
        if (rc != MPC_LOWCOMM_SUCCESS) {
                goto err;
        }

        //NOTE: pkg must be a bcopy, so free buffer. This would have to change
        //      if zcopy is implemented.
        sctk_free(pkg->buf);
        sctk_free(pkg);

err:
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

int lcr_tbsm_is_reachable(sctk_rail_info_t *rail, uint64_t uid) {
        UNUSED(rail);
        return uid == mpc_lowcomm_monitor_get_uid();
}


int lcr_tbsm_iface_init(sctk_rail_info_t *iface)
{
        int rc = MPC_LOWCOMM_SUCCESS;
        _mpc_lowcomm_tbsm_rail_info_t *tbsm_iface = &(iface->network.tbsm);
        struct _mpc_lowcomm_config_struct_net_driver_tbsm tbsm_info =
                iface->runtime_config_driver_config->driver.value.tbsm;
         
        /* Init queuing mecanism */
        mpc_common_spinlock_init(&(tbsm_iface->poll_lock), 0);
        mpc_common_spinlock_init(&(tbsm_iface->tx_lock), 0);
        mpc_common_spinlock_init(&(tbsm_iface->conn_lock), 0);
        mpc_queue_init_head(&(tbsm_iface->queue));
        tbsm_iface->seqn = tbsm_iface->expected_seqn = 0;

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
        iface->iface_pack_memp    = lcr_tbsm_pack_rkey; 
        iface->iface_unpack_memp  = lcr_tbsm_unpack_rkey; 
        iface->rail_pin_region    = lcr_tbsm_mem_register;
        iface->rail_unpin_region  = lcr_tbsm_mem_unregister;
        iface->iface_get_attr     = lcr_tbsm_get_attr;
        iface->iface_progress     = lcr_tbsm_iface_progress;
        iface->connect_on_demand  = lcr_tbsm_connect_on_demand;
        iface->iface_is_reachable = lcr_tbsm_is_reachable;

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

int lcr_tbsm_iface_open(__UNUSED__ const char *device_name, int id,
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
