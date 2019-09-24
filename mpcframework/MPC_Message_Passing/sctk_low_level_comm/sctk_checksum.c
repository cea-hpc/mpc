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

#include <sctk_runtime_config.h>
#include "sctk_checksum.h"
#ifdef SCTK_USE_CHECKSUM
#include "mpc_common_io_helper.h"
#include <zlib.h>

static bool checksum_enabled;

unsigned long sctk_checksum_message ( sctk_thread_ptp_message_t *send,
                                      sctk_thread_ptp_message_t *recv )
{
	unsigned long adler = 0;

	if ( !checksum_enabled )
		return 0;

	switch ( recv->tail.message_type )
	{
		case SCTK_MESSAGE_CONTIGUOUS:
		{
			size_t size;
			size = SCTK_MSG_SIZE ( send );

			if ( size > recv->tail.message.contiguous.size )
			{
				size = recv->tail.message.contiguous.size;
			}

			adler = adler32 ( adler, ( unsigned char * ) recv->tail.message.contiguous.addr, size );
			break;
		}

		case SCTK_MESSAGE_NETWORK:
		{
			size_t size;
			void *body;

			size = SCTK_MSG_SIZE ( recv );
			body = ( char * ) recv + sizeof ( sctk_thread_ptp_message_t );

			adler = adler32 ( adler, ( unsigned char * ) body, size );
			break;
		}

		case SCTK_MESSAGE_PACK:
		{
			size_t i;
			size_t j;
			size_t size;

			for ( i = 0; i < recv->tail.message.pack.count; i++ )
				for ( j = 0; j < recv->tail.message.pack.list.std[i].count; j++ )
				{
					size = ( recv->tail.message.pack.list.std[i].ends[j] -
					         recv->tail.message.pack.list.std[i].begins[j] +
					         1 ) * recv->tail.message.pack.list.std[i].elem_size;
					adler = adler32 ( adler, ( ( unsigned char * ) ( recv->tail.message.pack.list.std[i].addr ) ) +
					                  recv->tail.message.pack.list.std[i].begins[j] *
					                  recv->tail.message.pack.list.std[i].elem_size, size );
				}

			break;
		}

		case SCTK_MESSAGE_PACK_ABSOLUTE:
		{
			size_t i;
			size_t j;
			size_t size;

			for ( i = 0; i < recv->tail.message.pack.count; i++ )
				for ( j = 0; j < recv->tail.message.pack.list.absolute[i].count; j++ )
				{
					size = ( recv->tail.message.pack.list.absolute[i].ends[j] -
					         recv->tail.message.pack.list.absolute[i].begins[j] +
					         1 ) * recv->tail.message.pack.list.absolute[i].elem_size;
					adler = adler32 ( adler, ( ( unsigned char * ) ( recv->tail.message.pack.list.absolute[i].addr ) ) +
					                  recv->tail.message.pack.list.absolute[i].begins[j] *
					                  recv->tail.message.pack.list.absolute[i].elem_size, size );
				}

			break;
		}

		default:
			not_reachable();
	}

	return adler;
}

unsigned long sctk_checksum_buffer ( char *body,
                                     sctk_thread_ptp_message_t *msg )
{
	uLong adler = 0;

	if ( !checksum_enabled )
		return 0;

	switch ( msg->tail.message_type )
	{
		case SCTK_MESSAGE_CONTIGUOUS:
		{
			size_t size;
			size = SCTK_MSG_SIZE ( msg );

			if ( size > msg->tail.message.contiguous.size )
			{
				size = msg->tail.message.contiguous.size;
			}

			adler = adler32 ( adler, ( unsigned char * ) body, size );

			break;
		}

		case SCTK_MESSAGE_PACK:
		{
			size_t i;
			size_t j;
			size_t size;

			for ( i = 0; i < msg->tail.message.pack.count; i++ )
				for ( j = 0; j < msg->tail.message.pack.list.std[i].count; j++ )
				{
					size = ( msg->tail.message.pack.list.std[i].ends[j] -
					         msg->tail.message.pack.list.std[i].begins[j] +
					         1 ) * msg->tail.message.pack.list.std[i].elem_size;
					adler = adler32 ( adler, ( unsigned char * ) body, size );
					body += size;
				}

			break;
		}

		case SCTK_MESSAGE_PACK_ABSOLUTE:
		{
			size_t i;
			size_t j;
			size_t size;

			for ( i = 0; i < msg->tail.message.pack.count; i++ )
				for ( j = 0; j < msg->tail.message.pack.list.absolute[i].count; j++ )
				{
					size = ( msg->tail.message.pack.list.absolute[i].ends[j] -
					         msg->tail.message.pack.list.absolute[i].begins[j] +
					         1 ) * msg->tail.message.pack.list.absolute[i].elem_size;

					adler = adler32 ( adler, ( unsigned char * ) body, size );
					body += size;
				}

			break;
		}

		default:
			not_reachable();
	}

	return adler;
}


void sctk_checksum_register ( sctk_thread_ptp_message_t *msg )
{
	msg->body.checksum = sctk_checksum_message ( msg, msg );
}
void sctk_checksum_unregister ( sctk_thread_ptp_message_t *msg )
{
	msg->body.checksum = 0;
}

unsigned long sctk_checksum_verify ( sctk_thread_ptp_message_t *send, sctk_thread_ptp_message_t *recv )
{
	unsigned long adler = 0;

	if ( !checksum_enabled )
		return 0;

	/* If Checksum != 0, we verify it because the user asked for it */
	if ( send->body.checksum )
	{
		adler = sctk_checksum_message ( send, recv );

		if ( adler != send->body.checksum )
		{
			sctk_error ( "Got wrong checksum (got:%lu, expected:%lu)", adler, send->body.checksum );
			sctk_abort();
		}
		else
		{
			sctk_debug ( "Got GOOD checksum (got:%lu, expected:%lu)", adler, send->body.checksum );
		}
	}

	return adler;
}

void sctk_checksum_init()
{
	if ( mpc_common_get_process_rank() == 0 )
	{
	fprintf ( stderr, SCTK_COLOR_RED_BOLD ( WARNING: inter - node message checking enabled! ) "\n" );
	}

	checksum_enabled = sctk_runtime_config_get()->modules.low_level_comm.checksum;
}
#else
void sctk_checksum_init()
{
	/* nothing to do */
}

#endif
