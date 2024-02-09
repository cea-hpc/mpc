#include "tbsm.h"

#include "lcr/lcr_def.h"
#include "mpc_common_debug.h"
#include "mpc_common_spinlock.h"
#include "mpc_lowcomm_monitor.h"
#include "mpc_lowcomm_types.h"

#include <rail.h>
#include <sctk_alloc.h>

#include <stdint.h>

#include <lcr/lcr_component.h>

void lcr_tbsm_connect_on_demand(sctk_rail_info_t *rail, uint64_t uid)
{
	_mpc_lowcomm_tbsm_rail_info_t *tbsm_info = &(rail->network.tbsm);

        if (sctk_rail_get_any_route_to_process(rail, uid) == NULL) {

                _mpc_lowcomm_endpoint_t *ep;

                ep = sctk_malloc(sizeof(_mpc_lowcomm_endpoint_t));
                if (ep == NULL) {
                        mpc_common_debug_error("TBSM: could not allocate ep");
                        mpc_common_spinlock_unlock(&(tbsm_info->conn_lock));
                        return;
                }

                _mpc_lowcomm_endpoint_init(ep, uid, rail,
                                           _MPC_LOWCOMM_ENDPOINT_DYNAMIC);

                /* Append endpoint to table */
                sctk_rail_add_dynamic_route(rail, ep);

                /* Make sure the route is flagged connected */
                _mpc_lowcomm_endpoint_set_state(ep, _MPC_LOWCOMM_ENDPOINT_CONNECTED);
        }

        return;
}

//TODO: write finalize function...

int lcr_tbsm_get_attr(sctk_rail_info_t *rail,
                      lcr_rail_attr_t *attr)
{
        _mpc_lowcomm_tbsm_rail_info_t *tbsm_iface = &(rail->network.tbsm);

        //NOTE: This should already include the payload necessary for sending
        //      data by the current rail (header added by the transport layer
        //      for example).
        attr->iface.cap.am.max_iovecs = 6; //FIXME: arbitrary value...
        attr->iface.cap.am.max_bcopy  = 0;
        attr->iface.cap.am.max_zcopy  = tbsm_iface->max_msg_size;

        attr->iface.cap.tag.max_bcopy  = 0; /* No tag-matching capabilities */
        attr->iface.cap.tag.max_zcopy  = 0; /* No tag-matching capabilities */
        attr->iface.cap.tag.max_iovecs = 0; /* No tag-matching capabilities */

        attr->iface.cap.rndv.max_put_zcopy = 0; /* No rendez-vous for tbsm comm */
        attr->iface.cap.rndv.max_get_zcopy = 0; /* No rendez-vous for tbsm comm */
        attr->iface.cap.rndv.min_frag_size = 0; /* No rendez-vous for tbsm comm */

        return MPC_LOWCOMM_SUCCESS;
}

int lcr_tbsm_is_reachable(sctk_rail_info_t *rail, uint64_t uid) {
        UNUSED(rail);
        uint64_t my_uid = mpc_lowcomm_monitor_get_uid();
        return (mpc_lowcomm_peer_get_set(uid) == mpc_lowcomm_peer_get_set(my_uid)) &&
                (mpc_lowcomm_peer_get_rank(uid) == mpc_lowcomm_peer_get_rank(my_uid));
}

int lcr_tbsm_iface_init(sctk_rail_info_t *iface)
{
        int rc = MPC_LOWCOMM_SUCCESS;
        _mpc_lowcomm_tbsm_rail_info_t *tbsm_iface = &(iface->network.tbsm);
        struct _mpc_lowcomm_config_struct_net_driver_tbsm tbsm_info =
                iface->runtime_config_driver_config->driver.value.tbsm;

        /* Init queuing mecanism */
        mpc_common_spinlock_init(&(tbsm_iface->conn_lock), 0);

        /* Init capabilities */
        iface->cap = 0;

        tbsm_iface->bcopy_buf_size = tbsm_info.bcopy_buf_size;
        tbsm_iface->eager_limit    = tbsm_info.eager_limit;
        tbsm_iface->max_msg_size   = tbsm_info.max_msg_size;

        //FIXME: set progress function to null since all communication are down
        //       within LCP, see lcp_self.c
        iface->iface_progress     = NULL;
        iface->connect_on_demand  = lcr_tbsm_connect_on_demand;
        iface->iface_is_reachable = lcr_tbsm_is_reachable;
        iface->iface_get_attr     = lcr_tbsm_get_attr;
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
                        sctk_rail_info_t **iface_p,
                        unsigned flags)
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

//NOTE: Thread-based Shared Memory component is still present as it could be
//      preferable to keep some of the lcp semantic that requires an actual
//      interface:
//      - endpoint creation: for lcp_ep_get for example, use of interface
//      capabilities and thresholds (eager, bcopy, zcopy).

lcr_component_t tbsm_component = {
        .name = { "tbsmmpi" },
        .query_devices = lcr_tbsm_query_devices,
        .iface_open = lcr_tbsm_iface_open,
        .devices = NULL,
        .num_devices = 0,
        .flags = 0,
        .next = NULL
};
LCR_COMPONENT_REGISTER(&tbsm_component)
