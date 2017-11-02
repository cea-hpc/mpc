#ifdef MPC_USE_PORTALS

#include "sctk_route.h"
#include "sctk_io_helper.h" /* for MAX_STRING_SIZE */
#include "sctk_ptl_types.h"
#include "sctk_ptl_iface.h"

void sctk_ptl_add_route(int dest, sctk_ptl_id_t id, sctk_rail_info_t* rail, sctk_route_origin_t origin, sctk_endpoint_state_t state)
{
	not_implemented();
}

void sctk_ptl_create_ring ( sctk_rail_info_t *rail )
{
	int right_rank, left_rank, tmp_ret;
	sctk_ptl_id_t right_id, left_id;
	char right_rank_connection_infos[MAX_STRING_SIZE];
	char left_rank_connection_infos[MAX_STRING_SIZE];
	sctk_ptl_rail_info_t* srail = &rail->network.ptl;

	/** Portals initialization : Ring topology.
	 * 1. bind to right process through PMI
	 * 2. if nbProcs > 2 create a route to the process to the left
	 * => Bidirectional ring
	 */
	
	/* serialize the id_t to a string, compliant with PMI handling */
	srail->connection_infos_size = sctk_ptl_data_serialize(
			&srail->id,              /* the process it to serialize */
			sizeof (sctk_ptl_id_t),  /* size of the Portals ID struct */
			srail->connection_infos, /* the string to store the serialization */
			MAX_STRING_SIZE          /* max allowed string's size */
	);
	assert(srail->connection_infos_size > 0);

	/* register the serialized id into the PMI */
	tmp_ret = sctk_pmi_put_connection_info (
			srail->connection_infos,      /* the string to publish */
			srail->connection_infos_size, /* string size */
			rail->rail_number             /* rail ID: PMI tag */
	);
	assert(tmp_ret == 0);

	/* what are my neighbour ranks ? */
	right_rank = ( sctk_process_rank + 1 ) % sctk_process_number;
	left_rank = ( sctk_process_rank + sctk_process_number - 1 ) % sctk_process_number;

	/* wait for each process to register its own serialized ID */
	sctk_pmi_barrier();

	/* retrieve the right neighbour id struct */
	tmp_ret = sctk_pmi_get_connection_info (
			right_rank_connection_infos, /* the recv buffer */
			MAX_STRING_SIZE,             /* the recv buffer max size */
			rail->rail_number,           /* rail IB: PMI tag */
			right_rank                   /* which process we are targeting */
	);
	assert(tmp_ret == 0);
	
	/* de-serialize the string to retrive the real ptl_id_t */
	sctk_ptl_data_deserialize(
			right_rank_connection_infos, /* the buffer containing raw data */
			&right_id,                   /* the target struct */
			sizeof ( right_id )          /* target struct size */
	);

	//if we need an initialization to the left process (bidirectional ring)
	if(sctk_process_number > 2)
	{

		/* retrieve the left neighbour id struct */
		tmp_ret = sctk_pmi_get_connection_info (
				left_rank_connection_infos, /* the recv buffer */
				MAX_STRING_SIZE,             /* the recv buffer max size */
				rail->rail_number,           /* rail IB: PMI tag */
				left_rank                   /* which process we are targeting */
				);
		assert(tmp_ret == 0);

		/* de-serialize the string to retrive the real ptl_id_t */
		sctk_ptl_data_deserialize(
				left_rank_connection_infos, /* the buffer containing raw data */
				&left_id,                   /* the target struct */
				sizeof ( left_id )          /* target struct size */
				);
	}

	/* add routes */
	sctk_ptl_add_route (right_rank, right_id, rail, ROUTE_ORIGIN_STATIC, STATE_CONNECTED);
	if ( sctk_process_number > 2 )
	{
		sctk_ptl_add_route (left_rank, left_id, rail, ROUTE_ORIGIN_STATIC, STATE_CONNECTED);
	}

	//Wait for all processes to complete the ring topology init */
	sctk_pmi_barrier();
}

void sctk_ptl_init_interface(sctk_rail_info_t* rail)
{
	rail->network.ptl = sctk_ptl_hardware_init( rail );
	rail->network.ptl.pt_entries = sctk_ptl_software_init( &rail->network.ptl, 3 );

	assert(rail->network.ptl.pt_entries);
}

void sctk_ptl_fini_interface(sctk_rail_info_t* rail)
{
}

#endif
