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

#include <mpc_launch_pmi.h>
#include <mpc_common_rank.h>
#include <mpc_common_flags.h>
#include <mpc_common_datastructure.h>
#include "communicator.h"

#include <stdio.h>
#include <unistd.h>

#include <sctk_alloc.h>

#include "multirail.h"

/************************************************************************/
/* Rails Storage                                                        */
/************************************************************************/

static struct sctk_rail_array __rails = { NULL, 0, 0, 0 };

/************************************************************************/
/* Rail Init and Getters                                                */
/************************************************************************/

void sctk_rail_allocate ( int count )
{
	__rails.rails = sctk_calloc ( count , sizeof ( sctk_rail_info_t ) );
	assume ( __rails.rails != NULL );
	__rails.rail_number = count;
}

int lcr_rail_init(lcr_rail_config_t *rail_config,
                  lcr_driver_config_t *driver_config,
                  sctk_rail_info_t **rail_p)
{
        int rc = MPC_LOWCOMM_SUCCESS;
	sctk_rail_info_t * rail;

        rail = sctk_malloc(sizeof(sctk_rail_info_t));
        if (rail == NULL) {
                mpc_common_debug_error("LCR: could not allocate rail");
                rc = MPC_LOWCOMM_ERROR;
                goto err;
        }
        memset(rail, 0, sizeof(sctk_rail_info_t));

	/* Load Config */
	rail->runtime_config_rail = rail_config;
	rail->runtime_config_driver_config = driver_config;

	/* Init Empty route table */
	rail->route_table = _mpc_lowcomm_endpoint_table_new();

	/* Load and save Rail Device (NULL if not found) */
	rail->rail_device = mpc_topology_device_get_from_handle(rail_config->device);

        if(!rail->rail_device ) {
                if(strcmp(rail_config->device, "default") && 
                   rail_config->device[0] != '!' ) {
                        mpc_common_debug_error("Device not found with hwloc %s", 
                                               rail_config->device );
                }
        }

	/* Initialize Polling */
	int target_core = 0;

	/* If the device is known load polling topology */
	if(rail->rail_device) {
		target_core = rail->rail_device->root_core;
	} else {
		/* Otherwise just consider that the work is done on "0"*/
		target_core = 0;
	}

	/* Load Polling Config */
	struct _mpc_lowcomm_config_struct_topological_polling * any_source = 
                &rail_config->any_source_polling;

	/* Set any source Polling */
        sctk_topological_polling_tree_init(&rail->any_source_polling_tree,
                                           any_source->trigger,
                                           any_source->range,
                                           target_core );

	/* Checkout is RDMA */
	int is_rdma = rail_config->rdma;
	rail->is_rdma = is_rdma;

	/* Retrieve priority */
	rail->priority = rail_config->priority;

        *rail_p = rail;

        return rc;
err:
        sctk_free(rail);

        return rc;
}

static inline sctk_rail_info_t * sctk_rail_register_with_parent( struct _mpc_lowcomm_config_struct_net_rail *runtime_config_rail,
                                         struct _mpc_lowcomm_config_struct_net_driver_config *runtime_config_driver_config,
                                         sctk_rail_info_t * parent,
                                         int subrail_id,
					 int reuse_id)
{
	/* Store rail */

	sctk_rail_info_t * new_rail = NULL;
	if(reuse_id < 0)
	{
		/* This avoid dynamic rail creation (rail array is statically sized) */
		if ( __rails.rail_current_id ==  sctk_rail_count() )
		{
			mpc_common_debug_fatal ( "Error : Rail overflow detected\n" );
		}

		/* Set rail Number */
		new_rail = &__rails.rails[__rails.rail_current_id];
		new_rail->rail_number = __rails.rail_current_id;
		__rails.rail_current_id++;
		assert(new_rail->state == SCTK_RAIL_ST_INIT);
	} 
	else
	{
		new_rail = &__rails.rails[reuse_id];
		assert(new_rail->state == SCTK_RAIL_ST_DISABLED);
		memset(new_rail, 0, sizeof(sctk_rail_info_t));
		new_rail->rail_number = reuse_id;
	}


	/* Load Config */
	new_rail->runtime_config_rail = runtime_config_rail;
	new_rail->runtime_config_driver_config = runtime_config_driver_config;


	new_rail->subrail_id = subrail_id;

	/* Set parent if present */
	new_rail->parent_rail = parent;

	/* Init Empty route table */
	assert(!new_rail->route_table);
	new_rail->route_table = _mpc_lowcomm_endpoint_table_new();

	/* Load and save Rail Device (NULL if not found) */
	new_rail->rail_device = mpc_topology_device_get_from_handle( runtime_config_rail->device );

    if( ! new_rail->rail_device )
	{
		if( strcmp( runtime_config_rail->device, "default") && runtime_config_rail->device[0] != '!' )
		{
			mpc_common_debug_fatal("No such device %s", runtime_config_rail->device );
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
	struct _mpc_lowcomm_config_struct_topological_polling * any_source = &runtime_config_rail->any_source_polling;

	/* Set any source Polling */
	sctk_topological_polling_tree_init( &new_rail->any_source_polling_tree,
				      					any_source->trigger,
	                                    any_source->range,
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
			struct _mpc_lowcomm_config_struct_net_rail *subrail_rail_conf = &runtime_config_rail->subrails[i];
			/* Here we have to query in order to handle hierachies with different drivers */
			struct _mpc_lowcomm_config_struct_net_driver_config  * subrail_driver_conf = sctk_get_driver_config_by_name( subrail_rail_conf->config );

			if( !subrail_driver_conf )
			{
				mpc_common_debug_fatal("Could not locate driver config for subrail %s", subrail_rail_conf->config );
			}

			/* Now do the init */
			sctk_rail_info_t * child_rail = sctk_rail_register_with_parent( subrail_rail_conf, subrail_driver_conf, new_rail, i , -1 );

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


sctk_rail_info_t * sctk_rail_register( struct _mpc_lowcomm_config_struct_net_rail *runtime_config_rail,
                                         struct _mpc_lowcomm_config_struct_net_driver_config *runtime_config_driver_config )
{
	return sctk_rail_register_with_parent( runtime_config_rail, runtime_config_driver_config, NULL, -1, -1 );
}

void sctk_rail_disable(sctk_rail_info_t* rail)
{

        /* we don't disable abstract rails */
        if(rail->subrail_count) return;

	 /*TODO: We will have a race condition some day here
	 */
	rail->state = SCTK_RAIL_ST_DISABLED;

	/* driver-specific call */
	if(rail->driver_finalize)
		rail->driver_finalize(rail);

	/* rail-generic call */
	rail->route_init = sctk_route_none_init;
	rail->topology_name = "none";
	rail->connect_on_demand = NULL;
	rail->on_demand = 0;
	rail->requires_bootstrap_ring = 0;

        if(rail->parent_rail)
	        _mpc_lowcomm_endpoint_table_clear(&rail->parent_rail->route_table);

	_mpc_lowcomm_endpoint_table_free(&rail->route_table);

	/* TODO: We have to remove routes from topological rails ! */
	_mpc_lowcomm_multirail_table_prune();
}

void sctk_rail_enable(sctk_rail_info_t *rail)
{
	assert(rail);
        if(rail->state != SCTK_RAIL_ST_DISABLED) return;
        assert(!rail->subrail_count); /* should not be a composed rail */
	sctk_rail_register_with_parent(rail->runtime_config_rail, rail->runtime_config_driver_config, rail->parent_rail, rail->subrail_id,  rail->rail_number);
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

			mpc_common_debug_fatal("Found two rails with the RDMA flag up (%s and %s)\n"
			           "this is not allowed, please make sure that only one is set",
				   rail->network_name, previous->network_name);
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

		/*if( !get_process_rank() )*/
		/*{*/
				/*mpc_common_debug_warning("No RDMA capable rail found (using emulated calls)");*/
		/*}*/
	}

}

#define SCTK_RAIL_TYPE_STR(u) rail_type_str[SCTK_RAIL_TYPE(u)]
const char * rail_type_str[] = {
	"None",
	"Infiniband",
	"Portals",
	"TCP/IP",
	"TCP/IP (RMDA)",
	"SHM",
	"Topological"
};

static inline size_t sctk_rail_print_infos( sctk_rail_info_t * rail, char * start, size_t sz, int depth)
{
	assert(depth >= 0);
	assert(rail);
	assert(start);

	int i;
	size_t cur_sz = 0;

	cur_sz += snprintf (start, sz, "\n");
	/* indent */
	for (i = 0; i < depth; ++i) {
		cur_sz += snprintf(start + cur_sz, sz - cur_sz, "|  ");
	}

	cur_sz += snprintf ( start + cur_sz, sz - cur_sz, "%s Rail %d: \"%s\" (Type: %s) (Init: %s) (Device: %s) (Priority: %d) %s%s", (rail->state == SCTK_RAIL_ST_ENABLED) ? "+":"-", rail->rail_number, rail->network_name, SCTK_RAIL_TYPE_STR(rail), rail->topology_name , rail->runtime_config_rail->device, rail->priority, rail->is_rdma?"+rdma":"", rail->on_demand?"+od":"");

	if(rail->subrails)
	{
		char * cursor = start + cur_sz;

		for (i = 0; i < rail->subrail_count; ++i)
		{
			cur_sz += sctk_rail_print_infos(rail->subrails[i], cursor, sz - cur_sz, depth + 1);
			assert(cur_sz > 0);
			assert(sz > cur_sz);
			cursor = start + cur_sz;
		}
	}

	return cur_sz;
}

size_t sctk_rail_print_topology( char * start, size_t sz)
{
	int i = 0, nb_rails = sctk_rail_count();
	size_t cur_sz = 0;
	char *cursor = start;
	sctk_rail_info_t *rail;

	for (i = 0; i < nb_rails; ++i) {
		rail = sctk_rail_get_by_id(i);
		if(rail->parent_rail) continue; /* printed by parent rail */

		cur_sz += sctk_rail_print_infos(rail, cursor, sz - cur_sz, 0);
		assert(cur_sz > 0);
		assert(sz > cur_sz);

		cursor = start + cur_sz;
	}
	return cur_sz;
}

/* Finalize Rails (call the rail route init func ) */
void sctk_rail_commit()
{
	/* First proceed with the RDMA rail ellection */
	rdma_rail_ellection();

	/* Display network context */
	char *net_name;
	int i;
	size_t sz;

	net_name = sctk_malloc ( sctk_rail_count() * 1024 );

	for ( i = 0; i <  sctk_rail_count(); i++ )
	{
		sctk_rail_info_t *  rail = sctk_rail_get_by_id ( i );

		if(rail->route_init)
		{
			rail->route_init( rail );
		}

		rail->state = SCTK_RAIL_ST_ENABLED;
	}

	sz = sctk_rail_print_topology(net_name, sctk_rail_count() * 1024);
	assert(sz <= (size_t)sctk_rail_count() * 1024);
	mpc_common_get_flags()->sctk_network_description_string = net_name;
}

struct sctk_rail_dump_context
{
	FILE * file;
	int is_static;
};



void __sctk_rail_dump_routes(  _mpc_lowcomm_endpoint_t *table, void * arg  )
{
	int src = mpc_common_get_process_rank();
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

	int rank = mpc_common_get_process_rank();
	int size = mpc_common_get_process_count();
	int local_task_rank = mpc_common_get_local_task_rank();

	if( local_task_rank )
		return;

	mpc_common_debug_error(" %d / %d ", rank , size  );
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
		_mpc_lowcomm_endpoint_table_walk_static ( rail->route_table, __sctk_rail_dump_routes, (void *)&ctx );

		/* Walk dynamic */
		ctx.is_static = 0;
		_mpc_lowcomm_endpoint_table_walk_dynamic ( rail->route_table, __sctk_rail_dump_routes, (void *)&ctx );

		fclose( f );
	}

 mpc_launch_pmi_barrier();

	/* Now process 0 unifies in a single .dot file */
	if( !rank )
	{
		for ( i = 0; i <  sctk_rail_count(); i++ )
		{
			sctk_rail_info_t *  rail = sctk_rail_get_by_id ( i );
			printf("Dumping '%s', priority: %d\n", rail->network_name, rail->priority);

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

			for( j = 0 ; j < size ; j++ ) {
				fprintf(global_f, "%d\n", j);
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
/* Handlers                                                             */
/************************************************************************/
int lcr_iface_set_am_handler(sctk_rail_info_t *rail, uint8_t id,
		lcr_am_callback_t cb, void *arg, uint64_t flags)
{
	int rc;
	if (id >= LCR_AM_ID_MAX) {
		mpc_common_debug_error("LCP: active message out of range: \
				       id=%d, id max=%d.", id, LCR_AM_ID_MAX);
		rc = MPC_LOWCOMM_ERROR;
		goto err;
	}
	
	rail->am[id].cb = cb;
	rail->am[id].arg = arg;
	rail->am[id].flags = flags;

	rc = MPC_LOWCOMM_SUCCESS;
err:	
	return rc;
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

	/* Make sure the rail did define pin */
	assume(rdma_rail->rail_pin_region != NULL);

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

	/* Make sure the rail did define unpin */
	assume(rdma_rail->rail_unpin_region != NULL);
	
	rdma_rail->rail_unpin_region( rdma_rail, ctx->list );
}



/************************************************************************/
/* route_init                                                           */
/************************************************************************/

void sctk_rail_init_route ( sctk_rail_info_t *rail, char *topology, void (*on_demand)( struct sctk_rail_info_s * rail , mpc_lowcomm_peer_uid_t dest ) )
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
					mpc_common_debug_fatal("No such topology %s", topology);
				}
			}
		}
	}
}

/**************************/

void sctk_rail_add_static_route (sctk_rail_info_t *rail, _mpc_lowcomm_endpoint_t *tmp )
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
		_mpc_lowcomm_endpoint_table_add_static_route ( rail->route_table, tmp );
		/* Add in parent rail without pushing in multirail */
		_mpc_lowcomm_endpoint_table_add_static_route ( rail->parent_rail->route_table, tmp);
#ifndef MPC_LOWCOMM_PROTOCOL
		/* Push in multirail */
		_mpc_lowcomm_multirail_table_push_endpoint( tmp );
#endif
	}
	else
	{
		/* NO PARENT : Just add the route and register in multirail */
		_mpc_lowcomm_endpoint_table_add_static_route ( rail->route_table, tmp);
#ifndef MPC_LOWCOMM_PROTOCOL
		_mpc_lowcomm_multirail_table_push_endpoint(tmp);
#endif
	}
}



void sctk_rail_add_dynamic_route ( sctk_rail_info_t *rail, _mpc_lowcomm_endpoint_t *tmp )
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
		_mpc_lowcomm_endpoint_table_add_dynamic_route (  rail->route_table, tmp);
		/* Add in parent rail without pushing in multirail */
		_mpc_lowcomm_endpoint_table_add_dynamic_route ( rail->parent_rail->route_table, tmp);
#ifndef MPC_LOWCOMM_PROTOCOL
                //NOTE: multirail replaced by lcp so tables not instanciated
		/* Push in multirail */
		_mpc_lowcomm_multirail_table_push_endpoint( tmp );
#endif
	}
	else
	{
		/* NO PARENT : Just add the route and register in multirail */
		_mpc_lowcomm_endpoint_table_add_dynamic_route (  rail->route_table, tmp);
#ifndef MPC_LOWCOMM_PROTOCOL
                //NOTE: multirail replaced by lcp so tables not instanciated
		_mpc_lowcomm_multirail_table_push_endpoint(tmp);
#endif
	}

}

void sctk_rail_add_dynamic_route_no_lock (  sctk_rail_info_t * rail, _mpc_lowcomm_endpoint_t *tmp )
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
		_mpc_lowcomm_endpoint_table_add_dynamic_route_no_lock (  rail->route_table, tmp);
		/* Add in parent rail without pushing in multirail */
		_mpc_lowcomm_endpoint_table_add_dynamic_route_no_lock ( rail->parent_rail->route_table, tmp);
		/* Push in multirail */
		_mpc_lowcomm_multirail_table_push_endpoint( tmp );
	}
	else
	{
		/* NO PARENT : Just add the route and register in multirail */
		_mpc_lowcomm_endpoint_table_add_dynamic_route_no_lock (  rail->route_table, tmp);
		_mpc_lowcomm_multirail_table_push_endpoint(tmp);
	}

}


/* Get a static route with no routing (can return NULL ) */
_mpc_lowcomm_endpoint_t * sctk_rail_get_static_route_to_process ( sctk_rail_info_t *rail, mpc_lowcomm_peer_uid_t dest )
{
	return _mpc_lowcomm_endpoint_table_get_static_route(   rail->route_table , dest );
}

/* Get a route (static / dynamic) with no routing (can return NULL) */
_mpc_lowcomm_endpoint_t * sctk_rail_get_any_route_to_process ( sctk_rail_info_t *rail, mpc_lowcomm_peer_uid_t dest )
{
	_mpc_lowcomm_endpoint_t *tmp;

	/* First try static routes */
	tmp = _mpc_lowcomm_endpoint_table_get_static_route( rail->route_table , dest );

	/* It it fails look for dynamic routes on current rail */
	if ( tmp == NULL )
	{
		tmp = _mpc_lowcomm_endpoint_table_get_dynamic_route(   rail->route_table , dest );
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
_mpc_lowcomm_endpoint_t * sctk_rail_add_or_reuse_route_dynamic ( sctk_rail_info_t *rail, mpc_lowcomm_peer_uid_t dest, _mpc_lowcomm_endpoint_t * ( *create_func ) (), void ( *init_func ) ( mpc_lowcomm_peer_uid_t dest, sctk_rail_info_t *rail, _mpc_lowcomm_endpoint_t *route_table, int ondemand ), int *added, char is_initiator )
{

	*added = 0;

	mpc_common_spinlock_write_lock ( &rail->route_table->dynamic_route_table_lock );

	_mpc_lowcomm_endpoint_t * tmp =  _mpc_lowcomm_endpoint_table_get_dynamic_route_no_lock( rail->route_table , dest );

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
		if ( _mpc_lowcomm_endpoint_get_state ( tmp ) == _MPC_LOWCOMM_ENDPOINT_RECONNECTING )
		{
			_MPC_LOWCOMM_ENDPOINT_LOCK ( tmp );
			_mpc_lowcomm_endpoint_set_state ( tmp, _MPC_LOWCOMM_ENDPOINT_DECONNECTED );
			init_func ( dest, rail, tmp, 1 );
			tmp->is_initiator = is_initiator;
			_MPC_LOWCOMM_ENDPOINT_UNLOCK ( tmp );
			*added = 1;
		}
	}

	mpc_common_spinlock_write_unlock ( &rail->route_table->dynamic_route_table_lock );
	return tmp;
}


_mpc_lowcomm_endpoint_t * sctk_rail_get_dynamic_route_to_process ( sctk_rail_info_t *rail, mpc_lowcomm_peer_uid_t dest )
{
	return  _mpc_lowcomm_endpoint_table_get_dynamic_route( rail->route_table, dest );
}



/* Get the route to a given task (on demand mode) */
_mpc_lowcomm_endpoint_t * sctk_rail_get_any_route_to_task_or_on_demand ( sctk_rail_info_t *rail, mpc_lowcomm_peer_uid_t dest )
{
	_mpc_lowcomm_endpoint_t *tmp;
	int process;

	process = mpc_lowcomm_group_process_rank_from_world ( dest );
	tmp = sctk_rail_get_any_route_to_process_or_on_demand ( rail, process );
	return tmp;
}


/* Get a route to a process with no ondemand connexions (relies on both static and dynamic routes without creating
 * routes => Relies on routing ) */
inline _mpc_lowcomm_endpoint_t * sctk_rail_get_any_route_to_process_or_forward ( sctk_rail_info_t *rail, mpc_lowcomm_peer_uid_t dest )
{
	_mpc_lowcomm_endpoint_t *tmp;
	/* Try to get a dynamic / static route until this process */
	tmp = sctk_rail_get_any_route_to_process ( rail, dest );

	if ( tmp == NULL )
	{
		/* Otherwise route until target process */
		int new_dest = rail->route ( dest, rail );

		if( new_dest == dest )
		{
			mpc_common_debug_fatal("Routing loop identified");
		}

		/* Use the same function which does not create new routes */
		return sctk_rail_get_any_route_to_process_or_forward ( rail, new_dest );
	}

	return tmp;
}



/* Get a route to a process only on static routes (does not create new routes => Relies on routing) */
inline _mpc_lowcomm_endpoint_t * sctk_rail_get_static_route_to_process_or_forward ( sctk_rail_info_t *rail , mpc_lowcomm_peer_uid_t dest )
{
	_mpc_lowcomm_endpoint_t *tmp;
	tmp = sctk_rail_get_static_route_to_process ( rail, dest );

	if ( tmp == NULL )
	{
		int new_dest = rail->route ( dest, rail );

		if( new_dest == dest )
		{
			mpc_common_debug_fatal("Routing loop identified");
		}

		return sctk_rail_get_static_route_to_process_or_forward ( rail, new_dest );
	}

	return tmp;
}





/* Get a route to process, creating the route if not present */
_mpc_lowcomm_endpoint_t * sctk_rail_get_any_route_to_process_or_on_demand ( sctk_rail_info_t *rail, mpc_lowcomm_peer_uid_t dest )
{
	_mpc_lowcomm_endpoint_t *tmp;

	/* Try to find a direct route with no routing */
	tmp = sctk_rail_get_any_route_to_process ( rail, dest );

	if ( tmp )
	{
		mpc_common_nodebug ( "Directly connected to %d", dest );
	}
	else
	{
		mpc_common_nodebug ( "NOT Directly connected to %d", dest );
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
				mpc_common_debug_fatal("Routing loop identified");
			}

			return sctk_rail_get_any_route_to_process_or_forward ( rail, new_dest );

		}

	}

	return tmp;
}
