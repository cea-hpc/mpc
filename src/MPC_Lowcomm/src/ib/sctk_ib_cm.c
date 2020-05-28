/* ############################# MPC License ############################## */
/* # Wed Nov 19 15:19:19 CET 2008                                         # */
/* # Copyright or (C) or Copr. Commissariat a l'Energie Atomique          # */
/* # Copyright or (C) or Copr. 2010-2012 Université de Versailles         # */
/* # St-Quentin-en-Yvelines                                               # */
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
/* # Authors:                                                             # */
/* #   - PERACHE Marc marc.perache@cea.fr                                 # */
/* #   - DIDELOT Sylvain sylvain.didelot@exascale-computing.eu            # */
/* #                                                                      # */
/* ######################################################################## */

#include "sctk_ib.h"
#include "sctk_ib_cm.h"
#include "sctk_ib_polling.h"
#include "sctk_ib_eager.h"

#include "sctk_ibufs_rdma.h"
#include "sctk_route.h"
#include "mpc_common_asm.h"

/* IB debug macros */
#if defined SCTK_IB_MODULE_NAME
#error "SCTK_IB_MODULE already defined"
#endif
#define SCTK_IB_MODULE_DEBUG
#define SCTK_IB_MODULE_NAME "CM"
#include "sctk_ib_toolkit.h"

#include "sctk_control_messages.h"

/*-----------------------------------------------------------
 *  Interface for Infiniband connexions.
 *
 *  - RACE CONDITITONS: for avoid race conditions between peers, a
 *  process responds to a request with a positive ack only when
 *  it has not initiated a request or its rank is higher than the
 *  source rank
 *----------------------------------------------------------*/

struct sctk_ib_qp_s;

/*-----------------------------------------------------------
 *  FUNCTIONS
 *----------------------------------------------------------*/

/* Change the state of a remote process */
static void sctk_ib_cm_change_state_connected ( sctk_rail_info_t *rail,  sctk_endpoint_t *endpoint )
{
    struct sctk_ib_qp_s *remote = endpoint->data.ib.remote;

    sctk_endpoint_state_t state_before_connexion;
    state_before_connexion = sctk_endpoint_get_state ( endpoint );

    switch ( sctk_endpoint_get_origin ( endpoint ) )
    {

        case ROUTE_ORIGIN_STATIC: /* Static route */
            ROUTE_LOCK ( endpoint );
            sctk_endpoint_set_state ( endpoint, STATE_CONNECTED );
            ROUTE_UNLOCK ( endpoint );

            sctk_ib_debug ( "[%d] Static QP connected to process %d", rail->rail_number, remote->rank );
            break;

        case ROUTE_ORIGIN_DYNAMIC: /* Dynamic route */
            ROUTE_LOCK ( endpoint );
            sctk_endpoint_set_state ( endpoint, STATE_CONNECTED );
            ROUTE_UNLOCK ( endpoint );

            if ( state_before_connexion == STATE_RECONNECTING )
                sctk_ib_debug ( "[%d] OD QP reconnected to process %d", rail->rail_number, remote->rank );
            else
                sctk_ib_debug ( "[%d] OD QP connected to process %d", rail->rail_number, remote->rank );



            break;

        default:
            not_reachable();
            mpc_common_debug_abort(); /* Not reachable */
    }
}

/* Change the state of a remote process */
static void sctk_ib_cm_change_state_to_rtr ( sctk_rail_info_t *rail,
        sctk_endpoint_t *endpoint,
        enum sctk_ib_cm_change_state_type_e type )
{
    struct sctk_ib_qp_s *remote = endpoint->data.ib.remote;
    sctk_endpoint_state_t state;
    char txt[256];
    sctk_ib_rail_info_t *rail_ib = &rail->network.ib;
    LOAD_DEVICE ( rail_ib );

    if ( type == CONNECTION )
    {
        state = sctk_ibuf_rdma_cas_remote_state_rtr ( remote, STATE_CONNECTING, STATE_CONNECTED );
        sprintf ( txt, MPC_COLOR_GREEN ( RTR CONNECTED ) );
        assume ( state == STATE_CONNECTING );

    }
    else
        if ( type == RESIZING )
        {
            if ( remote->rdma.pool->resizing_request.recv_keys.nb == 0 )
            {
                /* We deconnect */
                /* Change the state of the route */
                state = sctk_ibuf_rdma_cas_remote_state_rtr ( remote, STATE_FLUSHED, STATE_DECONNECTED );
                sprintf ( txt, MPC_COLOR_RED ( RTR DECONNECTED ) );

            }
            else
            {
                /* We connect */
                state = sctk_ibuf_rdma_cas_remote_state_rtr ( remote, STATE_FLUSHED, STATE_CONNECTED );
                sprintf ( txt, MPC_COLOR_BLUE ( RTR RESIZED ) );

            }

            assume ( state == STATE_FLUSHED );
        }
        else
        {
            not_reachable();
        }

    sctk_ib_debug ( "%s with process %d (nb:%d->%d, size:%d->%d rdma_connections:%d allocated:%.0fko)",   txt,
            remote->rank,
            remote->rdma.pool->region[REGION_RECV].nb_previous,
            remote->rdma.pool->region[REGION_RECV].nb,
            remote->rdma.pool->region[REGION_RECV].size_ibufs_previous,
            remote->rdma.pool->region[REGION_RECV].size_ibufs,
            device->eager_rdma_connections,
            (double) sctk_ibuf_rdma_get_regions_get_allocate_size ( remote ) / 1024.0 );
}

/* Change the state of a remote process */
static void sctk_ib_cm_change_state_to_rts ( sctk_rail_info_t *rail,
        sctk_endpoint_t *endpoint,
        enum sctk_ib_cm_change_state_type_e type )
{
    struct sctk_ib_qp_s *remote = endpoint->data.ib.remote;
    sctk_ib_rail_info_t *rail_ib = &rail->network.ib;
    LOAD_DEVICE ( rail_ib );
    sctk_endpoint_state_t state;
    char txt[256];

    if ( type == CONNECTION )
    {
        state = sctk_ibuf_rdma_cas_remote_state_rts ( remote, STATE_CONNECTING, STATE_CONNECTED );
        sprintf ( txt, MPC_COLOR_GREEN ( RTS CONNECTED ) );
        assume ( state == STATE_CONNECTING );

    }
    else
        if ( type == RESIZING )
        {
            if ( remote->rdma.pool->resizing_request.send_keys.nb == 0 )
            {
                /* We deconnect */
                state = sctk_ibuf_rdma_cas_remote_state_rts ( remote, STATE_FLUSHED, STATE_DECONNECTED );
                sprintf ( txt, MPC_COLOR_RED ( RTS DISCONNECTED ) );

            }
            else
            {
                /* We connect */
                state = sctk_ibuf_rdma_cas_remote_state_rts ( remote, STATE_FLUSHED, STATE_CONNECTED );
                sprintf ( txt, MPC_COLOR_BLUE ( RTS RESIZED ) );

            }

            assume ( state == STATE_FLUSHED );
        }
        else
        {
            not_reachable();
        }

    /* Change the state of the route */
    sctk_ib_debug ( "%s with process %d (nb:%d->%d, size:%d->%d, rdma_connections:%d  allocated:%0.fko ts:%f)", txt,
            remote->rank,
            remote->rdma.pool->region[REGION_SEND].nb_previous,
            remote->rdma.pool->region[REGION_SEND].nb,
            remote->rdma.pool->region[REGION_SEND].size_ibufs_previous,
            remote->rdma.pool->region[REGION_SEND].size_ibufs,
            device->eager_rdma_connections,
            (double) sctk_ibuf_rdma_get_regions_get_allocate_size ( remote ) / 1024.0,
            mpc_arch_get_timestamp_gettimeofday() - remote->rdma.creation_timestamp );

}

/*-----------------------------------------------------------
 *  STATIC CONNEXIONS : intialization to a ring topology
 *  Messages are exchange in a string format. This is because
 *  we are using the PMI library.
 *----------------------------------------------------------*/
void sctk_ib_cm_connect_ring ( sctk_rail_info_t *rail )
{
    sctk_ib_rail_info_t *rail_ib = &rail->network.ib;
    int dest_rank;
    int src_rank;
    sctk_endpoint_t *endpoint_src, *endpoint_dest;
    sctk_ib_route_info_t *route_dest, *route_src;
    sctk_ib_cm_qp_connection_t keys;

    ib_assume ( rail->send_message_from_network != NULL );

    dest_rank = ( mpc_common_get_process_rank() + 1 ) % mpc_common_get_process_count();
    src_rank = ( mpc_common_get_process_rank() + mpc_common_get_process_count() - 1 ) % mpc_common_get_process_count();

    if ( mpc_common_get_process_count() > 2 )
    {
        /* XXX: Set QP in a Ready-To-Send mode. Ideally, we should check that
         * the remote QP has sent an ack */

        /* create remote for dest */
        endpoint_dest = sctk_ib_create_remote();
        sctk_ib_init_remote ( dest_rank, rail, endpoint_dest, 0 );
        route_dest = &endpoint_dest->data.ib;

        /* create remote for src */
        endpoint_src = sctk_ib_create_remote();
        sctk_ib_init_remote ( src_rank, rail, endpoint_src, 0 );
        route_src = &endpoint_src->data.ib;

        mpc_common_debug_warning("SENDING RAIL %d DEST %d", rail_ib, route_dest->remote);
        sctk_ib_qp_keys_send ( rail_ib, route_dest->remote );

        mpc_launch_pmi_barrier();

        /* change state to RTR */
        
        mpc_common_debug_warning("RCVING RAIL %d DEST %d", rail_ib, src_rank);
        keys = sctk_ib_qp_keys_recv ( rail_ib, src_rank );
        sctk_ib_qp_allocate_rtr ( rail_ib, route_src->remote, &keys );
        sctk_ib_qp_allocate_rts ( rail_ib, route_src->remote );
        sctk_ib_qp_keys_send ( rail_ib, route_src->remote );
        mpc_launch_pmi_barrier();

        keys = sctk_ib_qp_keys_recv ( rail_ib, dest_rank );
        sctk_ib_qp_allocate_rtr ( rail_ib, route_dest->remote, &keys );
        sctk_ib_qp_allocate_rts ( rail_ib, route_dest->remote );
        mpc_launch_pmi_barrier();

        sctk_rail_add_static_route ( rail, endpoint_dest );
        sctk_rail_add_static_route (  rail, endpoint_src );

        /* Change to connected */
        sctk_ib_cm_change_state_connected ( rail, endpoint_dest );
        sctk_ib_cm_change_state_connected ( rail, endpoint_src );
    }
    else
    {
        mpc_common_nodebug ( "Send msg to rail %d", rail->rail_number );
        /* create remote for dest */
        endpoint_dest = sctk_ib_create_remote();
        sctk_ib_init_remote ( dest_rank, rail, endpoint_dest, 0 );
        route_dest = &endpoint_dest->data.ib;

        mpc_common_debug_warning("BBB SENDING RAIL %d DEST %d", rail_ib, route_dest->remote);
        sctk_ib_qp_keys_send ( rail_ib, route_dest->remote );
        mpc_launch_pmi_barrier();


        mpc_common_debug_warning("BBB RCV RAIL %d DEST %d", rail_ib, src_rank);
        /* change state to RTR */
        keys = sctk_ib_qp_keys_recv ( rail_ib, src_rank );
        sctk_ib_qp_allocate_rtr ( rail_ib, route_dest->remote, &keys );
        sctk_ib_qp_allocate_rts ( rail_ib, route_dest->remote );
        mpc_launch_pmi_barrier();

        sctk_rail_add_static_route (  rail, endpoint_dest );

        /* Change to connected */
        sctk_ib_cm_change_state_connected ( rail, endpoint_dest );
    }

    mpc_common_nodebug ( "Recv from %d, send to %d", src_rank, dest_rank );
}


/*-----------------------------------------------------------
 *  STATIC CONNEXIONS : process to process connexions during intialization
 *  time
 *  Messages are sent using raw data (not like ring where messages are converted into
 *  string).
 *----------------------------------------------------------*/
void sctk_ib_cm_connect_to ( int from, int to, sctk_rail_info_t *rail )
{
    /* We assume that the node we want to connect to is not the current  */
    assume ( from != to );
    sctk_ib_rail_info_t *rail_ib = &rail->network.ib;
    LOAD_DEVICE ( rail_ib );
    sctk_endpoint_t *endpoint;
    struct sctk_ib_qp_s *remote;
    sctk_ib_cm_qp_connection_t send_keys;
    sctk_ib_cm_qp_connection_t recv_keys;
    sctk_ib_cm_done_t done;
    mpc_common_nodebug ( "Connection TO from %d to %d", from, to );

    /* create remote for dest */
    endpoint = sctk_ib_create_remote();
    sctk_ib_init_remote ( from, rail, endpoint, 0 );
    remote = endpoint->data.ib.remote;
    sctk_ib_debug ( "[%d] QP connection request to process %d", rail->rail_number, remote->rank );

    sctk_route_messages_recv ( from, to, MPC_LOWCOMM_CONTROL_MESSAGE_INTERNAL, CM_OD_STATIC_TAG, &recv_keys, sizeof ( sctk_ib_cm_qp_connection_t ) );
    sctk_ib_qp_allocate_rtr ( rail_ib, remote, &recv_keys );

    sctk_ib_qp_key_fill ( &send_keys, device->port_attr.lid,
            remote->qp->qp_num, remote->psn );
    send_keys.rail_id = rail->rail_number;

    sctk_route_messages_send ( to, from, MPC_LOWCOMM_CONTROL_MESSAGE_INTERNAL, CM_OD_STATIC_TAG , &send_keys, sizeof ( sctk_ib_cm_qp_connection_t ) );
    sctk_route_messages_recv ( from, to, MPC_LOWCOMM_CONTROL_MESSAGE_INTERNAL, CM_OD_STATIC_TAG , &done,
            sizeof ( sctk_ib_cm_done_t ) );
    sctk_ib_qp_allocate_rts ( rail_ib, remote );
    /* Add route */
    sctk_rail_add_static_route (  rail, endpoint );
    /* Change to connected */
    sctk_ib_cm_change_state_connected ( rail, endpoint );
}

void sctk_ib_cm_connect_from ( int from, int to, sctk_rail_info_t *rail )
{
    /* We assume that the node we want to connect to is not the current  */
    assume ( from != to );
    sctk_ib_rail_info_t *rail_ib = &rail->network.ib;
    LOAD_DEVICE ( rail_ib );
    sctk_endpoint_t *endpoint;
    struct sctk_ib_qp_s *remote;
    sctk_ib_cm_qp_connection_t send_keys;
    sctk_ib_cm_qp_connection_t recv_keys;

    /* Message to exchange to the peer */
    sctk_ib_cm_done_t done =
    {
        .rail_id = rail->rail_number,
        .done = 1,
    };
    mpc_common_nodebug ( "Connection FROM from %d to %d", from, to );

    /* create remote for dest */
    endpoint = sctk_ib_create_remote();
    sctk_ib_init_remote ( to, rail, endpoint, 0 );
    remote = endpoint->data.ib.remote;
    sctk_ib_debug ( "[%d] QP connection  request to process %d", rail->rail_number, remote->rank );

    assume ( remote->qp );
    sctk_ib_qp_key_fill ( &send_keys, device->port_attr.lid,
            remote->qp->qp_num, remote->psn );
    send_keys.rail_id = rail->rail_number;

    sctk_route_messages_send ( from, to, MPC_LOWCOMM_CONTROL_MESSAGE_INTERNAL, CM_OD_STATIC_TAG, &send_keys, sizeof ( sctk_ib_cm_qp_connection_t ) );
    sctk_route_messages_recv ( to, from, MPC_LOWCOMM_CONTROL_MESSAGE_INTERNAL, CM_OD_STATIC_TAG, &recv_keys, sizeof ( sctk_ib_cm_qp_connection_t ) );
    sctk_ib_qp_allocate_rtr ( rail_ib, remote, &recv_keys );
    sctk_ib_qp_allocate_rts ( rail_ib, remote );
    mpc_common_nodebug ( "FROM: Ready to send to %d", to );

    sctk_route_messages_send ( from, to, MPC_LOWCOMM_CONTROL_MESSAGE_INTERNAL, CM_OD_STATIC_TAG, &done,
            sizeof ( sctk_ib_cm_done_t ) );
    /* Add route */
    sctk_rail_add_static_route (  rail, endpoint );
    /* Change to connected */
    sctk_ib_cm_change_state_connected ( rail, endpoint );
}

/*-----------------------------------------------------------
 *  ON DEMAND CONNEXIONS: process to process connexions during run time.
 *  Messages are sent using raw data (not like ring where messages are converted into
 *  string).
 *----------------------------------------------------------*/
static inline void sctk_ib_cm_on_demand_recv_done ( sctk_rail_info_t *rail, int src )
{
    sctk_ib_rail_info_t *rail_ib_targ = &rail->network.ib;

    sctk_endpoint_t *endpoint = sctk_rail_get_dynamic_route_to_process ( rail, src );
    ib_assume ( endpoint );
    struct sctk_ib_qp_s *remote = endpoint->data.ib.remote;
    ib_assume ( remote );

    ib_assume ( sctk_ib_qp_allocate_get_rts ( remote ) == 0 );
    sctk_ib_qp_allocate_rts ( rail_ib_targ, remote );

    /* Change to connected */
    sctk_ib_cm_change_state_connected ( rail, endpoint );
}

static inline void sctk_ib_cm_on_demand_recv_ack ( sctk_rail_info_t *rail, void *ack, int src )
{
    sctk_ib_rail_info_t *rail_ib_targ = &rail->network.ib;

    sctk_ib_cm_qp_connection_t recv_keys;
    sctk_ib_cm_done_t done =
    {
        .rail_id = * ( ( int * ) ack ),
        .done = 1,
    };

    sctk_ib_nodebug ( "OD QP connexion ACK received from process %d %s", src, ack );
    sctk_endpoint_t *endpoint = sctk_rail_get_dynamic_route_to_process ( rail, src );
    ib_assume ( endpoint );
    struct sctk_ib_qp_s *remote = endpoint->data.ib.remote;
    ib_assume ( remote );
    memcpy ( &recv_keys, ack, sizeof ( sctk_ib_cm_qp_connection_t ) );

    ib_assume ( sctk_ib_qp_allocate_get_rtr ( remote ) == 0 );
    sctk_ib_qp_allocate_rtr ( rail_ib_targ, remote, &recv_keys );

    ib_assume ( sctk_ib_qp_allocate_get_rts ( remote ) == 0 );
    sctk_ib_qp_allocate_rts ( rail_ib_targ, remote );

    /* SEND MESSAGE */
    sctk_control_messages_send_rail ( src, CM_OD_DONE_TAG, 0, &done, sizeof ( sctk_ib_cm_done_t ) , rail->rail_number);

    /* Change to connected */
    sctk_ib_cm_change_state_connected ( rail, endpoint );
}

int sctk_ib_cm_on_demand_recv_request ( sctk_rail_info_t *rail, void *request, int src )
{
    sctk_ib_rail_info_t *rail_ib_targ = &rail->network.ib;
    LOAD_DEVICE ( rail_ib_targ );
    sctk_ib_cm_qp_connection_t send_keys;
    sctk_ib_cm_qp_connection_t recv_keys;
    int added;

    /* create remote for source */
    sctk_endpoint_t *endpoint = sctk_rail_add_or_reuse_route_dynamic ( rail, src, sctk_ib_create_remote, sctk_ib_init_remote, &added, 0 );
    ib_assume ( endpoint );

    ROUTE_LOCK ( endpoint );

    if ( sctk_endpoint_get_state ( endpoint ) == STATE_DECONNECTED )
    {
        sctk_endpoint_set_state ( endpoint, STATE_CONNECTING );
    }

    ROUTE_UNLOCK ( endpoint );

    sctk_ib_debug ( "[%d] OD QP connexion request from %d (initiator:%d) START",
            rail->rail_number, src, sctk_endpoint_get_is_initiator ( endpoint ) );

    /* RACE CONDITION AVOIDING -> positive ACK */
    if ( sctk_endpoint_get_is_initiator ( endpoint ) == 0 || mpc_common_get_process_rank() > src )
    {
        struct sctk_ib_qp_s *remote = endpoint->data.ib.remote;
        sctk_ib_debug ( "[%d] OD QP connexion request to process %d (initiator:%d)",
                rail->rail_number, remote->rank, sctk_endpoint_get_is_initiator ( endpoint ) );
        memcpy ( &recv_keys, request, sizeof ( sctk_ib_cm_qp_connection_t ) );

        ib_assume ( sctk_ib_qp_allocate_get_rtr ( remote ) == 0 );
        sctk_ib_qp_allocate_rtr ( rail_ib_targ, remote, &recv_keys );

        sctk_ib_qp_key_fill ( &send_keys, device->port_attr.lid,
                remote->qp->qp_num, remote->psn );
        send_keys.rail_id = rail->rail_number;

        /* Send ACK */
        sctk_ib_debug ( "OD QP ack to process %d: (tag:%d)", src, CM_OD_ACK_TAG );

        sctk_control_messages_send_rail ( src, CM_OD_ACK_TAG, 0,&send_keys, sizeof ( sctk_ib_cm_qp_connection_t ), rail->rail_number );
    }

    return 0;
}


/*
 * Send a connexion request to the process 'dest'
 */
sctk_endpoint_t *sctk_ib_cm_on_demand_request ( int dest, sctk_rail_info_t *rail )
{
    sctk_ib_rail_info_t *rail_ib_targ = &rail->network.ib;
    LOAD_DEVICE ( rail_ib_targ );
    sctk_endpoint_t *endpoint;
    sctk_ib_cm_qp_connection_t send_keys;
    /* Message to exchange to the peer */
    int added;
    /* If we need to send the request */
    int send_request = 0;

    /* create remote for dest if it does not exist */
    endpoint = sctk_rail_add_or_reuse_route_dynamic ( rail, dest, sctk_ib_create_remote, sctk_ib_init_remote, &added, 1 );
    ib_assume ( endpoint );

    ROUTE_LOCK ( endpoint );

    if ( sctk_endpoint_get_state ( endpoint ) == STATE_DECONNECTED )
    {
        sctk_endpoint_set_state ( endpoint, STATE_CONNECTING );
        send_request = 1;
    }

    ROUTE_UNLOCK ( endpoint );

    /* If we are the first to access the route and if the state
     * is deconnected, so we can proceed to a connection*/
    if ( send_request )
    {
        struct sctk_ib_qp_s *remote = endpoint->data.ib.remote;

        sctk_ib_qp_key_fill ( &send_keys, device->port_attr.lid, remote->qp->qp_num, remote->psn );
        send_keys.rail_id = rail->rail_number;

        sctk_ib_debug ( "[%d] OD QP connexion requested to %d", rail->rail_number, remote->rank );
        sctk_control_messages_send_rail ( dest, CM_OD_REQ_TAG, 0, &send_keys, sizeof ( sctk_ib_cm_qp_connection_t ), rail->rail_number );
    }

    return endpoint;
}


/*-----------------------------------------------------------
 *  ON DEMAND RDMA CONNEXIONS:
 *  Messages are sent using raw data (not like ring where messages are converted into
 *  string).
 *----------------------------------------------------------*/

/* Function which returns if a remote can be connected using RDMA */
int sctk_ib_cm_on_demand_rdma_check_request ( __UNUSED__ sctk_rail_info_t *rail, struct sctk_ib_qp_s *remote )
{
    int send_request = 0;
    ib_assume ( sctk_endpoint_get_state ( remote->endpoint ) == STATE_CONNECTED );

    /* If several threads call this function, only 1 will send a request to
     * the remote process */
    ROUTE_LOCK ( remote->endpoint );

    if ( sctk_ibuf_rdma_get_remote_state_rts ( remote ) == STATE_DECONNECTED )
    {
        sctk_ibuf_rdma_set_remote_state_rts ( remote, STATE_CONNECTING );
        send_request = 1;
    }

    ROUTE_UNLOCK ( remote->endpoint );

    return send_request;
}

/*
 * Send a connexion request to the process 'dest'.
 * Data exchanged:
 * - size of slots
 * - number of slots
 */
int sctk_ib_cm_on_demand_rdma_request ( sctk_rail_info_t *rail, struct sctk_ib_qp_s *remote, int entry_size, int entry_nb )
{
    sctk_ib_rail_info_t *rail_ib_targ = &rail->network.ib;
    LOAD_DEVICE ( rail_ib_targ );
    /* If we need to send the request */
    ib_assume ( sctk_endpoint_get_state ( remote->endpoint ) == STATE_CONNECTED );


    /* If we are the first to access the route and if the state
     * is deconnected, so we can proceed to a connection*/

    if ( sctk_ibuf_rdma_is_connectable ( rail_ib_targ ) )
    {
        /* Can connect to RDMA */

        sctk_ib_cm_rdma_connection_t send_keys;
        mpc_common_nodebug ( "Can connect to remote %d", remote->rank );

        send_keys.connected = 1;
        /* We fill the request and we save how many slots are requested as well
         * as the size of each slot */
        remote->od_request.nb = send_keys.nb = entry_nb;
        remote->od_request.size_ibufs = send_keys.size = entry_size;

        sctk_ib_debug ( "[%d] OD QP RDMA connexion requested to %d (size:%d nb:%d rdma_connections:%d rdma_cancel:%d)",
                rail->rail_number, remote->rank, send_keys.size, send_keys.nb, device->eager_rdma_connections, remote->rdma.cancel_nb );

        send_keys.rail_id = rail->rail_number;
        sctk_control_messages_send_rail ( remote->rank, CM_OD_RDMA_REQ_TAG, 0, &send_keys, sizeof ( sctk_ib_cm_rdma_connection_t ), rail->rail_number );
    }
    else
    {
        /* Cannot connect to RDMA */
        mpc_common_debug( "[%d] Cannot connect to remote %d", rail->rail_number, remote->rank );
        /* We reset the state to deconnected */
        /* FIXME: state to reset */
        sctk_ibuf_rdma_set_remote_state_rts ( remote, STATE_DECONNECTED );
    }

    return 1;
}

static inline void sctk_ib_cm_on_demand_rdma_done_recv ( sctk_rail_info_t *rail, void *done, int src )
{

    sctk_ib_cm_rdma_connection_t *recv_keys = ( sctk_ib_cm_rdma_connection_t * ) done;

    /* get the route to process */
    sctk_endpoint_t *endpoint = sctk_rail_get_any_route_to_process ( rail, src );
    ib_assume ( endpoint );
    struct sctk_ib_qp_s *remote = endpoint->data.ib.remote;
    ib_assume ( remote );

    sctk_ib_debug ( "[%d] OD QP connexion DONE REQ received from process %d (%p:%u)", rail->rail_number, src,
            recv_keys->addr, recv_keys->rkey );

    ib_assume ( recv_keys->connected == 1 );
    /* Update the RDMA regions */
    sctk_ibuf_rdma_update_remote_addr ( remote, recv_keys, REGION_SEND );
    sctk_ib_cm_change_state_to_rtr ( rail, endpoint, CONNECTION );
}

static inline void sctk_ib_cm_on_demand_rdma_recv_ack ( sctk_rail_info_t *rail, void *ack, int src )
{
    sctk_ib_rail_info_t *rail_ib_targ = &rail->network.ib;
    sctk_ib_cm_rdma_connection_t send_keys;
    sctk_ib_cm_rdma_connection_t *recv_keys = ( sctk_ib_cm_rdma_connection_t * ) ack;

    /* get the route to process */
    sctk_endpoint_t *endpoint = sctk_rail_get_any_route_to_process ( rail, src );
    ib_assume ( endpoint );
    struct sctk_ib_qp_s *remote = endpoint->data.ib.remote;
    ib_assume ( remote );

    sctk_ib_debug ( "[%d] OD QP connexion ACK received from process %d (%p:%u)", rail->rail_number, src,
            recv_keys->addr, recv_keys->rkey );

    /* If the remote peer is connectable */
    if ( recv_keys->connected == 1 )
    {
        /* Allocate the buffer */
        sctk_ibuf_rdma_pool_init ( remote );
        /* We create the SEND region */
        sctk_ibuf_rdma_region_init ( rail_ib_targ, remote,
                &remote->rdma.pool->region[REGION_SEND],
                RDMA_CHANNEL | SEND_CHANNEL, remote->od_request.nb, remote->od_request.size_ibufs );

        /* Update the RDMA regions */
        sctk_ibuf_rdma_update_remote_addr ( remote, recv_keys, REGION_RECV );
        /* Fill the keys */
        sctk_ibuf_rdma_fill_remote_addr ( remote, &send_keys, REGION_SEND );
        send_keys.rail_id = * ( ( int * ) ack );

        /* Send the message */
        send_keys.connected = 1;
        sctk_control_messages_send_rail ( src, CM_OD_RDMA_DONE_TAG, 0,  &send_keys, sizeof ( sctk_ib_cm_rdma_connection_t ), rail->rail_number );

        sctk_ib_cm_change_state_to_rts ( rail, endpoint, CONNECTION );

    }
    else
    {
        /* cannot connect */
        /* We cancel the RDMA connection */
        sctk_ibuf_rdma_connection_cancel ( rail_ib_targ, remote );
    }
}

/*
 * Receive a request for a source process
 * Data exchanged:
 * - size of slots
 * - number of slots
 * - Address of the send region
 * - Address of the recv region
 */
static inline void sctk_ib_cm_on_demand_rdma_recv_request ( sctk_rail_info_t *rail, void *request, int src )
{
    sctk_ib_rail_info_t *rail_ib_targ = &rail->network.ib;
    sctk_ib_cm_rdma_connection_t send_keys;
    memset ( &send_keys, 0, sizeof ( sctk_ib_cm_rdma_connection_t ) );

    /* get the route to process */
    sctk_endpoint_t *endpoint = sctk_rail_get_any_route_to_process ( rail, src );
    ib_assume ( endpoint );
    /* We assume the route is connected */
    struct sctk_ib_qp_s *remote = endpoint->data.ib.remote;
    ib_assume ( sctk_endpoint_get_state ( endpoint ) == STATE_CONNECTED );


    ROUTE_LOCK ( endpoint );

    if ( sctk_ibuf_rdma_get_remote_state_rtr ( remote ) == STATE_DECONNECTED )
    {
        sctk_ibuf_rdma_set_remote_state_rtr ( remote, STATE_CONNECTING );
    }

    ROUTE_UNLOCK ( endpoint );

    sctk_ib_cm_rdma_connection_t *recv_keys = ( sctk_ib_cm_rdma_connection_t * ) request;
    sctk_ib_debug ( "[%d] OD RDMA connexion REQUEST to process %d (connected:%d size:%d nb:%d )",
            rail->rail_number, remote->rank, recv_keys->connected, recv_keys->size, recv_keys->nb );

    /* We do not send a request if we do not want to be connected
     * using RDMA. This is stupid :-) */
    ib_assume ( recv_keys->connected == 1 );

    /* We check if we can also be connected using RDMA */
    if ( sctk_ibuf_rdma_is_connectable ( rail_ib_targ ) )
    {
        /* Can connect to RDMA */

        /* We can change to RTR because we are RDMA connectable */
        send_keys.connected = 1;

        /* We firstly allocate the main structure. 'ibuf_rdma_pool_init'
         * implicitely does not allocate memory if already created */
        sctk_ibuf_rdma_pool_init ( remote );
        /* We create the RECV region */
        sctk_ibuf_rdma_region_init ( rail_ib_targ, remote,
                &remote->rdma.pool->region[REGION_RECV],
                RDMA_CHANNEL | RECV_CHANNEL, recv_keys->nb, recv_keys->size );

        /* Fill the keys */
        sctk_ibuf_rdma_fill_remote_addr ( remote, &send_keys, REGION_RECV );
    }
    else
    {
        /* Cannot connect to RDMA */
        mpc_common_nodebug ( "Cannot connect to remote %d", remote->rank );

        send_keys.connected = 0;
        send_keys.size = 0;
        send_keys.nb = 0;
        send_keys.rail_id = * ( ( int * ) request );
    }

    /* Send ACK */
    sctk_ib_debug ( "[%d] OD QP ack to process %d (%p:%u)", rail->rail_number, src,
            send_keys.addr, send_keys.rkey );

    sctk_control_messages_send_rail ( src, CM_OD_RDMA_ACK_TAG, 0, &send_keys, sizeof ( sctk_ib_cm_rdma_connection_t ) , rail->rail_number);
}


/*-----------------------------------------------------------
 *  ON DEMAND RDMA RESIZING:
 *  Messages are sent using raw data (not like ring where messages are converted into
 *  string).
 *----------------------------------------------------------*/

/*
 * Send a connexion request to the process 'dest'.
 * Data exchanged:
 * - size of slots
 * - number of slots. If 0, the remote needs to be deconnected
 *
 */
int sctk_ib_cm_resizing_rdma_request ( sctk_rail_info_t *rail, struct sctk_ib_qp_s *remote, int entry_size, int entry_nb )
{

    /* Assume that the route is in a flushed state */
    ib_assume ( sctk_ibuf_rdma_get_remote_state_rts ( remote ) == STATE_FLUSHED );
    /* Assume there is no more pending messages */
    ib_assume ( OPA_load_int ( &remote->rdma.pool->busy_nb[REGION_SEND] ) == 0 );

    sctk_ib_cm_rdma_connection_t send_keys;

    send_keys.connected = 1;
    send_keys.nb = entry_nb;
    send_keys.size = entry_size;
    send_keys.rail_id = rail->rail_number;

    sctk_ib_nodebug ( "[%d] Sending RDMA RESIZING request to %d (size:%d nb:%d resizing_nb:%d)",
            rail->rail_number, remote->rank, send_keys.size, send_keys.nb,
            remote->rdma.resizing_nb );

    sctk_control_messages_send_rail ( remote->rank, CM_RESIZING_RDMA_REQ_TAG, 0, &send_keys, sizeof ( sctk_ib_cm_rdma_connection_t ), rail->rail_number );

    return 1;
}

void sctk_ib_cm_resizing_rdma_ack ( sctk_rail_info_t *rail,  struct sctk_ib_qp_s *remote, sctk_ib_cm_rdma_connection_t *send_keys )
{

    ib_assume ( sctk_endpoint_get_state ( remote->endpoint ) == STATE_CONNECTED );
    /* Assume that the route is in a flushed state */
    ib_assume ( sctk_ibuf_rdma_get_remote_state_rtr ( remote ) == STATE_FLUSHED );
    /* Assume there is no more pending messages */
    ib_assume ( OPA_load_int ( &remote->rdma.pool->busy_nb[REGION_RECV] ) == 0 );

    sctk_ib_nodebug ( "[%d] Sending RDMA RESIZING ACK to %d (addr:%p - rkey:%u)",
            rail->rail_number, remote->rank, send_keys->addr, send_keys->rkey );

    send_keys->rail_id = rail->rail_number;
    sctk_control_messages_send_rail ( remote->rank, CM_RESIZING_RDMA_ACK_TAG, 0, send_keys, sizeof ( sctk_ib_cm_rdma_connection_t ) , rail->rail_number );
}

static inline void sctk_ib_cm_resizing_rdma_done_recv ( sctk_rail_info_t *rail, void *done, int src )
{
    sctk_ib_cm_rdma_connection_t *recv_keys = ( sctk_ib_cm_rdma_connection_t * ) done;

    /* get the route to process */
    sctk_endpoint_t *endpoint = sctk_rail_get_any_route_to_process ( rail, src );
    ib_assume ( endpoint );
    struct sctk_ib_qp_s *remote = endpoint->data.ib.remote;
    ib_assume ( remote );

    sctk_ib_nodebug ( "[%d] RDMA RESIZING done req received from process %d (%p:%u)", rail->rail_number, src,
            recv_keys->addr, recv_keys->rkey );

    ib_assume ( recv_keys->connected == 1 );

    /* Update the RDMA regions */
    sctk_ibuf_rdma_update_remote_addr ( remote, recv_keys, REGION_SEND );

    sctk_ib_cm_change_state_to_rtr ( rail, endpoint, RESIZING );
}


static inline void sctk_ib_cm_resizing_rdma_ack_recv ( sctk_rail_info_t *rail, void *ack, int src )
{
    sctk_ib_rail_info_t *rail_ib_targ = &rail->network.ib;
    sctk_ib_cm_rdma_connection_t *recv_keys = ( sctk_ib_cm_rdma_connection_t * ) ack;

    /* get the route to process */
    sctk_endpoint_t *endpoint = sctk_rail_get_any_route_to_process_or_forward ( rail, src );
    ib_assume ( endpoint );
    struct sctk_ib_qp_s *remote = endpoint->data.ib.remote;
    ib_assume ( remote );

    sctk_ib_nodebug ( "[%d] RDMA RESIZING ACK received from process %d (addr:%p rkey:%u)", rail->rail_number, src, recv_keys->addr, recv_keys->rkey );

    /* Update the RDMA regions */
    /* FIXME: the rail number should be determinated */
    sctk_ibuf_rdma_update_remote_addr ( remote, recv_keys, REGION_RECV );

    /* If the remote peer is connectable */
    sctk_ib_cm_rdma_connection_t *send_keys =
        &remote->rdma.pool->resizing_request.send_keys;
    /* FIXME: do some cool stuff here */
    /* Resizing the RDMA buffer */
    sctk_ibuf_rdma_region_reinit ( rail_ib_targ, remote,
            &remote->rdma.pool->region[REGION_SEND],
            RDMA_CHANNEL | SEND_CHANNEL,
            send_keys->nb, send_keys->size );

    OPA_incr_int ( &remote->rdma.resizing_nb );
    send_keys->connected = 1;
    send_keys->rail_id = rail_ib_targ->rail->rail_number;
    sctk_ibuf_rdma_fill_remote_addr ( remote, send_keys, REGION_SEND );

    sctk_control_messages_send_rail ( src, CM_RESIZING_RDMA_DONE_TAG, 0, send_keys, sizeof ( sctk_ib_cm_rdma_connection_t ), rail->rail_number );

    sctk_ib_cm_change_state_to_rts ( rail, endpoint, RESIZING );
}

/*
 * Receive a request for a source process
 * Data exchanged:
 * - size of slots
 * - number of slots
 * - Address of the send region
 * - Address of the recv region
 */
static inline int sctk_ib_cm_resizing_rdma_recv_request ( sctk_rail_info_t *rail, void *request, int src )
{
    sctk_ib_rail_info_t *rail_ib_targ = &rail->network.ib;
    sctk_ib_cm_rdma_connection_t send_keys;
    memset ( &send_keys, 0, sizeof ( sctk_ib_cm_rdma_connection_t ) );

    /* get the route to process */
    sctk_endpoint_t *endpoint = sctk_rail_get_any_route_to_process_or_forward ( rail, src );
    ib_assume ( endpoint );
    /* We assume the route is connected */
    ib_assume ( sctk_endpoint_get_state ( endpoint ) == STATE_CONNECTED );
    struct sctk_ib_qp_s *remote = endpoint->data.ib.remote;
    ib_assume ( remote );

    sctk_ib_cm_rdma_connection_t *recv_keys = ( sctk_ib_cm_rdma_connection_t * ) request;
    ib_assume ( recv_keys->connected == 1 );
    remote->rdma.pool->resizing_request.recv_keys.nb   = recv_keys->nb;
    remote->rdma.pool->resizing_request.recv_keys.size = recv_keys->size;

    sctk_ib_nodebug ( "[%d] Receiving RDMA RESIZING request to process %d (connected:%d size:%d nb:%d)",
            rail->rail_number, remote->rank, recv_keys->connected, recv_keys->size, recv_keys->nb );

    /* Start flushing */
    sctk_ibuf_rdma_flush_recv ( rail_ib_targ, remote );

    return 0;
}


/*-----------------------------------------------------------
 *  Handler of OD connexions
 *----------------------------------------------------------*/

void sctk_ib_cm_control_message_handler( struct sctk_rail_info_s * rail, int process_src, __UNUSED__ int source_rank, char subtype, __UNUSED__ char param, void * payload, __UNUSED__ size_t size )
{

    switch ( subtype )
    {
        /* QP connection */
        case CM_OD_REQ_TAG:
            sctk_ib_cm_on_demand_recv_request ( rail, payload, process_src );
            break;

        case CM_OD_ACK_TAG:
            sctk_ib_cm_on_demand_recv_ack ( rail,  payload, process_src );
            break;

        case CM_OD_DONE_TAG:
            sctk_ib_cm_on_demand_recv_done ( rail,  process_src );
            break;

            /* RDMA connection */
        case CM_OD_RDMA_REQ_TAG:
            sctk_ib_cm_on_demand_rdma_recv_request ( rail,  payload, process_src );
            break;

        case CM_OD_RDMA_ACK_TAG:
            sctk_ib_cm_on_demand_rdma_recv_ack ( rail,  payload, process_src );
            break;

        case CM_OD_RDMA_DONE_TAG:
            sctk_ib_cm_on_demand_rdma_done_recv ( rail,  payload, process_src );
            break;

            /* RDMA resizing */
        case CM_RESIZING_RDMA_REQ_TAG:
            sctk_ib_cm_resizing_rdma_recv_request ( rail,  payload, process_src );
            break;

        case CM_RESIZING_RDMA_ACK_TAG:
            sctk_ib_cm_resizing_rdma_ack_recv ( rail,  payload, process_src );
            break;

        case CM_RESIZING_RDMA_DONE_TAG:
            sctk_ib_cm_resizing_rdma_done_recv ( rail,  payload, process_src );
            break;

        default:
            mpc_common_debug_error ( "Message Type Not Handled" );
            not_reachable();
            break;

    }

}
