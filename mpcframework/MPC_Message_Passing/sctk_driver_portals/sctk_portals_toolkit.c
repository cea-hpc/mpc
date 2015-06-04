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

#include <sctk_portals_toolkit.h>
/************ TOOLS **********************************/
void test_event_type ( ptl_event_t *event, int rank, const char *type )
{
    sctk_nodebug ( "%s -> %d: ", type, rank );

    switch ( event->type )
    {
        case PTL_EVENT_GET:
            sctk_nodebug ( "GET" );
            break;

        case PTL_EVENT_GET_OVERFLOW:
            sctk_nodebug ( "GETOVERFLOW" );
            break;

        case PTL_EVENT_PUT:
            sctk_nodebug ( "PUT" );
            break;

        case PTL_EVENT_PUT_OVERFLOW:
            sctk_nodebug ( "PUTOVERFLOW" );
            break;

        case PTL_EVENT_ATOMIC:
            sctk_nodebug ( "ATOMIC" );
            break;

        case PTL_EVENT_ATOMIC_OVERFLOW:
            sctk_nodebug ( "ATOMIC OVERFLOW: " );
            break;

        case PTL_EVENT_FETCH_ATOMIC:
            sctk_nodebug ( "FETCHATOMIC: " );
            break;

        case PTL_EVENT_FETCH_ATOMIC_OVERFLOW:
            sctk_nodebug ( "FETCHATOMIC OVERFLOW: " );
            break;

        case PTL_EVENT_REPLY:
            sctk_nodebug ( "REPLY" );
            break;

        case PTL_EVENT_SEND:
            sctk_nodebug ( "SEND" );
            break;

        case PTL_EVENT_ACK:
            sctk_nodebug ( "ACK" );
            break;

        case PTL_EVENT_PT_DISABLED:
            sctk_nodebug ( "PTDISABLED" );
            break;

        case PTL_EVENT_AUTO_UNLINK:
            sctk_nodebug ( "UNLINK" );
            break;

        case PTL_EVENT_AUTO_FREE:
            sctk_nodebug ( "FREE" );
            break;

        case PTL_EVENT_SEARCH:
            sctk_nodebug ( "SEARCH" );
            break;

        case PTL_EVENT_LINK:
            sctk_nodebug ( "LINK" );
            break;

        default:
            sctk_nodebug ( "%p %d UNKNOWN\n", event, event->type );
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

void setBitstoOne ( long int *ptr, int size )
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

void sctk_initLimits ( ptl_ni_limits_t *desired )
{
    setBitstoOne ( ( long int * ) &desired->max_unexpected_headers, sizeof ( int ) );
    setBitstoOne ( ( long int * ) &desired->max_entries, sizeof ( int ) );
    setBitstoOne ( ( long int * ) &desired->max_mds, sizeof ( int ) );
    setBitstoOne ( ( long int * ) &desired->max_cts, sizeof ( int ) );
    setBitstoOne ( ( long int * ) &desired->max_eqs, sizeof ( int ) );
    setBitstoOne ( ( long int * ) &desired->max_pt_index, sizeof ( int ) );
    setBitstoOne ( ( long int * ) &desired->max_iovecs, sizeof ( int ) );
    setBitstoOne ( ( long int * ) &desired->max_list_size, sizeof ( int ) );
    setBitstoOne ( ( long int * ) &desired->max_triggered_ops, sizeof ( int ) );
    setBitstoOne ( ( long int * ) &desired->max_msg_size, sizeof ( int ) );
    setBitstoOne ( ( long int * ) &desired->max_atomic_size, sizeof ( int ) + 1 );
    setBitstoOne ( ( long int * ) &desired->max_waw_ordered_size, sizeof ( int ) );
    setBitstoOne ( ( long int * ) &desired->max_war_ordered_size, sizeof ( int ) );
    setBitstoOne ( ( long int * ) &desired->max_volatile_size, sizeof ( int ) + 1 );
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

void set_Match_Ignore_Bits ( ptl_match_bits_t *match, ptl_match_bits_t *ignore, unsigned id, int tag )
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


    sctk_Event_t *event 					= ( sctk_Event_t * ) send->tail.portals_message_info_t; // get the event informations
    sctk_portals_message_t *ptrmsg 			= &event->msg;// get the portals struct message

    if ( ptrmsg == NULL )
        abort();

    if ( event == NULL )
        abort();


    sctk_portals_rail_info_t *portals_info	= ( sctk_portals_rail_info_t * ) send->tail.portals_info_t; //get the portal rail
    event->ptrmsg.msg_recv 			= recv;
    ptl_handle_ni_t *ni_h 			= &portals_info->ni_handle_phys;//get the network interface
    set_Match_Ignore_Bits ( &match, &ignore, ptrmsg->my_idThread, ptrmsg->tag );

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

    sctk_nodebug ( "message free %p", msg );
    //sctk_free(msg);
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
inline int sctk_get_local_number_tasks ( int proc_id )
{
    int i, p = 0;

    int total_number = sctk_get_total_tasks_number();
    int rap = total_number / sctk_process_number;
    int rest = ( total_number % sctk_process_number );

    p = rap;

    if ( proc_id < rest )
        p++;

    sctk_nodebug ( "tasks: %d", p );
    return p;

}

/* get the max number of threads in a proc */
inline int sctk_get_max_local_number_tasks()
{
    int i, p = 0;

    int total_number = sctk_get_total_tasks_number();
    int rap = total_number / sctk_process_number;
    int rest = ( total_number % sctk_process_number );

    p = rap;

    if ( rest != 0 )
        p++;

    return p;

}

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

    sctk_nodebug ( "%s %d", __FUNCTION__, task_id );

    for ( i = 0; i < total_number + rap + add; i += rap + add, p++ )
    {
        if ( i > task_id )
        {
            sctk_nodebug ( "%s %d", __FUNCTION__, p - 1 );
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
void ListAllocMsg ( sctk_EventQ_t *EvQ )
{
    int i;
    sctk_EventL_t *currList = &EvQ->ListMsg;

    for ( i = 0; i < EvQ->SizeMsgList - 1; i++ )
        currList = currList->next;

    currList->next = malloc ( sizeof ( sctk_EventL_t ) );
    currList->next->nb_elems = 0;
    EvQ->SizeMsgList++;
}

static inline void reserve ( sctk_portals_rail_info_t *portals_info, int ind, sctk_EventQ_t *EvQ, unsigned res )
{
    int i = 0, r = 0, k;
    sctk_EventL_t *currList = &EvQ->ListMsg;

    for ( i = 0; i < EvQ->SizeMsgList; i++ )
    {
        if ( currList->nb_elems < SIZE_QUEUE_EVENTS )
        {
            for ( k = 0; k < SIZE_QUEUE_EVENTS; k++ )
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

                    CHECK_RETURNVAL ( APPEND ( portals_info->ni_handle_phys, portals_info->pt_index[ind], &currList->events[k].msg.me, PTL_PRIORITY_LIST, NULL, &currList->events[k].msg.me_handle ) );
                    r++;

                    if ( r == res )
                        return;
                }
            }
        }

        currList = currList->next;
    }

}

static inline void evalReserve ( sctk_portals_rail_info_t *portals_info, int ind, sctk_EventQ_t *EvQ )
{

    int i;
    unsigned reservations = 0;
    unsigned free_space	  = 0;
    sctk_EventL_t *currList = &EvQ->ListMsg;

    for ( i = 0; i < EvQ->SizeMsgList; i++ )
    {
        reservations += currList->nb_elems_headers;
        free_space += SIZE_QUEUE_EVENTS - currList->nb_elems;
        currList = currList->next;
    }

    if ( reservations < SIZE_QUEUE_HEADERS_MIN )
    {
        if ( free_space < SIZE_QUEUE_HEADERS - reservations )
        {
            ListAllocMsg ( EvQ );
        }

        reserve ( portals_info, ind, EvQ, SIZE_QUEUE_HEADERS - reservations );
    }
}

/* append a new sending message in the list */
void ListAppendMsg ( sctk_portals_rail_info_t *portals_info, sctk_thread_ptp_message_t *msg, sctk_portals_route_info_t *data, int type, sctk_portals_message_t **msgPortals )
{

    int found = 0;
    uint64_t match;
    uint64_t ignore;
    int my_idThread;
    int peer_idThread;


    my_idThread = sctk_get_local_id ( SCTK_MSG_SRC_PROCESS ( msg ) ); //id of source
    peer_idThread = sctk_get_local_id ( SCTK_MSG_DEST_PROCESS ( msg ) ); //id of destination

    set_Match_Ignore_Bits ( &match, &ignore, peer_idThread, SCTK_MSG_TAG ( msg ) ); //set the match and ignore
    ptl_handle_ni_t *ni_h 	= & ( portals_info->ni_handle_phys ); //network interface

    sctk_spinlock_lock ( &portals_info->lock[my_idThread] ); //to be thread safe

    sctk_EventQ_t *EvQ 		= &portals_info->EvQ[my_idThread];//get the event queue of the good index

    unsigned currPos = 0;
    sctk_EventL_t *currList = &EvQ->ListMsg;
    evalReserve ( portals_info, my_idThread, EvQ );

    while ( !found )
    {

        if ( currList->nb_elems < SIZE_QUEUE_EVENTS )
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
                        sctk_nodebug ( "dealing with pack" );

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

                CHECK_RETURNVAL ( APPEND ( *ni_h, currList->events[pos].msg.my_idThread, & ( currList->events[pos].msg.me ), PTL_PRIORITY_LIST, NULL,
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
            if ( currPos + 1 >= EvQ->SizeMsgList )
                ListAllocMsg ( EvQ );

            currPos++;
            currList = currList->next;

        }
    }
    sctk_spinlock_unlock ( &portals_info->lock[my_idThread] ); //to be thread safe
}
void ListAppendMsgReq ( sctk_rail_info_t *rail, ptl_event_t *event, int peer_idThread )
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
    set_Match_Ignore_Bits ( &match, &ignore, my_idThread, tag );
    ptl_handle_ni_t *ni_h 	= & ( portals_info->ni_handle_phys );

    sctk_EventQ_t *EvQ 		= &portals_info->EvQ[peer_idThread];//we get the good index

    sctk_EventL_t *currList = &EvQ->ListMsg;
    evalReserve ( portals_info, peer_idThread, EvQ );

    while ( !found )
    {

        if ( currList->nb_elems < SIZE_QUEUE_EVENTS && currList->nb_elems_headers > 0 )
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
void ListFree ( sctk_portals_rail_info_t *portals_info, sctk_EventQ_t *EvQ, int list, int pos, int tab )
{
    int i;

    if ( tab == MSG_ARRAY )
    {
        sctk_EventL_t *currList = &EvQ->ListMsg;

        for ( i = 0; i < list; i++ )
            currList = currList->next;

        if ( currList->events[pos].used == IN_USE )
        {

            sctk_nodebug ( "free (type %d pos %d) size (list %d %d) at %p and msg (%p,%p)", currList->events[pos].msg.type, pos, i, currList->nb_elems, &currList->events[pos], currList->events[pos].ptrmsg.msg_send, currList->events[pos].ptrmsg.msg_recv );


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
                sctk_nodebug ( "entering scfm" );
                sctk_complete_and_free_message ( currList->events[pos].ptrmsg.msg_send );
                sctk_nodebug ( "leaving scfm" );
                currList->events[pos].ptrmsg.msg_send = NULL;
            }
            else
            {
                currList->nb_elems_headers++;
                /* the read is done so we can free memory */

                sctk_nodebug ( "entering smcf %p %p", SCTK_MSG_COMPLETION_FLAG ( currList->events[pos].ptrmsg.msg_send )  , SCTK_MSG_COMPLETION_FLAG ( currList->events[pos].ptrmsg.msg_recv ) );

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
                sctk_nodebug ( "leaving smcf" );

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

                CHECK_RETURNVAL ( APPEND ( portals_info->ni_handle_phys, portals_info->pt_index[currList->events[pos].pt_index], &currList->events[pos].msg.me, PTL_PRIORITY_LIST, NULL, &currList->events[pos].msg.me_handle ) );

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

    sctk_nodebug ( "free leaving" );

}

/*check if a message was received or sended*/
void clearList ( sctk_rail_info_t *rail, int id )
{

    sctk_portals_rail_info_t *portals_info = &rail->network.portals;

    if ( sctk_spinlock_trylock ( &portals_info->lock[id] ) == TRYLOCK_SUCCESS )
        //to be thread safe
    {


        ptl_handle_ni_t *ni_h = &portals_info->ni_handle_phys;
        sctk_EventQ_t *EvQ = &portals_info->EvQ[id];
        ptl_handle_eq_t *eq_h = &portals_info->eq_h[id];

        sctk_EventL_t *currList = &EvQ->ListMsg;
        ptl_ct_event_t test, test2;
        int i, j, k, size;


        for ( j = 0; j < EvQ->SizeMsgList; j++ )
            //check all the lists
        {
            size = currList->nb_elems;
            i = 0;

            for ( k = 0; k < size; k++, i++ )
                //and all the elements
            {
                while ( i < SIZE_QUEUE_EVENTS && currList->events[i].used != IN_USE )
                {
                    i++;
                }

                if ( i == SIZE_QUEUE_EVENTS ) //to avoid bugs
                    break;

                if ( currList->events[i].msg.type == WRITE )
                {


                    CHECK_RETURNVAL ( PtlCTGet ( currList->events[i].msg.me.ct_handle, &test ) ); //the message was getted

                    if ( test.success == 1 )
                    {

                        sctk_nodebug ( "free in index %d", id );
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

                            sctk_nodebug ( "free in index %d", id );
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
    ptl_handle_eq_t *eq_h = &portals_info->eq_h[id];

    ptl_event_t event;

    sctk_spinlock_lock ( &portals_info->lock[id] ); //to be thread safe

    while ( PtlEQGet ( *eq_h, &event ) == PTL_OK && event.type == PTL_EVENT_PUT )
        //we give all events
    {
        ListAppendMsgReq ( rail, &event, id );
    }

    sctk_spinlock_unlock ( &portals_info->lock[id] ); //to be thread safe

}

void portals_on_demand_connection_handler( sctk_rail_info_t *rail, int dest_process )
{
}
void sctk_network_init_portals_all ( sctk_rail_info_t *rail )
{
    int dest_rank;
    int src_rank;
    ptl_ni_limits_t desired;

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

    sctk_pmi_barrier();

    assume ( PtlInit() == PTL_OK );
    sctk_initLimits ( &desired );
    assume ( PtlNIInit ( PTL_IFACE_DEFAULT, PTL_NI_MATCHING | PTL_NI_PHYSICAL,
                PTL_PID_ANY,
                &desired, &rail->network.portals.actual, &rail->network.portals.ni_handle_phys ) == PTL_OK );

    assume ( PtlGetPhysId ( rail->network.portals.ni_handle_phys, &rail->network.portals.my_id ) ==  PTL_OK );

    /*
     *printf("NI actual limits\n  max_entries:            %d\n  max_unexpected_headers: %d\n  max_mds:                %d\n  max_cts:                %d\n  max_eqs:                %d\n  max_pt_index:           %d\n  max_iovecs:             %d\n  max_list_size:          %d\n  max_msg_size:           %d\n  max_atomic_size:        %d\n  max_waw_ordered_size:   %d\n  max_war_ordered_size:   %d\n  max_volatile_size:      %d\n",
     *  rail->network.portals.actual.max_entries,
     *  rail->network.portals.actual.max_unexpected_headers,
     *  rail->network.portals.actual.max_mds,
     *  rail->network.portals.actual.max_cts,
     *  rail->network.portals.actual.max_eqs,
     *  rail->network.portals.actual.max_pt_index,
     *  rail->network.portals.actual.max_iovecs,
     *  rail->network.portals.actual.max_list_size,
     *  (int)rail->network.portals.actual.max_msg_size,
     *  (int)rail->network.portals.actual.max_atomic_size,
     *  (int)rail->network.portals.actual.max_waw_ordered_size,
     *  (int)rail->network.portals.actual.max_war_ordered_size,
     *  (int)rail->network.portals.actual.max_volatile_size);
     */

    sctk_nodebug ( "ni_handle_phys %p my_id.nid %u my_id.pid %u", rail->network.portals.ni_handle_phys, rail->network.portals.my_id.phys.nid, rail->network.portals.my_id.phys.pid );
    
    int ntasks = sctk_get_max_local_number_tasks();//the same number of indexes for everybody
    sctk_nodebug ( "tasks max %d", ntasks );
    rail->network.portals.ntasks = ntasks;
    rail->network.portals.EvQ = sctk_malloc ( sizeof ( sctk_EventQ_t ) * ntasks );
    rail->network.portals.eq_h = sctk_malloc ( sizeof ( ptl_handle_eq_t ) * ntasks );
    rail->network.portals.lock = sctk_malloc ( sizeof ( sctk_spinlock_t ) * ntasks );
    //rail->network.portals.pre_matching = sctk_malloc(sizeof(ENTRY_T)*ntasks*(SIZE_QUEUE_HEADERS+2));
    //rail->network.portals.pre_matching_handle = sctk_malloc(sizeof(HANDLE_T)*ntasks*(SIZE_QUEUE_HEADERS+2));
    rail->network.portals.pt_index = sctk_malloc ( sizeof ( ptl_pt_index_t ) * ntasks );
    bzero ( &rail->network.portals.zeroCounter, sizeof ( ptl_ct_event_t ) );
    ENTRY_T init;
    HANDLE_T init_h;
    int idle = 1, j, k;
    init.start  = &idle;
    init.length = sizeof ( unsigned );
    init.uid    = PTL_UID_ANY;
    init.match_id.phys.nid = PTL_NID_ANY;
    init.match_id.phys.pid = PTL_PID_ANY;

    init.match_bits    = 0x0000000000000000;
    init.ignore_bits   = 0xFFFFFFFFFFFFFFFF;

    init.options = OPTIONS;
    CHECK_RETURNVAL ( PtlCTAlloc ( rail->network.portals.ni_handle_phys, &init.ct_handle ) );

    ENTRY_T init2;
    HANDLE_T init2_h;

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
        rail->network.portals.lock[j]									= SCTK_SPINLOCK_INITIALIZER;
        rail->network.portals.EvQ[j].SizeMsgList 						= 1;
        rail->network.portals.EvQ[j].ListMsg.nb_elems					= 0;
        rail->network.portals.EvQ[j].ListMsg.nb_elems_headers			= SIZE_QUEUE_HEADERS;

        CHECK_RETURNVAL ( PtlEQAlloc ( rail->network.portals.ni_handle_phys, SIZE_QUEUE_EVENTS, &rail->network.portals.eq_h[j] ) );

        CHECK_RETURNVAL ( PtlPTAlloc ( rail->network.portals.ni_handle_phys, 0, rail->network.portals.eq_h[j], j, &rail->network.portals.pt_index[j] ) );

        assert ( rail->network.portals.pt_index[j] == j );

        if ( j == 0 )
        {
            //for initialisation
            CHECK_RETURNVAL ( APPEND ( rail->network.portals.ni_handle_phys, 0, &init, PTL_PRIORITY_LIST, NULL, &init_h ) );

            if ( sctk_process_number > 2 )
                CHECK_RETURNVAL ( APPEND ( rail->network.portals.ni_handle_phys, 0, &init2, PTL_PRIORITY_LIST, NULL, &init2_h ) );
        }

        for ( k = 0; k < SIZE_QUEUE_HEADERS; k++ )
        {
            //we add SIZE_QUEUE_HEADERS entries on the table in order to receive put operations
            rail->network.portals.EvQ[j].ListMsg.events[k].ptrmsg.msg_send = sctk_malloc ( sizeof ( sctk_thread_ptp_message_t ) );
            bzero ( rail->network.portals.EvQ[j].ListMsg.events[k].ptrmsg.msg_send, sizeof ( sctk_thread_ptp_message_t ) );

            rail->network.portals.EvQ[j].ListMsg.events[k].msg.me.start  = rail->network.portals.EvQ[j].ListMsg.events[k].ptrmsg.msg_send;
            rail->network.portals.EvQ[j].ListMsg.events[k].msg.me.length = sizeof ( sctk_thread_ptp_message_body_t ); //
            rail->network.portals.EvQ[j].ListMsg.events[k].msg.me.uid    = PTL_UID_ANY;
            rail->network.portals.EvQ[j].ListMsg.events[k].used = RESERVED;
            rail->network.portals.EvQ[j].ListMsg.events[k].msg.allocs = 0;
            rail->network.portals.EvQ[j].ListMsg.nb_elems++;



            rail->network.portals.EvQ[j].ListMsg.events[k].msg.me.match_id.phys.nid = PTL_NID_ANY;
            rail->network.portals.EvQ[j].ListMsg.events[k].msg.me.match_id.phys.pid = PTL_PID_ANY;


            rail->network.portals.EvQ[j].ListMsg.events[k].msg.me.match_bits    = FLAG_REQ;
            rail->network.portals.EvQ[j].ListMsg.events[k].msg.me.ignore_bits   = REQ_IGN;

            rail->network.portals.EvQ[j].ListMsg.events[k].msg.me.options = OPTIONS_HEADER;
            CHECK_RETURNVAL ( PtlCTAlloc ( rail->network.portals.ni_handle_phys, &rail->network.portals.EvQ[j].ListMsg.events[k].msg.me.ct_handle ) );
            CHECK_RETURNVAL ( APPEND ( rail->network.portals.ni_handle_phys, rail->network.portals.pt_index[j], &rail->network.portals.EvQ[j].ListMsg.events[k].msg.me, PTL_PRIORITY_LIST, NULL, &rail->network.portals.EvQ[j].ListMsg.events[k].msg.me_handle ) );
            set_allocBits ( &rail->network.portals.EvQ[j].ListMsg.events[k].msg.allocs, ME_ALLOCATED );
        }
    }

    {
        int key_max = sctk_pmi_get_max_key_len();
        int val_max = sctk_pmi_get_max_val_len();
        char key[key_max];
        char val[key_max];

        snprintf ( key, key_max, "PORTALS-%d-%06d", rail->rail_number , sctk_process_rank );
        encode ( &rail->network.portals.my_id, sizeof ( rail->network.portals.my_id ), val,
                val_max );
        sctk_nodebug ( "%d + %d = %d ?", sizeof ( rail->network.portals.my_id.phys.nid ), sizeof ( rail->network.portals.my_id.phys.pid ), sizeof ( rail->network.portals.my_id ) );

        sctk_nodebug ( "Send KEY %s\t%s", key, val );
        sctk_pmi_put_connection_info_str ( val, val_max, key );
    }

    sctk_pmi_barrier();

    if( rail->requires_bootstrap_ring == 0 )
    {
        return;
    }

    //TODO: Need to build ring (more properly)

    dest_rank = ( sctk_process_rank + 1 ) % sctk_process_number;
    src_rank = ( sctk_process_rank + sctk_process_number - 1 ) % sctk_process_number;

    {
        int key_max = sctk_pmi_get_max_key_len();
        int val_max = sctk_pmi_get_max_val_len();
        char key[key_max];
        char val[key_max];
        sctk_endpoint_t *tmp;

        snprintf ( key, key_max, "PORTALS-%d-%06d", rail->rail_number , dest_rank );
        sctk_pmi_get_connection_info_str ( val, val_max, key );
        sctk_nodebug ( "Got KEY %s\t%s", key, val ); //key is the process_rank ,val is the nid and pid encoded

        tmp = sctk_malloc ( sizeof ( sctk_endpoint_t ) );
        memset ( tmp, 0, sizeof ( sctk_endpoint_t ) );
        /*Store connection info*/
        decode ( val, & ( tmp->data.portals.id ),
                sizeof ( tmp->data.portals.id ) );
        sctk_nodebug ( "Got id %lu\t%lu", tmp->data.portals.id.phys.nid, tmp->data.portals.id.phys.pid );
        sctk_rail_add_static_route (  rail, dest_rank, tmp );

        ptl_md_t md;
        ptl_handle_md_t md_handle;

        int idle2 = 0;
        CHECK_RETURNVAL ( PtlCTAlloc ( rail->network.portals.ni_handle_phys, &md.ct_handle ) );

        md.start  		= &idle2;

        md.length 		= sizeof ( int );
        md.options  	= PTL_MD_EVENT_CT_REPLY;
        md.eq_handle 	= PTL_EQ_NONE;   		// i.e. don't queue receive events

        CHECK_RETURNVAL ( PtlMDBind ( rail->network.portals.ni_handle_phys, &md, &md_handle ) );



        ptl_ct_event_t ctc;

        CHECK_RETURNVAL ( PtlGet ( md_handle, 0, md.length, tmp->data.portals.id, 0, 0, 0, NULL ) ); //getting the init buf
        CHECK_RETURNVAL ( PtlCTWait ( md.ct_handle, 1, &ctc ) ); //we need to wait the message for routing

        sctk_nodebug ( "recv init passed" );
    }
    sctk_pmi_barrier();

    if ( sctk_process_number > 2 )
    {
        int key_max = sctk_pmi_get_max_key_len();
        int val_max = sctk_pmi_get_max_val_len();
        char key[key_max];
        char val[key_max];
        sctk_endpoint_t *tmp;

        snprintf ( key, key_max, "PORTALS-%d-%06d", rail->rail_number , src_rank );
        sctk_pmi_get_connection_info_str ( val, val_max, key );
        sctk_nodebug ( "Got KEY %s\t%s", key, val ); //key is the process_rank ,val is the nid and pid encoded

        tmp = sctk_malloc ( sizeof ( sctk_endpoint_t ) );
        memset ( tmp, 0, sizeof ( sctk_endpoint_t ) );
        /*Store connection info*/
        decode ( val, & ( tmp->data.portals.id ),
                sizeof ( tmp->data.portals.id ) );
        sctk_nodebug ( "Got id %lu\t%lu", tmp->data.portals.id.phys.nid, tmp->data.portals.id.phys.pid );
        sctk_rail_add_static_route (  rail, src_rank, tmp );

        ptl_md_t md;
        ptl_handle_md_t md_handle;

        int idle2 = 0;
        CHECK_RETURNVAL ( PtlCTAlloc ( rail->network.portals.ni_handle_phys, &md.ct_handle ) );

        md.start  		= &idle2;

        md.length 		= sizeof ( int );
        md.options  	= PTL_MD_EVENT_CT_REPLY;
        md.eq_handle 	= PTL_EQ_NONE;   		// i.e. don't queue receive events

        CHECK_RETURNVAL ( PtlMDBind ( rail->network.portals.ni_handle_phys, &md, &md_handle ) );


        ptl_ct_event_t ctc;

        CHECK_RETURNVAL ( PtlGet ( md_handle, 0, md.length, tmp->data.portals.id, 0, 0, 0, NULL ) ); //getting the init buf
        CHECK_RETURNVAL ( PtlCTWait ( md.ct_handle, 1, &ctc ) ); //we need to wait the message for routing

        sctk_nodebug ( "recv init passed" );
    }

    sctk_pmi_barrier();


}
#endif
