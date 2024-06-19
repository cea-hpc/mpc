/* ############################# MPC License ############################## */
/* # Thu May  6 10:26:16 CEST 2021                                        # */
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
/* # - CANAT Paul pcanat@paratools.fr                                     # */
/* # - BESNARD Jean-Baptiste jbbesnard@paratools.com                      # */
/* # - MOREAU Gilles gilles.moreau@cea.fr                                 # */
/* #                                                                      # */
/* ######################################################################## */

#include "tbsm.h"

#include "lcr/lcr_def.h"
#include "mpc_common_debug.h"
#include "mpc_common_spinlock.h"
#include "mpc_lowcomm_monitor.h"
#include "mpc_lowcomm_types.h"

#include <rail.h>
#include <sctk_alloc.h>

#include <stdint.h>
#include <alloca.h>
#include <sys/uio.h>

#include <lcr/lcr_component.h>

ssize_t lcr_tbsm_send_am_bcopy(_mpc_lowcomm_endpoint_t *ep,
                               uint8_t id,
                               lcr_pack_callback_t pack,
                               void *arg,
                               unsigned flags)
{
        _mpc_lowcomm_tbsm_rail_info_t *iface = &(ep->rail->network.tbsm);
        UNUSED(flags);
        void* start                = NULL;
        ssize_t size               = 0;

        start = mpc_mpool_pop(&iface->buf_mp);
        if (start == NULL) {
                //FIXME: Should be MPC_LOWCOMM_NO_RESOURCE but LCP does not
                //       handle pending request yet.
                mpc_common_debug_error("LCR TBSM: could not allocate bcopy buffer.");
                size = -1;
                goto err;
        }
        size = pack(start, arg);
        if (size < 0) {
                goto err;
        }
        assert((size_t)size <= iface->eager_limit);

        lcr_am_handler_t handler = ep->rail->am[id];
        if (handler.cb == NULL) {
                mpc_common_debug_fatal("LCR TBSM: handler id %d not supported.", id);
        }

        size = handler.cb(handler.arg, start, size, LCR_IFACE_AM_LAYOUT_BUFFER);
	if (size != MPC_LOWCOMM_SUCCESS) {
		mpc_common_debug_error("LCR TBSM: handler id %d failed.", id);
                goto err;
	}

        mpc_mpool_push(start);
err:
        return size;
}

int lcr_tbsm_send_am_zcopy(_mpc_lowcomm_endpoint_t *ep,
                           uint8_t id,
                           const void *header,
                           unsigned header_length,
                           const struct iovec *iov,
                           size_t iovcnt,
                           unsigned flags, 
                           lcr_completion_t *comp) 
{
        UNUSED(flags);
        UNUSED(comp);
        int i, rc = MPC_LOWCOMM_SUCCESS;
	sctk_rail_info_t *rail = ep->rail;
        struct iovec *tbsm_iov = NULL;
        size_t tbsm_iov_cnt    = 0;
        size_t send_length     = 0;

        mpc_common_debug("LCR TBSM: send zcopy. ep=%p", ep);

        if (iovcnt > rail->network.tbsm.max_iov) {
                mpc_common_debug_error("LCR TBSM: exceeded maximal number if iov");
                rc = MPC_LOWCOMM_ERROR;
                goto err;
        }

        tbsm_iov = alloca(iovcnt + 1);

        tbsm_iov[0].iov_len  = header_length;
        tbsm_iov[0].iov_base = (void *)header;
        tbsm_iov_cnt++;
        for (i = 0; i < (int)iovcnt; i++) {
                tbsm_iov[tbsm_iov_cnt].iov_len  = iov[i].iov_len;
                tbsm_iov[tbsm_iov_cnt].iov_base = iov[i].iov_base;
                send_length += iov[i].iov_len;
                tbsm_iov_cnt++;
        }

        if (send_length > rail->network.tbsm.eager_limit) {
                mpc_common_debug_error("LCR TBSM: exceeded maximal eager data "
                                       "length.");
                rc = MPC_LOWCOMM_ERROR;
                goto err;
        }

        lcr_am_handler_t handler = rail->am[id];
        if (handler.cb == NULL) {
                mpc_common_debug_fatal("LCR TBSM: handler id %d not supported.", id);
        }

        rc = handler.cb(handler.arg, tbsm_iov, tbsm_iov_cnt, LCR_IFACE_AM_LAYOUT_IOV);
	if (rc != MPC_LOWCOMM_SUCCESS) {
		mpc_common_debug_error("LCR TBSM: handler id %d failed.", id);
                goto err;
	}

        /* At this point, data has either been pushed to unexpected queue or
         * copied to destination buffer. Therefore, request is remotely
         * completed and completion can be called. Note that, sent length should
         * be the payload length only, see upper completion implementation. */
        comp->sent = send_length; 
        comp->comp_cb(comp);
err: 
        return rc;
}

int lcr_tbsm_send_put_zcopy(_mpc_lowcomm_endpoint_t *ep,
                            uint64_t local_addr,
                            uint64_t remote_addr,
                            lcr_memp_t *local_key,
                            lcr_memp_t *remote_key,
                            size_t size,
                            lcr_completion_t *comp) 
{
        UNUSED(ep);
        UNUSED(local_key);
        UNUSED(remote_key);
        memcpy((void *)remote_addr, (void *)local_addr, size);

        comp->sent = size;
        comp->comp_cb(comp);
        return MPC_LOWCOMM_SUCCESS;
}

int lcr_tbsm_send_get_zcopy(_mpc_lowcomm_endpoint_t *ep,
                            uint64_t local_addr,
                            uint64_t remote_addr,
                            lcr_memp_t *local_key,
                            lcr_memp_t *remote_key,
                            size_t size,
                            lcr_completion_t *comp) 
{
        UNUSED(ep);
        UNUSED(local_key);
        UNUSED(remote_key);
        memcpy((void *)local_addr, (void *)remote_addr, size);

        comp->sent = size;
        comp->comp_cb(comp);
        return MPC_LOWCOMM_SUCCESS;
}

int lcr_tbsm_flush_mem_ep(sctk_rail_info_t *rail,
                         _mpc_lowcomm_endpoint_t *ep,
                         struct sctk_rail_pin_ctx_list *list,
                         lcr_completion_t *comp,
                         unsigned flags) 
{
       UNUSED(rail); 
       UNUSED(ep); 
       UNUSED(list); 
       UNUSED(flags); 

       comp->comp_cb(comp);
       return MPC_LOWCOMM_SUCCESS;
}

int lcr_tbsm_flush_ep(sctk_rail_info_t *rail,
                      _mpc_lowcomm_endpoint_t *ep,
                      lcr_completion_t *comp,
                      unsigned flags) 
{
       UNUSED(rail); 
       UNUSED(ep); 
       UNUSED(flags); 

       comp->comp_cb(comp);
       return MPC_LOWCOMM_SUCCESS;
}

int lcr_tbsm_flush_mem(sctk_rail_info_t *rail,
                       struct sctk_rail_pin_ctx_list *list,
                       lcr_completion_t *comp,
                       unsigned flags) 
{
       UNUSED(rail); 
       UNUSED(list); 
       UNUSED(flags); 

       comp->comp_cb(comp);
       return MPC_LOWCOMM_SUCCESS;
}

int lcr_tbsm_flush_iface(sctk_rail_info_t *rail,
                         lcr_completion_t *comp,
                         unsigned flags) 
{
       UNUSED(rail); 
       UNUSED(flags); 

       comp->comp_cb(comp);
       return MPC_LOWCOMM_SUCCESS;
}

int lcr_tbsm_mem_register(struct sctk_rail_info_s *rail, 
                           lcr_memp_t *list, const void *addr, 
                           size_t size, unsigned flags)
{
        UNUSED(rail);
        UNUSED(list);
        UNUSED(addr);
        UNUSED(size);
        UNUSED(flags);
        return MPC_LOWCOMM_SUCCESS;
}

int lcr_tbsm_mem_unregister(struct sctk_rail_info_s *rail, 
                            lcr_memp_t *list)
{
        UNUSED(rail);
        UNUSED(list);
        return MPC_LOWCOMM_SUCCESS;
}

int lcr_tbsm_pack_rkey(sctk_rail_info_t *rail,
                       lcr_memp_t *memp, void *dest)
{
        UNUSED(rail);
        UNUSED(memp);
        UNUSED(dest);
        
        return 0;
}

int lcr_tbsm_unpack_rkey(sctk_rail_info_t *rail,
                        lcr_memp_t *memp, void *dest)
{
        UNUSED(rail);
        UNUSED(memp);
        UNUSED(dest);

        return 0;
}

void lcr_tbsm_connect_on_demand(sctk_rail_info_t *rail, uint64_t uid)
{
	_mpc_lowcomm_tbsm_rail_info_t *tbsm_info = &(rail->network.tbsm);

        _mpc_lowcomm_endpoint_t *ep = sctk_rail_get_any_route_to_process(rail, uid);
        if (ep != NULL) {
                mpc_common_debug_warning("LCR TBSM: connect on demand. endpoint %llu "
                                         "already exists.");
                return;
        }

        ep = sctk_malloc(sizeof(_mpc_lowcomm_endpoint_t));
        if (ep == NULL) {
                mpc_common_debug_error("TBSM: could not allocate ep");
                mpc_common_spinlock_unlock(&(tbsm_info->conn_lock));
                return;
        }

        _mpc_lowcomm_endpoint_init(ep, uid, rail, _MPC_LOWCOMM_ENDPOINT_DYNAMIC);

        sctk_rail_add_dynamic_route(rail, ep);

        /* Append endpoint to table */
        sctk_rail_add_dynamic_route(rail, ep);

        /* Make sure the route is flagged connected */
        _mpc_lowcomm_endpoint_set_state(ep, _MPC_LOWCOMM_ENDPOINT_CONNECTED);

        return;
}

int lcr_tbsm_get_attr(sctk_rail_info_t *rail,
                      lcr_rail_attr_t *attr)
{
        _mpc_lowcomm_tbsm_rail_info_t *tbsm_iface = &(rail->network.tbsm);

        //NOTE: This should already include the payload necessary for sending
        //      data by the current rail (header added by the transport layer
        //      for example).
        //NOTE: eager messages are sent through zcopy API. Rendez-vous uses the
        //      bcopy API to send control messages. 
        attr->iface.cap.am.max_iovecs = tbsm_iface->max_iov; //FIXME: arbitrary value...
        attr->iface.cap.am.max_bcopy  = 0;
        attr->iface.cap.am.max_zcopy  = tbsm_iface->eager_limit;

        attr->iface.cap.tag.max_bcopy  = 0; /* No tag-matching capabilities */
        attr->iface.cap.tag.max_zcopy  = 0; /* No tag-matching capabilities */
        attr->iface.cap.tag.max_iovecs = 0; /* No tag-matching capabilities */

        attr->iface.cap.rndv.max_put_zcopy = tbsm_iface->max_msg_size; /* No rendez-vous for tbsm comm */
        attr->iface.cap.rndv.max_get_zcopy = tbsm_iface->max_msg_size; /* No rendez-vous for tbsm comm */
        attr->iface.cap.rndv.min_frag_size = 0; /* No rendez-vous for tbsm comm */

        attr->iface.cap.rma.max_get_bcopy = 0;
        attr->iface.cap.rma.max_put_bcopy = 0;
        attr->iface.cap.rma.max_get_zcopy = tbsm_iface->max_msg_size;
        attr->iface.cap.rma.max_put_zcopy = tbsm_iface->max_msg_size;

        attr->mem.size_packed_mkey = 0;

        attr->iface.cap.flags = rail->cap;

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
        iface->cap = LCR_IFACE_CAP_RMA |
                LCR_IFACE_CAP_REMOTE;

        tbsm_iface->max_iov        = tbsm_info.max_iov;
        tbsm_iface->bcopy_buf_size = tbsm_info.bcopy_buf_size;
        tbsm_iface->eager_limit    = tbsm_info.eager_limit;
        tbsm_iface->max_msg_size   = tbsm_info.max_msg_size;

        //FIXME: set progress function to null since all communication are done
        //       within LCP, see lcp_self.c
        iface->send_am_bcopy        = lcr_tbsm_send_am_bcopy;
        iface->send_am_zcopy        = lcr_tbsm_send_am_zcopy;
        iface->put_zcopy            = lcr_tbsm_send_put_zcopy;
        iface->get_zcopy            = lcr_tbsm_send_get_zcopy;
        iface->iface_register_mem   = lcr_tbsm_mem_register;
        iface->iface_unregister_mem = lcr_tbsm_mem_unregister;
        iface->iface_pack_memp      = lcr_tbsm_pack_rkey;
        iface->iface_unpack_memp    = lcr_tbsm_unpack_rkey;

        iface->flush_iface          = lcr_tbsm_flush_iface;
        iface->flush_ep             = lcr_tbsm_flush_ep;
        iface->flush_mem            = lcr_tbsm_flush_mem;
        iface->flush_mem_ep         = lcr_tbsm_flush_mem_ep;

        iface->iface_progress     = NULL;
        iface->connect_on_demand  = lcr_tbsm_connect_on_demand;
        iface->iface_is_reachable = lcr_tbsm_is_reachable;
        iface->iface_get_attr     = lcr_tbsm_get_attr;

        /* Initialize pool of copy-in buffers. */
        //FIXME: define suitable size for memory pool.
        mpc_mempool_param_t mp_buf_params = {
                .alignment = MPC_COMMON_SYS_CACHE_LINE_SIZE,
                .elem_per_chunk = 128,
                .elem_size = tbsm_iface->bcopy_buf_size,
                .max_elems = 1024,
                .malloc_func = sctk_malloc,
                .free_func = sctk_free
        };

        rc = mpc_mpool_init(&tbsm_iface->buf_mp, &mp_buf_params);

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
                mpc_common_debug_error("LCR TBSM: could not allocate devices");
                rc = MPC_LOWCOMM_ERROR;
                goto err;
        }

        strcpy(devices[0].name, "tbsm"); /* not used */

        *devices_p     = devices;
        *num_devices_p = num_devices;

err:
        return rc;
}

int lcr_tbsm_iface_open(int mngr_id, __UNUSED__ const char *device_name, int id,
                        lcr_rail_config_t *rail_config, 
                        lcr_driver_config_t *driver_config,
                        sctk_rail_info_t **iface_p,
                        unsigned flags)
{
        UNUSED(flags);
        UNUSED(mngr_id);
        int rc = MPC_LOWCOMM_SUCCESS;
        sctk_rail_info_t *iface = NULL;
        UNUSED(device_name);

        lcr_rail_init(rail_config, driver_config, &iface);
        if (iface == NULL) {
                mpc_common_debug_error("LCR TBSM: could not allocate rail");
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
        .name = { "tbsmmpi" },
        .query_devices = lcr_tbsm_query_devices,
        .iface_open = lcr_tbsm_iface_open,
        .devices = NULL,
        .num_devices = 0,
        .flags = 0,
        .next = NULL
};
LCR_COMPONENT_REGISTER(&tbsm_component)
