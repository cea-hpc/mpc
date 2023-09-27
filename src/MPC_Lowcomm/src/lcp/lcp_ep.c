#include "lcp_ep.h"

#include "mpc_common_datastructure.h"
#include "mpc_common_debug.h"
#include "sctk_alloc.h"
#include "rail.h"

#include "lcp_context.h"
#include "lcp_common.h"
#include "rail.h"

#include <assert.h>
#include <limits.h>
#include "uthash.h"

static int lcp_ep_check_if_valid(lcp_ep_h ep)
{
	/* Endpoint must have at least one transport interface */
	return !mpc_bitmap_is_zero(ep->conn_map);
}

//FIXME: Cannot be used by TAG offload data path since multiplexing not allowed.
int lcp_ep_get_next_cc(lcp_ep_h ep)
{
	int            i;
	lcp_chnl_idx_t cc_idx;

	/* Set ep comm channel to next. Both are first set as priority channel
	 * during request initialization. */
	ep->cc = ep->next_cc;

	/* Set next comm channel */
	if(ep->num_chnls > 1 && ep->ctx->config.multirail_enabled)
	{
		cc_idx = ep->cc + 1;
		for(i = 0; i < ep->ctx->num_resources; i++)
		{
			if(MPC_BITMAP_GET(ep->conn_map, cc_idx) )
			{
				ep->next_cc = cc_idx;
				break;
			}
			cc_idx = (cc_idx + 1) % ep->ctx->num_resources;
		}
	}

	return ep->cc;
}

int lcp_ep_create_base(lcp_context_h ctx, lcp_ep_h *ep_p)
{
	int      rc     = LCP_SUCCESS;
	bmap_t   ep_map = MPC_BITMAP_INIT;
	lcp_ep_h ep;

	ep = sctk_malloc(sizeof(struct lcp_ep) );
	if(ep == NULL)
	{
		mpc_common_debug_error("LCP: failed to allocate endpoint");
		rc = LCP_ERROR;
		goto err;
	}
	memset(ep, 0, sizeof(struct lcp_ep) );

	/* Allocate as many transport endpoints as resources */
	ep->lct_eps = sctk_malloc(ctx->num_resources * sizeof(_mpc_lowcomm_endpoint_t *) );
	if(ep->lct_eps == NULL)
	{
		mpc_common_debug_error("LCP: could not allocate endpoint rails.");
		rc = LCP_ERROR;
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
int lcp_ep_init_config(lcp_context_h ctx, lcp_ep_h ep)
{
	int i;
	int max_prio = 0, prio_idx = 0;

	ep->ep_config.am.max_bcopy       = SIZE_MAX;
	ep->ep_config.am.max_zcopy       = SIZE_MAX;
	ep->ep_config.am.max_iovecs      = SIZE_MAX;
	ep->ep_config.tag.max_bcopy      = SIZE_MAX;
	ep->ep_config.tag.max_zcopy      = SIZE_MAX;
	ep->ep_config.tag.max_iovecs     = SIZE_MAX;
	ep->ep_config.rndv.max_put_zcopy = SIZE_MAX;
	ep->ep_config.rndv.max_get_zcopy = SIZE_MAX;
	ep->ep_config.rma.max_put_bcopy  = SIZE_MAX;
	ep->ep_config.rma.max_put_zcopy  = SIZE_MAX;
	ep->ep_config.offload            = 0;

	for(i = 0; i < ctx->num_resources; i++)
	{
		/* Only append config of used endpoint interfaces */
		if(!MPC_BITMAP_GET(ep->avail_map, i) )
		{
			continue;
		}

		lcp_rsc_desc_t    if_desc = ctx->resources[i];
		sctk_rail_info_t *iface   = ctx->resources[i].iface;
		lcr_rail_attr_t   attr;

		if(max_prio < if_desc.priority)
		{
			max_prio = if_desc.priority;
			prio_idx = i;
		}

		iface->iface_get_attr(iface, &attr);
		if(iface->runtime_config_rail->offload)
		{
			ep->ep_config.offload       = 1;
			ep->ep_config.tag.max_bcopy = LCP_MIN(ep->ep_config.tag.max_bcopy,
			                                      attr.iface.cap.tag.max_bcopy);
			ep->ep_config.tag.max_zcopy = LCP_MIN(ep->ep_config.tag.max_zcopy,
			                                      attr.iface.cap.tag.max_zcopy);
			ep->ep_config.tag.max_iovecs = LCP_MIN(ep->ep_config.tag.max_iovecs,
			                                       attr.iface.cap.tag.max_iovecs);
		}
		//FIXME: if shared + portals for example, eager and rndv frag
		//       will be limited by portals capabilities. Is this the
		//       correct behavior ?
		ep->ep_config.am.max_bcopy = LCP_MIN(ep->ep_config.am.max_bcopy,
		                                     attr.iface.cap.am.max_bcopy);
		ep->ep_config.am.max_zcopy = LCP_MIN(ep->ep_config.am.max_zcopy,
		                                     attr.iface.cap.am.max_zcopy);
		ep->ep_config.am.max_iovecs = LCP_MIN(ep->ep_config.am.max_iovecs,
		                                      attr.iface.cap.am.max_iovecs);
		ep->ep_config.rndv.max_get_zcopy = LCP_MIN(ep->ep_config.rndv.max_get_zcopy,
		                                           attr.iface.cap.rndv.max_get_zcopy);
		ep->ep_config.rndv.max_put_zcopy = LCP_MIN(ep->ep_config.rndv.max_put_zcopy,
		                                           attr.iface.cap.rndv.max_put_zcopy);

		if(iface->runtime_config_rail->rdma)
		{
			ep->ep_config.rma.max_put_bcopy = LCP_MIN(ep->ep_config.rma.max_put_bcopy,
			                                          attr.iface.cap.rma.max_put_bcopy);
			ep->ep_config.rma.max_put_bcopy = LCP_MIN(ep->ep_config.rma.max_put_zcopy,
			                                          attr.iface.cap.rma.max_put_zcopy);
		}
	}

	//FIXME: should it be two distinct threshold? One for tag, one for am?
	ep->ep_config.rndv_threshold = ep->ep_config.am.max_zcopy;

	ep->priority_chnl = ep->cc = ep->next_cc = prio_idx;
	if(ep->ep_config.offload)
	{
		ep->tag_chnl = prio_idx;
	}

	return LCP_SUCCESS;
}

/**
 * @brief Add an endpoint in a context
 *
 * @param ctx context to add the endpoint to
 * @param ep endpoint to be added
 * @return int MPI_SUCCESS in case of success
 */
int lcp_ep_insert(lcp_context_h ctx, lcp_ep_h ep)
{
	int rc;

	lcp_ep_ctx_t *elem = sctk_malloc(sizeof(lcp_ep_ctx_t) );

	if(elem == NULL)
	{
		mpc_common_debug_error("LCP: could not allocate endpoint table entry.");
		rc = LCP_ERROR;
		goto err;
	}
	memset(elem, 0, sizeof(lcp_ep_ctx_t) );

	/* Init table entry */
	elem->ep_key = ep->uid;
	elem->ep     = ep;

	mpc_common_hashtable_set(&ctx->ep_htable, elem->ep_key, elem);

	/* Update context */
	ctx->num_eps++;

	rc = LCP_SUCCESS;
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
int lcp_ep_init_channels(lcp_context_h ctx, lcp_ep_h ep)
{
	int rc, i;
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
	for(i = 0; i < ctx->num_resources; i++)
	{
		_mpc_lowcomm_endpoint_t *lcr_ep;
		lcp_rsc_desc_t           if_desc = ctx->resources[i];

		/* If uid cannot be reached through this interface, continue */
		if(!if_desc.iface->iface_is_reachable(if_desc.iface, ep->uid) )
		{
			continue;
		}

		/* Set selected rail priority */
		if(if_desc.priority >= selected_prio)
		{
			selected_prio = if_desc.priority;
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


		mpc_common_debug_error("Route to %llu using %s", ep->uid, if_desc.name);

		/* Check transport endpoint availability */
		lcr_ep = sctk_rail_get_any_route_to_process(if_desc.iface, ep->uid);
		/* If endpoint does not exist yet, connect on demand */
		if(lcr_ep == NULL)
		{
			assert(if_desc.iface->connect_on_demand != NULL);
			if_desc.iface->connect_on_demand(if_desc.iface, ep->uid);
			/* Get newly created endpoint */
			lcr_ep = sctk_rail_get_any_route_to_process(if_desc.iface,
			                                            ep->uid);
			if(lcr_ep == NULL)
			{
				continue;
			}
		}

		/* Add endpoint to list and set bitmap entry */
		ep->lct_eps[i] = lcr_ep;
		ep->num_chnls++;

		/* Transport endpoint is connected */
		MPC_BITMAP_SET(ep->conn_map, i);
	}

	/* Protocol endpoint connected only if there are available interfaces
	 * and all are connected */
	int equal = !mpc_bitmap_is_zero(ep->avail_map) &&
	            MPC_BITMAP_AND(ep->conn_map, ep->avail_map);

	if(!equal)
	{
		ep->state = LCP_EP_FLAG_CONNECTING;
	}
	else
	{
		ep->state = LCP_EP_FLAG_CONNECTED;
	}
	mpc_common_debug("LCP: ep state=%s.", ep->state ? "CONNECTING" : "CONNECTED");

	rc = LCP_SUCCESS;

	return rc;
}

/**
 * @brief update the progress of routes (like lcp_ep_init_channels but without initializing the endpoint)
 *
 * @param ctx context to get the devices from
 * @param ep endpoint to fill
 * @return int MPI_SUCCESS in case of success
 */
int lcp_ep_progress_conn(lcp_context_h ctx, lcp_ep_h ep)
{
	int i;

	for(i = 0; i < ctx->num_resources; i++)
	{
		_mpc_lowcomm_endpoint_t *lcr_ep;
		lcp_rsc_desc_t           if_desc = ctx->resources[i];

		/* If transport endpoint already connected, continue */
		if(MPC_BITMAP_GET(ep->conn_map, i) )
		{
			continue;
		}

		lcr_ep = ep->lct_eps[i];
		if(lcr_ep == NULL)
		{
			/* Check transport endpoint availability */
			lcr_ep = sctk_rail_get_any_route_to_process(if_desc.iface, ep->uid);
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
int lcp_context_ep_create(lcp_context_h ctx, lcp_ep_h *ep_p,
                          uint64_t uid, __UNUSED__ unsigned flags)
{
	int      rc;
	lcp_ep_h ep;

	/* Allocation endpoint */
	rc = lcp_ep_create_base(ctx, &ep);
	if(rc != LCP_SUCCESS)
	{
		goto err;
	}

	ep->uid = uid;
	ep->ctx = ctx;
	OPA_store_int(&ep->seqn, 0);

	/* Create all transport endpoints */
	rc = lcp_ep_init_channels(ctx, ep);
	if(rc != LCP_SUCCESS)
	{
		goto err_unalloc;
	}

	/* Init endpoint config */
	lcp_ep_init_config(ctx, ep);

	/* Check if protocol endpoint is valid */
	if(!lcp_ep_check_if_valid(ep) )
	{
		mpc_common_debug_error("LCP: endpoint not valid.");
		rc = LCP_ERROR;
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

/**
 * @brief Allocate an endpoint (with `lcp_context_ep_create`) and add it to the context table
 *
 * @param ctx context used to initialize the endpoint
 * @param ep_p address of the endpoint to be created
 * @param uid id of the endpoint
 * @param flags flag passed to lcp_context_ep_create
 * @return int
 */
int lcp_ep_create(lcp_context_h ctx, lcp_ep_h *ep_p,
                  mpc_lowcomm_peer_uid_t uid, unsigned flags)
{
	lcp_ep_h ep = NULL;
	int      rc = LCP_SUCCESS;

	LCP_CONTEXT_LOCK(ctx);

	lcp_ep_get(ctx, uid, &ep);
	if(ep != NULL)
	{
		mpc_common_debug_warning("LCP: ep already exists. uid=%llu.",
		                         uid);
		*ep_p = ep;
		goto unlock;
	}

	/* Create and connect endpoint */
	rc = lcp_context_ep_create(ctx, &ep, uid, flags);
	if(rc != LCP_SUCCESS)
	{
		mpc_common_debug_error("LCP: could not create endpoint. "
		                       "uid=%llu.", uid);
		goto unlock;
	}

	/* Insert endpoint in the context table, only once */
	rc = lcp_ep_insert(ctx, ep);
	if(rc != LCP_SUCCESS)
	{
		goto unlock;
	}
	mpc_common_debug_info("LCP: created ep : %p towards : %lu", ep, uid);

	*ep_p = ep;

unlock:
	LCP_CONTEXT_UNLOCK(ctx);
	return rc;
}

//FIXME: should it be the users responsability to delete it ?
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

void lcp_ep_get(lcp_context_h ctx, mpc_lowcomm_peer_uid_t uid, lcp_ep_h *ep_p)
{
	lcp_ep_ctx_t *elem = mpc_common_hashtable_get(&ctx->ep_htable, uid);

	if(elem == NULL)
	{
		*ep_p = NULL;
	}
	else
	{
		*ep_p = elem->ep;
	}
}

/**
 * @brief Get an endpoint. Create it if it does not exist.
 *
 * @param arg context
 * @param comm_id id of the target communicator (32 bits written on weak bits of int64)
 * @param length destination process id
 * @param ep_p target output endpoint
 * @param flags unused
 * @return __UNUSED__ static int
 */
int lcp_ep_get_or_create(lcp_context_h ctx, mpc_lowcomm_peer_uid_t uid, lcp_ep_h *ep_p, unsigned flags)
{
	lcp_ep_get(ctx, uid, ep_p);
	if(!*ep_p)
	{
		return lcp_ep_create(ctx, ep_p, uid, flags);
	}
	return LCP_SUCCESS;
}

/**
 * @brief connect an endpoint (unused)
 *
 * @param arg context
 * @param data destination endpoint
 * @param length unused
 * @param flags unused
 * @return __UNUSED__ static int
 */
__UNUSED__ static int lcp_ep_connection_handler(void *arg, void *data,
                                                __UNUSED__ size_t length,
                                                __UNUSED__ unsigned flags)
{
	int                     rc;
	lcp_context_h           ctx    = arg;
	mpc_lowcomm_peer_uid_t *dest   = data;
	lcp_ep_ctx_t *          ctx_ep = NULL;
	lcp_ep_h *              ep     = NULL;

	mpc_common_spinlock_lock(&(ctx->ctx_lock) );

	ctx_ep = mpc_common_hashtable_get(&ctx->ep_htable, dest);

	if(ctx_ep != NULL)
	{
		rc = LCP_SUCCESS;
		goto out;
	}

	rc = lcp_context_ep_create(ctx, ep, *dest, 0);

	/* Insert endpoint in the context table */
	rc = lcp_ep_insert(ctx, *ep);
	if(rc != LCP_SUCCESS)
	{
		goto out;
	}
out:
	mpc_common_spinlock_unlock(&(ctx->ctx_lock) );
	return rc;
}
