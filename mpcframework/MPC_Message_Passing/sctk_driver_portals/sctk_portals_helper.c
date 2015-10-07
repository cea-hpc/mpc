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
/* #   - ADAM Julien adamj@paratools.fr                                   # */
/* #                                                                      # */
/* ######################################################################## */

#ifdef MPC_USE_PORTALS

#include <sctk_portals_helper.h>
#include <sctk_debug.h>
#include <sctk_accessor.h>
#include <sctk_atomics.h>

void sctk_portals_helper_print_event_type ( ptl_event_t *event)
{
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

inline int sctk_portals_helper_compute_nb_portals_entries()
{
    int tasks = sctk_get_task_number();
    int processes = sctk_get_process_number();
	int nb = tasks / processes;
    int rest = ( tasks % processes );

    if ( rest )
        nb++;

    return nb ;

}

inline void sctk_portals_helper_set_max ( long int *ptr, int size )
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

inline void sctk_portals_helper_init_boundaries ( sctk_portals_limits_t *desired )
{
    sctk_portals_helper_set_max ( ( long int * ) &desired->max_unexpected_headers, sizeof ( int ) );
    sctk_portals_helper_set_max ( ( long int * ) &desired->max_entries, sizeof ( int ) );
    sctk_portals_helper_set_max ( ( long int * ) &desired->max_mds, sizeof ( int ) );
    sctk_portals_helper_set_max ( ( long int * ) &desired->max_cts, sizeof ( int ) );
    sctk_portals_helper_set_max ( ( long int * ) &desired->max_eqs, sizeof ( int ) );
    sctk_portals_helper_set_max ( ( long int * ) &desired->max_pt_index, sizeof ( int ) );
    sctk_portals_helper_set_max ( ( long int * ) &desired->max_iovecs, sizeof ( int ) );
    sctk_portals_helper_set_max ( ( long int * ) &desired->max_list_size, sizeof ( int ) );
    sctk_portals_helper_set_max ( ( long int * ) &desired->max_triggered_ops, sizeof ( int ) );
    sctk_portals_helper_set_max ( ( long int * ) &desired->max_msg_size, sizeof ( int ) );
    sctk_portals_helper_set_max ( ( long int * ) &desired->max_atomic_size, sizeof ( int ) + 1 );
    sctk_portals_helper_set_max ( ( long int * ) &desired->max_waw_ordered_size, sizeof ( int ) );
    sctk_portals_helper_set_max ( ( long int * ) &desired->max_war_ordered_size, sizeof ( int ) );
    sctk_portals_helper_set_max ( ( long int * ) &desired->max_volatile_size, sizeof ( int ) + 1 );

    sctk_debug("NI actual limits\n"
	"max_entries:            %lu\n"
	"max_unexpected_headers: %lu\n"
	"max_mds:                %lu\n"
	"max_cts:                %lu\n"
	"max_eqs:                %lu\n"
	"max_pt_index:           %lu\n"
	"max_iovecs:             %lu\n"
	"max_list_size:          %lu\n"
	"max_msg_size:           %lu\n"
	"max_atomic_size:        %lu\n"
	"max_waw_ordered_size:   %lu\n"
	"max_war_ordered_size:   %lu\n"
	"max_volatile_size:      %lu\n",
      desired->max_entries,
      desired->max_unexpected_headers,
      desired->max_mds,
      desired->max_cts,
      desired->max_eqs,
      desired->max_pt_index,
      desired->max_iovecs,
      desired->max_list_size,
      (int)desired->max_msg_size,
      (int)desired->max_atomic_size,
      (int)desired->max_waw_ordered_size,
      (int)desired->max_war_ordered_size,
      (int)desired->max_volatile_size);
}

void sctk_portals_helper_lib_init(sctk_portals_interface_handler_t *interface, sctk_portals_process_id_t* id, sctk_portals_table_t *ptable){
	int cpt = 0; // universal counter
	//starting Portals Implementation library
	sctk_portals_assume(PtlInit());
	ptable->table_lock = SCTK_SPINLOCK_INITIALIZER;

	//filling ptl_limits with max possible values (covering lot of cases)
	sctk_portals_limits_t max_values;
	sctk_portals_helper_init_boundaries(&max_values);

	//Init Portals network interface
	sctk_portals_assume(PtlNIInit(
		PTL_IFACE_DEFAULT, //default network interface (probably 0
		PTL_NI_MATCHING | PTL_NI_PHYSICAL, // matching mode + physical NI usage
		PTL_PID_ANY, // process id range
		&max_values, //expected max values
		NULL, //effective portals boundaries
		interface
	));

	//assign an unique identifier for calling process
	sctk_portals_assume(PtlGetPhysId(
		*interface, //initialized network interface handler
		id //set id for calling process
	));

	/** portals table initialization */
	//as much entries than the number of tasks in current process
	ptable->nb_entries = sctk_portals_helper_compute_nb_portals_entries();
	ptable->head = sctk_malloc(sizeof(sctk_portals_table_entry_t*)*(ptable->nb_entries+2));

	//create portals entries
	for(cpt = 0; cpt < ptable->nb_entries+2; cpt++)
	{
		ptable->head[cpt] = (sctk_portals_table_entry_t*)sctk_malloc(sizeof(sctk_portals_table_entry_t));
		sctk_portals_helper_init_table_entry(ptable->head[cpt], interface, cpt);
	}

	//init MDs queue
	sctk_portals_pending_msg_list_t *mds = &ptable->pending_list;
	mds->head = NULL;
	mds->nb_msg  = 0;
	mds->msg_lock = SCTK_SPINLOCK_INITIALIZER;
}

void sctk_portals_helper_init_table_entry(sctk_portals_table_entry_t* entry, sctk_portals_interface_handler_t *interface, int ind){

	int i;
	assert(entry != NULL);
	assert(interface != NULL);
	entry->lock = SCTK_SPINLOCK_INITIALIZER;
	entry->event_list = sctk_malloc(sizeof(ptl_handle_eq_t));
	entry->index = ind; // set index to desired : index of init loop
	sctk_atomics_store_int(&entry->entry_cpt, 1);

	//init event queue for this entry
	sctk_portals_assume(PtlEQAlloc(
		*interface,                     // NI handler
		SCTK_PORTALS_EVENTS_QUEUE_SIZE, // event queue size initialization
		entry->event_list               //output : an event queue is created
	));

	//init its Portals reference
	sctk_portals_assume(PtlPTAlloc(
		*interface,         // NI handler
		0,                  // possible Portals options
		*entry->event_list, // associated event queue
		entry->index,       // desired index id
		&entry->index       // output: effective index id
	));

	for (i = 0; i < SCTK_PORTALS_HEADERS_ME_SIZE; ++i) {
	    ptl_me_t me;
	    ptl_handle_me_t me_handle;
	    sctk_thread_ptp_message_t* slot;

	    slot = sctk_malloc(sizeof( sctk_thread_ptp_message_t));

	    sctk_portals_helper_init_new_entry(&me, interface, slot, sizeof(sctk_thread_ptp_message_t), SCTK_PORTALS_BITS_HEADER, SCTK_PORTALS_ME_PUT_OPTIONS );
	    sctk_portals_helper_register_new_entry(interface, ind, &me, NULL);
	}
}

int sctk_portals_helper_from_str ( const char *inval, void *outval, int outvallen )
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
int sctk_portals_helper_to_str ( const void *inval, int invallen, char *outval, int outvallen )
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

void sctk_portals_helper_init_new_entry(ptl_me_t* me, sctk_portals_interface_handler_t *ni_handler, void* start, size_t size, ptl_match_bits_t match, unsigned int option)
{
	//Data will be stored in this pointer when message will arrive
	me->start = start;
	me->length = (ptl_size_t)size;
	me->uid = PTL_UID_ANY;               // no user restriction
	me->match_id.phys.nid = PTL_NID_ANY; // no process restriction
	me->match_id.phys.pid = PTL_PID_ANY; // no process restriction
	me->match_bits = match;
	me->ignore_bits = SCTK_PORTALS_BITS_INIT;
	me->options = option;

	sctk_portals_assume(PtlCTAlloc(*ni_handler, &me->ct_handle));
}

ptl_handle_me_t sctk_portals_helper_register_new_entry(ptl_handle_ni_t* handler, ptl_pt_index_t index, ptl_me_t* slot, void* ptr)
{
	ptl_handle_me_t slot_handler;

	sctk_portals_assume(PtlMEAppend(
		*handler,
		index,
		slot,
		PTL_PRIORITY_LIST,
		ptr,
		&slot_handler
		));

	ptl_process_t temp;
	sctk_portals_assume(PtlGetPhysId(*handler, &temp));
	sctk_debug("PORTALS: REGISTER MR - %lu at (%lu,%lu)", temp.phys.pid, index, slot->match_bits);

	return slot_handler;
}

void sctk_portals_helper_init_memory_descriptor(ptl_md_t* md, sctk_portals_interface_handler_t *ni_handler, void* start, size_t size, unsigned int option){
	md->start = start;
	md->length = size;
	md->options = option;

	sctk_portals_assume(PtlCTAlloc(*ni_handler, &md->ct_handle));
	sctk_portals_assume(PtlEQAlloc(*ni_handler, SCTK_PORTALS_EVENTS_QUEUE_SIZE, &md->eq_handle));
}

void sctk_portals_helper_set_bits_from_msg(ptl_match_bits_t* match, void*atomic)
{
	*match = sctk_atomics_fetch_and_incr_int(atomic);
}

void sctk_portals_helper_get_request(sctk_portals_pending_msg_list_t* list,
			void* start, size_t size, size_t remote_offset,
			ptl_handle_ni_t* handler, ptl_process_t remote,
			ptl_pt_index_t index, ptl_match_bits_t match, void* ptr,
			sctk_portals_request_type_t req_type){

	sctk_portals_pending_msg_t* msg = sctk_malloc(sizeof(sctk_portals_pending_msg_t));

	//attach to MD cat msg + msg
	msg->ack_type = SCTK_PORTALS_NO_ACK_MSG;
	msg->data = *((sctk_portals_list_entry_extra_t*)ptr);

	sctk_portals_helper_init_memory_descriptor(&msg->md, handler, start, size, SCTK_PORTALS_MD_GET_OPTIONS);

	//attach MD, load data
	sctk_portals_assume(PtlMDBind(*handler, &msg->md, &msg->md_handler));

	sctk_debug("PORTALS: GET REQUEST - %lu at (%lu,%lu)", remote.phys.pid, index, match);
	sctk_portals_assume(PtlGet(msg->md_handler, 0, size, remote, index, match, remote_offset, ptr));

	if(req_type == SCTK_PORTALS_BLOCKING_REQUEST){
		ptl_ct_event_t event;
		sctk_debug("PORTALS: GET REQUEST - %lu at (%lu,%lu) WAITING", remote.phys.pid, index, match);
		while(PtlCTGet(msg->md.ct_handle, &event) == PTL_OK){
			assume(event.failure==0);
			if(event.success > 0)
			    break;

		}
	}

	sctk_spinlock_lock(&list->msg_lock);
	sctk_debug("PORTALS: GET PUSH MD - %lu at (%lu/%lu)", remote.phys.pid, index, match);
	LL_APPEND(list->head, msg );
	sctk_spinlock_unlock(&list->msg_lock);
}

void sctk_portals_helper_put_request(sctk_portals_pending_msg_list_t* list,
				void* start, size_t size, size_t remote_offset,
				ptl_handle_ni_t* handler, ptl_process_t remote,
				ptl_pt_index_t index, ptl_match_bits_t match, void* ptr,
				ptl_hdr_data_t extra, sctk_portals_request_type_t req_type,
				sctk_portals_ack_msg_type_t ack_requested)
{
	sctk_portals_pending_msg_t* msg = sctk_malloc(sizeof(sctk_portals_pending_msg_t));

	msg->ack_type = ack_requested;
	msg->data = *((sctk_portals_list_entry_extra_t*)ptr);

	sctk_portals_helper_init_memory_descriptor(&msg->md, handler, start, size, SCTK_PORTALS_MD_PUT_OPTIONS);

	//attach MD, load data
	sctk_portals_assume(PtlMDBind(*handler, &msg->md, &msg->md_handler));
	sctk_debug("PORTALS: PUT REQUEST - %lu at (%lu,%lu)", remote.phys.pid, index, match);
	sctk_portals_assume(PtlPut(
				msg->md_handler,
				0,
				size,
				ack_requested,
				remote,
				index,
				match,
				remote_offset,
				ptr,
				extra
				));

	if(req_type == SCTK_PORTALS_BLOCKING_REQUEST){
		ptl_ct_event_t event;
		int cpt = (ack_requested == SCTK_PORTALS_ACK_MSG) ? 2 : 1;
		while(PtlCTGet(msg->md.ct_handle, &event) == PTL_OK){
		    sctk_nodebug("PORTALS: PUT REQUEST - %lu at (%lu,%lu) WAITING", remote.phys.pid, index, match);
		    assume(event.failure==0);
		    if(event.success > 0)
			break;
		}
		assume(event.failure == 0);
	}

	sctk_spinlock_lock(&list->msg_lock);
	sctk_debug("PORTALS: PUT PUSH MD - %lu at (%lu,%lu)", remote.phys.pid, index, match);
	LL_APPEND(list->head, msg);
	sctk_spinlock_unlock(&list->msg_lock);
}

void sctk_portals_helper_fetchAtomic_request(sctk_portals_pending_msg_list_t* list,
		    void* start_new, size_t new_offset, void* start_res, size_t res_offset,
		    size_t type_size, size_t remote_offset,
		    ptl_handle_ni_t* handler, ptl_process_t remote,
		    ptl_pt_index_t index, ptl_match_bits_t match,
		    void* ptr, ptl_hdr_data_t extra,
		    ptl_op_t operation, ptl_datatype_t datatype)
{
	sctk_portals_pending_msg_t* msg_put = sctk_malloc(sizeof(sctk_portals_pending_msg_t));
	sctk_portals_pending_msg_t* msg_get = sctk_malloc(sizeof(sctk_portals_pending_msg_t));

	msg_put->ack_type = SCTK_PORTALS_NO_ACK_MSG;
	msg_put->data = *((sctk_portals_list_entry_extra_t*)ptr);

	msg_get->ack_type = SCTK_PORTALS_NO_ACK_MSG;
	msg_get->data = *((sctk_portals_list_entry_extra_t*)ptr);

	sctk_portals_helper_init_memory_descriptor(&msg_put->md, handler, start_new, type_size, SCTK_PORTALS_MD_PUT_OPTIONS);
	sctk_portals_helper_init_memory_descriptor(&msg_get->md, handler, start_res, type_size, SCTK_PORTALS_MD_GET_OPTIONS);

	//attach MD, load data
	sctk_portals_assume(PtlMDBind(*handler, &msg_put->md, &msg_put->md_handler));
	sctk_portals_assume(PtlMDBind(*handler, &msg_get->md, &msg_get->md_handler));

	sctk_debug("PORTALS: FETCH-ATOMIC REQUEST - %lu at (%lu,%lu)", remote.phys.pid, index, match);
	sctk_portals_assume(PtlFetchAtomic(
				msg_get->md_handler,
				res_offset,
				msg_put->md_handler,
				new_offset,
				type_size,
				remote,
				index,
				match,
				remote_offset,
				ptr,
				extra,
				operation,
				datatype
				));

	sctk_spinlock_lock(&list->msg_lock);
	sctk_debug("PORTALS: FETCH-ATOMIC PUSH MDS - %lu at (%lu,%lu)", remote.phys.pid, index, match);
	LL_APPEND(list->head, msg_put);
	LL_APPEND(list->head, msg_get);
	sctk_spinlock_unlock(&list->msg_lock);
}

void sctk_portals_helper_swap_request(sctk_portals_pending_msg_list_t* list,
		    void* start_new, size_t new_offset, void* start_res, size_t res_offset,
		    size_t type_size, size_t remote_offset,
		    ptl_handle_ni_t* handler, ptl_process_t remote,
		    ptl_pt_index_t index, ptl_match_bits_t match,
		    void* ptr, ptl_hdr_data_t extra,
		    void* operand, ptl_op_t operation, ptl_datatype_t datatype)
{
	sctk_portals_pending_msg_t* msg_put = sctk_malloc(sizeof(sctk_portals_pending_msg_t));
	sctk_portals_pending_msg_t* msg_get = sctk_malloc(sizeof(sctk_portals_pending_msg_t));

	msg_put->ack_type = SCTK_PORTALS_NO_ACK_MSG;
	msg_put->data = *((sctk_portals_list_entry_extra_t*)ptr);

	msg_get->ack_type = SCTK_PORTALS_NO_ACK_MSG;
	msg_get->data = *((sctk_portals_list_entry_extra_t*)ptr);

	sctk_portals_helper_init_memory_descriptor(&msg_put->md, handler, start_new, type_size, SCTK_PORTALS_MD_PUT_OPTIONS);
	sctk_portals_helper_init_memory_descriptor(&msg_get->md, handler, start_res, type_size, SCTK_PORTALS_MD_GET_OPTIONS);

	//attach MD, load data
	sctk_portals_assume(PtlMDBind(*handler, &msg_put->md, &msg_put->md_handler));
	sctk_portals_assume(PtlMDBind(*handler, &msg_get->md, &msg_get->md_handler));

	sctk_debug("PORTALS: SWAP REQUEST - %lu at (%lu,%lu)", remote.phys.pid, index, match);
	sctk_portals_assume(PtlSwap(
				msg_get->md_handler,
				res_offset,
				msg_put->md_handler,
				new_offset,
				type_size,
				remote,
				index,
				match,
				remote_offset,
				ptr,
				extra,
				operand,
				operation,
				datatype
				));

	sctk_spinlock_lock(&list->msg_lock);
	sctk_debug("PORTALS: SWAP PUSH MDS - %lu at (%lu,%lu)", remote.phys.pid, index, match);
	LL_APPEND(list->head, msg_put);
	LL_APPEND(list->head, msg_get);
	sctk_spinlock_unlock(&list->msg_lock);
}
#endif // MPC_USE_PORTALS
