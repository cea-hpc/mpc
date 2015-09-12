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
#include <sctk_atomics.h>
#include <sctk_spinlock.h>
#include <sctk_wchar.h>

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
	
	sctk_nodebug("%s WIN %d on %d is %p", __FUNCTION__, win_id, sctk_get_task_rank() , ret);

	
	return ret;
}

static void sctk_win_delete( struct sctk_window * win )
{
	struct win_id_to_win * cell = NULL;
	
	sctk_spinlock_lock( &__window_ht_lock );
	HASH_FIND_INT(__id_to_win_ht, &win->id, cell);
	
	if( cell )
	{
		sctk_nodebug("%s on %d", __FUNCTION__, sctk_get_task_rank() );
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
	sctk_nodebug("%s win_id %d on %d", __FUNCTION__, win->id, sctk_get_task_rank() );
	int ret = 0;

	sctk_spinlock_lock( &win->lock );
	
	win->refcounter--;
	ret = win->refcounter;

	sctk_spinlock_unlock( &win->lock );	
	
	return ret;
}


int sctk_win_relax( sctk_window_t win_id )
{
	sctk_error("%s", __FUNCTION__ );
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

	sctk_nodebug("CREATING WIN (%p) %d on %d RC %d", win, win->id, sctk_get_task_rank(), win->refcounter );

	return win->id;
}


void sctk_window_release( sctk_window_t win_id  )
{
	struct sctk_window * win = sctk_win_translate( win_id );
	
	sctk_nodebug("%s %p win_id %d on %d RC %d", __FUNCTION__, win, win_id, sctk_get_task_rank() , win->refcounter);
	
	if( !win )
	{
		sctk_fatal("No such window ID");
	}	
	
	/* Do we need to signal a remote win ? */
	if( (win->owner != sctk_get_task_rank())
	&&   sctk_is_net_message( win->owner ) )
	{
		sctk_nodebug("Remote is %d on %d", win->remote_id, win->owner  );
		/* This is a remote window sigal release
		 * to remote rank */
		assume( 0 <= win->remote_id );
		
		/* Signal release to remote */
		sctk_control_messages_send ( sctk_get_process_rank_from_task_rank ( win->owner ), SCTK_CONTROL_MESSAGE_PROCESS, SCTK_PROCESS_RDMA_WIN_RELAX, 0, (void *)&win->remote_id, sizeof(int)  );
	}

	/* Now work on the local window */
	
	/* Decrement refcounter */
	int refcount = _sctk_win_relax( win );
	
	/* Did the refcounter reach "0" ? */
	if( refcount != 0 )
	{
		sctk_nodebug("NO REL %s win_id %d on %d (RC %d)", __FUNCTION__, win_id, sctk_get_task_rank(), refcount );
		return;
	}
	
	sctk_nodebug("REL %s win_id %d on %d (RC %d)", __FUNCTION__, win_id, sctk_get_task_rank(), refcount );
	
	/* If we are at "0" we free the window */
	
	/* Make sure that the window is not emulated and
	 * that it is not a remote one (pinned remotely) */
	if( (!win->is_emulated) && (win->remote_id < 0 ) )
	{
		/* If we were not emulated, unpin */
		sctk_rail_pin_ctx_release( &win->pin );
	}

	/* Free content */
	sctk_win_delete( win );
}


int sctk_window_map_remote( int remote_rank, sctk_window_t win_id )
{
	sctk_nodebug("%s winid is %d", __FUNCTION__, win_id );
	
	struct sctk_window_map_request mr;
	sctk_window_map_request_init( &mr, remote_rank, win_id );

	/* Case where we are local */
	if( (mr.source_rank == mr.remote_rank)
	||  (! sctk_is_net_message( mr.remote_rank ) ) ) /* If the target is in the same proces
					          * just work locally */
	{	
		
		struct sctk_window * win = sctk_win_translate( win_id );

		if(!win)
		{
			/* No such window */
			sctk_error("No such win");
			return -1;
		}
		
		/* Increment refcount */
		_sctk_win_acquire( win );
		
		sctk_nodebug("Win is local");
		
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
		sctk_error("NO SUCH remote");
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
	
	sctk_nodebug("%s win is %p returining (RC %d) ID %d", __FUNCTION__, new_win, new_win->refcounter, new_win->remote_id );
	
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

	sctk_nodebug("%s win is %p target is %d (RC %d)", __FUNCTION__, win, win_id , win->refcounter);
	
	if( !win )
	{
		sctk_warning("No such window %d in remote relax", win_id);
		return;
	}

	sctk_window_release( win_id );
}



/************************************************************************/
/* Window Operations                                                    */
/************************************************************************/


/* WRITE ======================= */


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

void sctk_window_RDMA_write_net( struct sctk_window * win, sctk_rail_pin_ctx_t * src_pin, void * src_addr, size_t size, size_t dest_offset, sctk_request_t  * req  )
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
	
	int did_local_pin = 0;
	
	/* Pin local segment */
	if( !src_pin )
	{
		sctk_rail_pin_ctx_t local_pin;
		src_pin = &local_pin;
		sctk_rail_pin_ctx_init( src_pin, src_addr, size );
		did_local_pin = 1;
	}
	
	sctk_thread_ptp_message_t * msg = sctk_create_header (win->owner,SCTK_MESSAGE_CONTIGUOUS);

	sctk_set_header_in_message (msg, -8, win->comm, sctk_get_task_rank (), win->owner, req, size,SCTK_RDMA_MESSAGE, SCTK_DATATYPE_IGNORE );

	rdma_rail->rdma_write( rdma_rail, msg, src_addr, src_pin->list, dest_address, win->pin.list, size );
	
	if( did_local_pin )
	{
		/* Note that we use the cache here */
		sctk_rail_pin_ctx_release( src_pin );
	}
}


void __sctk_window_RDMA_write( sctk_window_t win_id, sctk_rail_pin_ctx_t * src_pin, void * src_addr, size_t size, size_t dest_offset, sctk_request_t  * req  )
{
	struct sctk_window * win = sctk_win_translate( win_id );
	
	sctk_error("RDMA WRITE");
	
	if( !win )
	{
		sctk_fatal("No such window ID");
	}
	
	/* Set an empty request */
	sctk_init_request(req, win->comm, REQUEST_SEND );

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
		sctk_window_RDMA_write_net( win, src_pin, src_addr, size, dest_offset, req  );
	}
	
}


void sctk_window_RDMA_write( sctk_window_t win_id, void * src_addr, size_t size, size_t dest_offset, sctk_request_t  * req  )
{
	__sctk_window_RDMA_write( win_id, NULL, src_addr, size, dest_offset,  req  );
}


void sctk_window_RDMA_write_win( sctk_window_t src_win_id, size_t src_offset, size_t size,  sctk_window_t dest_win_id, size_t dest_offset, sctk_request_t  * req  )
{
	struct sctk_window * src_win = sctk_win_translate( src_win_id );

	if( !src_win )
	{
		sctk_fatal("No such window ID");
	}
	
	void * src_addr = src_win->start_addr + src_offset * src_win->disp_unit;
	void * win_end_addr = src_win->start_addr + src_win->size;
	
	
	if( win_end_addr < (src_addr + size ) )
	{
		sctk_fatal("Error RDMA write operation overflows the window");
	}		
	
	__sctk_window_RDMA_write( dest_win_id, &src_win->pin, src_addr, size, dest_offset,  req  );
}





/* READ ======================= */

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
		sctk_fatal("Error RDMA emulated write operation overflows the window\n"
		           " WIN S : %ld , offset %ld, disp %ld, actual off %ld",
		            win->size, erma->offset, win->disp_unit, offset);
	}
	
	sctk_request_t data_req;
	sctk_message_isend_class_src( erma->remote_rank, erma->source_rank, win->start_addr + offset, erma->size , TAG_RDMA_READ, win->comm, SCTK_RDMA_WINDOW_MESSAGES, &data_req );
	sctk_wait_message ( &data_req );
}


void sctk_window_RDMA_read_local( struct sctk_window * win,  void * dest_addr, size_t size, size_t src_offset )
{
	size_t offset = src_offset * win->disp_unit;

	if( win->size < ( offset + size ) )
	{
		sctk_fatal("Error RDMA read operation overflows the window");
	} 

	/* Do the local RDMA */
	memcpy( dest_addr, win->start_addr + offset, size );
}

void sctk_window_RDMA_read_net( struct sctk_window * win, sctk_rail_pin_ctx_t * dest_pin, void * dest_addr, size_t size, size_t src_offset, sctk_request_t  * req  )
{
	sctk_rail_info_t * rdma_rail = sctk_rail_get_rdma ();
	
	if( !rdma_rail->rdma_write )
	{
		sctk_fatal("This rails despite flagged RDMA did not define rdma_write");
	}
	

	void * src_address = win->start_addr + win->disp_unit * src_offset;

	size_t offset = src_offset * win->disp_unit;

	if( win->size < ( offset + size ) )
	{
		sctk_fatal("Error RDMA read operation overflows the window");
	} 

	int did_pin = 0;

	/* Pin local segment */
	if( ! dest_pin )
	{
		sctk_rail_pin_ctx_t local_pin;
		dest_pin = &local_pin;
		sctk_rail_pin_ctx_init( dest_pin, dest_addr, size );
		did_pin = 1;
	}
	
	sctk_thread_ptp_message_t * msg = sctk_create_header (win->owner,SCTK_MESSAGE_CONTIGUOUS);

	sctk_set_header_in_message (msg, -8, win->comm,  win->owner, sctk_get_task_rank (), req, size,SCTK_RDMA_MESSAGE, SCTK_DATATYPE_IGNORE );

	rdma_rail->rdma_read( rdma_rail, msg, src_address, win->pin.list, dest_addr, dest_pin->list, size );


	if( did_pin )
	{
		/* Note that we use the cache here */
		sctk_rail_pin_ctx_release( dest_pin );
	}
}

void __sctk_window_RDMA_read( sctk_window_t win_id, sctk_rail_pin_ctx_t * dest_pin, void * dest_addr, size_t size, size_t src_offset, sctk_request_t  * req )
{
	struct sctk_window * win = sctk_win_translate( win_id );
	
	sctk_error("RDMA READ");
	
	if( !win )
	{
		sctk_fatal("No such window ID %d", win_id);
	}

	/* Set an empty request */
	sctk_init_request(req, win->comm, REQUEST_RECV );

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
		sctk_window_RDMA_read_net( win, dest_pin, dest_addr, size, src_offset, req  );
	}
}

void sctk_window_RDMA_read( sctk_window_t win_id, void * dest_addr, size_t size, size_t src_offset, sctk_request_t  * req )
{
	__sctk_window_RDMA_read( win_id, NULL, dest_addr, size, src_offset, req );
}


void sctk_window_RDMA_read_win( sctk_window_t src_win_id, size_t src_offset, size_t size, sctk_window_t dest_win_id, size_t dest_offset, sctk_request_t  * req )
{
	struct sctk_window * dest_win = sctk_win_translate( dest_win_id );

	if( !dest_win )
	{
		sctk_fatal("No such window ID");
	}
	
	void * dest_addr = dest_win->start_addr + dest_offset * dest_win->disp_unit;
	void * win_end_addr = dest_win->start_addr + dest_win->size;
	
	
	if( win_end_addr < (dest_addr + size ) )
	{
		sctk_fatal("Error RDMA write operation overflows the window");
	}		
	
	__sctk_window_RDMA_read( src_win_id, &dest_win->pin, dest_addr, size, src_offset, req );
}




/* Fetch and Add =================== */


size_t RDMA_type_size( RDMA_type type )
{
	switch( type )
	{
		case RDMA_TYPE_CHAR:
			return sizeof( char );
		case RDMA_TYPE_DOUBLE:
			return sizeof(double);
		case RDMA_TYPE_FLOAT:
			return sizeof(float);
		case RDMA_TYPE_INT:
			return sizeof(int);
		case RDMA_TYPE_LONG:
			return sizeof(long);
		case RDMA_TYPE_LONG_DOUBLE:
			return sizeof(long double);
		case RDMA_TYPE_LONG_LONG:
			return sizeof(long long);
		case RDMA_TYPE_LONG_LONG_INT:
			return sizeof( long long int );
		case RDMA_TYPE_SHORT:
			return sizeof( short );
		case RDMA_TYPE_SIGNED_CHAR:
			return sizeof( signed char );
		case RDMA_TYPE_UNSIGNED:
			return sizeof( unsigned );
		case RDMA_TYPE_UNSIGNED_CHAR:
			return sizeof( unsigned char );
		case RDMA_TYPE_UNSIGNED_LONG:
			return sizeof( unsigned long );
		case RDMA_TYPE_UNSIGNED_LONG_LONG:
			return sizeof( unsigned long long );
		case RDMA_TYPE_UNSIGNED_SHORT:
			return sizeof( unsigned short );
		case RDMA_TYPE_WCHAR:
			return sizeof( sctk_wchar_t );
	}
	
	sctk_fatal("Found an unhandled RDMA datatype");
}



void sctk_window_fetch_and_op_operate_int( RDMA_op op, void * add, void * src, void * dest )
{
	int result = 0;
	
	int val = 0;
	
	int * to_add = ((int *) add );
	
	switch( op )
	{
		case RDMA_SUM:
			result = sctk_atomics_fetch_and_add_int( src, *to_add );
			break;
			
		case RDMA_INC:
			result = sctk_atomics_fetch_and_incr_int( src);
			break;
			
		case RDMA_DEC:
			result = sctk_atomics_fetch_and_decr_int( src );
			break;
		
		case RDMA_MIN :
			val = sctk_atomics_load_int( src );
			
			if( *to_add < val )
			{
				sctk_atomics_store_int( src, *to_add );
			}
			
			result = val;
			break;
		case RDMA_MAX :
			val = sctk_atomics_load_int( src );
			
			if( val < *to_add )
			{
				sctk_atomics_store_int( src, *to_add );
			}
			
			result = val;
			break;
		case RDMA_PROD :
			val = sctk_atomics_load_int( src );
			sctk_atomics_store_int( src, *to_add * val );
			result = val;
			break;
		case RDMA_LAND :
			val = sctk_atomics_load_int( src );
			sctk_atomics_store_int( src, *to_add && val );
			result = val;
			break;

		case RDMA_BAND :
			val = sctk_atomics_load_int( src );
			sctk_atomics_store_int( src, *to_add & val );
			result = val;
			break;

		case RDMA_LOR :
			val = sctk_atomics_load_int( src );
			sctk_atomics_store_int( src, *to_add || val );
			result = val;
			break;

		case RDMA_BOR :
			val = sctk_atomics_load_int( src );
			sctk_atomics_store_int( src, *to_add | val );
			result = val;
			break;

		case RDMA_LXOR :
			val = sctk_atomics_load_int( src );
			sctk_atomics_store_int( src, *to_add != val );
			result = val;
			break;

		case RDMA_BXOR :
			val = sctk_atomics_load_int( src );
			sctk_atomics_store_int( src, *to_add ^ val );
			result = val;
			break;

	}

	memcpy( dest, &result, sizeof( sctk_atomics_int ) );
}

#define RDMA_OP_def( type, type2, type3 ) \
static sctk_spinlock_t __RDMA_OP_ ## type ## type2 ## type3 ## _lock = SCTK_SPINLOCK_INITIALIZER;\
\
void sctk_window_fetch_and_op_operate_ ## type ## type2 ## type3 ## _( RDMA_op op, void * add, void * src, void * dest )\
{\
	type type2 type3 result;\
	type type2 type3 * to_add = ( type type2 type3 *) add;\
	type type2 type3 * src_addr = ( type type2 type3 *) src;\
	type type2 type3 * dest_addr = ( type type2 type3 *)dest;\
\
	sctk_spinlock_lock( &__RDMA_OP_ ## type ## type2 ## type3  ##_lock );\
	\
	*dest_addr = *src_addr;\
	\
	switch( op )\
	{\
		case RDMA_SUM:\
			*src_addr = *src_addr + *to_add;\
			break;\
			\
		case RDMA_INC:\
			*src_addr = *src_addr + 1;\
			break;\
			\
		case RDMA_DEC:\
			*src_addr = *src_addr - 1;\
			break;\
		\
		case RDMA_MIN:\
			if( *to_add < *src_addr )\
			{\
				*src_addr = *to_add;\
			}\
			break;\
		case RDMA_MAX :\
			if( *src_addr < *to_add )\
			{\
				*src_addr = *to_add;\
			}\
			break;\
		case RDMA_PROD :\
			*src_addr *= *to_add;\
			break;\
		case RDMA_LAND :\
			*src_addr = *src_addr && *to_add;\
			break;\
\
		case RDMA_BAND :\
			*src_addr = *src_addr & *to_add;\
			break;\
\
		case RDMA_LOR :\
			*src_addr = *src_addr || *to_add;\
			break;\
\
		case RDMA_BOR :\
			*src_addr = *src_addr | *to_add;\
			break;\
\
		case RDMA_LXOR :\
			*src_addr = *src_addr != *to_add;\
			break;\
\
		case RDMA_BXOR :\
			*src_addr = *src_addr ^ *to_add;\
			break;\
\
	}\
\
	sctk_spinlock_unlock( &__RDMA_OP_ ## type ## type2 ## type3 ##_lock );\
}

#define RDMA_OP_def_nobin( type, type2 ) \
static sctk_spinlock_t __RDMA_OP_ ## type ## type2 ## _lock = SCTK_SPINLOCK_INITIALIZER;\
\
void sctk_window_fetch_and_op_operate_ ## type ## type2 ## _( RDMA_op op, void * add, void * src, void * dest )\
{\
	type type2 result;\
	type type2 * to_add = ( type type2 *) add;\
	type type2 * src_addr = ( type type2 *) src;\
	type type2 * dest_addr = ( type type2 *)dest;\
\
	sctk_spinlock_lock( &__RDMA_OP_ ## type ## type2 ##_lock );\
	\
	*dest_addr = *src_addr;\
	\
	switch( op )\
	{\
		case RDMA_SUM:\
			*src_addr += *to_add;\
			break;\
			\
		case RDMA_INC:\
			*src_addr = *src_addr + 1;\
			break;\
			\
		case RDMA_DEC:\
			*src_addr = *src_addr - 1;\
			break;\
		\
		case RDMA_MIN:\
			if( *to_add < *src_addr )\
			{\
				*src_addr = *to_add;\
			}\
			break;\
		case RDMA_MAX :\
			if( *src_addr < *to_add )\
			{\
				*src_addr = *to_add;\
			}\
			break;\
		case RDMA_PROD :\
			*src_addr *= *to_add;\
			break;\
		case RDMA_LAND :\
			*src_addr = *src_addr && *to_add;\
			break;\
\
		case RDMA_BAND :\
			sctk_fatal("RDMA Binary operand is not defined for %s", #type);\
			break;\
\
		case RDMA_LOR :\
			*src_addr = *src_addr || *to_add;\
			break;\
\
		case RDMA_BOR :\
			sctk_fatal("RDMA Binary operand is not defined for %s", #type);\
			break;\
\
		case RDMA_LXOR :\
			*src_addr = *src_addr != *to_add;\
			break;\
\
		case RDMA_BXOR :\
			sctk_fatal("RDMA Binary operand is not defined for %s", #type);\
			break;\
\
	}\
\
	sctk_spinlock_unlock( &__RDMA_OP_ ## type ## type2 ##_lock );\
}


RDMA_OP_def( char, , )
RDMA_OP_def_nobin( double, )
RDMA_OP_def_nobin( float, )
RDMA_OP_def( long, , )
RDMA_OP_def_nobin( long, double )
RDMA_OP_def( long, long ,)
RDMA_OP_def( long, long , int )
RDMA_OP_def( short, ,)
RDMA_OP_def( signed, char,)
RDMA_OP_def( unsigned, ,)
RDMA_OP_def( unsigned, char, )
RDMA_OP_def( unsigned, long , )
RDMA_OP_def( unsigned, long , long )
RDMA_OP_def( unsigned, short , )
RDMA_OP_def( sctk_wchar_t, , )

void sctk_window_fetch_and_op_operate( RDMA_op op, RDMA_type type, void * add, void * src, void * dest )
{
	switch( type )
	{
		case RDMA_TYPE_CHAR:
			sctk_window_fetch_and_op_operate_char_( op, add, src, dest );
			return;
		case RDMA_TYPE_DOUBLE:
			sctk_window_fetch_and_op_operate_double_( op, add, src, dest );
			return;
		case RDMA_TYPE_FLOAT:
			sctk_window_fetch_and_op_operate_float_( op, add, src, dest );
			return;
		case RDMA_TYPE_INT:
			sctk_window_fetch_and_op_operate_int( op, add, src, dest );
			return;
		case RDMA_TYPE_LONG:
			sctk_window_fetch_and_op_operate_long_( op, add, src, dest );
			return;
		case RDMA_TYPE_LONG_DOUBLE:
			sctk_window_fetch_and_op_operate_longdouble_( op, add, src, dest );
			return;
		case RDMA_TYPE_LONG_LONG:
			sctk_window_fetch_and_op_operate_longlong_( op, add, src, dest );
			return;
		case RDMA_TYPE_LONG_LONG_INT:
			sctk_window_fetch_and_op_operate_longlongint_( op, add, src, dest );
			return;
		case RDMA_TYPE_SHORT:
			sctk_window_fetch_and_op_operate_short_( op, add, src, dest );
			return;
		case RDMA_TYPE_SIGNED_CHAR:
			sctk_window_fetch_and_op_operate_signedchar_( op, add, src, dest );
			return;
		case RDMA_TYPE_UNSIGNED:
			sctk_window_fetch_and_op_operate_unsigned_( op, add, src, dest );
			return;
		case RDMA_TYPE_UNSIGNED_CHAR:
			sctk_window_fetch_and_op_operate_unsignedchar_( op, add, src, dest );
			return;
		case RDMA_TYPE_UNSIGNED_LONG:
			sctk_window_fetch_and_op_operate_long_( op, add, src, dest );
			return;
		case RDMA_TYPE_UNSIGNED_LONG_LONG:
			sctk_window_fetch_and_op_operate_unsignedlonglong_( op, add, src, dest );
			return;
		case RDMA_TYPE_UNSIGNED_SHORT:
			return;
			sctk_window_fetch_and_op_operate_unsignedshort_( op, add, src, dest );
		case RDMA_TYPE_WCHAR:
			sctk_window_fetch_and_op_operate_sctk_wchar_t_( op, add, src, dest );
			return;
	}
}



void sctk_window_RDMA_fetch_and_op_ctrl_msg_handler( struct sctk_window_emulated_fetch_and_op_RDMA *fop )
{
	struct sctk_window * win = sctk_win_translate( fop->rdma.win_id );
	
	if( !win )
	{
		sctk_fatal("No such window in emulated RDMA fetch and op");
	}
	
	size_t offset = fop->rdma.offset * win->disp_unit;

	if( win->size < ( offset + fop->rdma.size ) )
	{
		sctk_fatal("Error RDMA emulated feth anb op operation overflows the window\n"
		           " WIN S : %ld , offset %ld, disp %ld, actual off %ld",
		            win->size, fop->rdma.offset, win->disp_unit, offset);
	}
	
	char fetch[16];
	
	sctk_window_fetch_and_op_operate( fop->op, fop->type, fop->add, win->start_addr + offset, &fetch );
	
	sctk_request_t data_req;
	sctk_message_isend_class_src( fop->rdma.remote_rank, fop->rdma.source_rank, &fetch, fop->rdma.size , TAG_RDMA_FETCH_AND_OP, win->comm, SCTK_RDMA_MESSAGE, &data_req );
	sctk_wait_message ( &data_req );
}



void sctk_window_RDMA_fetch_and_op_local( sctk_window_t remote_win_id, size_t remote_offset,  void * fetch_addr, void * add, RDMA_op op, RDMA_type type, sctk_request_t  * req )
{
	struct sctk_window * win = sctk_win_translate( remote_win_id );
	
	//sctk_error("Fetch and add");
	
	if( !win )
	{
		sctk_fatal("No such window ID %d", remote_win_id);
	}

	size_t offset = remote_offset * win->disp_unit;

	if( win->size < ( offset + RDMA_type_size( type ) ) )
	{
		sctk_fatal("Error RDMA read operation overflows the window");
	}

	void * remote_addr = win->start_addr + offset;
	
	sctk_window_fetch_and_op_operate( op, type, add, remote_addr, fetch_addr );
}


void sctk_window_RDMA_fetch_and_op_net( sctk_window_t remote_win_id, size_t remote_offset,  sctk_rail_pin_ctx_t * fetch_pin, void * fetch_addr,  void * add, RDMA_op op, RDMA_type type, sctk_request_t  * req )
{
	sctk_rail_info_t * rdma_rail = sctk_rail_get_rdma ();
	
	if( !rdma_rail->rdma_fetch_and_op )
	{
		sctk_fatal("This rails despite flagged RDMA did not define fetch and op");
	}

	struct sctk_window * win = sctk_win_translate( remote_win_id );
	
	if( !win )
	{
		sctk_fatal("No such window ID %d", remote_win_id);
	}

	void * src_address = win->start_addr + win->disp_unit * remote_offset;

	size_t offset = remote_offset * win->disp_unit;

	if( win->size < ( offset + RDMA_type_size( type ) ) )
	{
		sctk_fatal("Error RDMA read operation overflows the window");
	} 

	int did_pin = 0;

	/* Pin local segment */
	if( ! fetch_pin )
	{
		sctk_rail_pin_ctx_t local_pin;
		fetch_pin = &local_pin;
		sctk_rail_pin_ctx_init( fetch_pin, fetch_addr, RDMA_type_size( type ) );
		did_pin = 1;
	}
	
	sctk_thread_ptp_message_t * msg = sctk_create_header (win->owner,SCTK_MESSAGE_CONTIGUOUS);

	sctk_set_header_in_message (msg, -8, win->comm,  win->owner, sctk_get_task_rank (), req, RDMA_type_size( type ), SCTK_RDMA_MESSAGE, SCTK_DATATYPE_IGNORE );

	rdma_rail->rdma_fetch_and_op(  rdma_rail,
							       msg,
							       fetch_addr,
							       fetch_pin->list,
							       src_address,
							       win->pin.list,
							       add,
							       op,
							       type );

	if( did_pin )
	{
		/* Note that we use the cache here */
		sctk_rail_pin_ctx_release( fetch_pin );
	}
}



void __sctk_window_RDMA_fetch_and_op( sctk_window_t remote_win_id, size_t remote_offset, sctk_rail_pin_ctx_t * fetch_pin, void * fetch_addr, void * add, RDMA_op op,  RDMA_type type, sctk_request_t  * req )
{
	struct sctk_window * win = sctk_win_translate( remote_win_id );
	
	//sctk_error("Fetch and add");
	
	if( !win )
	{
		sctk_fatal("No such window ID %d", remote_win_id);
	}

	/* Now try to see if we pass the RDMA rail gate function for fetch and op */

	sctk_rail_info_t * rdma_rail = sctk_rail_get_rdma ();
	
	int RDMA_fetch_and_op_gate_passed = 0;
	
	if( rdma_rail )
	{
		if( rdma_rail->rdma_fetch_and_op_gate )
		{
			RDMA_fetch_and_op_gate_passed = (rdma_rail->rdma_fetch_and_op_gate)( rdma_rail, RDMA_type_size( type ), op, type );
			/* At this point if RDMA_fetch_and_op_gate_passed == 1 , the NET can handle this fetch and op */
		}
	}

	/* Set an empty request */
	sctk_init_request(req, win->comm, REQUEST_RECV );

	int my_rank = sctk_get_task_rank();
	
	if( (my_rank == win->owner) /* Same rank */
	||  (! sctk_is_net_message( win->owner ) ) /* Same process */  )
	{
		/* Shared Memory */
		sctk_window_RDMA_fetch_and_op_local( remote_win_id, remote_offset, fetch_addr, add, op, type, req );
		return;
	}
	else if( win->is_emulated || (RDMA_fetch_and_op_gate_passed == 0 /* Network does not support this RDMA atomic fallback to emulated */) )
	{
		struct sctk_window_emulated_fetch_and_op_RDMA fop;
		sctk_window_emulated_fetch_and_op_RDMA_init( &fop, win->owner, remote_offset, win->remote_id, op, type, add );
		
		sctk_control_messages_send ( sctk_get_process_rank_from_task_rank (win->owner), SCTK_CONTROL_MESSAGE_PROCESS, SCTK_PROCESS_RDMA_EMULATED_FETCH_AND_OP, 0, &fop, sizeof(struct sctk_window_emulated_fetch_and_op_RDMA) );

		/* Note that we store the data transfer req in the request */
		sctk_message_irecv_class( win->owner, fetch_addr, fop.rdma.size , TAG_RDMA_FETCH_AND_OP, win->comm, SCTK_RDMA_MESSAGE, req );
	}
	else
	{
		sctk_window_RDMA_fetch_and_op_net( remote_win_id, remote_offset,  fetch_pin, fetch_addr,  add, op, type,  req );
	}
}


void sctk_window_RDMA_fetch_and_op( sctk_window_t remote_win_id, size_t remote_offset, void * fetch_addr, void * add, RDMA_op op,  RDMA_type type, sctk_request_t  * req )
{
	__sctk_window_RDMA_fetch_and_op( remote_win_id, remote_offset, NULL, fetch_addr, add, op, type, req );	
}


void sctk_window_RDMA_fetch_and_add_win( sctk_window_t remote_win_id, size_t remote_offset, sctk_window_t local_win_id, size_t fetch_offset, void * add, RDMA_op op,  RDMA_type type, sctk_request_t  * req )
{
struct sctk_window * local_win = sctk_win_translate( local_win_id );

	if( !local_win )
	{
		sctk_fatal("No such window ID");
	}
	
	void * dest_addr = local_win->start_addr + fetch_offset * local_win->disp_unit;
	void * win_end_addr = local_win->start_addr + local_win->size;
	
	
	if( win_end_addr < (dest_addr + RDMA_type_size( type ) ) )
	{
		sctk_fatal("Error RDMA write operation overflows the window");
	}
	
	
	__sctk_window_RDMA_fetch_and_op( remote_win_id, remote_offset, &local_win->pin, dest_addr, add, op, type, req );

}


/* Compare and SWAP =================== */


void sctk_window_RDMA_CAS_int_local( void * cmp , void * new, void * target )
{
	int oldv = *((int *)cmp );
	int newv = *((int *)new );
	sctk_atomics_int * ptarget = (sctk_atomics_int *)target;
	
	sctk_atomics_cas_int(ptarget, oldv, newv);
}

void sctk_window_RDMA_CAS_ptr_local( void * cmp , void * new, void * target )
{
	sctk_atomics_ptr * ptarget = (sctk_atomics_ptr *)target;
	sctk_atomics_cas_ptr(ptarget, cmp, new);
}

static sctk_spinlock_t __RDMA_CAS_16_lock = SCTK_SPINLOCK_INITIALIZER;

void sctk_window_RDMA_CAS_16_local( void * cmp , void * new, void * target )
{
	sctk_spinlock_lock( &__RDMA_CAS_16_lock );
	
	if( memcmp( target, cmp, 16 ) == 0 )
	{
		memcpy( target, new, 16 );
	}
	
	sctk_spinlock_unlock( &__RDMA_CAS_16_lock );
}

static sctk_spinlock_t __RDMA_CAS_any_lock = SCTK_SPINLOCK_INITIALIZER;

void sctk_window_RDMA_CAS_any_local( void * cmp , void * new, void * target, size_t size )
{
	sctk_spinlock_lock( &__RDMA_CAS_any_lock );
	
	if( memcmp( target, cmp, size ) == 0 )
	{
		memcpy( target, new, size );
	}
	
	sctk_spinlock_unlock( &__RDMA_CAS_any_lock );
}


void sctk_window_RDMA_CAS_local( sctk_window_t remote_win_id, size_t remote_offset, void * comp, void * new_data,  RDMA_type type )
{
	struct sctk_window * win = sctk_win_translate( remote_win_id );

	if( !win )
	{
		sctk_fatal("No such window ID %d", remote_win_id);
	}

	size_t offset = remote_offset * win->disp_unit;

	if( win->size < ( offset + RDMA_type_size( type ) ) )
	{
		sctk_fatal("Error RDMA read operation overflows the window");
	}

	void * remote_addr = (sctk_atomics_int *)(win->start_addr + offset);
	
	size_t type_size = RDMA_type_size( type );
	
	if( type_size == sizeof( sctk_atomics_int ) )
	{
		sctk_window_RDMA_CAS_int_local( comp , new_data, remote_addr );
	}
	else if( type_size == sizeof( sctk_atomics_ptr ) )
	{
		sctk_window_RDMA_CAS_ptr_local( comp , new_data, remote_addr );
	}
	else if( type_size == 16 )
	{
		sctk_window_RDMA_CAS_16_local( comp , new_data, remote_addr );
	}
	else
	{
		sctk_window_RDMA_CAS_any_local( comp , new_data, remote_addr,  type_size);
	}

}



void sctk_window_RDMA_CAS_ctrl_msg_handler( struct sctk_window_emulated_CAS_RDMA *fcas )
{	
	sctk_window_RDMA_CAS_local( fcas->rdma.win_id, fcas->rdma.offset, fcas->comp, fcas->new,  fcas->type );
}


void sctk_window_RDMA_CAS_net( sctk_window_t remote_win_id, size_t remote_offset, void * comp, void * new_data,  RDMA_type type, sctk_request_t  * req )
{
	sctk_rail_info_t * rdma_rail = sctk_rail_get_rdma ();
	
	if( !rdma_rail->rdma_cas )
	{
		sctk_fatal("This rails despite flagged RDMA did not define CAS");
	}

	struct sctk_window * win = sctk_win_translate( remote_win_id );
	
	if( !win )
	{
		sctk_fatal("No such window ID %d", remote_win_id);
	}

	void * dest_addr = win->start_addr + win->disp_unit * remote_offset;

	size_t offset = remote_offset * win->disp_unit;

	if( win->size < ( offset + RDMA_type_size( type ) ) )
	{
		sctk_fatal("Error RDMA CAS operation overflows the window");
	} 

	sctk_thread_ptp_message_t * msg = sctk_create_header (win->owner,SCTK_MESSAGE_CONTIGUOUS);

	sctk_set_header_in_message (msg, -8, win->comm,  win->owner, sctk_get_task_rank (), req, RDMA_type_size( type ), SCTK_RDMA_MESSAGE, SCTK_DATATYPE_IGNORE );

	sctk_error("CAS GOT NET");

	rdma_rail->rdma_cas(  rdma_rail,
						   msg,
						   dest_addr,
						   win->pin.list,
						   comp,
						   new_data,
						   type );

}




void sctk_window_RDMA_CAS( sctk_window_t remote_win_id, size_t remote_offset, void * comp, void * new_data,  RDMA_type type, sctk_request_t  * req )
{
	struct sctk_window * win = sctk_win_translate( remote_win_id );
	
	sctk_error("CAS");
	
	if( !win )
	{
		sctk_fatal("No such window ID %d", remote_win_id);
	}

	/* Now try to see if we pass the RDMA rail gate function for fetch and op */

	sctk_rail_info_t * rdma_rail = sctk_rail_get_rdma ();
	
	int RDMA_CAS_gate_passed = 0;
	
	if( rdma_rail )
	{
		if( rdma_rail->rdma_cas_gate )
		{
			RDMA_CAS_gate_passed = (rdma_rail->rdma_cas_gate)( rdma_rail, RDMA_type_size( type ), type );
			/* At this point if RDMA_CAS_gate_passed == 1 , the NET can handle CAS */
		}
	}

	/* Set an empty request */
	sctk_init_request(req, win->comm, REQUEST_RECV );

	int my_rank = sctk_get_task_rank();
	
	if( (my_rank == win->owner) /* Same rank */
	||  (! sctk_is_net_message( win->owner ) ) /* Same process */  )
	{
		sctk_error("CAS SHARED");
	
		
		/* Shared Memory */
		sctk_window_RDMA_CAS_local( remote_win_id, remote_offset, comp, new_data,  type );
		return;
	}
	else if( win->is_emulated || (RDMA_CAS_gate_passed == 0 /* Network does not support this RDMA atomic fallback to emulated */) )
	{
		sctk_error("CAS EMU");
		struct sctk_window_emulated_CAS_RDMA fcas;
		sctk_window_emulated_CAS_RDMA_init( &fcas, win->owner, remote_offset, win->remote_id, type, comp, new_data );

		sctk_control_messages_send ( sctk_get_process_rank_from_task_rank (win->owner), SCTK_CONTROL_MESSAGE_PROCESS, SCTK_PROCESS_RDMA_EMULATED_CAS, 0, &fcas, sizeof(struct sctk_window_emulated_CAS_RDMA) );
	}
	else
	{
		sctk_error("CAS NET");
		sctk_window_RDMA_CAS_net( remote_win_id, remote_offset, comp, new_data,  type, req );
	}
}




/* FENCE =================== */

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
		sctk_control_message_fence( win->owner );
	}

}
