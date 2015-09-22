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

	//allowing one extra list -> spcial messages
    return nb + 1;

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
	ptable->head = sctk_malloc(sizeof(sctk_portals_table_entry_t*)*ptable->nb_entries);
	//create portals entries
	for(cpt = 0; cpt < ptable->nb_entries; cpt++){
		ptable->head[cpt] = (sctk_portals_table_entry_t*)sctk_malloc(sizeof(sctk_portals_table_entry_t));
		sctk_portals_helper_init_table_entry(ptable->head[cpt], interface, cpt);
	}
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
		*interface, // NI handler
		SCTK_PORTALS_EVENTS_QUEUE_SIZE,  // event queue size initialization 
		entry->event_list                //output : an event queue is created
	));
	
	//init its Portals reference
	sctk_portals_assume(PtlPTAlloc(
		*interface, // NI handler
		0,                               // possible Portals options
		*entry->event_list,                      // associated event queue
		entry->index,                           // desired index id
		&entry->index                           // output: effective index id
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
    /*sctk_debug("before = %lu - %lu", ((sctk_portals_process_id_t*)inval)->phys.nid, ((sctk_portals_process_id_t*)inval)->phys.pid);*/
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
	/*sctk_debug("after = %s", outval);*/
    return 0;
}

void sctk_portals_helper_bind_to(sctk_portals_interface_handler_t*interface, ptl_process_t remote){
    ptl_md_t md;
    ptl_handle_md_t md_handle;

    int idle = 0;
    sctk_portals_assume ( PtlCTAlloc (*interface, &md.ct_handle ) );

    md.start  		= &idle;
    md.length 		= sizeof ( int );
    md.options  	= PTL_MD_EVENT_CT_REPLY;
    md.eq_handle 	= PTL_EQ_NONE;   		// i.e. don't queue receive events

    sctk_portals_assume ( PtlMDBind (*interface, &md, &md_handle ) );

    ptl_ct_event_t ctc;

    sctk_portals_assume ( PtlGet ( md_handle, 0, md.length, remote, 0, 0, 0, NULL ) ); //getting the init buf
    sctk_portals_assume ( PtlCTWait ( md.ct_handle, 1, &ctc ) ); //we need to wait the message for routing
	assert(ctc.failure ==0);
}

void sctk_portals_helper_init_new_entry(ptl_me_t* me, sctk_portals_interface_handler_t *ni_handler, void* start, size_t size, ptl_match_bits_t match, unsigned int option)
{
	//Data will be stored in this pointer when message will arrive
	me->start = start;
	me->length = (ptl_size_t)size;
	me->uid = PTL_UID_ANY;              // no user restriction
	me->match_id.phys.nid = PTL_NID_ANY; //no process restriction
	me->match_id.phys.pid = PTL_PID_ANY; //no process restriction
	me->match_bits = match;
	me->ignore_bits = SCTK_PORTALS_BITS_INIT;
	me->options = option;
	
	sctk_portals_assume(PtlCTAlloc(*ni_handler, &me->ct_handle));	
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

void sctk_portals_helper_get_request(void* start, size_t size, ptl_handle_ni_t* handler, ptl_process_t remote, ptl_pt_index_t index, ptl_match_bits_t match, void* ptr){
	
	ptl_md_t data;
	ptl_handle_md_t data_handler;
	ptl_ct_event_t event;
	
	sctk_portals_helper_init_memory_descriptor(&data, handler, start, size, SCTK_PORTALS_MD_GET_OPTIONS);

	//attach MD, load data
	sctk_portals_assume(PtlMDBind(*handler, &data, &data_handler));
	sctk_debug("PORTALS: Get (%lu - %lu - %lu)", remote.phys.pid, index, match);
	sctk_portals_assume(PtlGet(data_handler, 0, size, remote, index, match, 0, ptr));

	//wait until ptlGet finish
	sctk_portals_assume(PtlCTWait(data.ct_handle, 1, &event));
	assume(event.failure == 0);
	
	//release MD entry
	sctk_portals_assume(PtlMDRelease(data_handler));
}

void sctk_portals_helper_put_request(void* start, size_t size, ptl_handle_ni_t* handler, ptl_process_t remote, ptl_pt_index_t index, ptl_match_bits_t match, void* ptr, ptl_hdr_data_t extra)
{
	ptl_md_t data;
	ptl_handle_md_t data_handler;
	ptl_ct_event_t event;
	
	sctk_portals_helper_init_memory_descriptor(&data, handler, start, size, SCTK_PORTALS_MD_PUT_OPTIONS);

	//attach MD, load data
	sctk_portals_assume(PtlMDBind(*handler, &data, &data_handler));
	sctk_portals_assume(PtlPut(
				data_handler,
				0,
				size,
				PTL_NO_ACK_REQ,
				remote,
				index,
				match,
				0,
				ptr,
				extra
				));
	sctk_portals_assume(PtlCTWait(data.ct_handle, 1, &event));
	assume(event.failure == 0);
	sctk_portals_assume(PtlMDRelease(data_handler));
}

inline void sctk_portals_helper_register_new_entry(ptl_handle_ni_t* handler, ptl_pt_index_t index, ptl_me_t* slot, void* ptr)
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
}

