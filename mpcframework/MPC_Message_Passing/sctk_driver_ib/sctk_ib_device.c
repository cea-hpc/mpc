/* ############################# MPC License ############################## */
/* # Wed Nov 19 15:19:19 CET 2008                                         # */
/* # Copyright or (C) or Copr. Commissariat a l'Energie Atomique          # */
/* # Copyright or (C) or Copr. 2010-2012 Université de Versailles         # */
/* # St-Quentin-en-Yvelines                                               # */
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
/* #   - DIDELOT Sylvain sylvain.didelot@exascale-computing.eu            # */
/* #                                                                      # */
/* ######################################################################## */
#include "sctk_ib_device.h"
#include "sctk_ib_toolkit.h"

#include <string.h>

sctk_ib_device_t *sctk_ib_device_init ( struct sctk_ib_rail_info_s *rail_ib )
{
	struct ibv_device **dev_list;
	sctk_ib_device_t *device;
	int devices_nb;

	dev_list = ibv_get_device_list ( &devices_nb );

	/* Check parameters */
	if ( !dev_list )
	{
		SCTK_IB_ABORT ( "No IB devices found." );
	}


	device = sctk_malloc ( sizeof ( sctk_ib_device_t ) );
	ib_assume ( device );
	rail_ib->device = device;
	
	rail_ib->device->dev_list = dev_list;
	rail_ib->device->dev_nb = devices_nb;
	
	rail_ib->device->eager_rdma_connections = 0;
	/* Specific to ondemand (de)connexions */
	rail_ib->device->ondemand.qp_list = NULL;
	rail_ib->device->ondemand.qp_list_ptr = NULL;
	rail_ib->device->ondemand.lock = SCTK_SPINLOCK_INITIALIZER;
	rail_ib->device->send_comp_channel = NULL;
	rail_ib->device->recv_comp_channel = NULL;

	return device;
}



sctk_ib_device_t *sctk_ib_device_open ( struct sctk_ib_rail_info_s *rail_ib, char * device_name )
{
	LOAD_DEVICE ( rail_ib );
	struct ibv_device **dev_list = device->dev_list;
	int devices_nb = device->dev_nb;
	int link_width = -1;
	char *link_rate_string = "unknown";
	int link_rate = -1;
	int data_rate = -1;
	int i;
	
	int device_id = 0;
	
	/* Try to resolve the device name to a device ID only if provided */
	if( device_name )
	{
		/* Is it empty string ? Or is "default" */
		if( strlen( device_name ) && strcmp("default", device_name ) )
		{
			/* Try to resolve the device */
			device_id = sctk_device_get_id_from_handle( device_name );
			
			if( device_id < 0 )
			{
				sctk_warning("Could not locate a device with name %s assuming default device '0'", device_name );
				device_id = 0;
			}
		}
	}
	

	if ( device_id >= devices_nb )
	{
		SCTK_IB_ABORT ( "Cannot open rail. You asked rail %d on %d", device_id, devices_nb );
	}

	sctk_ib_nodebug ( "Opening rail %d on %d", device_id, devices_nb );

	device->context = ibv_open_device ( dev_list[device_id] );

	if ( !device->context )
	{
		SCTK_IB_ABORT_WITH_ERRNO ( "Cannot get device context." );
	}

	if ( ibv_query_device ( device->context, &device->dev_attr ) != 0 )
	{
		SCTK_IB_ABORT_WITH_ERRNO ( "Unable to get device attr." );
	}

	if ( ibv_query_port ( device->context, 1, &device->port_attr ) != 0 )
	{
		SCTK_IB_ABORT ( "Unable to get port attr" );
	}

	/* Get the link width */
	switch ( device->port_attr.active_width )
	{
		case 1:
			link_width = 1;
			break;

		case 2:
			link_width = 4;
			break;

		case 4:
			link_width = 8;
			break;

		case 8:
			link_width = 12;
			break;
	}

	/* Get the link rate */
	switch ( device->port_attr.active_speed )
	{
		case 0x01:
			link_rate = 2;
			link_rate_string = "SDR";
			break;

		case 0x02:
			link_rate = 4;
			link_rate_string = "DDR";
			break;

		case 0x04:
			link_rate = 8;
			link_rate_string = "QDR";
			break;

		case 0x08:
			link_rate = 14;
			link_rate_string = "FDR";
			break;

		case 0x10:
			link_rate = 25;
			link_rate_string = "EDR";
			break;
	}

	data_rate = link_width * link_rate;

	rail_ib->device->data_rate = data_rate;
	rail_ib->device->link_width = link_width;
	strcpy ( rail_ib->device->link_rate, link_rate_string );

	sctk_nodebug ( "device %d", device->dev_attr.max_qp_wr );
	srand48 ( getpid () * time ( NULL ) );
	rail_ib->device->dev_index = device_id;
	rail_ib->device->dev = dev_list[device_id];
	return device;
}


struct ibv_pd *sctk_ib_pd_init ( sctk_ib_device_t *device )
{
	device->pd = ibv_alloc_pd ( device->context );

	if ( !device->pd )
	{
		SCTK_IB_ABORT ( "Cannot get IB protection domain." );
	}

	return device->pd;
}

struct ibv_comp_channel *sctk_ib_comp_channel_init ( sctk_ib_device_t *device )
{
	struct ibv_comp_channel *comp_channel;

	comp_channel = ibv_create_comp_channel ( device->context );

	if ( !comp_channel )
	{
		SCTK_IB_ABORT_WITH_ERRNO ( "Cannot create Completion Channel." );
	}

	return comp_channel;
}


/** \brief Create a completion queue and associate it a completion channel.
*/
struct ibv_cq *sctk_ib_cq_init ( sctk_ib_device_t *device,
                                 struct sctk_runtime_config_struct_net_driver_infiniband *config,
                                 struct ibv_comp_channel *comp_channel )
{
	struct ibv_cq *cq;
	cq = ibv_create_cq ( device->context, config->cq_depth, NULL,
	                     comp_channel, 0 );

	if ( !cq )
	{
		SCTK_IB_ABORT_WITH_ERRNO ( "Cannot create Completion Queue." );
	}

	if ( comp_channel != NULL )
	{
		int ret = ibv_req_notify_cq ( cq, 0 );

		if ( ret != 0 )
		{
			SCTK_IB_ABORT_WITH_ERRNO ( "Couldn't request CQ notification" );
		}
	}

	return cq;
}
