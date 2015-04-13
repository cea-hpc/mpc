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

sctk_rail_info_t * sctk_rail_new ( struct sctk_runtime_config_struct_net_rail *runtime_config_rail,
                                         struct sctk_runtime_config_struct_net_driver_config *runtime_config_driver_config )
{
	if ( __rails.rail_current_id ==  sctk_rail_count() )
	{
		sctk_fatal ( "Error : Rail overflow detected\n" );
	}

	sctk_rail_info_t *new_rail = &__rails.rails[__rails.rail_current_id];

	new_rail->runtime_config_rail = runtime_config_rail;
	new_rail->runtime_config_driver_config = runtime_config_driver_config;
	new_rail->rail_number = __rails.rail_current_id;
	
	new_rail->route_table = sctk_route_table_new();

	__rails.rail_current_id++;

	return new_rail;
}

int sctk_rail_count()
{
	return __rails.rail_number;
}


sctk_rail_info_t * sctk_rail_get_by_id ( int i )
{
	return & ( __rails.rails[i] );
}



/* Finalize Rails (call the rail route init func ) */
void sctk_rail_commit()
{

	char *net_name;
	int i;
	char *name_ptr;

	net_name = sctk_malloc ( sctk_rail_count() * 1024 );
	name_ptr = net_name;

	for ( i = 0; i <  sctk_rail_count(); i++ )
	{
		sctk_rail_info_t *  rail = sctk_rail_get_by_id ( i );
		rail->route_init( rail );
		sprintf ( name_ptr, "\nRail %d/%d [%s (%s)]", i + 1,  sctk_rail_count(), rail->network_name, rail->topology_name );
		name_ptr = net_name + strlen ( net_name );
		sctk_pmi_barrier();
	}

	sctk_network_mode = net_name;
	__rails.rails_committed = 1;
}



/* Returs wether the route has been finalized */
int sctk_rail_committed()
{
	return __rails.rails_committed;
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


/**************************/




void sctk_rail_add_static_route (sctk_rail_info_t *rail, int dest, sctk_endpoint_t *tmp )
{
	sctk_route_table_add_static_route ( rail->route_table, tmp );
}



void sctk_rail_add_dynamic_route ( sctk_rail_info_t *rail, int dest, sctk_endpoint_t *tmp )
{
	sctk_route_table_add_dynamic_route (  rail->route_table, tmp );
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
		
		sctk_route_table_add_dynamic_route_no_lock(  rail->route_table, tmp );
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

		/* On demand enabled */
		if ( rail->on_demand )
		{
			/* Network has an on-demand function */
			if( rail->on_demand_connection )
				return (rail->on_demand_connection)( rail, dest );
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
