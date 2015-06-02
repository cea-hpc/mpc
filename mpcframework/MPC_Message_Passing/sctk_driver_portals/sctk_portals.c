/* ############################# MPC License ############################## */
/* # Wed Nov 19 15:19:19 CET 2008                                         # */
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
/* # Authors:                                                             # */
/* #   - PERACHE Marc marc.perache@cea.fr                                 # */
/* #   - GONCALVES Thomas thomas.goncalves@cea.fr                         # */
/* #                                                                      # */
/* ######################################################################## */

#ifdef MPC_USE_PORTALS
#include <sctk_debug.h>
#include <sctk_portals.h>
#include <sctk_inter_thread_comm.h>

/************ INTER_THEAD_COMM HOOOKS ****************/
//TODO: Refactor and extract portals routine into sctk_portals_toolkit.c
static void sctk_network_send_message_endpoint_portals ( sctk_thread_ptp_message_t *msg, sctk_endpoint_t *rail )
{
    sctk_endpoint_t *tmp;
    size_t size;
    int fd;


    if ( 0 )
    {
        tmp = sctk_rail_get_any_route_to_process_or_on_demand ( rail->rail, SCTK_MSG_DEST_PROCESS ( msg ) ); //proc
    }
    else
    {
        tmp = sctk_rail_get_any_route_to_task_or_on_demand( rail->rail, SCTK_MSG_DEST_TASK ( msg ) ); //task
    }

    sctk_nodebug ( "my %d dest %d", rail->data.portals.my_id.phys.pid, tmp->data.portals.id.phys.pid );
    sctk_nodebug ( "send from %d to %d", SCTK_MSG_SRC_PROCESS ( msg ), SCTK_MSG_DEST_PROCESS ( msg ) );


    if ( sctk_get_peer_process_rank ( SCTK_MSG_SRC_PROCESS ( msg ) ) != sctk_process_rank )
    {
        sctk_Event_t *event 			= ( sctk_Event_t * ) msg->tail.portals_message_info_t; //get the evenet datas
        sctk_portals_message_t *ptrmsg 	= &event->msg;//get the struct message of portals
        int index = ptrmsg->peer_idThread;

        sctk_spinlock_lock ( &rail->data.portals.lock[index] ); //to be thread safe

        sctk_nodebug ( "%d =/ %d", sctk_get_peer_process_rank ( SCTK_MSG_SRC_PROCESS ( msg ) ), sctk_process_rank );

        if ( msg->tail.message.contiguous.size == SCTK_MSG_SIZE ( msg ) )
        {
            sctk_nodebug ( "useless" );
        }
        else
            msg->tail.message.contiguous.size = SCTK_MSG_SIZE ( msg );

        ptl_ct_event_t ctc;

        msg->tail.message.contiguous.addr = sctk_malloc ( msg->tail.message.contiguous.size ); //to get the buffer
        sctk_nodebug ( "allocated %p", msg->tail.portals_message_info_t );

        sctk_portals_rail_info_t *portals_info	= ( sctk_portals_rail_info_t * ) msg->tail.portals_info_t;
        ptl_handle_ni_t *ni_h 					= &portals_info->ni_handle_phys;
        ptl_match_bits_t match, ignore;
        set_Match_Ignore_Bits ( &match, &ignore, ptrmsg->my_idThread, ptrmsg->tag );
        ptrmsg->md.start  	 	= msg->tail.message.contiguous.addr;//to get the buffer

        ptrmsg->md.length 	 	= msg->tail.message.contiguous.size;

        ptrmsg->md.options   	= PTL_MD_EVENT_CT_REPLY;
        ptrmsg->md.eq_handle 	= PTL_EQ_NONE;   // i.e. don't queue get events
        CHECK_RETURNVAL ( PtlMDBind ( *ni_h, &ptrmsg->md, &ptrmsg->md_handle ) );
        CHECK_RETURNVAL ( PtlGet ( ptrmsg->md_handle, 0, ptrmsg->md.length, ptrmsg->peer,
                    ptrmsg->peer_idThread, match, 0, NULL ) );
        sctk_nodebug ( "waiting" );
        CHECK_RETURNVAL ( PtlCTWait ( ptrmsg->md.ct_handle, 1, &ctc ) ); //we need to wait the message for routing
        assert ( ctc.failure == 0 );
        sctk_nodebug ( "will free list" );

        sctk_EventQ_t *EvQ 		= &rail->data.portals.EvQ[index];
        sctk_EventL_t *currList = &EvQ->ListMsg;
        int i, pos = ptrmsg->append_pos;

        //free the structs
        for ( i = 0; i < ptrmsg->append_list; i++ )
            currList = currList->next;

        if ( currList->events[pos].used == IN_USE )
        {

            //we add a new entry on the table
            currList->nb_elems--;
            currList->nb_elems_headers++;
            bzero ( currList->events[pos].ptrmsg.msg_send, sizeof ( sctk_thread_ptp_message_t ) );
            currList->events[pos].used = RESERVED;

            currList->events[pos].msg.me.start  = currList->events[pos].ptrmsg.msg_send;
            currList->events[pos].msg.me.length = sizeof ( sctk_thread_ptp_message_body_t ); //
            currList->events[pos].msg.me.uid    = PTL_UID_ANY;


            currList->events[pos].msg.me.match_id.phys.nid = PTL_NID_ANY;
            currList->events[pos].msg.me.match_id.phys.pid = PTL_PID_ANY;


            currList->events[pos].msg.me.match_bits    = FLAG_REQ;
            currList->events[pos].msg.me.ignore_bits   = REQ_IGN;

            currList->events[pos].msg.me.options = OPTIONS_HEADER;

            CHECK_RETURNVAL ( APPEND ( rail->data.portals.ni_handle_phys, rail->data.portals.pt_index[currList->events[pos].pt_index], &currList->events[pos].msg.me, PTL_PRIORITY_LIST, NULL, &currList->events[pos].msg.me_handle ) );

            ptrmsg->append_pos = -1;

        }
        else
        {
            sctk_error ( "Error portals list free" );
            abort();
        }

        sctk_spinlock_unlock ( &rail->data.portals.lock[index] ); //to be thread safe
    }
    ListAppendMsg ( &rail->data.portals, msg, &tmp->data.portals, WRITE, NULL );
}

static void sctk_network_notify_recv_message_portals ( sctk_thread_ptp_message_t *msg, sctk_rail_info_t *rail )
{
    /*NOTHING TO DO*/
}

static void sctk_network_notify_matching_message_portals ( sctk_thread_ptp_message_t *msg, sctk_rail_info_t *rail )
{
    /*sctk_nodebug ( "message matched %p", msg );*/
    /*NOTHING TO DO*/
}

static void sctk_network_notify_perform_message_portals ( int remote, int remote_task_id, int polling_task_id, int blocking, sctk_rail_info_t *rail )
{
    /*sctk_nodebug ( "perform message through rail %d", rail->rail_number );*/
    /*int i;*/

    /*for ( i = 0; i < rail->network.portals.ntasks; i++ )*/
        /*notify ( rail, i );*/
}

static void sctk_network_notify_idle_message_portals () //plus de calcul,blocage
{
}

static void sctk_network_notify_any_source_message_portals ( int polling_task_id, int blocking, sctk_rail_info_t *rail )
{

    /*sctk_nodebug ( "any_source message through rail %d", rail->rail_number );*/
    /*int i;*/

    /*for ( i = 0; i < rail->network.portals.ntasks; i++ )*/
        /*notify ( rail, i );*/
}

static int sctk_send_message_from_network_portals ( sctk_thread_ptp_message_t *msg )
{
    if ( sctk_send_message_from_network_reorder ( msg ) == REORDER_NO_NUMBERING )
    {
        /* No reordering */
        sctk_send_message_try_check ( msg, 1 );
    }
}


/************ INIT ****************/
void sctk_network_init_portals (sctk_rail_info_t *rail)
{
    /* Register hooks in rail */
    rail->send_message_endpoint = sctk_network_send_message_endpoint_portals;
    rail->notify_recv_message = sctk_network_notify_recv_message_portals;
    rail->notify_matching_message = sctk_network_notify_matching_message_portals;
    rail->notify_perform_message = sctk_network_notify_perform_message_portals;
    rail->notify_idle_message = sctk_network_notify_idle_message_portals;
    rail->notify_any_source_message = sctk_network_notify_any_source_message_portals;
    rail->send_message_from_network = sctk_send_message_from_network_portals;
    rail->network_name = "PORTALS";

    sctk_rail_init_route ( rail, rail->runtime_config_rail->topology, portals_on_demand_connection_handler );
    sctk_network_init_portals_all ( rail );
}

#endif
