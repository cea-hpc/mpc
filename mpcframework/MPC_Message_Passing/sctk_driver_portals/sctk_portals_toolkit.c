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
/* #   - ADAM Julien julien.adam@cea.fr                                   # */
/* #                                                                      # */
/* ######################################################################## */

#ifdef MPC_USE_PORTALS

#include <sctk_route.h>
#include <sctk_portals_toolkit.h>
#include <sctk_pmi.h>
#include <sctk_control_messages.h>
/****************************************************************/
/*				|  ME  |  MEH |  MD  |  MDH |					*/

/*static inline int _md_is_allocated ( uint8_t alloc )*/
/*{*/
    /*unsigned mask = 1;*/
    /*alloc >>= 1;*/
    /*return ( alloc & mask );*/
/*}*/

/*static inline int _mdh_is_allocated ( uint8_t alloc )*/
/*{*/
    /*unsigned mask = 1;*/
    /*return ( alloc & mask );*/
/*}*/

/*static inline int _meh_is_allocated ( uint8_t alloc )*/
/*{*/
    /*unsigned mask = 1;*/
    /*alloc >>= 2;*/
    /*return ( alloc & mask );*/
/*}*/

/*static inline int _me_is_allocated ( uint8_t alloc )*/
/*{*/
    /*alloc >>= 3;*/
    /*return alloc;*/
/*}*/

/*static inline void set_allocBits ( uint8_t *alloc, uint8_t value )*/
/*{*/
    /**alloc += value;*/
/*}*/

/*[>*******************************************************************<]*/

/*void set_Match_Ignore_Bits ( ptl_match_bits_t *match, ptl_match_bits_t *ignore, unsigned id, int tag, char init)*/
/*{*/
    /*[>	[0			1  ------  -  2 -   -------	3			4			5			6			7			] <]*/
    /*[>	[	unused	|unused	 |flag|   thread pos in proc	| 				tag MPI of message 				| <]*/

    /**ignore 		= 0;*/
    /**match			= 0;*/

    /*if ( id != ANY_SOURCE )*/
    /*{*/
        /**match		= id;*/
        /**match		= ( *match << 32 );*/
    /*}*/
    /*else*/
        /**ignore		|= SOURCE_IGN;*/

    /*if ( tag == ANY_TAG )*/
        /**ignore		|= TAG_IGN;*/
    /*else*/
        /**match		+= tag;*/

    /*if(init)*/
        /**match = TAG_INIT + FLAG_REQ;*/
/*}*/

/*[> extract the message tag of the match_bits <]*/
/*inline void get_tag ( int *tag, ptl_match_bits_t match )*/
/*{*/
    /**tag = match & TAG_IGN;*/
/*}*/

/*[> extract the thread id of the peer of the match_bits <]*/
/*inline void get_idThread ( int *peer_idThread, ptl_match_bits_t match )*/
/*{*/
    /**peer_idThread = ( match & SOURCE_IGN ) >> 32;*/
/*}*/

/*[>***************************************************<]*/
/*[> when the matching occurs in mpc we get the message so we need 0 copy <]*/
/*void sctk_portals_message_copy ( sctk_message_to_copy_t *tmp )*/
/*{*/
    /*sctk_thread_ptp_message_t *send;*/
    /*sctk_thread_ptp_message_t *recv;*/

    /*ptl_match_bits_t match, ignore;*/
    /*send = tmp->msg_send;*/
    /*recv = tmp->msg_recv;*/


    /*sctk_portals_event_item_t *event 					= ( sctk_portals_event_item_t * ) send->tail.portals_message_info_t; // get the event informations*/
    /*sctk_portals_message_t *ptrmsg 			= &event->msg;// get the portals struct message*/

    /*if ( ptrmsg == NULL )*/
        /*abort();*/

    /*if ( event == NULL )*/
        /*abort();*/


    /*sctk_portals_rail_info_t *portals_info	= ( sctk_portals_rail_info_t * ) send->tail.portals_info_t; //get the portal rail*/
    /*event->ptrmsg.msg_recv 			= recv;*/
    /*ptl_handle_ni_t *ni_h 			= &portals_info->ni_handle_phys;//get the network interface*/
    /*set_Match_Ignore_Bits ( &match, &ignore, ptrmsg->my_idThread, ptrmsg->tag, 0);*/

    /*ptrmsg->md.length 	 = SCTK_MSG_SIZE ( recv );*/
    /*int aligned_size, page_size;*/

    /*switch ( recv->tail.message_type )*/
        /*//type of the message*/
    /*{*/
        /*case SCTK_MESSAGE_CONTIGUOUS:*/
            /*ptrmsg->md.start  	 = recv->tail.message.contiguous.addr;*/
            /*ptrmsg->md.length 	 = recv->tail.message.contiguous.size;*/

            /*break;*/

        /*case SCTK_MESSAGE_NETWORK:*/
            /*ptrmsg->md.start  	 = ( char * ) recv + sizeof ( sctk_thread_ptp_message_t );*/
            /*break;*/

        /*case SCTK_MESSAGE_PACK:*/
        /*case SCTK_MESSAGE_PACK_ABSOLUTE:*/
            /*aligned_size = SCTK_MSG_SIZE ( recv );*/
            /*page_size = getpagesize();*/

            /*if ( posix_memalign ( ( void ** ) &ptrmsg->buffer, page_size, aligned_size ) != 0 )*/
            /*{*/
                /*sctk_error ( "error allocation memalign" );*/
            /*}*/

            /*ptrmsg->md.start 	= ptrmsg->buffer;*/
            /*break;*/

        /*default:*/
            /*not_reachable();*/
            /*break;*/
    /*}*/

    /*ptrmsg->md.options   	= PTL_MD_EVENT_CT_REPLY;*/
    /*ptrmsg->md.eq_handle 	= PTL_EQ_NONE;   // i.e. don't queue recv events*/

    /*CHECK_RETURNVAL ( PtlMDBind ( *ni_h, &ptrmsg->md, &ptrmsg->md_handle ) );*/

    /*CHECK_RETURNVAL ( PtlGet ( ptrmsg->md_handle, 0, ptrmsg->md.length, ptrmsg->peer,*/
                /*ptrmsg->peer_idThread, match, 0, NULL ) ); //get the message*/

/*}*/

void sctk_portals_message_copy ( sctk_message_to_copy_t *tmp ){
}

void sctk_portals_free ( void *msg ) //free isn't atomatic because we reuse memory allocated
{

	sctk_debug ( "message free %p", msg );
	//sctk_free(msg);
}

void sctk_portals_add_route(int dest, ptl_process_t id, sctk_rail_info_t *rail, sctk_route_origin_t route_type ){
	sctk_endpoint_t *new_route;

	new_route = sctk_malloc ( sizeof ( sctk_endpoint_t ) );
	assume(new_route != NULL);
	sctk_endpoint_init(new_route, dest, rail, route_type);
	sctk_endpoint_set_state(new_route, STATE_CONNECTED);

	new_route->data.portals.dest = id;
	if(route_type == ROUTE_ORIGIN_STATIC){
		sctk_rail_add_static_route(rail, dest, new_route);
	} else {
		sctk_rail_add_dynamic_route(rail, dest, new_route);
	}

	/*[>*TODO: need a polling threads ? <]*/
}


/*[>***********************************************************************************************************<]*/
/*[> get the position of a thread in his proc <]*/
/*[>inline int sctk_get_local_id ( int task_id )<]*/
/*[>{<]*/
    /*[>int i, p = 0, add;<]*/

    /*[>int total_number = sctk_get_total_tasks_number();<]*/
    /*[>int rap = total_number / sctk_process_number;<]*/
    /*[>int rest = ( total_number % sctk_process_number );<]*/

    /*[>if ( rest != 0 )<]*/
        /*[>add = 1;<]*/
    /*[>else<]*/
        /*[>add = 0;<]*/

    /*[>for ( i = 0; i < total_number + rap + add; i += rap + add, p++ )<]*/
    /*[>{<]*/
        /*[>if ( i > task_id )<]*/
        /*[>{<]*/
            /*[>return task_id - ( i - ( rap + add ) );<]*/
        /*[>}<]*/

        /*[>if ( p == rest )<]*/
            /*[>add = 0;<]*/
    /*[>}<]*/

    /*[>sctk_error ( "fail in %s", __FUNCTION__ );<]*/
    /*[>return -1;<]*/
/*[>}<]*/

/*[> get the id of proc of a thread <]*/
/*inline int sctk_get_peer_process_rank ( int task_id )*/
/*{*/
    /*int i, p = 0, add;*/

    /*int total_number = sctk_get_total_tasks_number();*/
    /*int rap = total_number / sctk_process_number;*/
    /*int rest = ( total_number % sctk_process_number );*/

    /*if ( rest != 0 )*/
        /*add = 1;*/
    /*else*/
        /*add = 0;*/

    /*sctk_debug ( "%s %d", __FUNCTION__, task_id );*/

    /*for ( i = 0; i < total_number + rap + add; i += rap + add, p++ )*/
    /*{*/
        /*if ( i > task_id )*/
        /*{*/
            /*sctk_debug ( "%s %d", __FUNCTION__, p - 1 );*/
            /*return p - 1;*/
        /*}*/

        /*if ( p == rest )*/
            /*add = 0;*/
    /*}*/

    /*sctk_error ( "fail in %s", __FUNCTION__ );*/
    /*return -1;*/
/*}*/

/*[>*******************************************************************************<]*/

/*[> alloc a new array in the list <]*/
/*void ListAllocMsg ( sctk_portals_event_table_list_t *EvQ )*/
/*{*/
    /*int i;*/
    /*sctk_portals_event_table_t *currList = &EvQ->head;*/

    /*for ( i = 0; i < EvQ->nb_elements - 1; i++ )*/
        /*currList = currList->next;*/

    /*currList->next = malloc ( sizeof ( sctk_portals_event_table_t ) );*/
    /*currList->next->nb_elems = 0;*/
    /*EvQ->nb_elements++;*/
/*}*/

/*static inline void reserve ( sctk_portals_rail_info_t *portals_info, int ind, sctk_portals_event_table_list_t *EvQ, unsigned res )*/
/*{*/
    /*int i = 0, r = 0, k;*/
    /*sctk_portals_event_table_t *currList = &EvQ->head;*/

    /*for ( i = 0; i < EvQ->nb_elements; i++ )*/
    /*{*/
        /*if ( currList->nb_elems < SCTK_PORTALS_SIZE_EVENTS )*/
        /*{*/
            /*for ( k = 0; k < SCTK_PORTALS_SIZE_EVENTS; k++ )*/
            /*{*/
                /*if ( currList->events[k].used == IDLE )*/
                /*{*/

                    /*[>we add a new entry on the table with a pointer to a ptp_message<]*/


                    /*currList->events[k].ptrmsg.msg_send = sctk_malloc ( sizeof ( sctk_thread_ptp_message_t ) );*/
                    /*bzero ( currList->events[k].ptrmsg.msg_send, sizeof ( sctk_thread_ptp_message_t ) );*/
                    /*currList->events[k].msg.me.start  = currList->events[k].ptrmsg.msg_send;*/
                    /*currList->events[k].msg.me.length = sizeof ( sctk_thread_ptp_message_body_t ); //*/
                    /*currList->events[k].msg.me.uid    = PTL_UID_ANY;*/
                    /*currList->events[k].used = RESERVED;*/
                    /*currList->nb_elems++;*/
                    /*currList->nb_elems_headers++;*/

                    /*currList->events[k].msg.me.match_id.phys.nid = PTL_NID_ANY;*/
                    /*currList->events[k].msg.me.match_id.phys.pid = PTL_PID_ANY;*/

                    /*currList->events[k].msg.me.match_bits    = FLAG_REQ;*/
                    /*currList->events[k].msg.me.ignore_bits   = REQ_IGN;*/

                    /*currList->events[k].msg.me.options = OPTIONS_HEADER;*/

                    /*if ( !_me_is_allocated ( currList->events[k].msg.allocs ) )*/
                    /*{*/
                        /*CHECK_RETURNVAL ( PtlCTAlloc ( portals_info->ni_handle_phys, &currList->events[k].msg.me.ct_handle ) );*/
                        /*set_allocBits ( &currList->events[k].msg.allocs, ME_ALLOCATED );*/
                    /*}*/
                    /*else*/
                    /*{*/
                        /*CHECK_RETURNVAL ( PtlCTSet ( currList->events[k].msg.me.ct_handle, portals_info->zeroCounter ) );*/
                    /*}*/

                    /*CHECK_RETURNVAL ( PtlMEAppend ( portals_info->ni_handle_phys, portals_info->pt_index[ind], &currList->events[k].msg.me, PTL_PRIORITY_LIST, NULL, &currList->events[k].msg.me_handle ) );*/
                    /*r++;*/

                    /*if ( r == res )*/
                        /*return;*/
                /*}*/
            /*}*/
        /*}*/

        /*currList = currList->next;*/
    /*}*/

/*}*/

/*static inline void evalReserve ( sctk_portals_rail_info_t *portals_info, int ind, sctk_portals_event_table_list_t *EvQ )*/
/*{*/

    /*int i;*/
    /*unsigned reservations = 0;*/
    /*unsigned free_space	  = 0;*/
    /*sctk_portals_event_table_t *currList = &EvQ->head;*/

    /*for ( i = 0; i < EvQ->nb_elements; i++ )*/
    /*{*/
        /*reservations += currList->nb_elems_headers;*/
        /*free_space += SCTK_PORTALS_SIZE_EVENTS - currList->nb_elems;*/
        /*currList = currList->next;*/
    /*}*/

    /*if ( reservations < SCTK_PORTALS_SIZE_HEADERS_MIN )*/
    /*{*/
        /*if ( free_space < SCTK_PORTALS_SIZE_HEADERS - reservations )*/
        /*{*/
            /*ListAllocMsg ( EvQ );*/
        /*}*/

        /*reserve ( portals_info, ind, EvQ, SCTK_PORTALS_SIZE_HEADERS - reservations );*/
    /*}*/
/*}*/

/*[> append a new sending message in the list <]*/
void sctk_portals_send_put ( sctk_endpoint_t *endpoint, sctk_thread_ptp_message_t *msg)
{
	sctk_portals_rail_info_t * prail = &endpoint->rail->network.portals;
	sctk_portals_route_info_t *proute = &endpoint->data.portals;
	int task_rank = sctk_get_local_task_rank() % prail->ptable.nb_entries;
	ptl_md_t data;
	ptl_handle_md_t data_handler;
	ptl_me_t slot;
	ptl_handle_me_t slot_handler;
	ptl_pt_index_t remote_entry;
	ptl_ct_event_t event;

	remote_entry = SCTK_MSG_SRC_TASK(msg) % prail->ptable.nb_entries;

	sctk_portals_helper_init_memory_descriptor(&data, &prail->interface_handler, msg, sizeof(sctk_thread_ptp_message_body_t), SCTK_PORTALS_MD_PUT_OPTIONS);
	
	sctk_portals_assume(PtlMDBind(prail->interface_handler, &data, &data_handler));
	
	sctk_portals_helper_init_list_entry(&slot, &prail->interface_handler, msg->tail.message.contiguous.addr, msg->tail.message.contiguous.size, SCTK_PORTALS_ME_PUT_OPTIONS);

	sctk_portals_helper_set_bits_from_msg(&slot.match_bits, &prail->ptable.head[task_rank]->entry_cpt); 

	sctk_portals_assume(PtlMEAppend(
				prail->interface_handler, // NI handler
				task_rank, // pt entry for current task
				&slot, // ME to append
				PTL_PRIORITY_LIST, //global options
				NULL, //user data
				&slot_handler
				));

	sctk_portals_assume(PtlPut(
				data_handler, //handler on data
				0, //local offset in MD : none
				data.length, // data size
				PTL_NO_ACK_REQ, // no need of ACK
				proute->dest, //remote process
				remote_entry, //remote pt entry
				SCTK_PORTALS_BITS_HDR,
				0, // remote offst in ME : none
				NULL, //user data
				slot.match_bits //mark msg as a standard request
				));

	sctk_portals_assume(PtlCTWait(data.ct_handle, 1, &event));
	sctk_portals_assume(PtlMDRelease(data_handler));
}
/*void sctk_portals_recv_put_event ( sctk_rail_info_t *rail, ptl_event_t *event, int peer_idThread )*/
/*{*/
    /*[>TODO use infos of put req event to build the entry on the list ,the msg is created before but uninitialized (we will get it) <]*/
    /*uint64_t match;*/
    /*uint64_t ignore;*/

    /*int my_idThread;*/
    /*int tag, found = 0;*/
    /*int currPos = 0;*/
    /*//give the informations in the match_bits attribute*/
    /*get_idThread ( &my_idThread, event->match_bits );*/
    /*get_tag ( &tag, event->match_bits );*/

    /*sctk_portals_rail_info_t *portals_info = &rail->network.portals;*/
    /*set_Match_Ignore_Bits ( &match, &ignore, my_idThread, tag, 0);*/
    /*ptl_handle_ni_t *ni_h 	= & ( portals_info->ni_handle_phys );*/

    /*sctk_portals_event_table_list_t *EvQ 		= &portals_info->event_list[peer_idThread];//we get the good index*/

    /*sctk_portals_event_table_t *currList = &EvQ->head;*/
    /*evalReserve ( portals_info, peer_idThread, EvQ );*/
    /*if(event->match_bits == TAG_INIT + FLAG_REQ){*/
        /*sctk_debug("I get a init request !!!!!");*/
    /*}*/
    /*while ( !found )*/
    /*{*/

        /*if ( currList->nb_elems < SCTK_PORTALS_SIZE_EVENTS && currList->nb_elems_headers > 0 )*/
            /*//enough places ?*/
        /*{*/
            /*int pos = 0;*/

            /*while ( currList->events[pos].ptrmsg.msg_send != event->start )*/
            /*{*/
                /*pos++;    //same address = good pt_message*/
            /*}*/

            /*if ( !_md_is_allocated ( currList->events[pos].msg.allocs ) )*/
                /*//it's not necessary to reallocate when it was already allocated*/
            /*{*/

                /*CHECK_RETURNVAL ( PtlCTAlloc ( *ni_h, & ( currList->events[pos].msg.md.ct_handle ) ) );*/
                /*set_allocBits ( &currList->events[pos].msg.allocs, MD_ALLOCATED );*/
            /*}*/
            /*else*/
            /*{*/

                /*CHECK_RETURNVAL ( PtlCTSet ( currList->events[pos].msg.md.ct_handle, portals_info->zeroCounter ) );*/
            /*}*/

            /*currList->events[pos].used 					= IN_USE;*/
            /*currList->nb_elems_headers--;*/
            /*currList->events[pos].pt_index				= peer_idThread;*/
            /*currList->events[pos].msg.peer.phys.nid 	= event->initiator.phys.nid;*/
            /*currList->events[pos].msg.peer.phys.pid		= event->initiator.phys.pid;*/


            /*currList->events[pos].msg.match_bits		= match;*/
            /*currList->events[pos].msg.ignore_bits		= ignore;*/


            /*currList->events[pos].msg.type 				= READ;*/

            /*currList->events[pos].msg.my_idThread 		= my_idThread;*/
            /*currList->events[pos].msg.tag				= tag;*/
            /*currList->events[pos].msg.peer_idThread 	= peer_idThread;*/


            /*sctk_rebuild_header ( currList->events[pos].ptrmsg.msg_send ); //rebuild*/
            /*sctk_reinit_header ( currList->events[pos].ptrmsg.msg_send, sctk_portals_free, sctk_portals_message_copy );*/

            /*SCTK_MSG_COMPLETION_FLAG_SET ( currList->events[pos].ptrmsg.msg_send , NULL );*/
            /*currList->events[pos].ptrmsg.msg_send->tail.portals_message_info_t 	= &currList->events[pos];//save the event datas*/
            /*currList->events[pos].ptrmsg.msg_send->tail.portals_info_t			= portals_info;//save the rail info*/

            /*currList->events[pos].ptrmsg.msg_send->tail.message_type = SCTK_MESSAGE_CONTIGUOUS;*/
            /*sctk_spinlock_unlock ( &portals_info->lock[peer_idThread] );*/

            /*rail->send_message_from_network ( currList->events[pos].ptrmsg.msg_send );*/
            /*sctk_spinlock_lock ( &portals_info->lock[peer_idThread] );*/
            /*currList->events[pos].msg.append_pos		= pos;*/
            /*currList->events[pos].msg.append_list		= currPos;*/
            /*found = 1;*/
        /*}*/

        /*currList = currList->next;*/
        /*currPos++;*/
    /*}*/
/*}*/

/*[> delete the element at the position pos in the list number 'list'<]*/
/*void ListFree ( sctk_portals_rail_info_t *portals_info, sctk_portals_event_table_list_t *EvQ, int list, int pos, int tab )*/
/*{*/
    /*int i;*/

    /*if ( tab == MSG_ARRAY )*/
    /*{*/
        /*sctk_portals_event_table_t *currList = &EvQ->head;*/

        /*for ( i = 0; i < list; i++ )*/
            /*currList = currList->next;*/

        /*if ( currList->events[pos].used == IN_USE )*/
        /*{*/

            /*sctk_debug ( "free (type %d pos %d) size (list %d %d) at %p and msg (%p,%p)", currList->events[pos].msg.type, pos, i, currList->nb_elems, &currList->events[pos], currList->events[pos].ptrmsg.msg_send, currList->events[pos].ptrmsg.msg_recv );*/


            /*if ( currList->events[pos].msg.type == WRITE )*/
            /*{*/
                /*currList->nb_elems--;*/
                /*currList->events[pos].used = IDLE;*/
                /*[> the write is done so we can free memory <]*/


                /*switch ( currList->events[pos].ptrmsg.msg_send->tail.message_type )*/
                /*{*/
                    /*case SCTK_MESSAGE_CONTIGUOUS:*/


                        /*break;*/

                    /*case SCTK_MESSAGE_NETWORK:*/

                        /*break;*/

                    /*case SCTK_MESSAGE_PACK:*/
                    /*case SCTK_MESSAGE_PACK_ABSOLUTE:*/

                        /*currList->events[pos].msg.buffer = NULL;*/
                        /*break;*/

                    /*default:*/
                        /*not_reachable();*/
                        /*break;*/
                /*}*/
                /*sctk_debug ( "entering scfm" );*/
                /*sctk_complete_and_free_message ( currList->events[pos].ptrmsg.msg_send );*/
                /*sctk_debug ( "leaving scfm" );*/
                /*currList->events[pos].ptrmsg.msg_send = NULL;*/
            /*}*/
            /*else*/
            /*{*/
                /*currList->nb_elems_headers++;*/
                /*[> the read is done so we can free memory <]*/

                /*sctk_debug ( "entering smcf %p %p", SCTK_MSG_COMPLETION_FLAG ( currList->events[pos].ptrmsg.msg_send )  , SCTK_MSG_COMPLETION_FLAG ( currList->events[pos].ptrmsg.msg_recv ) );*/

                /*switch ( currList->events[pos].ptrmsg.msg_recv->tail.message_type )*/
                /*{*/
                    /*case SCTK_MESSAGE_CONTIGUOUS:*/


                        /*break;*/

                    /*case SCTK_MESSAGE_NETWORK:*/

                        /*break;*/

                    /*case SCTK_MESSAGE_PACK:*/
                    /*case SCTK_MESSAGE_PACK_ABSOLUTE:*/
                        /*sctk_net_message_copy_from_buffer ( currList->events[pos].msg.buffer, & ( currList->events[pos].ptrmsg ), 0 );*/

                        /*currList->events[pos].msg.buffer = NULL;*/
                        /*break;*/

                    /*default:*/
                        /*not_reachable();*/
                        /*break;*/

                /*}*/

                /*//we add a new entry on the table with a pointer to this ptp_message*/
                /*sctk_message_completion_and_free ( currList->events[pos].ptrmsg.msg_send, currList->events[pos].ptrmsg.msg_recv );*/
                /*sctk_debug ( "leaving smcf" );*/

                /*bzero ( currList->events[pos].ptrmsg.msg_send, sizeof ( sctk_thread_ptp_message_t ) );*/
                /*currList->events[pos].used = RESERVED;*/

                /*currList->events[pos].msg.me.start  = currList->events[pos].ptrmsg.msg_send;*/
                /*currList->events[pos].msg.me.length = sizeof ( sctk_thread_ptp_message_body_t ); //*/
                /*currList->events[pos].msg.me.uid    = PTL_UID_ANY;*/


                /*currList->events[pos].msg.me.match_id.phys.nid = PTL_NID_ANY;*/
                /*currList->events[pos].msg.me.match_id.phys.pid = PTL_PID_ANY;*/


                /*currList->events[pos].msg.me.match_bits    = FLAG_REQ;*/
                /*currList->events[pos].msg.me.ignore_bits   = REQ_IGN;*/

                /*currList->events[pos].msg.me.options = OPTIONS_HEADER;*/

                /*CHECK_RETURNVAL ( PtlMEAppend ( portals_info->ni_handle_phys, portals_info->pt_index[currList->events[pos].pt_index], &currList->events[pos].msg.me, PTL_PRIORITY_LIST, NULL, &currList->events[pos].msg.me_handle ) );*/

                /*currList->events[pos].ptrmsg.msg_recv = NULL;*/

            /*}*/

            /*CHECK_RETURNVAL ( PtlMDRelease ( currList->events[pos].msg.md_handle ) );*/
            /*currList->events[pos].msg.append_pos = -1;*/

        /*}*/
        /*else*/
        /*{*/
            /*sctk_error ( "Error portals list free" );*/
            /*abort();*/
        /*}*/

    /*}*/

    /*sctk_debug ( "free leaving" );*/

/*}*/

/*[>check if a message was received or sended<]*/
/*void clearList ( sctk_rail_info_t *rail, int id )*/
/*{*/

    /*sctk_portals_rail_info_t *portals_info = &rail->network.portals;*/

    /*if ( sctk_spinlock_trylock ( &portals_info->lock[id] ) == TRYLOCK_SUCCESS )*/
        /*//to be thread safe*/
    /*{*/


        /*ptl_handle_ni_t *ni_h = &portals_info->ni_handle_phys;*/
        /*sctk_portals_event_table_list_t *EvQ = &portals_info->event_list[id];*/
        /*ptl_handle_eq_t *eq_h = &portals_info->event_handler[id];*/

        /*sctk_portals_event_table_t *currList = &EvQ->head;*/
        /*ptl_ct_event_t test, test2;*/
        /*int i, j, k, size;*/


        /*for ( j = 0; j < EvQ->nb_elements; j++ )*/
            /*//check all the lists*/
        /*{*/
            /*size = currList->nb_elems;*/
            /*i = 0;*/

            /*for ( k = 0; k < size; k++, i++ )*/
                /*//and all the elements*/
            /*{*/
                /*while ( i < SCTK_PORTALS_SIZE_EVENTS && currList->events[i].used != IN_USE )*/
                /*{*/
                    /*i++;*/
                /*}*/

                /*if ( i == SCTK_PORTALS_SIZE_EVENTS ) //to avoid bugs*/
                    /*break;*/

                /*if ( currList->events[i].msg.type == WRITE )*/
                /*{*/


                    /*CHECK_RETURNVAL ( PtlCTGet ( currList->events[i].msg.me.ct_handle, &test ) ); //the message was getted*/

                    /*if ( test.success == 1 )*/
                    /*{*/

                        /*sctk_debug ( "free in index %d", id );*/
                        /*ListFree ( portals_info, EvQ, j, i, MSG_ARRAY );*/
                    /*}*/
                    /*else*/
                        /*if ( test.failure != 0 )*/
                        /*{*/
                            /*sctk_error ( "failure get portals send" );*/
                            /*abort();*/
                        /*}*/


                /*}*/
                /*else*/
                    /*if ( currList->events[i].msg.type == READ )*/
                    /*{*/


                        /*CHECK_RETURNVAL ( PtlCTGet ( currList->events[i].msg.md.ct_handle, &test ) );*/

                        /*if ( test.success == 1 )*/
                            /*//the message was getted*/
                        /*{*/

                            /*sctk_debug ( "free in index %d", id );*/
                            /*ListFree ( portals_info, EvQ, j, i, MSG_ARRAY );*/
                        /*}*/
                        /*else*/
                            /*if ( test.failure != 0 )*/
                            /*{*/

                                /*sctk_error ( "failure get portals %d", test.failure );*/

                                /*abort();*/
                            /*}*/
                    /*}*/

            /*}*/
        /*}*/

        /*sctk_spinlock_unlock ( &portals_info->lock[id] ); //to be thread safe*/
    /*}*/
    /*//sctk_spinlock_unlock(&portals_info->lock[0]);//to be thread safe*/
/*}*/

/*void notify ( sctk_rail_info_t *rail, int id )*/
/*{*/

    /*clearList ( rail, id ); //CT value is 1 = perform*/

    /*sctk_portals_rail_info_t *portals_info = &rail->network.portals;*/

    /*ptl_handle_ni_t *ni_h = &portals_info->ni_handle_phys;*/
    /*ptl_handle_eq_t *eq_h = &portals_info->event_handler[id];*/

    /*ptl_event_t event;*/

    /*sctk_spinlock_lock ( &portals_info->lock[id] ); //to be thread safe*/
    /*[>sctk_debug("A message have been caught from %d", id);<]*/
    /*while ( PtlEQGet ( *eq_h, &event ) == PTL_OK && event.type == PTL_EVENT_PUT )*/
        /*//we give all events*/
    /*{*/
        /*sctk_portals_recv_put_event ( rail, &event, id );*/
    /*}*/

    /*sctk_spinlock_unlock ( &portals_info->lock[id] ); //to be thread safe*/
    
/*}*/
void sctk_portals_ack_get (sctk_rail_info_t* rail, ptl_event_t* event){
	sctk_thread_ptp_message_t* content = event->start;
	sctk_complete_and_free_message(content);

	//need to reset the ME

}

void sctk_portals_recv_put (sctk_rail_info_t* rail, ptl_event_t* event){
	sctk_thread_ptp_message_t* content = event->start;

	content->tail.portals.remote = event->initiator;
	content->tail.portals.remote_index = event->pt_index;
	content->tail.portals.tag = (ptl_match_bits_t)event->hdr_data;
	sctk_rebuild_header(content);
	sctk_reinit_header(content, sctk_portals_free, sctk_portals_message_copy);
	SCTK_MSG_COMPLETION_FLAG_SET(content, NULL);

	rail->send_message_from_network(content);
}

static int sctk_portals_poll_one_queue(sctk_rail_info_t *rail, size_t id)
{
	int ret = 1;
	ptl_event_t event;
	sctk_portals_table_t* ptable = &rail->network.portals.ptable;
	ptl_handle_eq_t* queue = ptable->head[id]->event_list;
	

	if(sctk_spinlock_trylock(&ptable->head[id]->lock) == 0){
		while(PtlEQGet(*queue, &event) == PTL_OK){
			if(event.type == PTL_EVENT_GET)
			{
				//complete and free messages
				sctk_portals_ack_get(rail, &event);
			}
			//polling only header ME, ie those with Put request
			else if(event.type != PTL_EVENT_PUT) continue;
			else {
				sctk_portals_recv_put(rail, &event);
			}

		}
		sctk_spinlock_unlock(&ptable->head[id]->lock);
	}
	
	return ret;
}

int sctk_portals_polling_queue_for(sctk_rail_info_t*rail, size_t task_id){
	int ret = 0, ret_bef = 0, ret_aft = 0;
	size_t task_bef = 0, task_aft = 0, mytask = sctk_get_task_rank();
	size_t nb_entries = rail->network.portals.ptable.nb_entries;

	//a process which have to poll every queue 
	if(task_id == SCTK_PORTALS_POLL_ALL){
		int i;
		if(sctk_spinlock_trylock(&rail->network.portals.ptable.table_lock) == 0){
			for (i = 0; i < nb_entries; ++i) {
				sctk_portals_poll_one_queue(rail, (mytask+i)%nb_entries);
			}
			sctk_spinlock_unlock(&rail->network.portals.ptable.table_lock);
		}
		return 0;
	}

	ret = sctk_portals_poll_one_queue(rail, task_id);
	if(ret){
		task_bef = (task_id - 1) % nb_entries;
		task_aft = (task_id + 1) % nb_entries;

		ret_bef = sctk_portals_poll_one_queue(rail, task_bef);
		ret_aft = sctk_portals_poll_one_queue(rail, task_aft);
	}
	if(ret_bef || ret_aft)
		ret = 1; //no polled queue
	else ret = 0;

	return ret;
}


static void sctk_portals_network_connection_to(int from, int to, sctk_rail_info_t* rail, sctk_route_origin_t route_type)
{

}

static void sctk_portals_network_connection_to_ctx(int src, sctk_rail_info_t* rail,sctk_portals_connection_context_t * ctx, sctk_route_origin_t route_type)
{
	ptl_md_t md;
	ptl_handle_md_t md_handle;
	ptl_ct_event_t ctc;

	//Initialize memory descriptor w/ default
	sctk_portals_helper_init_memory_descriptor(&md, &rail->network.portals.interface_handler, (void*)&rail->network.portals.current_id, sizeof(sctk_portals_process_id_t), SCTK_PORTALS_MD_OPTIONS);

	//map md in Portals
	sctk_portals_assume(PtlMDBind(rail->network.portals.interface_handler, &md, &md_handle));

	//send info to remote process
	sctk_portals_assume(PtlPut(md_handle, 0, sizeof(sctk_portals_process_id_t),PTL_NO_ACK_REQ, ctx->from,ctx->entry,SCTK_PORTALS_BITS_INIT, 0,NULL,0));
	//waiting Put action completed on local process
	sctk_portals_assume(PtlCTWait(md.ct_handle, 1, &ctc ));
	// freeing resources
	sctk_portals_assume(PtlMDRelease(md_handle));

	//adding route to remote process
	sctk_portals_add_route(src, ctx->from, rail, route_type);
}

static void sctk_portals_network_connection_from(int from, int to, sctk_rail_info_t* rail, sctk_route_origin_t route_type) {
    sctk_portals_connection_context_t ctx;
    sctk_portals_process_id_t slot;
    ptl_ct_event_t ctc;
    ptl_me_t me;
    ptl_handle_me_t me_handler;

	// init ME w/ default
	sctk_portals_helper_init_list_entry(&me, &rail->network.portals.interface_handler, (void*)&slot, sizeof(sctk_portals_process_id_t), SCTK_PORTALS_ME_PUT_OPTIONS);
	// define specific flag for route connection
	sctk_portals_helper_set_bits_from_msg(&me.match_bits, &rail->network.portals.ptable.head[0]->entry_cpt);
	// create the entry in Portals structs
    sctk_portals_assume(PtlMEAppend(rail->network.portals.interface_handler, 0, &me,PTL_PRIORITY_LIST, NULL,  &me_handler));

	//prepare network context to send to remote process
    ctx.from = rail->network.portals.current_id;
    ctx.to   = to;
    ctx.entry = 0;
    
	//depending on creation type, route can be created as a dynamic or static one
	if(route_type == ROUTE_ORIGIN_STATIC){
        sctk_control_messages_send_rail(to,SCTK_PORTALS_CONTROL_MESSAGE_ON_DEMAND_STATIC,0, &ctx,sizeof(sctk_portals_connection_context_t), rail->rail_number );
    } else {
        sctk_control_messages_send_rail(to,SCTK_PORTALS_CONTROL_MESSAGE_ON_DEMAND_DYNAMIC, 0, &ctx,sizeof(sctk_portals_connection_context_t), rail->rail_number ); 
    }
    //wait remote to publish
    sctk_debug("Wait for %d to respond...", to);
	//wait for a message to arrive. When occurs ME counter is incremented.
    sctk_portals_assume(PtlCTWait(me.ct_handle, 1, &ctc));
    sctk_debug ( "PTL_PROCESS Written : nid %u pid %u", slot.phys.nid, slot.phys.pid );
	sctk_portals_assume(PtlMEUnlink(me_handler));

    //Finally, add a route to this process
   sctk_portals_add_route(to, slot, rail, route_type);
}

static void sctk_portals_connection_from(int from, int to , sctk_rail_info_t *rail){
    sctk_portals_network_connection_from(from, to, rail, ROUTE_ORIGIN_STATIC);
}
static void sctk_portals_connection_to(int from, int to , sctk_rail_info_t *rail){
    sctk_portals_network_connection_to(from, to, rail, ROUTE_ORIGIN_STATIC);

}
void sctk_portals_on_demand_connection_handler( sctk_rail_info_t *rail, int dest_process )
{
    sctk_portals_network_connection_from(sctk_process_rank, dest_process, rail, ROUTE_ORIGIN_DYNAMIC);
}
void sctk_portals_control_message_handler( sctk_rail_info_t *rail, int src_process, int source_rank, char subtype, char param, void * data, size_t size) {
    sctk_portals_control_message_t action = subtype;
	assume(size == sizeof(sctk_portals_connection_context_t));
    switch(action){
        case SCTK_PORTALS_CONTROL_MESSAGE_ON_DEMAND_STATIC:
            sctk_portals_network_connection_to_ctx(source_rank, rail, (sctk_portals_connection_context_t *)data, ROUTE_ORIGIN_STATIC);
            break;
        case SCTK_PORTALS_CONTROL_MESSAGE_ON_DEMAND_DYNAMIC:
            sctk_portals_network_connection_to_ctx(source_rank, rail, (sctk_portals_connection_context_t *)data, ROUTE_ORIGIN_DYNAMIC);
            break;
    }
}

void sctk_network_init_portals_all ( sctk_rail_info_t *rail )
{
    int right_rank;
    int left_rank;
    sctk_portals_limits_t desired;
    int ntasks;
    sctk_portals_process_id_t right_id, left_id;
    char right_rank_connection_infos[MAX_STRING_SIZE];
    char left_rank_connection_infos[MAX_STRING_SIZE];

    rail->connect_from = sctk_portals_connection_from;
    rail->connect_to = sctk_portals_connection_to;
    rail->control_message_handler = sctk_portals_control_message_handler;

	sctk_portals_helper_lib_init(&rail->network.portals.interface_handler, &rail->network.portals.current_id, &rail->network.portals.max_limits, &rail->network.portals.ptable);
    
	//if process binding w/ ring is not required, stop here
    if( rail->requires_bootstrap_ring == 0 )
    {
        return;
    }
   
	/** Portals initialization : Ring topology */
	/**
	 * 1. bind to right process through PMI
	 * 2. if nbProcs > 2 create a binding to the process to the left
	 * => Bidirectional ring
	 */
	// Process registration through PMI
    sctk_portals_helper_to_str( &rail->network.portals.current_id, sizeof (sctk_portals_process_id_t), rail->network.portals.connection_infos,MAX_STRING_SIZE);
	rail->network.portals.connection_infos_size = strlen(rail->network.portals.connection_infos);
    assume(rail->network.portals.connection_infos_size  < MAX_STRING_SIZE);
	assume(sctk_pmi_put_connection_info ( rail->network.portals.connection_infos, MAX_STRING_SIZE, rail->rail_number ) == 0);

	sctk_debug ( "Send KEY %s (%lu, %lu)", rail->network.portals.connection_infos, rail->network.portals.current_id.phys.nid, rail->network.portals.current_id.phys.pid );
    
	//compute left and right neighbour rank
    right_rank = ( sctk_process_rank + 1 ) % sctk_process_number;
    left_rank = ( sctk_process_rank + sctk_process_number - 1 ) % sctk_process_number;
    sctk_debug("for %d (%d/%d)", sctk_process_rank, left_rank, right_rank); 

	//SYNCING : Every process should upload its connection string before binding
    sctk_pmi_barrier();

    sctk_pmi_get_connection_info ( right_rank_connection_infos, MAX_STRING_SIZE, rail->rail_number, right_rank );
    sctk_portals_helper_from_str(right_rank_connection_infos, &right_id, sizeof ( right_id ) );
	sctk_debug ( "Got id %lu\t%lu", right_id.phys.nid, right_id.phys.pid );	

    sctk_pmi_barrier();
	//Register the right neighbour as a connection and let Portals make the binding
	sctk_portals_helper_bind_to(&rail->network.portals.interface_handler, right_id);
    sctk_debug ( "OK: Bind %d -> %d", sctk_process_rank, right_rank);

	//if we need an initialization to the left process (bidirectional ring)
    if(sctk_process_number > 2){
        sctk_pmi_get_connection_info ( left_rank_connection_infos, MAX_STRING_SIZE, rail->rail_number, left_rank );
        /*sctk_debug ( "Got KEY %s from %d", right_rank_connection_infos, right_rank);*/

        /*decode portals identification string into nid and pid*/
        sctk_portals_helper_from_str( left_rank_connection_infos, &left_id, sizeof (sctk_portals_process_id_t) );
        /*sctk_debug ( "Got id %lu\t%lu", left_id.phys.nid, left_id.phys.pid );	*/
		// register the left neighbour as a connection and let Portals make the binding
        sctk_portals_helper_bind_to(&rail->network.portals.interface_handler, left_id);
        sctk_debug ( "OK: Bind %d -> %d", sctk_process_rank, left_rank);
    }
   
	//Syncing before adding the route
    sctk_pmi_barrier();

	//add routes
    sctk_portals_add_route (right_rank, right_id, rail, ROUTE_ORIGIN_STATIC);
    if ( sctk_process_number > 2 )
    {
        sctk_portals_add_route (left_rank, left_id, rail, ROUTE_ORIGIN_STATIC);
    }
   
	//syncing with other processes
    sctk_pmi_barrier();
}
#endif
