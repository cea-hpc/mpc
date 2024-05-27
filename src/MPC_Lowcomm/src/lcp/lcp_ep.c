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

#include "lcp_ep.h"

#include "mpc_common_datastructure.h"
#include "mpc_common_debug.h"
#include "rail.h"

#include "lcp_context.h"
#include "lcp_manager.h"

#include <assert.h>
#include <mpc_common_helper.h>

static int lcp_ep_check_if_valid(lcp_ep_h ep)
{
	/* Endpoint must have at least one transport interface */
	return !mpc_bitmap_is_zero(ep->conn_map);
}

//NOTE: Cannot be used by TAG offload data path since multiplexing not allowed.
int lcp_ep_get_next_cc(lcp_ep_h ep)
{
	int            i;
	lcp_chnl_idx_t cc_idx;
        lcp_context_h  ctx = ep->mngr->ctx;

	/* Set ep comm channel to next. Both are first set as priority channel
	 * during endpoint creation. */
	ep->cc = ep->next_cc;

	/* Set next comm channel */
	if(ep->num_chnls > 1 && ctx->config.multirail_enabled)
	{
		cc_idx = ep->cc + 1;
		for(i = 0; i < ctx->num_resources; i++)
		{
			if(MPC_BITMAP_GET(ep->conn_map, cc_idx) )
			{
				ep->next_cc = cc_idx;
				break;
			}
			cc_idx = (cc_idx + 1) % ctx->num_resources;
		}
	}

	return ep->cc;
}

int lcp_ep_create_base(lcp_manager_h mngr, lcp_ep_h *ep_p)
{
	int      rc     = MPC_LOWCOMM_SUCCESS;
	bmap_t   ep_map = MPC_BITMAP_INIT;
        lcp_context_h ctx = mngr->ctx;
	lcp_ep_h ep;

	ep = sctk_malloc(sizeof(struct lcp_ep) );
	if(ep == NULL)
	{
		mpc_common_debug_error("LCP: failed to allocate endpoint");
		rc = MPC_LOWCOMM_ERROR;
		goto err;
	}
	memset(ep, 0, sizeof(struct lcp_ep) );

	/* Allocate as many transport endpoints as resources */
	ep->lct_eps = sctk_malloc(ctx->num_resources * sizeof(_mpc_lowcomm_endpoint_t *) );
	if(ep->lct_eps == NULL)
	{
		mpc_common_debug_error("LCP: could not allocate endpoint rails.");
		rc = MPC_LOWCOMM_ERROR;
		goto err_alloc_eps;
	}
	memset(ep->lct_eps, 0, ctx->num_resources * sizeof(_mpc_lowcomm_endpoint_t *) );

	/* Init channel transport endpoint bitmaps */
	ep->avail_map = ep->conn_map = ep_map;

	*ep_p = ep;

	return rc;

err_alloc_eps:
	sctk_free(ep);
err:
	return rc;
}

/**
 * @brief Initialize an endpoint.
 *
 * @param ctx context to retreive the resources
 * @param ep endpoint to initialize
 * @return int MPI_SUCCESS in case of success
 */
int lcp_ep_init_config(lcp_manager_h mngr, lcp_ep_h ep)
{
	int i;
        lcp_context_h ctx = mngr->ctx;
	int max_prio = 0, prio_idx = 0;

	ep->config.am.max_bcopy       = SIZE_MAX;
	ep->config.am.max_zcopy       = SIZE_MAX;
	ep->config.am.max_iovecs      = SIZE_MAX;
	ep->config.tag.max_bcopy      = SIZE_MAX;
	ep->config.tag.max_zcopy      = SIZE_MAX;
	ep->config.tag.max_iovecs     = SIZE_MAX;
	ep->config.rndv.max_put_zcopy = SIZE_MAX;
	ep->config.rndv.max_get_zcopy = SIZE_MAX;
	ep->config.rma.max_put_bcopy  = SIZE_MAX;
	ep->config.rma.max_put_zcopy  = SIZE_MAX;
	ep->config.rma.max_get_bcopy  = SIZE_MAX;
	ep->config.rma.max_get_zcopy  = SIZE_MAX;
	ep->config.offload            = 0;
        ep->cap                       = 0;
        ep->tag_chnl = ep->ato_chnl   = LCP_NULL_CHANNEL;

        //FIXME: endpoint configuration would need some refacto. It currently
        //       does not handle well heterogenous interfaces.
	for(i = 0; i < ctx->num_resources; i++)
	{
		/* Only append config of used endpoint interfaces */
		if(!MPC_BITMAP_GET(ep->avail_map, i) )
		{
			continue;
		}

		sctk_rail_info_t *iface   = mngr->ifaces[i];
		lcr_rail_attr_t   attr;

                //FIXME: in case of offload with multirail, the priority rail
                //       chosen will be the first interface. Maybe we could find
                //       a more formal way of setting it.
		if(max_prio < iface->priority)
		{
			max_prio = iface->priority;
			prio_idx = i;
		}

		iface->iface_get_attr(iface, &attr);
		if(attr.iface.cap.flags & LCR_IFACE_CAP_TAG_OFFLOAD)
		{
                        ep->config.offload = 1;
			ep->cap |= LCR_IFACE_CAP_TAG_OFFLOAD;
			ep->config.tag.max_bcopy = mpc_common_min(ep->config.tag.max_bcopy,
			                                      attr.iface.cap.tag.max_bcopy);
			ep->config.tag.max_zcopy = mpc_common_min(ep->config.tag.max_zcopy,
			                                      attr.iface.cap.tag.max_zcopy);
			ep->config.tag.max_iovecs = mpc_common_min(ep->config.tag.max_iovecs,
			                                       attr.iface.cap.tag.max_iovecs);

                        //NOTE: first interface is taken. Define a better
                        //      strategy.
                        if (ep->tag_chnl == LCP_NULL_CHANNEL) {
                                ep->tag_chnl = i;
                        }
		}

                if (attr.iface.cap.flags & LCR_IFACE_CAP_ATOMICS) {
			ep->cap |= LCR_IFACE_CAP_ATOMICS;
                        ep->config.ato.max_fetch_size = mpc_common_min(ep->config.ato.max_fetch_size,
                                                                       attr.iface.cap.ato.max_fetch_size);
                        ep->config.ato.max_fetch_size = mpc_common_min(ep->config.ato.max_post_size,
                                                                       attr.iface.cap.ato.max_post_size);

                        //NOTE: first interface is taken. Define a better
                        //      strategy.
                        if (ep->ato_chnl == LCP_NULL_CHANNEL) {
                                ep->ato_chnl = i;
                        }
                }

		//NOTE: if tbsm + portals for example, eager and rndv frag
		//       will be limited by portals capabilities. Is this the
		//       correct behavior ?
		ep->config.am.max_bcopy = mpc_common_min(ep->config.am.max_bcopy,
		                                     attr.iface.cap.am.max_bcopy);
		ep->config.am.max_zcopy = mpc_common_min(ep->config.am.max_zcopy,
		                                     attr.iface.cap.am.max_zcopy);
		ep->config.am.max_iovecs = mpc_common_min(ep->config.am.max_iovecs,
		                                      attr.iface.cap.am.max_iovecs);
		ep->config.rndv.max_get_zcopy = mpc_common_min(ep->config.rndv.max_get_zcopy,
		                                           attr.iface.cap.rndv.max_get_zcopy);
		ep->config.rndv.max_put_zcopy = mpc_common_min(ep->config.rndv.max_put_zcopy,
		                                           attr.iface.cap.rndv.max_put_zcopy);

		if(attr.iface.cap.flags & LCR_IFACE_CAP_RMA)
		{
			ep->cap |= LCR_IFACE_CAP_RMA;
			ep->config.rma.max_put_bcopy = mpc_common_min(ep->config.rma.max_put_bcopy,
			                                          attr.iface.cap.rma.max_put_bcopy);
			ep->config.rma.max_put_zcopy = mpc_common_min(ep->config.rma.max_put_zcopy,
			                                          attr.iface.cap.rma.max_put_zcopy);
			ep->config.rma.max_get_bcopy = mpc_common_min(ep->config.rma.max_get_bcopy,
			                                          attr.iface.cap.rma.max_get_bcopy);
			ep->config.rma.max_get_zcopy = mpc_common_min(ep->config.rma.max_get_zcopy,
			                                          attr.iface.cap.rma.max_get_zcopy);
		}
	}

	//FIXME: should it be two distinct thresholds? One for tag, one for am?
	ep->config.rndv_threshold = ep->config.am.max_zcopy;

	ep->cc = ep->next_cc = prio_idx;

	return MPC_LOWCOMM_SUCCESS;
}

/**
 * @brief Add an endpoint in a context
 *
 * @param ctx context to add the endpoint to
 * @param ep endpoint to be added
 * @return int MPI_SUCCESS in case of success
 */
int lcp_ep_insert(lcp_manager_h mngr, lcp_ep_h ep)
{
	int rc;

        //NOTE: insert in slow hash table if not in same set, otherwiser insert
        //      in fast preallocated table of endpoints.
        lcp_ep_ctx_t *elem = sctk_malloc(sizeof(lcp_ep_ctx_t) );

        if(elem == NULL)
        {
                mpc_common_debug_error("LCP: could not allocate endpoint table entry.");
                rc = MPC_LOWCOMM_ERROR;
                goto err;
        }
        memset(elem, 0, sizeof(lcp_ep_ctx_t) );

        /* Init table entry */
        elem->ep_key = ep->uid;
        elem->ep     = ep;

        mpc_common_hashtable_set(&mngr->eps, elem->ep_key, elem);

	/* Update context */
	mngr->num_eps++;

	rc = MPC_LOWCOMM_SUCCESS;
err:
	return rc;
}

/**
 * @brief Initialize the routes of an endpoint.
 * If a route is found, destination endpoint is set to `LCP_EP_FLAG_CONNECTED`.
 * If no route is found, destination endpoint is set to `LCP_EP_FLAG_CONNECTING`.
 *
 * @param ctx context to get the devices from
 * @param ep endpoint to fill
 * @return int MPI_SUCCESS in case of success
 */
//TODO: force homogeneity of transport for one endpoint. Select transport with
//      highest priority, then build transport endpoints for all allocated
//      devices.
int lcp_ep_init_channels(lcp_manager_h mngr, lcp_ep_h ep)
{
	int rc, i;
        lcp_context_h ctx = mngr->ctx;
	int selected_prio = -1;

	//NOTE: by default all resources are used except if communications are
	//      shared, in which cases only shared memory resource is used
	//NOTE: ctx->resources are sorted in descending order of priority
	//      already.
	//NOTE: rail homogeneity implies only one type of rail may be chosen.
	//      With multirail, this is true only if two heterogeneous rails
	//      have different priority. As a consequence, having heterogenous
	//      rails with same priority would result in heterogeneous
	//      multirail.
	for(i = 0; i < mngr->num_ifaces; i++)
	{
		_mpc_lowcomm_endpoint_t *lcr_ep;
                sctk_rail_info_t        *iface = mngr->ifaces[i];

		/* If uid cannot be reached through this interface, continue */
		if(!iface->iface_is_reachable(iface, ep->uid) )
		{
			continue;
		}

		/* Set selected rail priority */
		if(iface->priority >= selected_prio)
		{
			selected_prio = iface->priority;
		}
		else
		{
			/* Homogeneous rails with highest priority have been
			 * selected */
			break;
		}
		/* Resource can be used for communication. */
		MPC_BITMAP_SET(ep->avail_map, i);
		/* Flag resource as used to enable polling on it */
		ctx->resources[i].used = 1;


		mpc_common_debug("Route to %llu using %s", ep->uid, iface->network_name);

		/* Check transport endpoint availability */
		lcr_ep = sctk_rail_get_any_route_to_process(iface, ep->uid);
		/* If endpoint does not exist yet, connect on demand */
		if(lcr_ep == NULL)
		{
			assert(iface->connect_on_demand != NULL);
			iface->connect_on_demand(iface, ep->uid);
			/* Get newly created endpoint */
			lcr_ep = sctk_rail_get_any_route_to_process(iface,
			                                            ep->uid);
			if(lcr_ep == NULL)
			{
				continue;
			}
		}

		/* Make sure only to use connected endpoints (OFI may add connecting endpoints on the listen side with FI_EP_MSG)*/
		while (_mpc_lowcomm_endpoint_get_state(lcr_ep) == _MPC_LOWCOMM_ENDPOINT_CONNECTING)
		{
			lcp_progress(mngr);
		}

		/* Add endpoint to list and set bitmap entry */
		ep->lct_eps[i] = lcr_ep;

		/* Transport endpoint is connected */
		MPC_BITMAP_SET(ep->conn_map, i);
	}

	/* Protocol endpoint connected only if there are available interfaces
	 * and all are connected */
	int equal = !mpc_bitmap_is_zero(ep->avail_map) &&
	            MPC_BITMAP_AND(ep->conn_map, ep->avail_map);

        //FIXME: num_chnls is set to num_iface since in many places a loop need
        //       to be done on such range, channel are used depending on the
        //       connection bitmap. Should be adapted...
        ep->num_chnls = mngr->num_ifaces;

	if(!equal)
	{
		ep->state = LCP_EP_FLAG_CONNECTING;
	}
	else
	{
		ep->state = LCP_EP_FLAG_CONNECTED;
	}
	mpc_common_debug("LCP: ep state=%s.", ep->state ? "CONNECTING" : "CONNECTED");

	rc = MPC_LOWCOMM_SUCCESS;

	return rc;
}

/**
 * @brief update the progress of routes (like lcp_ep_init_channels but without initializing the endpoint)
 *
 * @param ctx context to get the devices from
 * @param ep endpoint to fill
 * @return int MPI_SUCCESS in case of success
 */
int lcp_ep_progress_conn(lcp_manager_h mngr, lcp_ep_h ep)
{
	int i;

	for(i = 0; i < mngr->num_ifaces; i++)
	{
		_mpc_lowcomm_endpoint_t *lcr_ep;
		sctk_rail_info_t        *iface = mngr->ifaces[i];

		/* If transport endpoint already connected, continue */
		if(MPC_BITMAP_GET(ep->conn_map, i) )
		{
			continue;
		}

		lcr_ep = ep->lct_eps[i];
		if(lcr_ep == NULL)
		{
			/* Check transport endpoint availability */
			lcr_ep = sctk_rail_get_any_route_to_process(iface, ep->uid);
			if(lcr_ep == NULL)
			{
				continue;
			}
			ep->lct_eps[i] = lcr_ep;
		}

		/* Transport endpoint is connected */
		MPC_BITMAP_SET(ep->conn_map, i);
	}

	/* Protocol endpoint connected only if all interfaces connected */
	int equal = MPC_BITMAP_AND(ep->conn_map, ep->avail_map);

	if(!equal)
	{
		ep->state = LCP_EP_FLAG_CONNECTING;
		return 0;
	}
	else
	{
		ep->state = LCP_EP_FLAG_CONNECTED;
		return 1;
	}
}

/**
 * @brief Allocate an endpoint (with `lcp_ep_create_base`) using a context
 *
 * @param ctx context used to initialize the endpoint
 * @param ep_p address of the endpoint to be created
 * @param uid id of the endpoint
 * @param flags unused
 * @return int MPI_SUCCESS in case of success
 */
int lcp_context_ep_create(lcp_manager_h mngr, lcp_ep_h *ep_p,
                          uint64_t uid, __UNUSED__ unsigned flags)
{
	int      rc;
	lcp_ep_h ep;

	/* Allocation endpoint */
	rc = lcp_ep_create_base(mngr, &ep);
	if(rc != MPC_LOWCOMM_SUCCESS)
	{
		goto err;
	}

	ep->uid = uid;
	ep->mngr = mngr;

	/* Create all transport endpoints */
	rc = lcp_ep_init_channels(mngr, ep);
	if(rc != MPC_LOWCOMM_SUCCESS)
	{
		goto err_unalloc;
	}

	/* Init endpoint config */
	lcp_ep_init_config(mngr, ep);

	/* Check if protocol endpoint is valid */
	if(!lcp_ep_check_if_valid(ep) )
	{
		mpc_common_debug_error("LCP: endpoint not valid.");
		rc = MPC_LOWCOMM_ERROR;
		goto err_unalloc;
	}

	*ep_p = ep;
	mpc_common_debug_info("LCP: created ep dest=%llu, dst_tsk=%d.",
	                      uid, uid);

	return rc;

err_unalloc:
	lcp_ep_delete(ep);
err:
	return rc;
}

int lcp_ep_create(lcp_manager_h mngr, lcp_ep_h *ep_p,
                  mpc_lowcomm_peer_uid_t uid, unsigned flags)
{
	lcp_ep_h ep = NULL;
	int      rc = MPC_LOWCOMM_SUCCESS;

	LCP_MANAGER_LOCK(mngr);

	ep = lcp_ep_get(mngr, uid);
	if(ep != NULL)
	{
		*ep_p = ep;
		goto unlock;
	}

	/* Create and connect endpoint */
	rc = lcp_context_ep_create(mngr, &ep, uid, flags);
	if(rc != MPC_LOWCOMM_SUCCESS)
	{
		mpc_common_debug_error("LCP: could not create endpoint. "
		                       "uid=%llu.", uid);
		goto unlock;
	}

	/* Insert endpoint in the context table, only once */
	rc = lcp_ep_insert(mngr, ep);
	if(rc != MPC_LOWCOMM_SUCCESS)
	{
		goto unlock;
	}
	mpc_common_debug_info("LCP: created ep : %p towards : %lu", ep, uid);

	*ep_p = ep;

unlock:
	LCP_MANAGER_UNLOCK(mngr);
	return rc;
}

//NOTE: should it be the users' responsibility to delete it?
void lcp_ep_delete(lcp_ep_h ep)
{
	int i;

	for(i = 0; i < ep->num_chnls; i++)
	{
		if(!MPC_BITMAP_GET(ep->conn_map, i) )
		{
			continue;
		}
		sctk_free(ep->lct_eps[i]);
	}

	sctk_free(ep->lct_eps);
	sctk_free(ep);

	ep = NULL;
}

lcp_ep_h lcp_ep_get(lcp_manager_h mngr, uint64_t uid)
{
        lcp_ep_ctx_t *elem = mpc_common_hashtable_get_no_lock(&mngr->eps, uid);
        if(elem == NULL)
        {
                return NULL;
        }
        return elem->ep;
}

int lcp_ep_get_or_create(lcp_manager_h mngr, uint64_t uid, 
                         lcp_ep_h *ep_p, unsigned flags)
{
	if(!(*ep_p = lcp_ep_get(mngr, uid)))
	{
		return lcp_ep_create(mngr, ep_p, uid, flags);
	}
	return MPC_LOWCOMM_SUCCESS;
}

//FIXME: add lcp_ep_close
