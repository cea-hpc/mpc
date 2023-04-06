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

#include "comm.h"
#include "sctk_low_level_comm.h"
#include "mpc_common_debug.h"
#include "sctk_net_tools.h"
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include <sctk_alloc.h>

#include <sys/uio.h>

struct iovec *
sctk_net_convert_msg_to_iovec( mpc_lowcomm_ptp_message_t *msg, int *iovlen, size_t max_size)
{
	struct iovec * result = NULL;

	switch ( msg->tail.message_type )
	{
		case MPC_LOWCOMM_MESSAGE_CONTIGUOUS:
		{
			*iovlen = 1;
			result = (struct iovec*) sctk_malloc(1*sizeof(struct iovec));
			result->iov_len = SCTK_MSG_SIZE ( msg );
			result->iov_base = msg->tail.message.contiguous.addr;
			break;
		}

		case MPC_LOWCOMM_MESSAGE_NETWORK:
		{
			*iovlen = 1;
			void *body = (char *) msg + sizeof (mpc_lowcomm_ptp_message_t);
			result = (struct iovec*) sctk_malloc(1*sizeof(struct iovec));
			result->iov_len = SCTK_MSG_SIZE ( msg );
			result->iov_base = body;
			break;
		}

		case MPC_LOWCOMM_MESSAGE_PACK:
		{
			int pos;
			char skip = 0;
			size_t i, j, size, total;
			void * body;
			
			total = 0;
			*iovlen = 0;
			pos = 0;
				
			if (SCTK_MSG_SIZE(msg) > 0)
			{
				for ( i = 0; ( ( i < msg->tail.message.pack.count ) && !skip ); i++ )
				{
					for ( j = 0; ( ( j < msg->tail.message.pack.list.absolute[i].count ) && !skip ); j++ )
					{
						size = ( msg->tail.message.pack.list.std[i].ends[j] -
						         msg->tail.message.pack.list.std[i].begins[j] +
						         1 ) * msg->tail.message.pack.list.std[i].elem_size;
						body = ((char *) (msg->tail.message.pack.list.std[i].addr)) +
							msg->tail.message.pack.list.std[i].begins[j] *
							msg->tail.message.pack.list.std[i].elem_size;
						 
						if ( total + size > max_size )
						{
							skip = 1;
							size = max_size - total;
						}

						pos++;
						result = (struct iovec*) sctk_realloc(result, pos*sizeof(struct iovec));
						result[pos-1].iov_len = size;
						result[pos-1].iov_base = body;  
						total += size;
						assume ( total <= max_size );
					}
				}
			}
			*iovlen = pos;

			break;
		}

		case MPC_LOWCOMM_MESSAGE_PACK_ABSOLUTE:
		{
			int pos;
			char skip = 0;
			size_t i, j, size, total;
			void * body;

			total = 0;
			*iovlen = 0;
			pos = 0;

			if ( SCTK_MSG_SIZE ( msg ) > 0 )
			{
				for ( i = 0; ((i < msg->tail.message.pack.count) && !skip ); i++ )
				{
					for ( j = 0; ((j < msg->tail.message.pack.list.absolute[i].count) && !skip ); j++ )
					{
						size = ( msg->tail.message.pack.list.absolute[i].ends[j] -
						         msg->tail.message.pack.list.absolute[i].begins[j] +
						         1 ) * msg->tail.message.pack.list.absolute[i].elem_size;
						body = (( char * ) (msg->tail.message.pack.list.absolute[i].addr )) +
						                 msg->tail.message.pack.list.absolute[i].begins[j] *
						                 msg->tail.message.pack.list.absolute[i].elem_size;
						
						if ( total + size > max_size )
						{
							skip = 1;
							size = max_size - total;
						}
						pos++;
						result = (struct iovec*) sctk_realloc(result, pos*sizeof(struct iovec));
						result[pos-1].iov_len = size;
						result[pos-1].iov_base = body;  
						total += size;
						assume ( total <= max_size );
					}
				}
			}
			*iovlen = pos;

			break;
		}

		default:
			not_reachable();
	}
	
	return result;
}

void sctk_net_copy_msg_from_iovec( mpc_lowcomm_ptp_message_content_to_copy_t *tmp, sctk_iovec_cpy_t driver_func )  
{
	int iovlen;
	mpc_lowcomm_ptp_message_t *send;
	mpc_lowcomm_ptp_message_t *recv;
	struct iovec * result;

	send = tmp->msg_send;
	recv = tmp->msg_recv;

	SCTK_MSG_COMPLETION_FLAG_SET ( send , NULL );
	
	/* MPI 1.3 : The length of the received message must be less than or equal 
		     to the length of the receive buffer */
	assume_m( SCTK_MSG_SIZE ( send ) <= SCTK_MSG_SIZE( recv ), "NORME 1.3 ...");

	if ( SCTK_MSG_SIZE ( send ) > 0 )
	{
		result = sctk_net_convert_msg_to_iovec(recv, &iovlen, SCTK_MSG_SIZE(recv));	
		driver_func(result, iovlen, send);
		sctk_free(result);		
	}
	_mpc_comm_ptp_message_commit_request(send,recv);
}

void sctk_net_read_in_fd ( mpc_lowcomm_ptp_message_t *msg,
                           int fd )
{
	TODO ( "Deal with partial reception" )

	switch ( msg->tail.message_type )
	{
		case MPC_LOWCOMM_MESSAGE_CONTIGUOUS:
		{
			size_t size;

			size = SCTK_MSG_SIZE ( msg );

			mpc_common_nodebug ( "MSG SEND |%s|", ( char * ) msg->tail.message.contiguous.addr );
			mpc_common_io_safe_read ( fd, msg->tail.message.contiguous.addr, size );
			break;
		}

		case MPC_LOWCOMM_MESSAGE_NETWORK:
		{
			size_t size;
			void *body;

			size = SCTK_MSG_SIZE ( msg );
			body = ( char * ) msg + sizeof ( mpc_lowcomm_ptp_message_t );

			mpc_common_io_safe_read ( fd, body, size );
			break;
		}

		case MPC_LOWCOMM_MESSAGE_PACK:
		{
			size_t i;
			size_t j;
			size_t size;
			size = SCTK_MSG_SIZE ( msg );

			if ( size > 0 )
			{
				for ( i = 0; i < msg->tail.message.pack.count; i++ )
					for ( j = 0; j < msg->tail.message.pack.list.std[i].count; j++ )
					{
						size = ( msg->tail.message.pack.list.std[i].ends[j] -
						         msg->tail.message.pack.list.std[i].begins[j] +
						         1 ) * msg->tail.message.pack.list.std[i].elem_size;
						mpc_common_io_safe_read ( fd, ( ( char * ) ( msg->tail.message.pack.list.std[i].addr ) ) +
						                 msg->tail.message.pack.list.std[i].begins[j] *
						                 msg->tail.message.pack.list.std[i].elem_size, size );
					}
			}

			break;
		}

		case MPC_LOWCOMM_MESSAGE_PACK_ABSOLUTE:
		{
			size_t i;
			size_t j;
			size_t size;
			size = SCTK_MSG_SIZE ( msg );

			if ( size > 0 )
			{
				for ( i = 0; i < msg->tail.message.pack.count; i++ )
					for ( j = 0; j < msg->tail.message.pack.list.absolute[i].count; j++ )
					{
						size = ( msg->tail.message.pack.list.absolute[i].ends[j] -
						         msg->tail.message.pack.list.absolute[i].begins[j] +
						         1 ) * msg->tail.message.pack.list.absolute[i].elem_size;
						mpc_common_io_safe_read ( fd, ( ( char * ) ( msg->tail.message.pack.list.absolute[i].addr ) ) +
						                 msg->tail.message.pack.list.absolute[i].begins[j] *
						                 msg->tail.message.pack.list.absolute[i].elem_size, size );
					}
			}

			break;
		}

		default:
			not_reachable();
	}
}

void sctk_net_write_in_fd ( mpc_lowcomm_ptp_message_t *msg,
                            int fd )
{
	switch ( msg->tail.message_type )
	{
		case MPC_LOWCOMM_MESSAGE_CONTIGUOUS:
		{
			size_t size;

			size = SCTK_MSG_SIZE ( msg );

			mpc_common_nodebug ( "MSG SEND |%s|", ( char * ) msg->tail.message.contiguous.addr );

			mpc_common_io_safe_write ( fd, msg->tail.message.contiguous.addr, size );
			break;
		}

		case MPC_LOWCOMM_MESSAGE_NETWORK:
		{
			size_t size;
			void *body;

			size = SCTK_MSG_SIZE ( msg );
			body = ( char * ) msg + sizeof ( mpc_lowcomm_ptp_message_t );

			mpc_common_io_safe_write ( fd, body, size );
			break;
		}

		case MPC_LOWCOMM_MESSAGE_PACK:
		{
			size_t i;
			size_t j;
			size_t size;
			size = SCTK_MSG_SIZE ( msg );

			if ( size > 0 )
			{
				for ( i = 0; i < msg->tail.message.pack.count; i++ )
					for ( j = 0; j < msg->tail.message.pack.list.std[i].count; j++ )
					{
						size = ( msg->tail.message.pack.list.std[i].ends[j] -
						         msg->tail.message.pack.list.std[i].begins[j] +
						         1 ) * msg->tail.message.pack.list.std[i].elem_size;
						mpc_common_io_safe_write ( fd, ( ( char * ) ( msg->tail.message.pack.list.std[i].addr ) ) +
						                  msg->tail.message.pack.list.std[i].begins[j] *
						                  msg->tail.message.pack.list.std[i].elem_size, size );
					}
			}

			break;
		}

		case MPC_LOWCOMM_MESSAGE_PACK_ABSOLUTE:
		{
			size_t i;
			size_t j;
			size_t size;
			size = SCTK_MSG_SIZE ( msg );

			if ( size > 0 )
			{
				for ( i = 0; i < msg->tail.message.pack.count; i++ )
					for ( j = 0; j < msg->tail.message.pack.list.absolute[i].count; j++ )
					{
						size = ( msg->tail.message.pack.list.absolute[i].ends[j] -
						         msg->tail.message.pack.list.absolute[i].begins[j] +
						         1 ) * msg->tail.message.pack.list.absolute[i].elem_size;
						mpc_common_io_safe_write ( fd, ( ( char * ) ( msg->tail.message.pack.list.absolute[i].addr ) ) +
						                  msg->tail.message.pack.list.absolute[i].begins[j] *
						                  msg->tail.message.pack.list.absolute[i].elem_size, size );
					}
			}

			break;
		}

		default:
			not_reachable();
	}
}

void sctk_get_iovec_in_buffer( mpc_lowcomm_ptp_message_t *msg, struct iovec **iov, int *iovlen )
{

	switch ( msg->tail.message_type )
	{
		case MPC_LOWCOMM_MESSAGE_CONTIGUOUS:
		{
			*iov = (struct iovec *) sctk_malloc( sizeof( struct iovec ) );
			*iovlen = 1;
			( *iov )->iov_base = msg->tail.message.contiguous.addr;
			( *iov )->iov_len = SCTK_MSG_SIZE( msg );
			break;
		}

		case MPC_LOWCOMM_MESSAGE_NETWORK:
		{
			*iov = (struct iovec *) sctk_malloc( sizeof( struct iovec ) );
			*iovlen = 1;
			( *iov )->iov_base = (char *) msg + sizeof( mpc_lowcomm_ptp_message_t );
			( *iov )->iov_len = SCTK_MSG_SIZE( msg );
			break;
		}

		default:
			abort();
	}
}


void sctk_net_copy_in_buffer ( mpc_lowcomm_ptp_message_t *msg,
                               char *buffer )
{
	switch ( msg->tail.message_type )
	{
		case MPC_LOWCOMM_MESSAGE_CONTIGUOUS:
		{
			size_t size;

			size = SCTK_MSG_SIZE ( msg );
			mpc_common_nodebug ( "MSG SEND |%s|", ( char * ) msg->tail.message.contiguous.addr );
			memcpy ( buffer, msg->tail.message.contiguous.addr, size );
			buffer += size;
			break;
		}

		case MPC_LOWCOMM_MESSAGE_NETWORK:
		{
			size_t size;
			void *body;

			size = SCTK_MSG_SIZE ( msg );
			body = ( char * ) msg + sizeof ( mpc_lowcomm_ptp_message_t );

			memcpy ( buffer, body, size );
			buffer += size;
			break;
		}

		case MPC_LOWCOMM_MESSAGE_PACK:
		{
			size_t i;
			size_t j;
			size_t size;
			size = SCTK_MSG_SIZE ( msg );

			if ( size > 0 )
			{
				for ( i = 0; i < msg->tail.message.pack.count; i++ )
					for ( j = 0; j < msg->tail.message.pack.list.std[i].count; j++ )
					{
						size = ( msg->tail.message.pack.list.std[i].ends[j] -
						         msg->tail.message.pack.list.std[i].begins[j] +
						         1 ) * msg->tail.message.pack.list.std[i].elem_size;
						memcpy ( buffer, ( ( char * ) ( msg->tail.message.pack.list.std[i].addr ) ) +
						         msg->tail.message.pack.list.std[i].begins[j] *
						         msg->tail.message.pack.list.std[i].elem_size, size );
						buffer += size;
					}
			}

			break;
		}

		case MPC_LOWCOMM_MESSAGE_PACK_ABSOLUTE:
		{
			size_t i;
			size_t j;
			size_t size;
			size = SCTK_MSG_SIZE ( msg );

			if ( size > 0 )
			{
				for ( i = 0; i < msg->tail.message.pack.count; i++ )
					for ( j = 0; j < msg->tail.message.pack.list.absolute[i].count; j++ )
					{
						size = ( msg->tail.message.pack.list.absolute[i].ends[j] -
						         msg->tail.message.pack.list.absolute[i].begins[j] +
						         1 ) * msg->tail.message.pack.list.absolute[i].elem_size;
						memcpy ( buffer, ( ( char * ) ( msg->tail.message.pack.list.absolute[i].addr ) ) +
						         msg->tail.message.pack.list.absolute[i].begins[j] *
						         msg->tail.message.pack.list.absolute[i].elem_size, size );
						buffer += size;
					}
			}

			break;
		}

		default:
			not_reachable();
	}
}


void *sctk_net_if_one_msg_in_buffer (  __UNUSED__ mpc_lowcomm_ptp_message_t *msg )
{
	not_implemented();
	return NULL;
}



size_t sctk_net_determine_message_size ( mpc_lowcomm_ptp_message_t *
                                         msg )
{
	return SCTK_MSG_SIZE ( msg );
}


int sctk_net_copy_frag_msg (
    const mpc_lowcomm_ptp_message_t *msg,
    char *buffer,
    const size_t curr_copy,
    const size_t max_copy )
{
	size_t tmp_size;

	switch ( msg->tail.message_type )
	{
		case MPC_LOWCOMM_MESSAGE_CONTIGUOUS:
		{
			size_t size;
			size = SCTK_MSG_SIZE ( msg );

			if ( ( size - curr_copy ) > max_copy )
				tmp_size = max_copy;
			else
				tmp_size = ( size - curr_copy );

			memcpy ( ( char * ) buffer + curr_copy,
			         ( char * ) msg->tail.message.contiguous.addr + curr_copy,
			         tmp_size );
			break;
		}

		case MPC_LOWCOMM_MESSAGE_NETWORK:
		{
			size_t size;
			void *body;

			size = SCTK_MSG_SIZE ( msg );
			body = ( char * ) msg + sizeof ( mpc_lowcomm_ptp_message_t );

			memcpy ( buffer, body, size );
			buffer += size;
			break;
		}

		case MPC_LOWCOMM_MESSAGE_PACK:
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
					memcpy ( buffer, ( ( char * ) ( msg->tail.message.pack.list.std[i].addr ) ) +
					         msg->tail.message.pack.list.std[i].begins[j] *
					         msg->tail.message.pack.list.std[i].elem_size, size );
					buffer += size;
				}

			break;
		}

		case MPC_LOWCOMM_MESSAGE_PACK_ABSOLUTE:
		{
			size_t i;
			size_t j;
			size_t size;
			not_implemented();

			for ( i = 0; i < msg->tail.message.pack.count; i++ )
				for ( j = 0; j < msg->tail.message.pack.list.absolute[i].count; j++ )
				{
					size = ( msg->tail.message.pack.list.absolute[i].ends[j] -
					         msg->tail.message.pack.list.absolute[i].begins[j] +
					         1 ) * msg->tail.message.pack.list.absolute[i].elem_size;
					memcpy ( buffer, ( ( char * ) ( msg->tail.message.pack.list.absolute[i].addr ) ) +
					         msg->tail.message.pack.list.absolute[i].begins[j] *
					         msg->tail.message.pack.list.absolute[i].elem_size, size );
					buffer += size;
				}

			break;
		}

		default:
			not_reachable();
	}

	not_implemented();
	return 0;
}

void sctk_net_message_copy ( mpc_lowcomm_ptp_message_content_to_copy_t *tmp )
{
	mpc_lowcomm_ptp_message_t *send;
	mpc_lowcomm_ptp_message_t *recv;
	char *body;

	send = tmp->msg_send;
	recv = tmp->msg_recv;

	body = ( char * ) send + sizeof ( mpc_lowcomm_ptp_message_t );

	SCTK_MSG_COMPLETION_FLAG_SET ( send , NULL );

        //	mpc_common_debug_error( "HERE REQ is %p", recv->tail.request );

        mpc_common_nodebug("MSG |%s|", (char *)body);

        switch (recv->tail.message_type) {
        case MPC_LOWCOMM_MESSAGE_CONTIGUOUS: {
          size_t size;
          size = SCTK_MSG_SIZE(send);
          size =
              mpc_common_min(SCTK_MSG_SIZE(send), recv->tail.message.contiguous.size);

          memcpy(recv->tail.message.contiguous.addr, body, size);

          _mpc_comm_ptp_message_commit_request(send, recv);
          break;
        }

        case MPC_LOWCOMM_MESSAGE_PACK: {
          size_t i;
          size_t j;
          size_t size;
          size_t total = 0;
          size_t recv_size = 0;
          size = SCTK_MSG_SIZE(send);

          if (size > 0) {
            for (i = 0; i < recv->tail.message.pack.count; i++) {
              for (j = 0; j < recv->tail.message.pack.list.std[i].count; j++) {
                size = (recv->tail.message.pack.list.std[i].ends[j] -
                        recv->tail.message.pack.list.std[i].begins[j] + 1) *
                       recv->tail.message.pack.list.std[i].elem_size;
                recv_size += size;
              }
            }

            /* MPI 1.3 : The length of the received message must be less than or
             * equal to the length of the receive buffer */
            assume(SCTK_MSG_SIZE(send) <= recv_size);
            char skip = 0;

            for (i = 0; ((i < recv->tail.message.pack.count) && !skip); i++) {
              for (j = 0;
                   ((j < recv->tail.message.pack.list.std[i].count) && !skip);
                   j++) {
                size = (recv->tail.message.pack.list.std[i].ends[j] -
                        recv->tail.message.pack.list.std[i].begins[j] + 1) *
                       recv->tail.message.pack.list.std[i].elem_size;

                if (total + size > SCTK_MSG_SIZE(send)) {
                  skip = 1;
                  size = SCTK_MSG_SIZE(send) - total;
                }

                memcpy(((char *)(recv->tail.message.pack.list.std[i].addr)) +
                           recv->tail.message.pack.list.std[i].begins[j] *
                               recv->tail.message.pack.list.std[i].elem_size,
                       body, size);
                body += size;
                total += size;
                assume(total <= SCTK_MSG_SIZE(send));
              }
            }
          }

          _mpc_comm_ptp_message_commit_request(send, recv);
          break;
        }

        case MPC_LOWCOMM_MESSAGE_PACK_ABSOLUTE: {
          size_t i;
          size_t j;
          size_t size;
          size_t total = 0;
          size_t recv_size = 0;
          size = SCTK_MSG_SIZE(send);

          if (size > 0) {
            for (i = 0; i < recv->tail.message.pack.count; i++) {
              for (j = 0; j < recv->tail.message.pack.list.absolute[i].count;
                   j++) {
                size =
                    (recv->tail.message.pack.list.absolute[i].ends[j] -
                     recv->tail.message.pack.list.absolute[i].begins[j] + 1) *
                    recv->tail.message.pack.list.absolute[i].elem_size;
                recv_size += size;
              }
            }

            /* MPI 1.3 : The length of the received message must be less than or
             * equal to the length of the receive buffer */
            assume(SCTK_MSG_SIZE(send) <= recv_size);
            char skip = 0;

            for (i = 0; ((i < recv->tail.message.pack.count) && !skip); i++) {
              for (j = 0;
                   ((j < recv->tail.message.pack.list.absolute[i].count) &&
                    !skip);
                   j++) {
                size =
                    (recv->tail.message.pack.list.absolute[i].ends[j] -
                     recv->tail.message.pack.list.absolute[i].begins[j] + 1) *
                    recv->tail.message.pack.list.absolute[i].elem_size;

                if (total + size > SCTK_MSG_SIZE(send)) {
                  skip = 1;
                  size = SCTK_MSG_SIZE(send) - total;
                }

                memcpy(
                    ((char *)(recv->tail.message.pack.list.absolute[i].addr)) +
                        recv->tail.message.pack.list.absolute[i].begins[j] *
                            recv->tail.message.pack.list.absolute[i].elem_size,
                    body, size);
                body += size;
                total += size;
                assume(total <= SCTK_MSG_SIZE(send));
              }
            }
          }

          _mpc_comm_ptp_message_commit_request(send, recv);
          break;
        }

        default:
          not_reachable();
        }
}

void sctk_net_message_copy_from_buffer ( char *body,
                                         mpc_lowcomm_ptp_message_content_to_copy_t *tmp, char free_headers )
{
	mpc_lowcomm_ptp_message_t *send;
	mpc_lowcomm_ptp_message_t *recv;

	mpc_common_nodebug ( "MSG RECV |%s|", ( char * ) body );
	send = tmp->msg_send;
	recv = tmp->msg_recv;

	switch ( recv->tail.message_type )
	{
		case MPC_LOWCOMM_MESSAGE_CONTIGUOUS:
		{
			size_t size;
			size = SCTK_MSG_SIZE ( send );
			size = mpc_common_min ( SCTK_MSG_SIZE ( send ), recv->tail.message.contiguous.size );

			memcpy ( recv->tail.message.contiguous.addr, body, size );

			if ( free_headers )
				_mpc_comm_ptp_message_commit_request ( send, recv );

			break;
		}

		case MPC_LOWCOMM_MESSAGE_PACK:
		{
			size_t i;
			size_t j;
			size_t size;
			size = SCTK_MSG_SIZE ( send );

			if ( size > 0 )
			{
				size_t total = 0;
				char skip = 0;

				for ( i = 0; i < recv->tail.message.pack.count; i++ )
					for ( j = 0; ( ( j < recv->tail.message.pack.list.std[i].count ) && !skip ); j++ )
					{
						size = ( recv->tail.message.pack.list.std[i].ends[j] -
						         recv->tail.message.pack.list.std[i].begins[j] +
						         1 ) * recv->tail.message.pack.list.std[i].elem_size;

						mpc_common_nodebug("%p - %p \n", recv->tail.message.pack.list.std[i].begins[j], recv->tail.message.pack.list.std[i].ends[j]); 
						if ( total + size > SCTK_MSG_SIZE ( send ) )
						{
							skip = 1;
							size = SCTK_MSG_SIZE ( send ) - total;
						}

						memcpy ( ( ( char * ) ( recv->tail.message.pack.list.std[i].addr ) ) +
						         recv->tail.message.pack.list.std[i].begins[j] *
						         recv->tail.message.pack.list.std[i].elem_size, body, size );
						body += size;
						total += size;
						assume ( total <= SCTK_MSG_SIZE ( send ) );
					}
			}

			if ( free_headers )
				_mpc_comm_ptp_message_commit_request ( send, recv );

			break;
		}

		case MPC_LOWCOMM_MESSAGE_PACK_ABSOLUTE:
		{
			size_t i;
			size_t j;
			size_t size;
			size = SCTK_MSG_SIZE ( send );

			if ( size > 0 )
			{
				size_t total = 0;
				char skip = 0;

				for ( i = 0; i < recv->tail.message.pack.count; i++ )
					for ( j = 0; ( ( j < recv->tail.message.pack.list.absolute[i].count ) && !skip ); j++ )
					{
						size = ( recv->tail.message.pack.list.absolute[i].ends[j] -
						         recv->tail.message.pack.list.absolute[i].begins[j] +
						         1 ) * recv->tail.message.pack.list.absolute[i].elem_size;

						mpc_common_nodebug("%p - %p \n", recv->tail.message.pack.list.std[i].begins[j], recv->tail.message.pack.list.std[i].ends[j]); 
						if ( total + size > SCTK_MSG_SIZE ( send ) )
						{
							skip = 1;
							size = SCTK_MSG_SIZE ( send ) - total;
						}

						memcpy ( ( ( char * ) ( recv->tail.message.pack.list.absolute[i].addr ) ) +
						         recv->tail.message.pack.list.absolute[i].begins[j] *
						         recv->tail.message.pack.list.absolute[i].elem_size, body, size );
						body += size;
						total += size;
						assume ( total <= SCTK_MSG_SIZE ( send ) );
					}
			}

			if ( free_headers )
				_mpc_comm_ptp_message_commit_request ( send, recv );

			break;
		}

		default:
			not_reachable();
	}
}

/* void mpc_common_io_safe_write(int fd, char* buf,size_t size){ */
/*   size_t done = 0;  */
/*   int res; */
/*   do{ */
/*     res = write(fd,buf + done ,size - done); */
/*     if(res < 0){ */
/*       perror("Write error"); */
/*       mpc_common_debug_abort(); */
/*     } */
/*     done += res; */
/*   }while(done < size); */
/* } */

/* void mpc_common_io_safe_read(int fd, char* buf,size_t size){ */
/*   size_t done = 0;  */
/*   int res; */
/*   do{ */
/*     res = read(fd,buf + done,size - done); */
/*     if(res < 0){ */
/*       perror("Read error"); */
/*       mpc_common_debug_abort(); */
/*     } */
/*     done += res; */
/*   }while(done < size); */
/* } */
