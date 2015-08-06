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
/* #   - BESNARD Jean-Baptiste jbbesnard@paratools.fr                     # */
/* #                                                                      # */
/* ######################################################################## */

#include "sctk_window.h"
#include "sctk_inter_thread_comm.h"
#include "uthash.h"
#include <sctk_alloc.h>
#include <sctk_control_messages.h>

/************************************************************************/
/* Window Numbering and translation                                     */
/************************************************************************/

sctk_spinlock_t __window_ht_lock = SCTK_SPINLOCK_INITIALIZER;
unsigned int __current_win_id = 0;

struct win_id_to_win
{
	int id;
	struct sctk_window win;
	UT_hash_handle hh; /**< This dummy data structure is required by UTHash is order to make this data structure hashable */ 
};

/* The ID to win translation HT */
struct win_id_to_win * __id_to_win_ht = NULL;

static struct sctk_window * sctk_win_register()
{
	struct win_id_to_win * new = sctk_malloc( sizeof( struct win_id_to_win ) );
	assume( new );
	
	sctk_spinlock_lock( &__window_ht_lock );
	
	new->id = __current_win_id++;
	memset( &new->win, 0, sizeof( struct sctk_window ) );
	new->win.id = new->id;
	
	HASH_ADD_INT( __id_to_win_ht, id, new );

	sctk_spinlock_unlock( &__window_ht_lock );
	
	return &new->win;
}

static struct sctk_window * sctk_win_translate( sctk_window_t win_id )
{
	static struct sctk_window * ret = NULL;
	struct win_id_to_win * cell = NULL;
	
	sctk_spinlock_lock( &__window_ht_lock );
	HASH_FIND_INT(__id_to_win_ht, &win_id, cell);
	
	if( cell )
	{
		ret = &cell->win;
	}
	
	sctk_spinlock_unlock( &__window_ht_lock );
	
	return ret;
}

static void sctk_win_delete( struct sctk_window * win )
{
	struct win_id_to_win * cell = NULL;
	
	sctk_spinlock_lock( &__window_ht_lock );
	HASH_FIND_INT(__id_to_win_ht, &win->id, cell);
	
	if( cell )
	{
		HASH_DEL(__id_to_win_ht, cell);
		sctk_free( cell );
	}
	
	sctk_spinlock_unlock( &__window_ht_lock );
}

/************************************************************************/
/* Definition of an RDMA window                                         */
/************************************************************************/

static void _sctk_win_acquire( struct sctk_window * win )
{
	sctk_spinlock_lock( &win->lock );
	
	win->refcounter++;

	sctk_spinlock_unlock( &win->lock );
}

void sctk_win_acquire( sctk_window_t win_id )
{
	struct sctk_window * win = sctk_win_translate( win_id );
	
	if( !win )
	{
		sctk_fatal("No such window ID");
	}
	
	_sctk_win_acquire( win );
}


static int _sctk_win_relax( struct sctk_window * win )
{
	int ret = 0;

	sctk_spinlock_lock( &win->lock );
	
	win->refcounter--;
	ret = win->refcounter;

	sctk_spinlock_unlock( &win->lock );	
	
	return ret;
}


int sctk_win_relax( sctk_window_t win_id )
{
	struct sctk_window * win = sctk_win_translate( win_id );
	
	if( !win )
	{
		sctk_fatal("No such window ID");
	}
	
	return _sctk_win_relax( win );
}


sctk_window_t sctk_window_init( void *addr, size_t size, size_t disp_unit, sctk_communicator_t comm )
{
	struct sctk_window * win = sctk_win_register();
	
	/* We are local and therefore have no remote id */
	win->remote_id = -1;
	
	/* Set window owner task */
	win->owner = sctk_get_task_rank();
	win->comm = comm;
	
	/* Save CTX */
	win->start_addr = addr;
	win->size = size;
	win->disp_unit = disp_unit;
	
	sctk_rail_info_t * rdma_rail = sctk_rail_get_rdma ();
	
	if( !rdma_rail )
	{
		/* No RDMA rail flag the window as emulated */
		win->is_emulated = 1;
	}
	else
	{
		/* We have a RDMA rail no need to emulate */
		win->is_emulated = 0;
		/* We need to pin the window */
		sctk_rail_pin_ctx_init( &win->pin, addr, size );
	}
	
	/* Set refcounter */
	win->lock = SCTK_SPINLOCK_INITIALIZER;
	win->refcounter = 1;

	return win->id;
}


void sctk_window_release( sctk_window_t win_id, void *addr, size_t size, size_t disp_unit )
{
	struct sctk_window * win = sctk_win_translate( win_id );
	
	if( !win )
	{
		sctk_fatal("No such window ID");
	}	
	
	/* Do we need to signal a remote win ? */
	if( win->owner != sctk_get_task_rank() )
	{
		/* This is a remote window sigal release
		 * to remote rank */
		assume( 0 <= win->remote_id );
		
		/* Signal release to remote */
		sctk_control_messages_send ( sctk_get_process_rank_from_task_rank ( win->owner ), SCTK_CONTROL_MESSAGE_PROCESS, SCTK_PROCESS_RDMA_WIN_RELAX, 0, NULL, 0  );
	}

	/* Now work on the local window */
	
	/* Decrement refcounter */
	int refcount = _sctk_win_relax( win );
	
	/* Did the refcounter reach "0" ? */
	if( refcount )
		return;
	
	/* If we are at "0" we free the window */
	
	if( ! win->is_emulated )
	{
		/* If we were not emulated, unpin */
		sctk_rail_pin_ctx_release( &win->pin );
	}
	
	/* Remove the window from the translation table */
	sctk_win_delete( win );
	
	/* Free content */
	memset( win, 0, sizeof( struct sctk_window ) );
}


int sctk_window_map_remote( int remote_rank, sctk_window_t win_id )
{
	struct sctk_window_map_request mr;
	sctk_window_map_request_init( &mr, remote_rank, win_id );

	struct sctk_window * win = sctk_win_translate( win_id );

	if(!win)
	{
		/* No such window */
		return -1;
	}

	/* Case where we are local */
	if( (mr.source_rank == mr.remote_rank)
	||  (! sctk_is_net_message( mr.remote_rank ) ) ) /* If the target is in the same proces
					          * just work locally */
	{	
		
		/* Increment refcount */
		_sctk_win_acquire( win );
		
		/* Return the same window */
		return win_id;
	}
	
	struct sctk_window remote_win_data;
	memset( &remote_win_data, 0, sizeof( struct sctk_window ) );
	
	/* Send a map request to remote task */
	sctk_control_messages_send ( sctk_get_process_rank_from_task_rank (remote_rank) , SCTK_CONTROL_MESSAGE_PROCESS, SCTK_PROCESS_RDMA_WIN_MAPTO, 0, &mr, sizeof(struct sctk_window_map_request)  );
	
	/* And now prepare to receive the remote win data */
	sctk_request_t req;

	sctk_message_irecv_class( remote_rank, &remote_win_data, sizeof(struct sctk_window) , TAG_RDMA_MAP, SCTK_COMM_WORLD, SCTK_RDMA_WINDOW_MESSAGES, &req );
	sctk_wait_message ( &req );
	
	if( remote_win_data.id < 0 )
	{
		/* No such remote */
		return -1;
	}
	
	/* If we are here we received the remote and it existed */
	
	/* Create a new window */
	struct sctk_window * new_win = sctk_win_register();
	
	int local_id = new_win->id;
	
	/* Copy remote inside it */
	memcpy( new_win, &remote_win_data, sizeof( struct sctk_window ) );
	
	/* Update refcounting (1 as it is a window mirror) */
	new_win->refcounter = 1;
	new_win->lock = SCTK_SPINLOCK_INITIALIZER;
	
	/* Restore local ID overwritten by the copy and save remote id */
	new_win->remote_id = new_win->id;
	
	assume( 0 <= new_win->remote_id );
	
	new_win->id = local_id;
	
	return new_win->id;
}

/* Control messages handler */

void sctk_window_map_remote_ctrl_msg_handler( struct sctk_window_map_request * mr )
{
	sctk_request_t req;
	struct sctk_window * win = sctk_win_translate( mr->win_id );

	/* Note that here we are called outside of a task context
	 * therefore we have to manually retrieve the remote task
	 * id for the map request to fill the messages accordingly 
	 * using sctk_message_isend_class_src */

	if(! win )
	{
		/* We did not find the requested win
		 * send a dummy window with a negative ID*/
		struct sctk_window dummy_win;
		memset( &dummy_win, 0, sizeof( struct sctk_window ) );
		dummy_win.id = -1;
		
		/* Send the dummy win */
		sctk_message_isend_class_src( mr->remote_rank, mr->source_rank, &dummy_win, sizeof(struct sctk_window) , TAG_RDMA_MAP, SCTK_COMM_WORLD, SCTK_RDMA_WINDOW_MESSAGES, &req );
		sctk_wait_message ( &req );
		
		/* Done */
		return;
	}
	
	/* We have a local window matching */
	
	/* Increment its refcounter */
	_sctk_win_acquire( win );
	
	/* Send local win infos to remote */

	sctk_message_isend_class_src( mr->remote_rank, mr->source_rank, win, sizeof(struct sctk_window) , TAG_RDMA_MAP, SCTK_COMM_WORLD, SCTK_RDMA_WINDOW_MESSAGES, &req );
	sctk_wait_message ( &req );
	/* DONE */
}

void sctk_window_relax_ctrl_msg_handler( sctk_window_t win_id )
{
	struct sctk_window * win = sctk_win_translate( win_id );
	
	if( !win )
	{
		sctk_warning("No such window %d in remote relax", win_id);
		return;
	}
	
	_sctk_win_relax( win );
}

void sctk_window_RDMA_emulated_write_ctrl_msg_handler( struct sctk_window_emulated_RDMA *erma )
{
	struct sctk_window * win = sctk_win_translate( erma->win_id );
	
	if( !win )
	{
		sctk_fatal("No such window in emulated RDMA write");
	}
	
	size_t offset = erma->offset * win->disp_unit;

	if( win->size < ( offset + erma->size ) )
	{
		sctk_fatal("Error RDMA emulated write operation overflows the window");
	} 
	
	//sctk_error("HANDLER WRITE RECV %d -> %d", erma->source_rank, erma->remote_rank );
	
	sctk_request_t data_req;
	
	/* Receive data */
	sctk_message_irecv_class_dest( erma->source_rank, erma->remote_rank, win->start_addr + offset, erma->size , TAG_RDMA_WRITE, win->comm, SCTK_RDMA_WINDOW_MESSAGES, &data_req );
	sctk_wait_message ( &data_req );
}

void sctk_window_RDMA_emulated_read_ctrl_msg_handler( struct sctk_window_emulated_RDMA *erma )
{
	struct sctk_window * win = sctk_win_translate( erma->win_id );
	
	if( !win )
	{
		sctk_fatal("No such window in emulated RDMA write");
	}
	
	size_t offset = erma->offset * win->disp_unit;

	if( win->size < ( offset + erma->size ) )
	{
		sctk_fatal("Error RDMA emulated write operation overflows the window");
	} 
	
	sctk_request_t data_req;
	sctk_message_isend_class_src( erma->remote_rank, erma->source_rank, win->start_addr + offset, erma->size , TAG_RDMA_READ, win->comm, SCTK_RDMA_WINDOW_MESSAGES, &data_req );
	sctk_wait_message ( &data_req );
}

/************************************************************************/
/* Window Operations                                                    */
/************************************************************************/

void sctk_window_RDMA_write_local( struct sctk_window * win,  void * src_addr, size_t size, size_t dest_offset )
{

	size_t offset = dest_offset * win->disp_unit;

	if( win->size < ( offset + size ) )
	{ 
		sctk_fatal("Error RDMA write operation overflows the window");
	} 

	/* Do the local RDMA */
	memcpy( win->start_addr + offset, src_addr, size );
}

void sctk_window_RDMA_write_net( struct sctk_window * win, void * src_addr, size_t size, size_t dest_offset, sctk_request_t  * req  )
{
	sctk_rail_info_t * rdma_rail = sctk_rail_get_rdma ();
	
	if( !rdma_rail->rdma_write )
	{
		sctk_fatal("This rails despite flagged RDMA did not define rdma_write");
	}


	void * dest_address = win->start_addr + win->disp_unit * dest_offset;
	void * win_end_addr = win->start_addr + win->size;
	
	
	if( win_end_addr < (dest_address + size ) )
	{
		sctk_fatal("Error RDMA write operation overflows the window");
	}
	
	sctk_error("RDMA WRITE NET");
	
	/* Pin local segment */
	sctk_rail_pin_ctx_t local_pin;
	sctk_rail_pin_ctx_init( &local_pin, src_addr, size );
	
	sctk_thread_ptp_message_t * msg = sctk_create_header (win->owner,SCTK_MESSAGE_CONTIGUOUS);

	sctk_set_header_in_message (msg, -8, win->comm, sctk_get_task_rank (), win->owner, req, size,SCTK_RDMA_MESSAGE, SCTK_DATATYPE_IGNORE );

	rdma_rail->rdma_write( rdma_rail, msg, src_addr, local_pin.list, dest_address, win->pin.list, size );
	
	/* Note that we use the cache here */
	sctk_rail_pin_ctx_release( &local_pin );
}


void sctk_window_RDMA_write( sctk_window_t win_id, void * src_addr, size_t size, size_t dest_offset, sctk_request_t  * req  )
{
	struct sctk_window * win = sctk_win_translate( win_id );
	
	sctk_error("RDMA WRITE");
	
	if( !win )
	{
		sctk_fatal("No such window ID");
	}
	
	/* Set an empty request */
	sctk_init_request(req, win->comm, REQUEST_NULL );

	int my_rank = sctk_get_task_rank();
	
	if( (my_rank == win->owner) /* Same rank */
	||  (! sctk_is_net_message( win->owner ) ) /* Same process */  )
	{
		/* Shared Memory */
		sctk_window_RDMA_write_local( win,  src_addr, size,  dest_offset );
		return;
	}
	else if( win->is_emulated )
	{
		/* Emulated write using control messages */
		struct sctk_window_emulated_RDMA erma;
		sctk_window_emulated_RDMA_init( &erma, win->owner, dest_offset, size, win->remote_id );
		sctk_control_messages_send ( sctk_get_process_rank_from_task_rank (win->owner), SCTK_CONTROL_MESSAGE_PROCESS, SCTK_PROCESS_RDMA_EMULATED_WRITE, 0, &erma, sizeof(struct sctk_window_emulated_RDMA) );
		
		/* Note that we store the data transfer req in the request */
		sctk_message_isend_class( win->owner, src_addr, size , TAG_RDMA_WRITE, win->comm, SCTK_RDMA_WINDOW_MESSAGES, req );
	}
	else
	{
		sctk_window_RDMA_write_net( win, src_addr, size, dest_offset, req  );
	}
	
}

void sctk_window_RDMA_read_local( struct sctk_window * win,  void * dest_addr, size_t size, size_t src_offset )
{
	size_t offset = src_offset * win->disp_unit;

	if( win->size < ( offset + size ) )
	{
		sctk_fatal("Error RDMA write operation overflows the window");
	} 

	/* Do the local RDMA */
	memcpy( dest_addr, win->start_addr + offset, size );
}

void sctk_window_RDMA_read_net( struct sctk_window * win, void * dest_addr, size_t size, size_t src_offset, sctk_request_t  * req  )
{
	sctk_rail_info_t * rdma_rail = sctk_rail_get_rdma ();
	
	if( !rdma_rail->rdma_write )
	{
		sctk_fatal("This rails despite flagged RDMA did not define rdma_write");
	}


	void * src_address = win->start_addr + win->disp_unit * src_offset;
	void * win_end_addr = win->start_addr + win->size;
	
	
	if( win_end_addr < (src_address + size ) )
	{
		sctk_fatal("Error RDMA write operation overflows the window");
	}
	
	
	/* Pin local segment */
	sctk_rail_pin_ctx_t local_pin;
	sctk_rail_pin_ctx_init( &local_pin, dest_addr, size );
	
	sctk_thread_ptp_message_t * msg = sctk_create_header (win->owner,SCTK_MESSAGE_CONTIGUOUS);

	sctk_set_header_in_message (msg, -8, win->comm,  win->owner, sctk_get_task_rank (), req, size,SCTK_RDMA_MESSAGE, SCTK_DATATYPE_IGNORE );

	rdma_rail->rdma_read( rdma_rail, msg, src_address, win->pin.list, dest_addr, local_pin.list, size );

	
	/* Note that we use the cache here */
	sctk_rail_pin_ctx_release( &local_pin );
}



void sctk_window_RDMA_read( sctk_window_t win_id, void * dest_addr, size_t size, size_t src_offset, sctk_request_t  * req )
{
	struct sctk_window * win = sctk_win_translate( win_id );
	
	if( !win )
	{
		sctk_fatal("No such window ID");
	}

	/* Set an empty request */
	sctk_init_request(req, win->comm, REQUEST_NULL );

	int my_rank = sctk_get_task_rank();
	
	if( (my_rank == win->owner) /* Same rank */
	||  (! sctk_is_net_message( win->owner ) ) /* Same process */  )
	{
		/* Shared Memory */
		sctk_window_RDMA_read_local( win,  dest_addr, size,  src_offset );
		return;
	}
	else if( win->is_emulated )
	{
		/* Emulated write using control messages */
		struct sctk_window_emulated_RDMA erma;
		sctk_window_emulated_RDMA_init( &erma, win->owner, src_offset, size, win->remote_id );
		sctk_control_messages_send ( sctk_get_process_rank_from_task_rank (win->owner), SCTK_CONTROL_MESSAGE_PROCESS, SCTK_PROCESS_RDMA_EMULATED_READ, 0, &erma, sizeof(struct sctk_window_emulated_RDMA) );
		
		/* Note that we store the data transfer req in the request */
		sctk_message_irecv_class( win->owner, dest_addr, size , TAG_RDMA_READ, win->comm, SCTK_RDMA_WINDOW_MESSAGES, req );
	}
	else
	{	
		sctk_window_RDMA_read_net( win, dest_addr, size, src_offset, req  );
	}
}


void sctk_window_RDMA_fence( sctk_window_t win_id )
{
	struct sctk_window * win = sctk_win_translate( win_id );
	
	if( !win )
	{
		sctk_fatal("No such window ID");
	}	
	
	int my_rank = sctk_get_task_rank();
	
	if( (my_rank ==  win->owner)
	||  (! sctk_is_net_message( win->owner ) ) )
	{
		/* Nothing to do all operations are local */
		return;
	}
	else if( win->is_emulated )
	{
		/* In this case we must make sure that the control
		 * message list is flushed before leaving the fence
		 * also the fence is remote we synchornise with a
		 * sendrecv with the remote window */
		 sctk_control_message_fence( win->owner );
	}
	else
	{
		/* TODO */
	}
	
	
	
	
}
