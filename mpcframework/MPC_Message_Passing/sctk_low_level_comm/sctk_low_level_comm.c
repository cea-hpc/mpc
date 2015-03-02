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

#include <sctk_low_level_comm.h>
#include <sctk.h>
#include <sctk_pmi.h>
#include <string.h>
#include "sctk_checksum.h"
#include "sctk_runtime_config.h"

/*Networks*/
#include <sctk_multirail_ib.h>
#include <sctk_route.h>
#include <sctk_multirail.h>

/************************************************************************/
/* Network Hooks                                                        */
/************************************************************************/

/********** SEND ************/

void sctk_network_send_message_default ( sctk_thread_ptp_message_t *msg )
{
	not_reachable();
}
static void ( *sctk_network_send_message_ptr ) ( sctk_thread_ptp_message_t * ) = sctk_network_send_message_default;

void sctk_network_send_message ( sctk_thread_ptp_message_t *msg )
{
	sctk_network_send_message_ptr ( msg );
}

void sctk_network_send_message_set ( void ( *sctk_network_send_message_val ) ( sctk_thread_ptp_message_t * ) )
{
	sctk_network_send_message_ptr = sctk_network_send_message_val;
}

/********** NOTIFY_RECV ************/

void sctk_network_notify_recv_message_default ( sctk_thread_ptp_message_t *msg )
{

}

static void ( *sctk_network_notify_recv_message_ptr ) ( sctk_thread_ptp_message_t * ) = sctk_network_notify_recv_message_default;

void sctk_network_notify_recv_message ( sctk_thread_ptp_message_t *msg )
{
	sctk_network_notify_recv_message_ptr ( msg );
}

void sctk_network_notify_recv_message_set ( void ( *sctk_network_notify_recv_message_val ) ( sctk_thread_ptp_message_t * ) )
{
	sctk_network_notify_recv_message_ptr = sctk_network_notify_recv_message_val;
}

/********** NOTIFY_MATCHING ************/

void sctk_network_notify_matching_message_default ( sctk_thread_ptp_message_t *msg )
{

}

static void ( *sctk_network_notify_matching_message_ptr ) ( sctk_thread_ptp_message_t * ) = sctk_network_notify_matching_message_default;

void sctk_network_notify_matching_message ( sctk_thread_ptp_message_t *msg )
{
	sctk_network_notify_matching_message_ptr ( msg );
}

void sctk_network_notify_matching_message_set ( void ( *sctk_network_notify_matching_message_val ) ( sctk_thread_ptp_message_t * ) )
{
	sctk_network_notify_matching_message_ptr = sctk_network_notify_matching_message_val;
}

/********** NOTIFY_PERFORM ************/

static void sctk_network_notify_perform_message_default ( int remote_proces, int remote_task_id, int polling_task_id, int blocking )
{

}

static void ( *sctk_network_notify_perform_message_ptr ) ( int, int, int, int ) =  sctk_network_notify_perform_message_default;

void sctk_network_notify_perform_message ( int remote_process, int remote_task_id, int polling_task_id, int blocking )
{
	sctk_network_notify_perform_message_ptr ( remote_process, remote_task_id, polling_task_id, blocking );
}

void sctk_network_notify_perform_message_set ( void ( *sctk_network_notify_perform_message_val ) ( int, int, int, int ) )
{
	sctk_network_notify_perform_message_ptr = sctk_network_notify_perform_message_val;
}

/********** NOTIFY_IDLE ************/

void sctk_network_notify_idle_message_default ()
{

}

static void ( *sctk_network_notify_idle_message_ptr ) () = sctk_network_notify_idle_message_default;

void sctk_network_notify_idle_message ()
{
	sctk_network_notify_idle_message_ptr();
}

void sctk_network_notify_idle_message_set ( void ( *sctk_network_notify_idle_message_val ) () )
{
	sctk_network_notify_idle_message_ptr = sctk_network_notify_idle_message_val;
}

/********** NOTIFY_ANY_SOURCE ************/

static void sctk_network_notify_any_source_message_default ( int polling_task_id, int blocking )
{

}

static void ( *sctk_network_notify_any_source_message_ptr ) ( int, int ) =  sctk_network_notify_idle_message_default;

void sctk_network_notify_any_source_message ( int polling_task_id, int blocking )
{
	sctk_network_notify_any_source_message_ptr ( polling_task_id, blocking );
}

void sctk_network_notify_any_source_message_set ( void ( *sctk_network_notify_any_source_message_val ) ( int polling_task_id, int blocking ) )
{
	sctk_network_notify_any_source_message_ptr = sctk_network_notify_any_source_message_val;
}

/************************************************************************/
/* Net Error Messages                                                   */
/************************************************************************/

static void sctk_network_not_implemented ( char *name )
{
	sctk_error ( "No configuration found for the network '%s'. Please check you '-net=' argument"
	             " and your configuration file", name );
	sctk_abort();
}

static void sctk_network_not_implemented_warn ( char *name )
{
	if ( sctk_process_rank == 0 )
	{
		sctk_warning ( "No configuration found for the network '%s'. Please check you '-net=' argument"
		               " and your configuration file. FALLBACK to TCP", name );
		/* sctk_abort(); */
	}
}

/************************************************************************/
/* PMI Init                                                             */
/************************************************************************/

void sctk_net_init_pmi()
{
	if ( sctk_process_number > 1 )
	{
		/* Initialize topology informations from PMI */
		sctk_pmi_get_process_rank ( &sctk_process_rank );
		sctk_pmi_get_process_number ( &sctk_process_number );
		sctk_pmi_get_process_on_node_rank ( &sctk_local_process_rank );
		sctk_pmi_get_process_on_node_number ( &sctk_local_process_number );
	}
}

/************************************************************************/
/* Config Helpers                                                       */
/************************************************************************/

/** \brief Get a pointer to the global network configuration
*   \return the network config (static struct)
*/
static inline  const struct sctk_runtime_config_struct_networks *sctk_net_get_config()
{
	return ( struct sctk_runtime_config_struct_networks * ) &sctk_runtime_config_get()->networks;
}

/** \brief Get a pointer to a given CLI configuration
*   \param name Name of the requested configuration
*   \return The configuration or NULL
*/
static inline struct sctk_runtime_config_struct_net_cli_option *sctk_get_net_config_by_name ( char *name )
{
	int k = 0;
	struct sctk_runtime_config_struct_net_cli_option *ret = NULL;

	for ( k = 0; k < sctk_net_get_config()->cli_options_size; ++k )
	{
		if ( strcmp ( name, sctk_net_get_config()->cli_options[k].name ) == 0 )
		{
			ret = &sctk_net_get_config()->cli_options[k];
			break;
		}
	}

	return ret;
}

/** \brief Get a pointer to a given rail
*   \param name Name of the requested rail
*   \return The rail or NULL
*/
static inline struct sctk_runtime_config_struct_net_rail *sctk_get_rail_config_by_name ( char *name )
{
	int l = 0;
	struct sctk_runtime_config_struct_net_rail *ret = NULL;

	for ( l = 0; l < sctk_net_get_config()->rails_size; ++l )
	{
		if ( strcmp ( name, sctk_net_get_config()->rails[l].name ) == 0 )
		{
			ret = &sctk_net_get_config()->rails[l];
			break;
		}
	}

	return ret;
}

/** \brief Get a pointer to a driver config
*   \param name Name of the requested driver config
*   \return The driver config or NULL
*/
static inline struct sctk_runtime_config_struct_net_driver_config *sctk_get_driver_config_by_name ( char *name )
{
	int j = 0;
	struct sctk_runtime_config_struct_net_driver_config *ret = NULL;

	for ( j = 0; j < sctk_net_get_config()->configs_size; ++j )
	{
		if ( strcmp ( name, sctk_net_get_config()->configs[j].name ) == 0 )
		{
			ret = &sctk_net_get_config()->configs[j];
			break;
		}
	}

	return ret;
}



/************************************************************************/
/* Network INIT                                                         */
/************************************************************************/

/** \brief Init MPC network configuration
 *
 *   This function also loads the default configuration from the command
 *   line including rails and network backends
 *
 *  \param name Name of the configuration from the command line (can be NULL)
 */
void sctk_net_init_driver ( char *name )
{	
restart:
	if ( sctk_process_number == 1 )
	{
		/* Nothing to do */
		return;
	}

	/* Initialize multi-rail engine */
	sctk_multirail_destination_table_init();

	int j, k, l;
	int rails_nb = 0;
	struct sctk_runtime_config_struct_net_cli_option *cli_option = NULL;

	/* Retrieve default network from config */
	char *option_name = sctk_runtime_config_get()->modules.low_level_comm.network_mode;

	/* If we have a network name from the command line (through sctk_launch.c)  */
	if ( name != NULL )
	{
		option_name = name;
	}

	/* Here we retrieve the network configuration from the network list
	   according to its name */
	sctk_nodebug ( "Run with driver %s", option_name );

	cli_option = sctk_get_net_config_by_name ( option_name );

	if ( cli_option == NULL )
	{
		/* We did not find this name in the network configurations */
		sctk_error ( "No configuration found for the network '%s'. Please check you '-net=' argument"
		             " and your configuration file", option_name );
		sctk_abort();
	}


	/* Allocate Rails Storage we need to do it at once as we are going to
	 * distribute pointers to rails to every modules therefore, they
	 * should not change during the whole execution */
	sctk_rail_allocate ( cli_option->rails_size );


	/* Compute the number of rails for each type: */
	int nb_rails_infiniband = 0;
	int nb_rails_portals = 0;


	for ( k = 0; k < cli_option->rails_size; ++k )
	{
		/* Get the rail */
		struct sctk_runtime_config_struct_net_rail *rail = sctk_get_rail_config_by_name ( cli_option->rails[k] );

		if ( rail == NULL )
		{
			sctk_error ( "Rail with name '%s' not found in config!", cli_option->rails[k] );
			sctk_abort();
		}

		/* Try to find the driver associated to the configuration */
		struct sctk_runtime_config_struct_net_driver_config *driver = sctk_get_driver_config_by_name ( rail->config );

		if ( driver == NULL )
		{
			sctk_error ( "Driver with name '%s' not found in config!", rail->config );
			continue;
		}

		/* Switch on the driver to use */
		switch ( driver->driver.type )
		{
#ifdef MPC_USE_INFINIBAND

			case SCTK_RTCFG_net_driver_infiniband: /* INFINIBAND */
				nb_rails_infiniband++ ;
				break;
#endif
			case SCTK_RTCFG_net_driver_portals: /* PORTALS */
				nb_rails_portals ++ ;
				break;
		}

	}

	/* End of rails computing. Now allocate ! */

	/* HERE note that we are going in reverse order so that
	 * rails are initialized in the same order than in the config
	 * this is important if no priority is given to make sure
	 * that the decalration order will be the gate order */
	for ( k = (cli_option->rails_size - 1); 0 <= k ; k-- )
	{
		/* For each RAIL */
		struct sctk_runtime_config_struct_net_rail *rail_config_struct = sctk_get_rail_config_by_name ( cli_option->rails[k] );

		if ( rail_config_struct == NULL )
		{
			sctk_error ( "Rail with name '%s' not found in config!", cli_option->rails[k] );
			sctk_abort();
		}

		sctk_nodebug ( "Found rail '%s' to init", rail->name );

		/* For this rail retrieve the config */
		struct sctk_runtime_config_struct_net_driver_config *driver_config = sctk_get_driver_config_by_name ( rail_config_struct->config );

		if ( driver_config == NULL )
		{
			sctk_error ( "Driver with name '%s' not found in config!", rail_config_struct->config );
			continue;
		}

		/* Set infos for the current rail */
		sctk_rail_info_t *new_rail = sctk_rail_new ( rail_config_struct, driver_config );

		/* Switch on the driver to use */
		switch ( driver_config->driver.type )
		{
#ifdef MPC_USE_INFINIBAND

			case SCTK_RTCFG_net_driver_infiniband: /* INFINIBAND */
				sctk_network_init_mpi_ib ( new_rail );
			break;
#endif
#ifdef MPC_USE_PORTALS

			case SCTK_RTCFG_net_driver_portals: /* TCP */
				sctk_network_init_multirail_portals ( new_rail, nb_rails_portals );
			break;
#endif
			case SCTK_RTCFG_net_driver_tcp:
				sctk_network_init_tcp ( new_rail );
			break;
			
			case SCTK_RTCFG_net_driver_tcprdma:
				sctk_network_init_tcp_rdma( new_rail );
			break;


			default:
				//sctk_network_not_implemented ( option_name );
				break;
		}

		/* Increment the number of rails used */
		rails_nb++;
	}



	sctk_rail_commit();
	sctk_checksum_init();

}


/********************************************************************/
/* Hybrid MPI+X                                                     */
/********************************************************************/
static int is_mode_hybrid = 0;

int sctk_net_is_mode_hybrid ()
{
	return is_mode_hybrid;
}

int sctk_net_set_mode_hybrid ()
{
	is_mode_hybrid = 1;
	return 0;
}
/********************************************************************/
/* Memory Allocator                                                 */
/********************************************************************/

size_t sctk_net_memory_allocation_hook ( size_t size_origin )
{
	size_t aligned_size;
#ifdef MPC_USE_INFINIBAND

	if ( sctk_network_is_ib_used() )
	{
		return sctk_network_memory_allocator_hook_ib ( size_origin );
	}

#endif
	return 0;
}

/********************************************************************/
/* Migration                                                        */
/********************************************************************/

int sctk_is_net_migration_available()
{
	if ( sctk_migration_mode == 1 )
	{
		not_implemented();
	}

	return sctk_migration_mode;
}
