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
/* #                                                                      # */
/* ######################################################################## */
#include "sctk_rail.h"
#include "sctk_net_topology.h"

#include "sctk_low_level_comm.h"

#include <sctk_route.h>
#include <sctk_pmi.h>
#include <sctk_accessor.h>

#include <stdio.h>
#include <unistd.h>

#include "sctk_multirail.h"

/************************************************************************/
/* Rails Storage                                                        */
/************************************************************************/

static struct sctk_rail_array __rails = { NULL, 0, 0, 0, -1 };

/************************************************************************/
/* Rail Init and Getters                                                */
/************************************************************************/

void sctk_rail_allocate ( int count )
{
	__rails.rails = sctk_calloc ( count , sizeof ( sctk_rail_info_t ) );
	assume ( __rails.rails != NULL );
	__rails.rail_number = count;
}



sctk_rail_info_t * sctk_rail_register_with_parent( struct sctk_runtime_config_struct_net_rail *runtime_config_rail,
                                         struct sctk_runtime_config_struct_net_driver_config *runtime_config_driver_config,
                                         sctk_rail_info_t * parent,
                                         int subrail_id )
{
	if ( __rails.rail_current_id ==  sctk_rail_count() )
	{
		sctk_fatal ( "Error : Rail overflow detected\n" );
	}

	/* Store rail */

	sctk_rail_info_t * new_rail = &__rails.rails[__rails.rail_current_id];
	/* Set rail Number */
	new_rail->rail_number = __rails.rail_current_id;
	__rails.rail_current_id++;


	/* Load Config */
	new_rail->runtime_config_rail = runtime_config_rail;
	new_rail->runtime_config_driver_config = runtime_config_driver_config;


	new_rail->subrail_id = subrail_id;

	/* Set parent if present */
	new_rail->parent_rail = parent;

	/* Init Empty route table */
	new_rail->route_table = sctk_route_table_new();

	/* Load and save Rail Device (NULL if not found) */
	new_rail->rail_device = sctk_device_get_from_handle( runtime_config_rail->device );

    if( ! new_rail->rail_device )
	{
		if( strcmp( runtime_config_rail->device, "default") && runtime_config_rail->device[0] != '!' )
		{
			sctk_fatal("No such device %s", runtime_config_rail->device );
		}
	}


	/* Initialize Polling */
	int target_core = 0;

	/* If the device is known load polling topology */
	if( new_rail->rail_device )
	{
		target_core = new_rail->rail_device->root_core;
	}
	else
	{
		/* Otherwise just consider that the work is done on "0"*/
		target_core = 0;
	}

	/* Load Polling Config */
	struct sctk_runtime_config_struct_topological_polling * idle = &runtime_config_rail->idle_polling;
	struct sctk_runtime_config_struct_topological_polling * any_source = &runtime_config_rail->any_source_polling;

	/* Set idle Polling */
	sctk_topological_polling_tree_init( &new_rail->idle_polling_tree,
	                                    sctk_rail_convert_polling_set_from_config(idle->trigger),
	                                    sctk_rail_convert_polling_set_from_config(idle->range),
	                                    target_core );

	/* Set any source Polling */
	sctk_topological_polling_tree_init( &new_rail->any_source_polling_tree,
				      sctk_rail_convert_polling_set_from_config(any_source->trigger),
	                                    sctk_rail_convert_polling_set_from_config(any_source->range),
	                                    target_core );

	/* Checkout is RDMA */
	int is_rdma = runtime_config_rail->rdma;
	new_rail->is_rdma = is_rdma;

	/* Retrieve priority */
	new_rail->priority = runtime_config_rail->priority;

	/* Is the rail topological ? meaning does it have subrails ?*/
	if( runtime_config_rail->subrails_size )
	{
		/* Intialize subrail array */
		new_rail->subrail_count = runtime_config_rail->subrails_size;
		new_rail->subrails = sctk_malloc( new_rail->subrail_count * sizeof( struct sctk_rail_info_s *) );

		assume( new_rail->subrails != NULL );

		/* And now register subrails */
		int i;
		for( i = 0 ; i < runtime_config_rail->subrails_size ; i++ )
		{
			struct sctk_runtime_config_struct_net_rail *subrail_rail_conf = &runtime_config_rail->subrails[i];
			/* Here we have to query in order to handle hierachies with different drivers */
			struct sctk_runtime_config_struct_net_driver_config  * subrail_driver_conf = sctk_get_driver_config_by_name( subrail_rail_conf->config );

			if( !subrail_driver_conf )
			{
				sctk_fatal("Could not locate driver config for subrail %s", subrail_rail_conf->config );
			}

			/* Now do the init */
			sctk_rail_info_t * child_rail = sctk_rail_register_with_parent( subrail_rail_conf, subrail_driver_conf, new_rail, i );

			/* Register the subrail in current rail */
			new_rail->subrails[i] = child_rail;
		}

		/* Initialize the parent RAIL now that subrails are started */
		sctk_rail_init_driver( new_rail, SCTK_RTCFG_net_driver_topological );

	}
	else
	{
		sctk_rail_init_driver( new_rail, runtime_config_driver_config->driver.type );
	}


	return new_rail;
}


sctk_rail_info_t * sctk_rail_register( struct sctk_runtime_config_struct_net_rail *runtime_config_rail,
                                         struct sctk_runtime_config_struct_net_driver_config *runtime_config_driver_config )
{
	return sctk_rail_register_with_parent( runtime_config_rail, runtime_config_driver_config, NULL, -1 );
}

void sctk_rail_unregister(sctk_rail_info_t* rail)
{
	rail->enabled = 0;

	/* first, close routes properly */
	/*sctk_multirail_on_demand_disconnection_rail(rail);*/

	/* driver-specific call */
	if(rail->route_finalize)
		rail->route_finalize(rail);

	/* rail-generic call */
	sctk_rail_finalize_route(rail);
}

int sctk_rail_count()
{
	return __rails.rail_number;
}


sctk_rail_info_t * sctk_rail_get_by_id ( int i )
{
	return & ( __rails.rails[i] );
}

int sctk_rail_get_rdma_id()
{
	return __rails.rdma_rail;
}


sctk_rail_info_t * sctk_rail_get_rdma ()
{
	int rdma_id = sctk_rail_get_rdma_id();

	if( rdma_id < 0 )
		return NULL;

	return sctk_rail_get_by_id ( rdma_id );
}


void rdma_rail_ellection()
{
	int * rails_to_skip = sctk_calloc( sctk_rail_count(), sizeof( int ) );
	assume( rails_to_skip );

	/* First detect all subrails which are part of
	 * a TOPOLOGICAL rail with the RDMA flag */
	int i;
	for ( i = 0; i <  sctk_rail_count(); i++ )
	{
		sctk_rail_info_t *  rail = sctk_rail_get_by_id ( i );

		/* Do not process rail if already flagged */
		if( rails_to_skip[rail->rail_number] )
			continue;

		if( !rail->is_rdma )
			continue;

		if( rail->subrail_count )
		{
			/* If we are here this rail is topological
			 * and RDMA enabled, we have to flag all
			 * the subrails as skipped to guarantee
			 * the "unicity" of the RDMA rail */
			int j;

			for( j = 0 ; j < rail->subrail_count; j++ )
			{
				rails_to_skip[rail->subrails[j]->rail_number] = 1;
			}
		}
	}

	/* Now that we flagged to skip topological RDMA subrails
	 * we check that only a single TOPO rail has been flagged */
	int rdma_rail_id = -1;

	for ( i = 0; i <  sctk_rail_count(); i++ )
	{
		sctk_rail_info_t *  rail = sctk_rail_get_by_id ( i );

		/* Do not process rail if flagged */
		if( rails_to_skip[rail->rail_number] )
			continue;

		if( !rail->is_rdma )
			continue;

		/* If we are here the RAIL is RDMA and not skipped */
		if( rdma_rail_id < 0 )
		{
			rdma_rail_id = rail->rail_number;
		}
		else
		{
			/* In this case we have two rails with the
			 * RDMA condition UP which is not allowed */
			sctk_rail_info_t * previous = sctk_rail_get_by_id ( rdma_rail_id );

			sctk_fatal("Found two rails with the RDMA flag up (%s and %s)\n"
			           "this is not allowed, please make sure that only one is set");
		}

	}

	if( 0 <= rdma_rail_id )
	{
		/* We found a RDMA rail set by the flag
		 * save it as the RDMA one */
		__rails.rdma_rail = rdma_rail_id;
		/* Done !*/
	}
	else
	{
		__rails.rdma_rail = -1;

		if( !sctk_get_process_rank() )
		{
				sctk_warning("No RDMA capable rail found (using emulated calls)");
		}
	}

}



/* Finalize Rails (call the rail route init func ) */
void sctk_rail_commit()
{
	/* First proceed with the RDMA rail ellection */
	rdma_rail_ellection();

	/* Display network context */
	char *net_name;
	int i;
	char *name_ptr;

	net_name = sctk_malloc ( sctk_rail_count() * 1024 );
	name_ptr = net_name;

	for ( i = 0; i <  sctk_rail_count(); i++ )
	{
		sctk_rail_info_t *  rail = sctk_rail_get_by_id ( i );
		rail->route_init( rail );
		sprintf ( name_ptr, "\n%sRail(%d) [%s (%s) (%s)] %s", (rail->parent_rail)?"\tSub-":"", rail->rail_number, rail->network_name, rail->topology_name , rail->runtime_config_rail->device, rail->is_rdma?"RDMA":"" );
		name_ptr = net_name + strlen ( net_name );
		rail->enabled = 1;
		sctk_pmi_barrier();
	}

	sctk_network_mode = net_name;
}


struct sctk_rail_dump_context
{
	FILE * file;
	int is_static;
};



void __sctk_rail_dump_routes(  sctk_endpoint_t *table, void * arg  )
{
	int src = sctk_get_process_rank();
	int dest = table->dest;

	struct sctk_rail_dump_context * ctx = (struct sctk_rail_dump_context *) arg;


	if( ctx->is_static )
	{
		fprintf(ctx->file, "%d -> %d [color=\"red\"]\n", src, dest );
	}
	else
	{
		fprintf(ctx->file, "%d -> %d\n", src, dest );
	}
}


void __sctk_rail_dump_routes_append_file_to_fd( FILE * fd, char * path_of_file_to_append )
{
	FILE * to_append = fopen( path_of_file_to_append, "r" );

	if( !to_append )
	{
		perror("fopen");
		return;
	}

	char c= ' ';
	int ret;

	do
	{
		ret = fread ( &c, sizeof( char ), 1, to_append );
		fprintf( fd, "%c", c );

	}while( ret == 1 );


	fclose(  to_append );
}



/* Finalize Rails (call the rail route init func ) */
void sctk_rail_dump_routes()
{
	int i;

	int rank = sctk_get_process_rank();
	int size = sctk_get_process_number();
	int local_task_rank = sctk_get_local_task_rank();

	if( local_task_rank )
		return;

	sctk_error(" %d / %d ", rank , size  );
	char path[512];

	/* Each Proces fill its local data */
	for ( i = 0; i <  sctk_rail_count(); i++ )
	{
		sctk_rail_info_t *  rail = sctk_rail_get_by_id ( i );

		if( rail->parent_rail )
			continue;


		snprintf( path, 512, "./%d.%d.%s.dot.tmp", rank, i, rail->network_name );

		FILE * f = fopen( path, "w" );

		if( !f )
		{
			perror( "fopen" );
			return;
		}

		struct sctk_rail_dump_context ctx;

		ctx.file = f;

		/* Walk static */
		ctx.is_static = 1;
		sctk_walk_all_static_routes ( rail->route_table, __sctk_rail_dump_routes, (void *)&ctx );

		/* Walk dynamic */
		ctx.is_static = 0;
		sctk_walk_all_dynamic_routes ( rail->route_table, __sctk_rail_dump_routes, (void *)&ctx );

		fclose( f );
	}

	sctk_pmi_barrier();

	/* Now process 0 unifies in a single .dot file */
	if( !rank )
	{
		for ( i = 0; i <  sctk_rail_count(); i++ )
		{
			sctk_rail_info_t *  rail = sctk_rail_get_by_id ( i );

			if( rail->parent_rail )
				continue;

			printf("Merging %s\n", rail->network_name );

			snprintf( path, 512, "./%d.%s.dot", i, rail->network_name );

			FILE * global_f = fopen( path, "w" );

			if( !global_f )
			{
				perror( "fopen" );
				return;
			}

			fprintf(global_f, "Digraph G\n{\n");

			int j;

			for( j = 0 ; j < size ; j++ )
			{

				char tmp_path[512];
				snprintf( tmp_path, 512, "./%d.%d.%s.dot.tmp", j, i,  rail->network_name );

				printf("\t process %d (%s)\n", j , tmp_path);

				__sctk_rail_dump_routes_append_file_to_fd( global_f, tmp_path );
				unlink( tmp_path );
			}

			fprintf(global_f, "\n}\n");

			fclose( global_f );
		}
	}
}

/************************************************************************/
/* Rail Pin CTX                                                         */
/************************************************************************/

void sctk_rail_pin_ctx_init( sctk_rail_pin_ctx_t * ctx, void * addr, size_t size )
{
	sctk_rail_info_t * rdma_rail = sctk_rail_get_rdma ();

	if( !rdma_rail )
	{
		/* Emulated mode no need to pin */
		return;
	}

	/* Clear the pin ctx */
	int i;

	for( i = 0 ; i < SCTK_PIN_LIST_SIZE; i++ )
	{
		memset( &ctx->list[i], 0, sizeof( struct sctk_rail_pin_ctx_list ) );
		ctx->list[i].rail_id = -1;
	}

	if( 0 < rdma_rail->subrail_count )
	{
		/* Here we have a topological rail we in for several rails */
		ctx->size = rdma_rail->subrail_count;
		/* Make sure we have enough room as we store these data
		 * in static to allow easy serialization between windows */
		assume( ctx->size < SCTK_PIN_LIST_SIZE );
	}
	else
	{
		/* By default we only pin for a single entry */
		ctx->size = 1;
	}

	rdma_rail->rail_pin_region( rdma_rail, ctx->list, addr, size );

	assume( ctx->list != NULL );
}

void sctk_rail_pin_ctx_release( sctk_rail_pin_ctx_t * ctx )
{
	sctk_rail_info_t * rdma_rail = sctk_rail_get_rdma ();

	if( !rdma_rail )
	{
		/* Emulated mode no need to pin */
		return;
	}

	rdma_rail->rail_unpin_region( rdma_rail, ctx->list );
}



/************************************************************************/
/* route_init                                                           */
/************************************************************************/

void sctk_rail_init_route ( sctk_rail_info_t *rail, char *topology, void (*on_demand)( struct sctk_rail_info_s * rail , int dest ) )
{
	/* First assume that the initial ring is required */
	rail->requires_bootstrap_ring = 1;


	/* Register On demand Handler if needed */
	if( rail->runtime_config_rail->ondemand )
	{
		rail->connect_on_demand = on_demand;
		rail->on_demand = 1;
	}
	else
	{
		rail->connect_on_demand = NULL;
		rail->on_demand = 0;
	}


	if ( strcmp ( "none", topology ) == 0 )
	{
		rail->route_init = sctk_route_none_init;
		rail->topology_name = "none";

		/* In this case we do not want any topology */
		rail->requires_bootstrap_ring = 0;
	}
	else
	{
		if ( strcmp ( "ring", topology ) == 0 )
		{
			rail->route_init = sctk_route_ring_init;
			rail->topology_name = "ring";
		}
		else
		{
			if ( strcmp ( "random", topology ) == 0 )
			{
				rail->route_init = sctk_route_random_init;
				rail->topology_name = "random";
			}
			else
			{
				if ( strcmp ( "fully", topology ) == 0 )
				{
					rail->route_init = sctk_route_fully_init;
					rail->topology_name = "fully connected";
				}
				else
				{
					if ( strcmp ( "torus", topology ) == 0 )
					{
						rail->route_init = sctk_route_torus_init;
						rail->topology_name = "torus";
					}
					else
					{
						sctk_fatal("No such topology %s", topology);
					}
				}
			}
		}
	}
}

void sctk_rail_finalize_route(sctk_rail_info_t* rail)
{
	rail->route_init = sctk_route_none_init;
	rail->topology_name = "none";
	rail->connect_on_demand = NULL;
	rail->disconnect_on_demand = NULL;
	rail->on_demand = 0;
	rail->requires_bootstrap_ring = 0;

	/*TODO: Should check route_table does not have remaining routes */
}

/**************************/

void sctk_rail_add_static_route (sctk_rail_info_t *rail, int dest, sctk_endpoint_t *tmp )
{
	/* Is this rail a subrail ? */
	if( rail->parent_rail )
	{
		/* Save SUBrail id inside the route */
		assume( 0 <= rail->subrail_id );
		tmp->subrail_id = rail->subrail_id;
		/* "Steal" the endpoint from the subrail */
		tmp->parent_rail = rail->parent_rail;
		/* Add in local rail without pushing in multirail */
		sctk_route_table_add_static_route ( rail->route_table, tmp, 0 );
		/* Add in parent rail without pushing in multirail */
		sctk_route_table_add_static_route ( rail->parent_rail->route_table, tmp, 0 );
		/* Push in multirail */
		sctk_multirail_destination_table_push_endpoint( tmp );
	}
	else
	{
		/* NO PARENT : Just add the route and register in multirail */
		sctk_route_table_add_static_route ( rail->route_table, tmp, 1 );
	}
}



void sctk_rail_add_dynamic_route ( sctk_rail_info_t *rail, int dest, sctk_endpoint_t *tmp )
{
	/* Is this rail a subrail ? */
	if( rail->parent_rail )
	{
		/* Save SUBrail id inside the route */
		assume( 0 <= rail->subrail_id );
		tmp->subrail_id = rail->subrail_id;
		/* "Steal" the endpoint from the subrail */
		tmp->parent_rail = rail->parent_rail;
		/* Add in local rail without pushing in multirail */
		sctk_route_table_add_dynamic_route (  rail->route_table, tmp, 0 );
		/* Add in parent rail without pushing in multirail */
		sctk_route_table_add_dynamic_route ( rail->parent_rail->route_table, tmp, 0 );
		/* Push in multirail */
		sctk_multirail_destination_table_push_endpoint( tmp );
	}
	else
	{
		/* NO PARENT : Just add the route and register in multirail */
		sctk_route_table_add_dynamic_route (  rail->route_table, tmp, 1 );
	}

}

void sctk_rail_add_dynamic_route_no_lock (  sctk_rail_info_t * rail, sctk_endpoint_t *tmp )
{
	/* Is this rail a subrail ? */
	if( rail->parent_rail )
	{
		/* Save SUBrail id inside the route */
		assume( 0 <= rail->subrail_id );
		tmp->subrail_id = rail->subrail_id;
		/* "Steal" the endpoint from the subrail */
		tmp->parent_rail = rail->parent_rail;
		/* Add in local rail without pushing in multirail */
		sctk_route_table_add_dynamic_route_no_lock (  rail->route_table, tmp, 0 );
		/* Push in multirail */
		sctk_multirail_destination_table_push_endpoint( tmp );
	}
	else
	{
		/* NO PARENT : Just add the route and register in multirail */
		sctk_route_table_add_dynamic_route_no_lock (  rail->route_table, tmp, 1 );
	}

}


/* Get a static route with no routing (can return NULL ) */
sctk_endpoint_t * sctk_rail_get_static_route_to_process ( sctk_rail_info_t *rail, int dest )
{
	return sctk_route_table_get_static_route(   rail->route_table , dest );
}

/* Get a route (static / dynamic) with no routing (can return NULL) */
sctk_endpoint_t * sctk_rail_get_any_route_to_process ( sctk_rail_info_t *rail, int dest )
{
	sctk_endpoint_t *tmp;

	/* First try static routes */
	tmp = sctk_route_table_get_static_route( rail->route_table , dest );

	/* It it fails look for dynamic routes on current rail */
	if ( tmp == NULL )
	{
		tmp = sctk_route_table_get_dynamic_route(   rail->route_table , dest );
	}

	return tmp;
}



/*
 * Return the route entry of the process 'dest'.
 * If the entry is not found, it is created using the 'create_func' function and
 * initialized using the 'init_funct' function.
 *
 * Return:
 *  - added: if the entry has been created
 *  - is_initiator: if the current process is the initiator of the entry creation.
 */
sctk_endpoint_t * sctk_rail_add_or_reuse_route_dynamic ( sctk_rail_info_t *rail, int dest, sctk_endpoint_t * ( *create_func ) (), void ( *init_func ) ( int dest, sctk_rail_info_t *rail, sctk_endpoint_t *route_table, int ondemand ), int *added, char is_initiator )
{

	*added = 0;

	sctk_spinlock_write_lock ( &rail->route_table->dynamic_route_table_lock );

	sctk_endpoint_t * tmp =  sctk_route_table_get_dynamic_route_no_lock( rail->route_table , dest );

	/* Entry not found, we create it */
	if ( tmp == NULL )
	{
		/* QP added on demand */
		tmp = create_func();
		init_func ( dest, rail, tmp, 1 );

		tmp->is_initiator = is_initiator;

		sctk_rail_add_dynamic_route_no_lock( rail, tmp );
		*added = 1;
	}
	else
	{
		if ( sctk_endpoint_get_state ( tmp ) == STATE_RECONNECTING )
		{
			ROUTE_LOCK ( tmp );
			sctk_endpoint_set_state ( tmp, STATE_DECONNECTED );
			init_func ( dest, rail, tmp, 1 );
			sctk_endpoint_set_low_memory_mode_local ( tmp, 0 );
			sctk_endpoint_set_low_memory_mode_remote ( tmp, 0 );
			tmp->is_initiator = is_initiator;
			ROUTE_UNLOCK ( tmp );
			*added = 1;
		}
	}

	sctk_spinlock_write_unlock ( &rail->route_table->dynamic_route_table_lock );
	return tmp;
}


sctk_endpoint_t * sctk_rail_get_dynamic_route_to_process ( sctk_rail_info_t *rail, int dest )
{
	return  sctk_route_table_get_dynamic_route( rail->route_table, dest );
}



/* Get the route to a given task (on demand mode) */
sctk_endpoint_t * sctk_rail_get_any_route_to_task_or_on_demand ( sctk_rail_info_t *rail, int dest )
{
	sctk_endpoint_t *tmp;
	int process;

	process = sctk_get_process_rank_from_task_rank ( dest );
	tmp = sctk_rail_get_any_route_to_process_or_on_demand ( rail, process );
	return tmp;
}


/* Get a route to a process with no ondemand connexions (relies on both static and dynamic routes without creating
 * routes => Relies on routing ) */
inline sctk_endpoint_t * sctk_rail_get_any_route_to_process_or_forward ( sctk_rail_info_t *rail, int dest )
{
	sctk_endpoint_t *tmp;
	/* Try to get a dynamic / static route until this process */
	tmp = sctk_rail_get_any_route_to_process ( rail, dest );

	if ( tmp == NULL )
	{
		/* Otherwise route until target process */
		int new_dest = rail->route ( dest, rail );

		if( new_dest == dest )
		{
			sctk_fatal("Routing loop identified");
		}

		/* Use the same function which does not create new routes */
		return sctk_rail_get_any_route_to_process_or_forward ( rail, new_dest );
	}

	return tmp;
}



/* Get a route to a process only on static routes (does not create new routes => Relies on routing) */
inline sctk_endpoint_t * sctk_rail_get_static_route_to_process_or_forward ( sctk_rail_info_t *rail , int dest )
{
	sctk_endpoint_t *tmp;
	tmp = sctk_rail_get_static_route_to_process ( rail, dest );

	if ( tmp == NULL )
	{
		int new_dest = rail->route ( dest, rail );

		if( new_dest == dest )
		{
			sctk_fatal("Routing loop identified");
		}

		return sctk_rail_get_static_route_to_process_or_forward ( rail, new_dest );
	}

	return tmp;
}





/* Get a route to process, creating the route if not present */
sctk_endpoint_t * sctk_rail_get_any_route_to_process_or_on_demand ( sctk_rail_info_t *rail, int dest )
{
	sctk_endpoint_t *tmp;

	/* Try to find a direct route with no routing */
	tmp = sctk_rail_get_any_route_to_process ( rail, dest );

	if ( tmp )
	{
		sctk_nodebug ( "Directly connected to %d", dest );
	}
	else
	{
		sctk_nodebug ( "NOT Directly connected to %d", dest );
	}

	if ( tmp == NULL )
	{
		/* Here we did not find a route therefore we instantiate it */

		/* On demand enabled and provided */
		if ( rail->on_demand && rail->connect_on_demand )
		{
			/* Network has an on-demand function */
			(rail->connect_on_demand)( rail, dest );
			/* Here the new endpoint has been pushed, just recall */
			return sctk_rail_get_any_route_to_process( rail, dest );
		}
		else
		{

			int new_dest = rail->route ( dest, rail );

			if( new_dest == dest )
			{
				sctk_fatal("Routing loop identified");
			}

			return sctk_rail_get_any_route_to_process_or_forward ( rail, new_dest );

		}

	}

	return tmp;
}
