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


#include <sctk_debug.h>
#include <sctk_inter_thread_comm.h>
#include <sctk_low_level_comm.h>
#include <sctk_multirail_tcp.h>
#include <sctk_route.h>
#include <sctk_tcp.h>
#include <opa_primitives.h>

/************************************************************************/
/* Multirail TCP context                                                */
/************************************************************************/

static int rails_nb = 0;
static sctk_rail_info_t **rails = NULL;

/************************************************************************/
/* Network Hooks                                                        */
/************************************************************************/

static void sctk_network_send_message_multirail_tcp ( sctk_thread_ptp_message_t *msg )
{
	int i ;

	if ( sctk_prepare_send_message_to_network_reorder ( msg ) == 0 )
	{
		/* Reordering available : we can use multirail */
		i = 0;

		TODO ( "Use a smart multirail" )

		if ( ( rails_nb >= 2 ) && ( SCTK_MSG_SIZE ( msg ) > 32768 ) )
		{
			i = 1;
		}
	}
	else
	{
		/* No reodering: we can't use multirail fall back to rail 0 */
		i = 0;
	}

	rails[i]->send_message ( msg, rails[i] );
}

static void sctk_network_notify_recv_message_multirail_tcp ( sctk_thread_ptp_message_t *msg )
{
	int i;

	for ( i = 0; i < rails_nb; i++ )
	{
		rails[i]->notify_recv_message ( msg, rails[i] );
	}
}

static void sctk_network_notify_matching_message_multirail_tcp ( sctk_thread_ptp_message_t *msg )
{
	int i;

	for ( i = 0; i < rails_nb; i++ )
	{
		rails[i]->notify_matching_message ( msg, rails[i] );
	}
}

static void sctk_network_notify_perform_message_multirail_tcp ( int remote, int remote_task_id, int polling_task_id, int blocking )
{
	int i;

	for ( i = 0; i < rails_nb; i++ )
	{
		rails[i]->notify_perform_message ( remote, remote_task_id, polling_task_id, blocking, rails[i] );
	}
}

static void sctk_network_notify_idle_message_multirail_tcp ()
{
	int i;

	for ( i = 0; i < rails_nb; i++ )
	{
		rails[i]->notify_idle_message ( rails[i] );
	}
}

static void sctk_network_notify_any_source_message_multirail_tcp ( int polling_task_id, int blocking )
{
	int i;

	for ( i = 0; i < rails_nb; i++ )
	{
		rails[i]->notify_any_source_message ( polling_task_id, blocking, rails[i] );
	}
}

static int sctk_send_message_from_network_multirail_tcp ( sctk_thread_ptp_message_t *msg )
{
	if ( sctk_send_message_from_network_reorder ( msg ) == REORDER_NO_NUMBERING )
	{
		/* No reordering */
		sctk_send_message_try_check ( msg, 1 );
	}

	return 1;
}

/************************************************************************/
/* Multirail TCP Init                                                   */
/************************************************************************/

static void __sctk_network_init_multirail_tcp_all ( sctk_rail_info_t *new_rail, int tcpoib )
{
	static int init_once = 0;

	rails = sctk_realloc ( rails, ( rails_nb + 1 ) * sizeof ( sctk_rail_info_t * ) );

	/* Initialize the newly allocated memory */
	memset ( ( rails + rails_nb ), 0, sizeof ( sctk_rail_info_t * ) );

	rails[rails_nb] = new_rail;

	/* Retrieve pointers to rail config */
	struct sctk_runtime_config_struct_net_rail *rail_config = rails[rails_nb]->runtime_config_rail;

	/* STANDARD TCP */
	rails[rails_nb]->send_message_from_network = sctk_send_message_from_network_multirail_tcp;

	sctk_rail_init_route ( rails[rails_nb], rail_config->topology );
	sctk_network_init_tcp ( rails[rails_nb], tcpoib );

	rails_nb++;

	if ( init_once == 0 )
	{
		sctk_network_send_message_set ( sctk_network_send_message_multirail_tcp );
		sctk_network_notify_recv_message_set ( sctk_network_notify_recv_message_multirail_tcp );
		sctk_network_notify_matching_message_set ( sctk_network_notify_matching_message_multirail_tcp );
		sctk_network_notify_perform_message_set ( sctk_network_notify_perform_message_multirail_tcp );
		sctk_network_notify_idle_message_set ( sctk_network_notify_idle_message_multirail_tcp );
		sctk_network_notify_any_source_message_set ( sctk_network_notify_any_source_message_multirail_tcp );
	}

	init_once = 1;
}

/************************************************************************/
/* Multirail TCP Init (Entry Points)                                    */
/************************************************************************/

void sctk_network_init_multirail_tcp ( sctk_rail_info_t *new_rail, int max_rails )
{
	__sctk_network_init_multirail_tcp_all ( new_rail, 0 );
}

void sctk_network_init_multirail_tcpoib ( sctk_rail_info_t *new_rail, int max_rails )
{
	__sctk_network_init_multirail_tcp_all ( new_rail, 1 );
}
