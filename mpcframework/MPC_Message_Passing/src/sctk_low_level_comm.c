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

#include <mpc_launch_pmi.h>
#include <string.h>
#include "sctk_checksum.h"
#include "sctk_runtime_config.h"
#include "sctk_control_messages.h"
#include <sctk_launch.h>

#include <mpc_common_rank.h>

/*Networks*/
#include <sctk_ib_mpi.h>
#include <sctk_route.h>
#include <sctk_multirail.h>
#ifdef MPC_Fault_Tolerance
#include "sctk_ft_iface.h"
#endif

/************************************************************************/
/* Network Hooks                                                        */
/************************************************************************/

/********** SEND ************/

/**
 * Default value when a rail did not defines it own send() method.
 *
 * This is clearly an issue with the development of a given driver if the default pointer to send a message is not
 * overriden by the rail initialization. This is the purpose of this function.
 * This function does NOT return !
 *
 * \param[in] msg the message to send
 */
void sctk_network_send_message_default (  __UNUSED__ mpc_mp_ptp_message_t *msg )
{
	not_reachable();
}

/** send_message() function pointer used by later trampolines */
static void ( *sctk_network_send_message_ptr ) ( mpc_mp_ptp_message_t * ) = sctk_network_send_message_default;
/** send_message() function pointer used by later trampolines */

/** trampoline function to set to the dyamically-defined send() routine.
 * \param[in] msg the message to send
 */
void sctk_network_send_message ( mpc_mp_ptp_message_t *msg )
{
	sctk_network_send_message_ptr ( msg );
}

/**
 * Set the routine used to send a message from latter calls.
 *
 * \param[in] sctk_network_send_message_val the function pointer
 */
void sctk_network_send_message_set ( void ( *sctk_network_send_message_val ) ( mpc_mp_ptp_message_t * ) )
{
	sctk_network_send_message_ptr = sctk_network_send_message_val;
}

/********** NOTIFY_RECV ************/

/**
 * Default handler when a RECV message is locally posted.
 *
 * This handler can be called if a given driver does not care about such notification.
 * That's why there is no `not_rechable()` call here.
 *
 * \param[in] msg the message hosting RECV infos
 */
void sctk_network_notify_recv_message_default ( __UNUSED__ mpc_mp_ptp_message_t *msg )
{
}

/** recv_mesg() function pointer used by later calls */
static void ( *sctk_network_notify_recv_message_ptr ) ( mpc_mp_ptp_message_t * ) = sctk_network_notify_recv_message_default;


/**
 * tranpoline function to set the the dynamically-defined recv() routine.
 *
 * \param[in] msg the message hosting RECV infos
 */
void sctk_network_notify_recv_message ( sctk_thread_ptp_message_t *msg )
{
	sctk_network_notify_recv_message_ptr ( msg );
}

/**
 * Set the routine used to get a local-recv notification.
 *
 * \param[in] sctk_network_notify_recv_message_val 
 */
void sctk_network_notify_recv_message_set ( void ( *sctk_network_notify_recv_message_val ) ( mpc_mp_ptp_message_t * ) )
{
	sctk_network_notify_recv_message_ptr = sctk_network_notify_recv_message_val;
}

/********** NOTIFY_MATCHING ************/

/**
 * Default handler when a network-received messaged matched with a local-recv (two-sided).
 * 
 * As some driver implementation could ignore such notification, this handler may be called by default. 
 * That's why there is no `not_reachable()` here.
 *
 * \param[in] msg the matched message
 */
/**
void sctk_network_notify_matching_message_default (  __UNUSED__ mpc_mp_ptp_message_t *msg )
{
}

/** notify_msg() function pointe rused by later calls */
static void ( *sctk_network_notify_matching_message_ptr ) ( mpc_mp_ptp_message_t * ) = sctk_network_notify_matching_message_default;

/**
 * trampoline function to set the dynamically-defined notify_matching() function.
 *
 * \param[in] msg the matching message
 */
void sctk_network_notify_matching_message ( mpc_mp_ptp_message_t *msg )
{
	sctk_network_notify_matching_message_ptr ( msg );
}

/** Set the routine used to notify local matching messages.
 * \param[in] msg the matching message
 */
void sctk_network_notify_matching_message_set ( void ( *sctk_network_notify_matching_message_val ) ( mpc_mp_ptp_message_t * ) )
{
	sctk_network_notify_matching_message_ptr = sctk_network_notify_matching_message_val;
}

/********** NOTIFY_PERFORM ************/

static void sctk_network_notify_perform_message_default ( __UNUSED__ int remote_proces, __UNUSED__ int remote_task_id, __UNUSED__ int polling_task_id, __UNUSED__ int blocking )
{

}

static void ( *sctk_network_notify_perform_message_ptr ) ( int, int, int, int ) =  sctk_network_notify_perform_message_default;

void sctk_network_notify_perform_message ( int remote_process, int remote_task_id, int polling_task_id, int blocking )
{
#ifdef MPC_Fault_Tolerance
	if(sctk_ft_no_suspend_start())
        {
#endif
	sctk_network_notify_perform_message_ptr ( remote_process, remote_task_id, polling_task_id, blocking );

#ifdef MPC_Fault_Tolerance
	sctk_ft_no_suspend_end();
       }
#endif
}

void sctk_network_notify_perform_message_set ( void ( *sctk_network_notify_perform_message_val ) ( int, int, int, int ) )
{
	sctk_network_notify_perform_message_ptr = sctk_network_notify_perform_message_val;
}

/********** NOTIFY_PROBE ************/

void sctk_network_notify_probe_message_default (__UNUSED__  sctk_thread_message_header_t* hdr, __UNUSED__ int *status)
{
        *status = -1; /* probe not supported */
}

static void  ( *sctk_network_notify_probe_message_ptr ) () = sctk_network_notify_probe_message_default;


void sctk_network_notify_probe_message  (sctk_thread_message_header_t* hdr, int *status)
{
#ifdef MPC_Fault_Tolerance
	if(sctk_ft_no_suspend_start())
        {
#endif
        *status = -1; /* not supported */
	sctk_network_notify_probe_message_ptr (hdr, status);

#ifdef MPC_Fault_Tolerance
	sctk_ft_no_suspend_end();
       }
#endif
	
}


/********** NOTIFY_IDLE ************/


/**
 * Default handler to poll new messages from network.
 * The behavior here should always be overriden by the network driver implementation to progress
 * messages. Nevertheless, this default handler could be called in some situations anyway
 */
void sctk_network_notify_idle_message_default ()
{

}

static void ( *sctk_network_notify_idle_message_ptr ) () = sctk_network_notify_idle_message_default;


/**
 * Main entry point from upper layers for polling messages.
 *
 * This function can be called from anywhere to request message progress. Each rail is then called
 * to progreess its own queues. This function is THE potential routine to avoid deadlock, despite the
 * call can be costly in some situation. For example, this function can be called in the context of
 * thread context switching to solve potential deadlocks.
 */
void sctk_network_notify_idle_message ()
{
	/* Fault Tolerance mechanism cannot allow any driver to be modified during a pending checkpoint.
	 * This is our way to maintain consistency for data to be saved.
	 */
#ifdef MPC_Fault_Tolerance
	if(sctk_ft_no_suspend_start())
        {
#endif

	sctk_network_notify_idle_message_ptr();
	sctk_control_message_process();

#ifdef MPC_Fault_Tolerance
	sctk_ft_no_suspend_end();
       }
#endif
}

void sctk_network_notify_idle_message_set ( void ( *sctk_network_notify_idle_message_val ) () )
{
	sctk_network_notify_idle_message_ptr = sctk_network_notify_idle_message_val;
}

void sctk_network_notify_probe_message_set (void ( *sctk_network_notify_probe_message_val) () )
{
	sctk_network_notify_probe_message_ptr = sctk_network_notify_probe_message_val;
}

/********** NOTIFY_ANY_SOURCE ************/

static void sctk_network_notify_any_source_message_default ( __UNUSED__ int polling_task_id, __UNUSED__ int blocking )
{

}

static void ( *sctk_network_notify_any_source_message_ptr ) ( int, int ) =  sctk_network_notify_any_source_message_default;

void sctk_network_notify_any_source_message ( int polling_task_id, int blocking )
{

#ifdef MPC_Fault_Tolerance
	if(sctk_ft_no_suspend_start())
        {
#endif
	sctk_network_notify_any_source_message_ptr ( polling_task_id, blocking );

#ifdef MPC_Fault_Tolerance
	sctk_ft_no_suspend_end();
        }
#endif
}

void sctk_network_notify_any_source_message_set ( void ( *sctk_network_notify_any_source_message_val ) ( int polling_task_id, int blocking ) )
{
	sctk_network_notify_any_source_message_ptr = sctk_network_notify_any_source_message_val;
}

/********** NOTIFY_NEW_COMM ************/
static void sctk_network_notify_new_communicator_default(__UNUSED__ int comm_idx, __UNUSED__ size_t comm_size)
{
}

static void (*sctk_network_notify_new_communicator_ptr)(int, size_t) = sctk_network_notify_new_communicator_default;

void sctk_network_notify_new_communicator(int comm_idx, size_t comm_size)
{
	sctk_network_notify_new_communicator_ptr(comm_idx, comm_size);
}

void sctk_network_notify_new_communicator_set( void (*func)(int, size_t))
{
	sctk_network_notify_new_communicator_ptr = func;
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
struct sctk_runtime_config_struct_net_rail *sctk_get_rail_config_by_name ( char *name )
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
struct sctk_runtime_config_struct_net_driver_config *sctk_get_driver_config_by_name ( char *name )
{
	int j = 0;
	struct sctk_runtime_config_struct_net_driver_config *ret = NULL;

	if( ! name )
		return NULL;

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

int sctk_unfold_rails( struct sctk_runtime_config_struct_net_cli_option *cli_option )
{
	/* Now compute the actual total number of rails knowing that
	 * some rails might contain subrails */
	
	int k;
	int total_rail_nb = 0;
	 
	for ( k = 0; k < cli_option->rails_size; ++k )
	{
		/* Get the rail */
		struct sctk_runtime_config_struct_net_rail *rail = sctk_get_rail_config_by_name ( cli_option->rails[k] );
		
		/* Here we have two cases, first the rail device name is a regexp (begins with !)
		 * or the subrails are already defined with their device manually set  */
		
		/* Case with preset subrails */
		if( rail->subrails_size )
		{
			/* Count subrails */
			total_rail_nb += rail->subrails_size;
			rail->topology = "none";
		}
		else
		{
			if( strlen( rail->device ) )
			{
				if( rail->device[0] == '!')
				{
					/* We need to rebuild subrails
					 * according to devices */
					int matching_rails = 0;
					
					/* +1 to skip the ! */
				 mpc_topology_device_t ** matching_device = mpc_topology_device_get_from_handle_regexp( rail->device + 1, &matching_rails );
					
					/* Now we build the subrail array
					 * we duplicate the config of current rail
					 * while just overriding device name with the 
					 * one we extracted from the device array */
					struct sctk_runtime_config_struct_net_rail * subrails = 
					     sctk_malloc( sizeof(  struct sctk_runtime_config_struct_net_rail ) * matching_rails );
					
					int i;
					
					for( i =  0 ; i < matching_rails ; i++ )
					{
						/* Copy Current rail (note that at this point the rail has no subrails ) */
						memcpy( &subrails[i], rail, sizeof( struct sctk_runtime_config_struct_net_rail ) );
						
						/* Update device name with matching device */
						subrails[i].device = matching_device[i]->name;
						
						/* Make sure that the topological rail has the highest priority
						 * this is important during the on-demand election process
						 * as we want the topological rail to make this choice */
						subrails[i].priority = rail->priority - 1;
						
						/* Make sure that subrails do not have a subrail */
						subrails[i].subrails_size = 0;
					}
					
					sctk_free( matching_device );
					
					/* Store the new subrail array in the rail */
					rail->subrails = subrails;
					rail->subrails_size = matching_rails;
					
					
					/* Increment total rails by matching rails */
					total_rail_nb += matching_rails;
				}
			}
			
		}
		
		/* Count this rail */
		total_rail_nb++;
	}
	
	return total_rail_nb;
}


void sctk_rail_init_driver( sctk_rail_info_t * rail, int driver_type )
{
	/* Switch on the driver to use */
	switch ( driver_type )
	{
#ifdef MPC_USE_INFINIBAND

		case SCTK_RTCFG_net_driver_infiniband: /* INFINIBAND */
			sctk_network_init_mpi_ib ( rail );
		break;
#endif
#ifdef MPC_USE_PORTALS
        	case SCTK_RTCFG_net_driver_portals: /* PORTALS */
			sctk_network_init_ptl ( rail );
		break;
#endif
		case SCTK_RTCFG_net_driver_topological:
			sctk_network_init_topological( rail );
		break;

		case SCTK_RTCFG_net_driver_tcp:
			sctk_network_init_tcp ( rail );
		break;
		
		case SCTK_RTCFG_net_driver_tcprdma:
			sctk_network_init_tcp_rdma( rail );
		break;

		case SCTK_RTCFG_net_driver_shm:
			sctk_network_init_shm( rail );
		break;
		

		default:
			sctk_fatal("No such network type");
			break;
	}
}


void sctk_net_init_task_level(int taskid, int vp)
{
	int i;
	for(i = 0; i < sctk_rail_count(); i++)
	{
		sctk_rail_info_t *rail = sctk_rail_get_by_id(i);
		if(rail->initialize_task)
			rail->initialize_task(rail, taskid, vp);
	}
}

void sctk_net_finalize_task_level(int taskid, int vp)
{
	int i;
	for(i = 0; i < sctk_rail_count(); i++)
	{
		sctk_rail_info_t *rail = sctk_rail_get_by_id(i);
		if(rail->finalize_task)
			rail->finalize_task(rail, taskid, vp);
	}

}

void sctk_net_init_driver ( char *name )
{	
	if ( mpc_common_get_process_count() == 1 )
	{
		/* Nothing to do */
		return;
	}

	/* Initialize multi-rail engine */
	sctk_multirail_destination_table_init();
	
	/* Init Polling for control messages */
	sctk_control_message_init();

	int k;

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

	
	/* This call counts rails while also unfolding subrails */
	int total_rail_nb = sctk_unfold_rails( cli_option );

	/* Allocate Rails Storage we need to do it at once as we are going to
	 * distribute pointers to rails to every modules therefore, they
	 * should not change during the whole execution */
	sctk_rail_allocate ( total_rail_nb );

	if( 255 < total_rail_nb )
	{
		sctk_fatal("There cannot be more than 255 rails");
		/* If you want to remove this limation make sure that 
		 * the rail ID is encoded with a larger type in
		 * struct sctk_control_message_header */
	}

	/* HERE note that we are going in reverse order so that
	 * rails are initialized in the same order than in the config
	 * this is important if no priority is given to make sure
	 * that the decalration order will be the gate order */
	for ( k = (cli_option->rails_size - 1); 0 <= k ; k-- )
	{
		/* For each RAIL */
		struct sctk_runtime_config_struct_net_rail *rail_config_struct;
#ifndef MPC_USE_INFINIBAND
		if(strcmp(cli_option->rails[k],"ib_mpi") == 0) {
		  sctk_warning("Network support %s not available switching to tcp_mpi",cli_option->rails[k]);
		  cli_option->rails[k] = "tcp_mpi";
		}
#endif
#ifndef MPC_USE_PORTALS
		if(strcmp(cli_option->rails[k],"portals_mpi") == 0) {
		  sctk_warning("Network support %s not available switching to tcp_mpi",cli_option->rails[k]);
		  cli_option->rails[k] = "tcp_mpi";
		}
#endif
		rail_config_struct = sctk_get_rail_config_by_name ( cli_option->rails[k] );

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
			/* This can only be accepted for topological rails */
			if( rail_config_struct->subrails_size == 0 )
			{
				sctk_error ( "Driver with name '%s' not found in config!", rail_config_struct->config );
				continue;
			}
		}

		/* Register and initalize new rail inside the rail array */
		sctk_rail_register( rail_config_struct, driver_config );
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

#define IB_MEM_THRESHOLD_ALIGNED_SIZE (256*1024) /* RDMA threshold */
#define IB_MEM_ALIGNMENT        (4096) /* Page size */

static inline size_t sctk_network_memory_allocator_hook_ib ( size_t size )
{
	if ( size > IB_MEM_THRESHOLD_ALIGNED_SIZE )
	{
		return ( ( size + ( IB_MEM_ALIGNMENT - 1 ) ) & ( ~ ( IB_MEM_ALIGNMENT - 1 ) ) );
	}

	return 0;
}


size_t sctk_net_memory_allocation_hook ( size_t size_origin )
{

#ifdef MPC_USE_INFINIBAND

	if ( sctk_network_is_ib_used() )
	{
		return sctk_network_memory_allocator_hook_ib ( size_origin );
	}
#else
	UNUSED(size_origin);
#endif
	return 0;
}

void sctk_net_memory_free_hook ( void * ptr , size_t size )
{

#ifdef MPC_USE_INFINIBAND

	if ( sctk_network_is_ib_used())
	{
		return sctk_network_memory_free_hook_ib ( ptr, size );
	}
#else
	UNUSED(ptr);
	UNUSED(size);
#endif
}
