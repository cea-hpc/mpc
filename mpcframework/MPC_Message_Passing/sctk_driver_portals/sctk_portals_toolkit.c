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
/************ TOOLS **********************************/
void test_event_type ( ptl_event_t *event, int rank, const char *type )
{
    sctk_debug ( "%s -> %d: ", type, rank );

    switch ( event->type )
    {
        case PTL_EVENT_GET:
            sctk_debug ( "GET" );
            break;

        case PTL_EVENT_GET_OVERFLOW:
            sctk_debug ( "GETOVERFLOW" );
            break;

        case PTL_EVENT_PUT:
            sctk_debug ( "PUT" );
            break;

        case PTL_EVENT_PUT_OVERFLOW:
            sctk_debug ( "PUTOVERFLOW" );
            break;

        case PTL_EVENT_ATOMIC:
            sctk_debug ( "ATOMIC" );
            break;

        case PTL_EVENT_ATOMIC_OVERFLOW:
            sctk_debug ( "ATOMIC OVERFLOW: " );
            break;

        case PTL_EVENT_FETCH_ATOMIC:
            sctk_debug ( "FETCHATOMIC: " );
            break;

        case PTL_EVENT_FETCH_ATOMIC_OVERFLOW:
            sctk_debug ( "FETCHATOMIC OVERFLOW: " );
            break;

        case PTL_EVENT_REPLY:
            sctk_debug ( "REPLY" );
            break;

        case PTL_EVENT_SEND:
            sctk_debug ( "SEND" );
            break;

        case PTL_EVENT_ACK:
            sctk_debug ( "ACK" );
            break;

        case PTL_EVENT_PT_DISABLED:
            sctk_debug ( "PTDISABLED" );
            break;

        case PTL_EVENT_AUTO_UNLINK:
            sctk_debug ( "UNLINK" );
            break;

        case PTL_EVENT_AUTO_FREE:
            sctk_debug ( "FREE" );
            break;

        case PTL_EVENT_SEARCH:
            sctk_debug ( "SEARCH" );
            break;

        case PTL_EVENT_LINK:
            sctk_debug ( "LINK" );
            break;

        default:
            sctk_debug ( "%p %d UNKNOWN\n", event, event->type );
    }
}

/************ INIT ****************/
    static int
encode ( const void *inval, int invallen, char *outval, int outvallen )
{
    static unsigned char encodings[] =
    {
        '0', '1', '2', '3', '4', '5', '6', '7', \
            '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'
    };
    int i;

    if ( invallen * 2 + 1 > outvallen )
    {
        return 1;
    }

    for ( i = 0; i < invallen; i++ )
    {
        outval[2 * i] = encodings[ ( ( unsigned char * ) inval ) [i] & 0xf];
        outval[2 * i + 1] = encodings[ ( ( unsigned char * ) inval ) [i] >> 4];
    }

    outval[invallen * 2] = '\0';

    return 0;
}


    static int
decode ( const char *inval, void *outval, int outvallen )
{
    int i;
    char *ret = ( char * ) outval;

    if ( outvallen != strlen ( inval ) / 2 )
    {
        return 1;
    }

    for ( i = 0 ; i < outvallen ; ++i )
    {
        if ( *inval >= '0' && *inval <= '9' )
        {
            ret[i] = *inval - '0';
        }
        else
        {
            ret[i] = *inval - 'a' + 10;
        }

        inval++;

        if ( *inval >= '0' && *inval <= '9' )
        {
            ret[i] |= ( ( *inval - '0' ) << 4 );
        }
        else
        {
            ret[i] |= ( ( *inval - 'a' + 10 ) << 4 );
        }

        inval++;
    }

    return 0;
}

void sctk_portals_set_max ( long int *ptr, int size )
{

    *ptr = 1;
    int i;

    for ( i = 0; i < size * 8 - 2; i++ )
    {
        *ptr = ( *ptr ) << 1;
        ( *ptr ) ++;
    }

    //printf("-> %ld (%d)\n",*ptr,size);
}

void sctk_portals_init_boundaries ( ptl_ni_limits_t *desired )
{
    sctk_portals_set_max ( ( long int * ) &desired->max_unexpected_headers, sizeof ( int ) );
    sctk_portals_set_max ( ( long int * ) &desired->max_entries, sizeof ( int ) );
    sctk_portals_set_max ( ( long int * ) &desired->max_mds, sizeof ( int ) );
    sctk_portals_set_max ( ( long int * ) &desired->max_cts, sizeof ( int ) );
    sctk_portals_set_max ( ( long int * ) &desired->max_eqs, sizeof ( int ) );
    sctk_portals_set_max ( ( long int * ) &desired->max_pt_index, sizeof ( int ) );
    sctk_portals_set_max ( ( long int * ) &desired->max_iovecs, sizeof ( int ) );
    sctk_portals_set_max ( ( long int * ) &desired->max_list_size, sizeof ( int ) );
    sctk_portals_set_max ( ( long int * ) &desired->max_triggered_ops, sizeof ( int ) );
    sctk_portals_set_max ( ( long int * ) &desired->max_msg_size, sizeof ( int ) );
    sctk_portals_set_max ( ( long int * ) &desired->max_atomic_size, sizeof ( int ) + 1 );
    sctk_portals_set_max ( ( long int * ) &desired->max_waw_ordered_size, sizeof ( int ) );
    sctk_portals_set_max ( ( long int * ) &desired->max_war_ordered_size, sizeof ( int ) );
    sctk_portals_set_max ( ( long int * ) &desired->max_volatile_size, sizeof ( int ) + 1 );

    /*printf("NI actual limits\n  max_entries:            %d\n  max_unexpected_headers: %d\n  max_mds:                %d\n  max_cts:                %d\n  max_eqs:                %d\n  max_pt_index:           %d\n  max_iovecs:             %d\n  max_list_size:          %d\n  max_msg_size:           %d\n  max_atomic_size:        %d\n  max_waw_ordered_size:   %d\n  max_war_ordered_size:   %d\n  max_volatile_size:      %d\n",*/
      /*rail->network.portals.actual.max_entries,*/
      /*rail->network.portals.actual.max_unexpected_headers,*/
      /*rail->network.portals.actual.max_mds,*/
      /*rail->network.portals.actual.max_cts,*/
      /*rail->network.portals.actual.max_eqs,*/
      /*rail->network.portals.actual.max_pt_index,*/
      /*rail->network.portals.actual.max_iovecs,*/
      /*rail->network.portals.actual.max_list_size,*/
      /*(int)rail->network.portals.actual.max_msg_size,*/
      /*(int)rail->network.portals.actual.max_atomic_size,*/
      /*(int)rail->network.portals.actual.max_waw_ordered_size,*/
      /*(int)rail->network.portals.actual.max_war_ordered_size,*/
      /*(int)rail->network.portals.actual.max_volatile_size);*/
}


/****************************************************************/
/*				|  ME  |  MEH |  MD  |  MDH |					*/

static inline int _md_is_allocated ( uint8_t alloc )
{
    unsigned mask = 1;
    alloc >>= 1;
    return ( alloc & mask );
}

static inline int _mdh_is_allocated ( uint8_t alloc )
{
    unsigned mask = 1;
    return ( alloc & mask );
}

static inline int _meh_is_allocated ( uint8_t alloc )
{
    unsigned mask = 1;
    alloc >>= 2;
    return ( alloc & mask );
}

static inline int _me_is_allocated ( uint8_t alloc )
{
    alloc >>= 3;
    return alloc;
}

static inline void set_allocBits ( uint8_t *alloc, uint8_t value )
{
    *alloc += value;
}

/*********************************************************************/

void set_Match_Ignore_Bits ( ptl_match_bits_t *match, ptl_match_bits_t *ignore, unsigned id, int tag, char init)
{
    /*	[0			1  ------  -  2 -   -------	3			4			5			6			7			] */
    /*	[	unused	|unused	 |flag|   thread pos in proc	| 				tag MPI of message 				| */

    *ignore 		= 0;
    *match			= 0;

    if ( id != ANY_SOURCE )
    {
        *match		= id;
        *match		= ( *match << 32 );
    }
    else
        *ignore		|= SOURCE_IGN;

    if ( tag == ANY_TAG )
        *ignore		|= TAG_IGN;
    else
        *match		+= tag;

    if(init)
        *match = TAG_INIT + FLAG_REQ;
}

/* extract the message tag of the match_bits */
inline void get_tag ( int *tag, ptl_match_bits_t match )
{
    *tag = match & TAG_IGN;
}

/* extract the thread id of the peer of the match_bits */
inline void get_idThread ( int *peer_idThread, ptl_match_bits_t match )
{
    *peer_idThread = ( match & SOURCE_IGN ) >> 32;
}

/*****************************************************/
/* when the matching occurs in mpc we get the message so we need 0 copy */
void sctk_portals_message_copy ( sctk_message_to_copy_t *tmp )
{
    sctk_thread_ptp_message_t *send;
    sctk_thread_ptp_message_t *recv;

    ptl_match_bits_t match, ignore;
    send = tmp->msg_send;
    recv = tmp->msg_recv;


    sctk_portals_event_item_t *event 					= ( sctk_portals_event_item_t * ) send->tail.portals_message_info_t; // get the event informations
    sctk_portals_message_t *ptrmsg 			= &event->msg;// get the portals struct message

    if ( ptrmsg == NULL )
        abort();

    if ( event == NULL )
        abort();


    sctk_portals_rail_info_t *portals_info	= ( sctk_portals_rail_info_t * ) send->tail.portals_info_t; //get the portal rail
    event->ptrmsg.msg_recv 			= recv;
    ptl_handle_ni_t *ni_h 			= &portals_info->ni_handle_phys;//get the network interface
    set_Match_Ignore_Bits ( &match, &ignore, ptrmsg->my_idThread, ptrmsg->tag, 0);

    ptrmsg->md.length 	 = SCTK_MSG_SIZE ( recv );
    int aligned_size, page_size;

    switch ( recv->tail.message_type )
        //type of the message
    {
        case SCTK_MESSAGE_CONTIGUOUS:
            ptrmsg->md.start  	 = recv->tail.message.contiguous.addr;
            ptrmsg->md.length 	 = recv->tail.message.contiguous.size;

            break;

        case SCTK_MESSAGE_NETWORK:
            ptrmsg->md.start  	 = ( char * ) recv + sizeof ( sctk_thread_ptp_message_t );
            break;

        case SCTK_MESSAGE_PACK:
        case SCTK_MESSAGE_PACK_ABSOLUTE:
            aligned_size = SCTK_MSG_SIZE ( recv );
            page_size = getpagesize();

            if ( posix_memalign ( ( void ** ) &ptrmsg->buffer, page_size, aligned_size ) != 0 )
            {
                sctk_error ( "error allocation memalign" );
            }

            ptrmsg->md.start 	= ptrmsg->buffer;
            break;

        default:
            not_reachable();
            break;
    }

    ptrmsg->md.options   	= PTL_MD_EVENT_CT_REPLY;
    ptrmsg->md.eq_handle 	= PTL_EQ_NONE;   // i.e. don't queue recv events

    CHECK_RETURNVAL ( PtlMDBind ( *ni_h, &ptrmsg->md, &ptrmsg->md_handle ) );

    CHECK_RETURNVAL ( PtlGet ( ptrmsg->md_handle, 0, ptrmsg->md.length, ptrmsg->peer,
                ptrmsg->peer_idThread, match, 0, NULL ) ); //get the message

}

void sctk_portals_free ( void *msg ) //free isn't atomatic because we reuse memory allocated
{

    sctk_debug ( "message free %p", msg );
    //sctk_free(msg);
}

static void sctk_portals_add_route(int dest, ptl_process_t id, sctk_rail_info_t *rail, sctk_route_origin_t route_type ){
    sctk_endpoint_t *new_route;

    new_route = sctk_malloc ( sizeof ( sctk_endpoint_t ) );
    assume(new_route != NULL);
    sctk_endpoint_init(new_route, dest, rail, route_type);
    sctk_endpoint_set_state(new_route, STATE_CONNECTED);

    new_route->data.portals.id = id;
    if(route_type == ROUTE_ORIGIN_STATIC){
        sctk_rail_add_static_route(rail, dest, new_route);
    } else {
        sctk_rail_add_dynamic_route(rail, dest, new_route);
    }

    /**TODO: need a polling threads ? */
}

static void sctk_portals_bind_to(sctk_rail_info_t *rail, ptl_process_t remote){
    ptl_md_t md;
    ptl_handle_md_t md_handle;

    int idle = 0;
    CHECK_RETURNVAL ( PtlCTAlloc ( rail->network.portals.ni_handle_phys, &md.ct_handle ) );

    md.start  		= &idle;
    md.length 		= sizeof ( int );
    md.options  	= PTL_MD_EVENT_CT_REPLY;
    md.eq_handle 	= PTL_EQ_NONE;   		// i.e. don't queue receive events

    CHECK_RETURNVAL ( PtlMDBind ( rail->network.portals.ni_handle_phys, &md, &md_handle ) );

    ptl_ct_event_t ctc;

    CHECK_RETURNVAL ( PtlGet ( md_handle, 0, md.length, remote, 0, 0, 0, NULL ) ); //getting the init buf
    CHECK_RETURNVAL ( PtlCTWait ( md.ct_handle, 1, &ctc ) ); //we need to wait the message for routing
}

/*************************************************************************************************************/
/* get the position of a thread in his proc */
inline int sctk_get_local_id ( int task_id )
{
    int i, p = 0, add;

    int total_number = sctk_get_total_tasks_number();
    int rap = total_number / sctk_process_number;
    int rest = ( total_number % sctk_process_number );

    if ( rest != 0 )
        add = 1;
    else
        add = 0;

    for ( i = 0; i < total_number + rap + add; i += rap + add, p++ )
    {
        if ( i > task_id )
        {
            return task_id - ( i - ( rap + add ) );
        }

        if ( p == rest )
            add = 0;
    }

    sctk_error ( "fail in %s", __FUNCTION__ );
    return -1;
}

/* get the number of threads in a proc */
/*inline int sctk_get_local_number_tasks ( int proc_id )*/
inline int sctk_get_local_number_tasks ()
{
    int i, p = 0;

    int total_number = sctk_get_total_tasks_number();
    int rap = total_number / sctk_process_number;
    int rest = ( total_number % sctk_process_number );

    p = rap;

    if ( sctk_process_rank < rest )
        p++;

    sctk_debug ( "tasks: %d", p );
    return p;

}

/*[> get the max number of threads in a proc <]*/
/*inline int sctk_get_max_local_number_tasks()*/
/*{*/
    /*int i, p = 0;*/

    /*int total_number = sctk_get_total_tasks_number();*/
    /*int rap = total_number / sctk_process_number;*/
    /*int rest = ( total_number % sctk_process_number );*/

    /*p = rap;*/

    /*if ( rest != 0 )*/
        /*p++;*/

    /*return p;*/

/*}*/

/* get the id of proc of a thread */
inline int sctk_get_peer_process_rank ( int task_id )
{
    int i, p = 0, add;

    int total_number = sctk_get_total_tasks_number();
    int rap = total_number / sctk_process_number;
    int rest = ( total_number % sctk_process_number );

    if ( rest != 0 )
        add = 1;
    else
        add = 0;

    sctk_debug ( "%s %d", __FUNCTION__, task_id );

    for ( i = 0; i < total_number + rap + add; i += rap + add, p++ )
    {
        if ( i > task_id )
        {
            sctk_debug ( "%s %d", __FUNCTION__, p - 1 );
            return p - 1;
        }

        if ( p == rest )
            add = 0;
    }

    sctk_error ( "fail in %s", __FUNCTION__ );
    return -1;
}

/*********************************************************************************/

/* alloc a new array in the list */
void ListAllocMsg ( sctk_portals_event_table_list_t *EvQ )
{
    int i;
    sctk_portals_event_table_t *currList = &EvQ->head;

    for ( i = 0; i < EvQ->nb_elements - 1; i++ )
        currList = currList->next;

    currList->next = malloc ( sizeof ( sctk_portals_event_table_t ) );
    currList->next->nb_elems = 0;
    EvQ->nb_elements++;
}

static inline void reserve ( sctk_portals_rail_info_t *portals_info, int ind, sctk_portals_event_table_list_t *EvQ, unsigned res )
{
    int i = 0, r = 0, k;
    sctk_portals_event_table_t *currList = &EvQ->head;

    for ( i = 0; i < EvQ->nb_elements; i++ )
    {
        if ( currList->nb_elems < SCTK_PORTALS_SIZE_EVENTS )
        {
            for ( k = 0; k < SCTK_PORTALS_SIZE_EVENTS; k++ )
            {
                if ( currList->events[k].used == IDLE )
                {

                    /*we add a new entry on the table with a pointer to a ptp_message*/


                    currList->events[k].ptrmsg.msg_send = sctk_malloc ( sizeof ( sctk_thread_ptp_message_t ) );
                    bzero ( currList->events[k].ptrmsg.msg_send, sizeof ( sctk_thread_ptp_message_t ) );
                    currList->events[k].msg.me.start  = currList->events[k].ptrmsg.msg_send;
                    currList->events[k].msg.me.length = sizeof ( sctk_thread_ptp_message_body_t ); //
                    currList->events[k].msg.me.uid    = PTL_UID_ANY;
                    currList->events[k].used = RESERVED;
                    currList->nb_elems++;
                    currList->nb_elems_headers++;

                    currList->events[k].msg.me.match_id.phys.nid = PTL_NID_ANY;
                    currList->events[k].msg.me.match_id.phys.pid = PTL_PID_ANY;

                    currList->events[k].msg.me.match_bits    = FLAG_REQ;
                    currList->events[k].msg.me.ignore_bits   = REQ_IGN;

                    currList->events[k].msg.me.options = OPTIONS_HEADER;

                    if ( !_me_is_allocated ( currList->events[k].msg.allocs ) )
                    {
                        CHECK_RETURNVAL ( PtlCTAlloc ( portals_info->ni_handle_phys, &currList->events[k].msg.me.ct_handle ) );
                        set_allocBits ( &currList->events[k].msg.allocs, ME_ALLOCATED );
                    }
                    else
                    {
                        CHECK_RETURNVAL ( PtlCTSet ( currList->events[k].msg.me.ct_handle, portals_info->zeroCounter ) );
                    }

                    CHECK_RETURNVAL ( PtlMEAppend ( portals_info->ni_handle_phys, portals_info->pt_index[ind], &currList->events[k].msg.me, PTL_PRIORITY_LIST, NULL, &currList->events[k].msg.me_handle ) );
                    r++;

                    if ( r == res )
                        return;
                }
            }
        }

        currList = currList->next;
    }

}

static inline void evalReserve ( sctk_portals_rail_info_t *portals_info, int ind, sctk_portals_event_table_list_t *EvQ )
{

    int i;
    unsigned reservations = 0;
    unsigned free_space	  = 0;
    sctk_portals_event_table_t *currList = &EvQ->head;

    for ( i = 0; i < EvQ->nb_elements; i++ )
    {
        reservations += currList->nb_elems_headers;
        free_space += SCTK_PORTALS_SIZE_EVENTS - currList->nb_elems;
        currList = currList->next;
    }

    if ( reservations < SCTK_PORTALS_SIZE_HEADERS_MIN )
    {
        if ( free_space < SCTK_PORTALS_SIZE_HEADERS - reservations )
        {
            ListAllocMsg ( EvQ );
        }

        reserve ( portals_info, ind, EvQ, SCTK_PORTALS_SIZE_HEADERS - reservations );
    }
}

/* append a new sending message in the list */
void sctk_portals_send_put_event ( sctk_portals_rail_info_t *portals_info, sctk_thread_ptp_message_t *msg, sctk_portals_route_info_t *data, int type, sctk_portals_message_t *msgPortals )
{

    int found = 0;
    uint64_t match;
    uint64_t ignore;
    int my_idThread;
    int peer_idThread;

    my_idThread = sctk_get_local_id ( SCTK_MSG_SRC_PROCESS ( msg ) ); //id of source
    peer_idThread = sctk_get_local_id ( SCTK_MSG_DEST_PROCESS ( msg ) ); //id of destination
    /*sctk_debug("check of msgPortals = %p", msgPortals);*/
    /*msgPortals->init_message = 1;*/
    set_Match_Ignore_Bits ( &match, &ignore, peer_idThread, SCTK_MSG_TAG ( msg ), 1 ); //set the match and ignore
    ptl_handle_ni_t *ni_h 	= & ( portals_info->ni_handle_phys ); //network interface

    sctk_spinlock_lock ( &portals_info->lock[my_idThread] ); //to be thread safe

    sctk_portals_event_table_list_t *EvQ 		= &portals_info->event_list[my_idThread];//get the event queue of the good index

    unsigned currPos = 0;
    sctk_portals_event_table_t *currList = &EvQ->head;
    evalReserve ( portals_info, my_idThread, EvQ );
    while ( !found )
    {

        if ( currList->nb_elems < SCTK_PORTALS_SIZE_EVENTS )
        {
            int pos = 0;

            while ( currList->events[pos].used == RESERVED || currList->events[pos].used == IN_USE )
                pos++;//while isn't free

            currList->nb_elems++;
            currList->events[pos].msg.peer.phys.nid 			= data->id.phys.nid;//proc
            currList->events[pos].msg.peer.phys.pid				= data->id.phys.pid;//proc

            currList->events[pos].msg.match_bits	= match;
            currList->events[pos].msg.ignore_bits	= ignore;
            currList->events[pos].used 				= IN_USE;//it is used now

            currList->events[pos].msg.type 				= type;//read or write

            currList->events[pos].msg.my_idThread 		= my_idThread;
            currList->events[pos].msg.tag				= SCTK_MSG_TAG ( msg );
            currList->events[pos].msg.peer_idThread 	= peer_idThread;


            currList->events[pos].msg.me.match_id.phys.nid		 	= data->id.phys.nid;
            currList->events[pos].msg.me.match_id.phys.pid 			= data->id.phys.pid;

            if ( type == WRITE )
            {
                currList->events[pos].ptrmsg.msg_send	= msg;
                currList->events[pos].msg.me.length 	= SCTK_MSG_SIZE ( msg );
                int aligned_size, page_size;

                switch ( msg->tail.message_type )
                    //type of the message
                {
                    case SCTK_MESSAGE_CONTIGUOUS:
                        currList->events[pos].msg.me.start  	= msg->tail.message.contiguous.addr;//then we have to send the message
                        currList->events[pos].msg.me.length 	= msg->tail.message.contiguous.size;


                        if ( msg->tail.message.contiguous.addr == NULL )
                        {
                            sctk_error ( "error addr null" );
                            abort();
                        }

                        break;

                    case SCTK_MESSAGE_NETWORK:
                        currList->events[pos].msg.me.start		= ( char * ) msg + sizeof ( sctk_thread_ptp_message_t );
                        break;

                    case SCTK_MESSAGE_PACK:
                        sctk_debug ( "dealing with pack" );

                    case SCTK_MESSAGE_PACK_ABSOLUTE:


                        aligned_size = SCTK_MSG_SIZE ( msg );
                        page_size = getpagesize();

                        if ( posix_memalign ( ( void ** ) &currList->events[pos].msg.buffer, page_size, aligned_size ) != 0 )
                        {
                            sctk_error ( "error allocation memalign" );
                        }

                        sctk_net_copy_in_buffer ( msg, currList->events[pos].msg.buffer );
                        currList->events[pos].msg.me.start 		= currList->events[pos].msg.buffer;

                        break;

                    default:
                        not_reachable();
                        break;
                }
                currList->events[pos].msg.me.uid    	= PTL_UID_ANY;
                currList->events[pos].msg.me.ignore_bits 		= ignore;
                currList->events[pos].msg.me.match_bits			= match;
                currList->events[pos].msg.me.options = OPTIONS;

                if ( !_me_is_allocated ( currList->events[pos].msg.allocs ) )
                    //it's not necessary to reallocate when it was already allocated
                {
                    CHECK_RETURNVAL ( PtlCTAlloc ( *ni_h, & ( currList->events[pos].msg.me.ct_handle ) ) );
                    set_allocBits ( &currList->events[pos].msg.allocs, ME_ALLOCATED );
                }
                else
                {
                    CHECK_RETURNVAL ( PtlCTSet ( currList->events[pos].msg.me.ct_handle, portals_info->zeroCounter ) ); //reinit
                }

                CHECK_RETURNVAL ( PtlMEAppend ( *ni_h, currList->events[pos].msg.my_idThread, & ( currList->events[pos].msg.me ), PTL_PRIORITY_LIST, NULL,
                            & ( currList->events[pos].msg.me_handle ) ) );

                /*send request*/
                currList->events[pos].msg.md.start  	= msg;
                currList->events[pos].msg.md.length 	= sizeof ( sctk_thread_ptp_message_body_t );
                currList->events[pos].msg.md.options    = 0;
                currList->events[pos].msg.md.eq_handle  = PTL_EQ_NONE;   // i.e. don't queue send events

                if ( !_md_is_allocated ( currList->events[pos].msg.allocs ) )
                    //it's not necessary to reallocate when it was already allocated
                {
                    CHECK_RETURNVAL ( PtlCTAlloc ( *ni_h, & ( currList->events[pos].msg.md.ct_handle ) ) );
                    set_allocBits ( &currList->events[pos].msg.allocs, MD_ALLOCATED );
                }
                else
                {
                    CHECK_RETURNVAL ( PtlCTSet ( currList->events[pos].msg.md.ct_handle, portals_info->zeroCounter ) );
                }

                CHECK_RETURNVAL ( PtlMDBind ( *ni_h, & ( currList->events[pos].msg.md ), & ( currList->events[pos].msg.md_handle ) ) );
                currList->events[pos].msg.append_pos		= pos;
                currList->events[pos].msg.append_list		= currPos;
                found = 1;

                //now we send a put with the header of the message
                CHECK_RETURNVAL ( PtlPut ( currList->events[pos].msg.md_handle, 0, sizeof ( sctk_thread_ptp_message_body_t ), PTL_NO_ACK_REQ, currList->events[pos].msg.me.match_id, currList->events[pos].msg.my_idThread, match + FLAG_REQ, 0, NULL, 0 ) ); //we send a request

            }
            else
            {
                sctk_error ( "unhandled case" );
                not_implemented();
            }
        }
        else
        {
            if ( currPos + 1 >= EvQ->nb_elements )
                ListAllocMsg ( EvQ );

            currPos++;
            currList = currList->next;

        }
    }
    sctk_spinlock_unlock ( &portals_info->lock[my_idThread] ); //to be thread safe
}
void sctk_portals_recv_put_event ( sctk_rail_info_t *rail, ptl_event_t *event, int peer_idThread )
{
    /*TODO use infos of put req event to build the entry on the list ,the msg is created before but uninitialized (we will get it) */
    uint64_t match;
    uint64_t ignore;

    int my_idThread;
    int tag, found = 0;
    int currPos = 0;
    //give the informations in the match_bits attribute
    get_idThread ( &my_idThread, event->match_bits );
    get_tag ( &tag, event->match_bits );

    sctk_portals_rail_info_t *portals_info = &rail->network.portals;
    set_Match_Ignore_Bits ( &match, &ignore, my_idThread, tag, 0);
    ptl_handle_ni_t *ni_h 	= & ( portals_info->ni_handle_phys );

    sctk_portals_event_table_list_t *EvQ 		= &portals_info->event_list[peer_idThread];//we get the good index

    sctk_portals_event_table_t *currList = &EvQ->head;
    evalReserve ( portals_info, peer_idThread, EvQ );
    if(event->match_bits == TAG_INIT + FLAG_REQ){
        sctk_debug("I get a init request !!!!!");
    }
    while ( !found )
    {

        if ( currList->nb_elems < SCTK_PORTALS_SIZE_EVENTS && currList->nb_elems_headers > 0 )
            //enough places ?
        {
            int pos = 0;

            while ( currList->events[pos].ptrmsg.msg_send != event->start )
            {
                pos++;    //same address = good pt_message
            }

            if ( !_md_is_allocated ( currList->events[pos].msg.allocs ) )
                //it's not necessary to reallocate when it was already allocated
            {

                CHECK_RETURNVAL ( PtlCTAlloc ( *ni_h, & ( currList->events[pos].msg.md.ct_handle ) ) );
                set_allocBits ( &currList->events[pos].msg.allocs, MD_ALLOCATED );
            }
            else
            {

                CHECK_RETURNVAL ( PtlCTSet ( currList->events[pos].msg.md.ct_handle, portals_info->zeroCounter ) );
            }

            currList->events[pos].used 					= IN_USE;
            currList->nb_elems_headers--;
            currList->events[pos].pt_index				= peer_idThread;
            currList->events[pos].msg.peer.phys.nid 	= event->initiator.phys.nid;
            currList->events[pos].msg.peer.phys.pid		= event->initiator.phys.pid;


            currList->events[pos].msg.match_bits		= match;
            currList->events[pos].msg.ignore_bits		= ignore;


            currList->events[pos].msg.type 				= READ;

            currList->events[pos].msg.my_idThread 		= my_idThread;
            currList->events[pos].msg.tag				= tag;
            currList->events[pos].msg.peer_idThread 	= peer_idThread;


            sctk_rebuild_header ( currList->events[pos].ptrmsg.msg_send ); //rebuild
            sctk_reinit_header ( currList->events[pos].ptrmsg.msg_send, sctk_portals_free, sctk_portals_message_copy );

            SCTK_MSG_COMPLETION_FLAG_SET ( currList->events[pos].ptrmsg.msg_send , NULL );
            currList->events[pos].ptrmsg.msg_send->tail.portals_message_info_t 	= &currList->events[pos];//save the event datas
            currList->events[pos].ptrmsg.msg_send->tail.portals_info_t			= portals_info;//save the rail info

            currList->events[pos].ptrmsg.msg_send->tail.message_type = SCTK_MESSAGE_CONTIGUOUS;
            sctk_spinlock_unlock ( &portals_info->lock[peer_idThread] );

            rail->send_message_from_network ( currList->events[pos].ptrmsg.msg_send );
            sctk_spinlock_lock ( &portals_info->lock[peer_idThread] );
            currList->events[pos].msg.append_pos		= pos;
            currList->events[pos].msg.append_list		= currPos;
            found = 1;
        }

        currList = currList->next;
        currPos++;
    }
}

/* delete the element at the position pos in the list number 'list'*/
void ListFree ( sctk_portals_rail_info_t *portals_info, sctk_portals_event_table_list_t *EvQ, int list, int pos, int tab )
{
    int i;

    if ( tab == MSG_ARRAY )
    {
        sctk_portals_event_table_t *currList = &EvQ->head;

        for ( i = 0; i < list; i++ )
            currList = currList->next;

        if ( currList->events[pos].used == IN_USE )
        {

            sctk_debug ( "free (type %d pos %d) size (list %d %d) at %p and msg (%p,%p)", currList->events[pos].msg.type, pos, i, currList->nb_elems, &currList->events[pos], currList->events[pos].ptrmsg.msg_send, currList->events[pos].ptrmsg.msg_recv );


            if ( currList->events[pos].msg.type == WRITE )
            {
                currList->nb_elems--;
                currList->events[pos].used = IDLE;
                /* the write is done so we can free memory */


                switch ( currList->events[pos].ptrmsg.msg_send->tail.message_type )
                {
                    case SCTK_MESSAGE_CONTIGUOUS:


                        break;

                    case SCTK_MESSAGE_NETWORK:

                        break;

                    case SCTK_MESSAGE_PACK:
                    case SCTK_MESSAGE_PACK_ABSOLUTE:

                        currList->events[pos].msg.buffer = NULL;
                        break;

                    default:
                        not_reachable();
                        break;
                }
                sctk_debug ( "entering scfm" );
                sctk_complete_and_free_message ( currList->events[pos].ptrmsg.msg_send );
                sctk_debug ( "leaving scfm" );
                currList->events[pos].ptrmsg.msg_send = NULL;
            }
            else
            {
                currList->nb_elems_headers++;
                /* the read is done so we can free memory */

                sctk_debug ( "entering smcf %p %p", SCTK_MSG_COMPLETION_FLAG ( currList->events[pos].ptrmsg.msg_send )  , SCTK_MSG_COMPLETION_FLAG ( currList->events[pos].ptrmsg.msg_recv ) );

                switch ( currList->events[pos].ptrmsg.msg_recv->tail.message_type )
                {
                    case SCTK_MESSAGE_CONTIGUOUS:


                        break;

                    case SCTK_MESSAGE_NETWORK:

                        break;

                    case SCTK_MESSAGE_PACK:
                    case SCTK_MESSAGE_PACK_ABSOLUTE:
                        sctk_net_message_copy_from_buffer ( currList->events[pos].msg.buffer, & ( currList->events[pos].ptrmsg ), 0 );

                        currList->events[pos].msg.buffer = NULL;
                        break;

                    default:
                        not_reachable();
                        break;

                }

                //we add a new entry on the table with a pointer to this ptp_message
                sctk_message_completion_and_free ( currList->events[pos].ptrmsg.msg_send, currList->events[pos].ptrmsg.msg_recv );
                sctk_debug ( "leaving smcf" );

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

                CHECK_RETURNVAL ( PtlMEAppend ( portals_info->ni_handle_phys, portals_info->pt_index[currList->events[pos].pt_index], &currList->events[pos].msg.me, PTL_PRIORITY_LIST, NULL, &currList->events[pos].msg.me_handle ) );

                currList->events[pos].ptrmsg.msg_recv = NULL;

            }

            CHECK_RETURNVAL ( PtlMDRelease ( currList->events[pos].msg.md_handle ) );
            currList->events[pos].msg.append_pos = -1;

        }
        else
        {
            sctk_error ( "Error portals list free" );
            abort();
        }

    }

    sctk_debug ( "free leaving" );

}

/*check if a message was received or sended*/
void clearList ( sctk_rail_info_t *rail, int id )
{

    sctk_portals_rail_info_t *portals_info = &rail->network.portals;

    if ( sctk_spinlock_trylock ( &portals_info->lock[id] ) == TRYLOCK_SUCCESS )
        //to be thread safe
    {


        ptl_handle_ni_t *ni_h = &portals_info->ni_handle_phys;
        sctk_portals_event_table_list_t *EvQ = &portals_info->event_list[id];
        ptl_handle_eq_t *eq_h = &portals_info->event_handler[id];

        sctk_portals_event_table_t *currList = &EvQ->head;
        ptl_ct_event_t test, test2;
        int i, j, k, size;


        for ( j = 0; j < EvQ->nb_elements; j++ )
            //check all the lists
        {
            size = currList->nb_elems;
            i = 0;

            for ( k = 0; k < size; k++, i++ )
                //and all the elements
            {
                while ( i < SCTK_PORTALS_SIZE_EVENTS && currList->events[i].used != IN_USE )
                {
                    i++;
                }

                if ( i == SCTK_PORTALS_SIZE_EVENTS ) //to avoid bugs
                    break;

                if ( currList->events[i].msg.type == WRITE )
                {


                    CHECK_RETURNVAL ( PtlCTGet ( currList->events[i].msg.me.ct_handle, &test ) ); //the message was getted

                    if ( test.success == 1 )
                    {

                        sctk_debug ( "free in index %d", id );
                        ListFree ( portals_info, EvQ, j, i, MSG_ARRAY );
                    }
                    else
                        if ( test.failure != 0 )
                        {
                            sctk_error ( "failure get portals send" );
                            abort();
                        }


                }
                else
                    if ( currList->events[i].msg.type == READ )
                    {


                        CHECK_RETURNVAL ( PtlCTGet ( currList->events[i].msg.md.ct_handle, &test ) );

                        if ( test.success == 1 )
                            //the message was getted
                        {

                            sctk_debug ( "free in index %d", id );
                            ListFree ( portals_info, EvQ, j, i, MSG_ARRAY );
                        }
                        else
                            if ( test.failure != 0 )
                            {

                                sctk_error ( "failure get portals %d", test.failure );

                                abort();
                            }
                    }

            }
        }

        sctk_spinlock_unlock ( &portals_info->lock[id] ); //to be thread safe
    }
    //sctk_spinlock_unlock(&portals_info->lock[0]);//to be thread safe
}

void notify ( sctk_rail_info_t *rail, int id )
{

    clearList ( rail, id ); //CT value is 1 = perform

    sctk_portals_rail_info_t *portals_info = &rail->network.portals;

    ptl_handle_ni_t *ni_h = &portals_info->ni_handle_phys;
    ptl_handle_eq_t *eq_h = &portals_info->event_handler[id];

    ptl_event_t event;

    sctk_spinlock_lock ( &portals_info->lock[id] ); //to be thread safe
    /*sctk_debug("A message have been caught from %d", id);*/
    while ( PtlEQGet ( *eq_h, &event ) == PTL_OK && event.type == PTL_EVENT_PUT )
        //we give all events
    {
        sctk_portals_recv_put_event ( rail, &event, id );
    }

    sctk_spinlock_unlock ( &portals_info->lock[id] ); //to be thread safe
    
}

typedef struct sctk_portals_connection_context_s
{
	ptl_process_t from;
	int to;
    ptl_pt_index_t entry;
} sctk_portals_connection_context_t;

typedef enum
{
	SCTK_PORTALS_CONTROL_MESSAGE_ON_DEMAND_STATIC,
	SCTK_PORTALS_CONTROL_MESSAGE_ON_DEMAND_DYNAMIC
}sctk_portals_control_message_t;

static void sctk_network_connection_to_portals(sctk_rail_info_t* rail,sctk_portals_connection_context_t * ctx, sctk_route_origin_t route_type)
{
    sctk_debug("RECEIVED DATA FOR ON -DEMAND");
}
static void sctk_network_connection_from_portals(int from, int to, sctk_rail_info_t* rail, sctk_route_origin_t route_type) {
    sctk_portals_connection_context_t ctx;
    ptl_process_t remote_id;
    ptl_ct_event_t ctc;
    ptl_me_t id_entry;
    ptl_handle_me_t id_handler;

    id_entry.start = &remote_id;
    id_entry.length= sizeof(remote_id);
    id_entry.uid = PTL_UID_ANY;
    id_entry.match_id.phys.nid = PTL_NID_ANY;
    id_entry.match_id.phys.pid = PTL_PID_ANY;
    id_entry.match_bits = TAG_INIT + FLAG_REQ;
    id_entry.ignore_bits = 0;
    id_entry.options = OPTIONS_HEADER;
    sctk_debug("Before ME appending with tag : %d", id_entry.match_bits);
    CHECK_RETURNVAL(PtlCTAlloc(rail->network.portals.ni_handle_phys, &id_entry.ct_handle));
    CHECK_RETURNVAL(PtlMEAppend(rail->network.portals.ni_handle_phys, 0, &id_entry,PTL_PRIORITY_LIST, NULL,  &id_handler));
    ctx.from = rail->network.portals.my_id;
    ctx.to   = to;
    ctx.entry = 0;
    sctk_debug("Sending control message to %d", to);
    if(route_type == ROUTE_ORIGIN_STATIC){
        sctk_control_messages_send_rail(to,SCTK_PORTALS_CONTROL_MESSAGE_ON_DEMAND_STATIC,&ctx,sizeof(sctk_portals_connection_context_t), rail->rail_number );
    } else {
        sctk_control_messages_send_rail(to,SCTK_PORTALS_CONTROL_MESSAGE_ON_DEMAND_DYNAMIC,&ctx,sizeof(sctk_portals_connection_context_t), rail->rail_number ); 
    }
    //wait remote to publish
    sctk_debug("Wait for %d to respond...", to);
    PtlCTWait(id_entry.ct_handle, 1, &ctc);
    sctk_debug ( "PTL_PROCESS Written : nid %u pid %u", remote_id.phys.nid, remote_id.phys.pid );

    //add route
   sctk_portals_add_route(to, remote_id, rail, ROUTE_ORIGIN_DYNAMIC);
}

static void sctk_portals_connection_from(int from, int to , sctk_rail_info_t *rail){
    sctk_network_connection_from_portals(from, to, rail, ROUTE_ORIGIN_STATIC);
}
static void sctk_portals_connection_to(int from, int to , sctk_rail_info_t *rail){

}
void sctk_portals_on_demand_connection_handler( sctk_rail_info_t *rail, int dest_process )
{
    sctk_debug("ON DEMAND ON PORTALS !!!!!!!!!!!!!!!!!!!!!!!!");
    sctk_network_connection_from_portals(sctk_process_rank, dest_process, rail, ROUTE_ORIGIN_DYNAMIC);
    sctk_debug("GET ON DEMAND...");
}
void sctk_portals_control_message_handler( sctk_rail_info_t *rail, int src_process, int source_rank, char subtype, char param, void * data) {
    sctk_portals_control_message_t action = subtype;
    switch(action){
        case SCTK_PORTALS_CONTROL_MESSAGE_ON_DEMAND_STATIC:
            sctk_network_connection_to_portals(rail, (sctk_portals_connection_context_t *)data, ROUTE_ORIGIN_STATIC);
            break;
        case SCTK_PORTALS_CONTROL_MESSAGE_ON_DEMAND_DYNAMIC:
            sctk_network_connection_to_portals(rail, (sctk_portals_connection_context_t *)data, ROUTE_ORIGIN_DYNAMIC);
            break;
    }
}

void sctk_network_init_portals_all ( sctk_rail_info_t *rail )
{
    int right_rank;
    int left_rank;
    int idle = 1, j, k;
    ptl_me_t init;
    ptl_handle_me_t init_h;
    ptl_ni_limits_t desired;
    int ntasks;
    ptl_process_t right_id, left_id;
    char right_rank_connection_infos[MAX_STRING_SIZE];
    char left_rank_connection_infos[MAX_STRING_SIZE];
/*
 *    {
 *#warning "GROS HACK"
 *        {
 *            static char rank[100];
 *            sprintf ( rank, "%d", sctk_process_rank );
 *            setenv ( "PORTALS4_RANK", rank, 1 );
 *        }
 *    }
 */

    /*sctk_pmi_barrier();*/
    rail->connect_from = sctk_portals_connection_from;
    rail->connect_to = sctk_portals_connection_to;
    rail->control_message_handler = sctk_portals_control_message_handler;
    CHECK_RETURNVAL(PtlInit());
    sctk_portals_init_boundaries ( &desired );
    
    CHECK_RETURNVAL( PtlNIInit ( PTL_IFACE_DEFAULT, PTL_NI_MATCHING |
                PTL_NI_PHYSICAL, PTL_PID_ANY, &desired,
                &rail->network.portals.actual,
                &rail->network.portals.ni_handle_phys ));

    CHECK_RETURNVAL( PtlGetPhysId ( rail->network.portals.ni_handle_phys, &rail->network.portals.my_id ));


    sctk_debug ( "ni_handle_phys %p my_id.nid %u my_id.pid %u", rail->network.portals.ni_handle_phys, rail->network.portals.my_id.phys.nid, rail->network.portals.my_id.phys.pid );
    
    ntasks = sctk_get_local_number_tasks();//the same number of indexes for everybody
    sctk_debug ( "tasks max %d", ntasks );
    rail->network.portals.nb_tasks_per_process = ntasks;
    rail->network.portals.event_list = sctk_malloc ( sizeof ( sctk_portals_event_table_list_t ) * ntasks );
    rail->network.portals.event_handler = sctk_malloc ( sizeof ( ptl_handle_eq_t ) * ntasks );
    rail->network.portals.lock = sctk_malloc ( sizeof ( sctk_spinlock_t ) * ntasks );
    //rail->network.portals.pre_matching = sctk_malloc(sizeof(ptl_me_t)*ntasks*(SCTK_PORTALS_SIZE_HEADERS+2));
    //rail->network.portals.pre_matching_handle = sctk_malloc(sizeof(ptl_handle_me_t)*ntasks*(SCTK_PORTALS_SIZE_HEADERS+2));
    rail->network.portals.pt_index = sctk_malloc ( sizeof ( ptl_pt_index_t ) * ntasks );
    bzero ( &rail->network.portals.zeroCounter, sizeof ( ptl_ct_event_t ) );
    init.start  = &idle;
    init.length = sizeof ( unsigned );
    init.uid    = PTL_UID_ANY;
    init.match_id.phys.nid = PTL_NID_ANY;
    init.match_id.phys.pid = PTL_PID_ANY;

    init.match_bits    = 0x0000000000000000;
    init.ignore_bits   = 0xFFFFFFFFFFFFFFFF;

    init.options = OPTIONS;
    CHECK_RETURNVAL ( PtlCTAlloc ( rail->network.portals.ni_handle_phys, &init.ct_handle ) );

    ptl_me_t init2;
    ptl_handle_me_t init2_h;

    init2.start  = &idle;
    init2.length = sizeof ( unsigned );
    init2.uid    = PTL_UID_ANY;
    init2.match_id.phys.nid = PTL_NID_ANY;
    init2.match_id.phys.pid = PTL_PID_ANY;


    init2.match_bits    = 0x0000000000000000;
    init2.ignore_bits   = 0xFFFFFFFFFFFFFFFF;

    init2.options = OPTIONS;
    CHECK_RETURNVAL ( PtlCTAlloc ( rail->network.portals.ni_handle_phys, &init2.ct_handle ) );

    for ( j = 0; j < ntasks; j++ )
    {
        //for each index
        rail->network.portals.lock[j] = SCTK_SPINLOCK_INITIALIZER;
        rail->network.portals.event_list[j].nb_elements	= 1;
        rail->network.portals.event_list[j].head.nb_elems = 0;
        rail->network.portals.event_list[j].head.nb_elems_headers = SCTK_PORTALS_SIZE_HEADERS;
        rail->network.portals.event_list[j].head.next = NULL;
        CHECK_RETURNVAL ( PtlEQAlloc ( rail->network.portals.ni_handle_phys, SCTK_PORTALS_SIZE_EVENTS, &rail->network.portals.event_handler[j] ) );

        CHECK_RETURNVAL ( PtlPTAlloc ( rail->network.portals.ni_handle_phys, 0, rail->network.portals.event_handler[j], j, &rail->network.portals.pt_index[j] ) );

        assert ( rail->network.portals.pt_index[j] == j );

        if ( j == 0 )
        {
            //for initialisation
            CHECK_RETURNVAL ( PtlMEAppend ( rail->network.portals.ni_handle_phys, 0, &init, PTL_PRIORITY_LIST, NULL, &init_h ) );

            if ( sctk_process_number > 2 )
                CHECK_RETURNVAL ( PtlMEAppend ( rail->network.portals.ni_handle_phys, 0, &init2, PTL_PRIORITY_LIST, NULL, &init2_h ) );
        }

        for ( k = 0; k < SCTK_PORTALS_SIZE_HEADERS; k++ )
        {
            //we add SCTK_PORTALS_SIZE_HEADERS entries on the table in order to receive put operations
            rail->network.portals.event_list[j].head.events[k].ptrmsg.msg_send = sctk_malloc ( sizeof ( sctk_thread_ptp_message_t ) );
            bzero ( rail->network.portals.event_list[j].head.events[k].ptrmsg.msg_send, sizeof ( sctk_thread_ptp_message_t ) );

            rail->network.portals.event_list[j].head.events[k].msg.me.start  = rail->network.portals.event_list[j].head.events[k].ptrmsg.msg_send;
            rail->network.portals.event_list[j].head.events[k].msg.me.length = sizeof ( sctk_thread_ptp_message_body_t ); //
            rail->network.portals.event_list[j].head.events[k].msg.me.uid    = PTL_UID_ANY;
            rail->network.portals.event_list[j].head.events[k].used = RESERVED;
            rail->network.portals.event_list[j].head.events[k].msg.allocs = 0;
            rail->network.portals.event_list[j].head.nb_elems++;
            rail->network.portals.event_list[j].head.events[k].msg.me.match_id.phys.nid = PTL_NID_ANY;
            rail->network.portals.event_list[j].head.events[k].msg.me.match_id.phys.pid = PTL_PID_ANY;
            rail->network.portals.event_list[j].head.events[k].msg.me.match_bits    = FLAG_REQ;
            rail->network.portals.event_list[j].head.events[k].msg.me.ignore_bits   = REQ_IGN;

            rail->network.portals.event_list[j].head.events[k].msg.me.options = OPTIONS_HEADER;
            CHECK_RETURNVAL ( PtlCTAlloc ( rail->network.portals.ni_handle_phys, &rail->network.portals.event_list[j].head.events[k].msg.me.ct_handle ) );
            CHECK_RETURNVAL ( PtlMEAppend ( rail->network.portals.ni_handle_phys, rail->network.portals.pt_index[j], &rail->network.portals.event_list[j].head.events[k].msg.me, PTL_PRIORITY_LIST, NULL, &rail->network.portals.event_list[j].head.events[k].msg.me_handle ) );
            set_allocBits ( &rail->network.portals.event_list[j].head.events[k].msg.allocs, ME_ALLOCATED );
        }
    }


    //create unique string from nid/pid combination as val for PMI
    encode ( &rail->network.portals.my_id, sizeof ( rail->network.portals.my_id ), rail->network.portals.connection_infos,MAX_STRING_SIZE);
	rail->network.portals.connection_infos_size = strlen ( rail->network.portals.connection_infos );
    sctk_debug ( "%d + %d = %d ?", sizeof ( rail->network.portals.my_id.phys.nid ), sizeof ( rail->network.portals.my_id.phys.pid ), sizeof ( rail->network.portals.my_id ) );
    assume(rail->network.portals.connection_infos_size  < MAX_STRING_SIZE);
    sctk_debug ( "Send KEY %s", rail->network.portals.connection_infos );
    
    if( rail->requires_bootstrap_ring == 0 )
    {
        return;
    }
    
    right_rank = ( sctk_process_rank + 1 ) % sctk_process_number;
    left_rank = ( sctk_process_rank + sctk_process_number - 1 ) % sctk_process_number;
    sctk_debug("for %d (%d/%d)", sctk_process_rank, left_rank, right_rank); 
	/*sctk_debug ( "Connection Infos (%d): %s", sctk_process_rank, rail->network.portals.connection_infos );*/
    assume(sctk_pmi_put_connection_info ( rail->network.portals.connection_infos, MAX_STRING_SIZE, rail->rail_number ) == 0);

    sctk_pmi_barrier();

    /*sctk_endpoint_t *tmp, *tmp2;*/
    
    /*snprintf ( key, key_max, "PORTALS-%d-%06d", rail->rail_number , right_rank );*/
    sctk_pmi_get_connection_info ( right_rank_connection_infos, MAX_STRING_SIZE, rail->rail_number, right_rank );
    sctk_debug ( "Got KEY %s from %d", right_rank_connection_infos, right_rank); //key is the process_rank ,val is the nid and pid encoded
    /*tmp = sctk_malloc ( sizeof ( sctk_endpoint_t ) );*/
    /*memset ( tmp, 0, sizeof ( sctk_endpoint_t ) );*/
    /*Store connection info*/
    /*sctk_debug ( "Got id %lu\t%lu", tmp->data.portals.id.phys.nid, tmp->data.portals.id.phys.pid );*/
    decode (right_rank_connection_infos, & ( right_id ), sizeof ( right_id ) );

    sctk_pmi_barrier();
    /*sctk_debug("%d retrieves %d(%s) [Right]\n", sctk_process_rank, right_rank, right_rank_connection_infos);*/
    sctk_portals_bind_to(rail, right_id);
    sctk_debug ( "OK: Bind %d -> %d", sctk_process_rank, right_rank);

    if(sctk_process_number > 2){
        /*snprintf ( key, key_max, "PORTALS-%d-%06d", rail->rail_number , left_rank );*/
        sctk_pmi_get_connection_info ( left_rank_connection_infos, MAX_STRING_SIZE, rail->rail_number, left_rank );
        sctk_debug ( "Got KEY %s from %d", right_rank_connection_infos, right_rank); //key is the process_rank ,val is the nid and pid encoded

        /*tmp2 = sctk_malloc ( sizeof ( sctk_endpoint_t ) );*/
        /*memset ( tmp2, 0, sizeof ( sctk_endpoint_t ) );*/
        /*Store connection info*/
        decode ( left_rank_connection_infos, & ( left_id ), sizeof ( left_id ) );
        sctk_debug ( "Got id %lu\t%lu", left_id.phys.nid, left_id.phys.pid );	
        sctk_portals_bind_to(rail, left_id);
        sctk_debug ( "OK: Bind %d -> %d", sctk_process_rank, left_rank);
    }
    
    sctk_pmi_barrier();

    sctk_portals_add_route (right_rank, right_id, rail, ROUTE_ORIGIN_STATIC);
    if ( sctk_process_number > 2 )
    {
        sctk_portals_add_route (left_rank, left_id, rail, ROUTE_ORIGIN_STATIC);
    }
    
    sctk_pmi_barrier();
}
#endif
