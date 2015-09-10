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

#ifndef SCTK_WINDOW_H
#define SCTK_WINDOW_H

#include "sctk_types.h"
#include "sctk_spinlock.h"
#include "sctk_rail.h"

/************************************************************************/
/* Definition of an RDMA window                                         */
/************************************************************************/

struct sctk_window
{
	int id;
	int remote_id;
	void * start_addr;
	size_t size;
	size_t disp_unit;
	int owner;
	sctk_rail_pin_ctx_t pin;
	sctk_communicator_t comm;
	int is_emulated;
	sctk_spinlock_t lock;
	unsigned int refcounter;
};

/* Local init and release */
sctk_window_t sctk_window_init( void *addr, size_t size, size_t disp_unit, sctk_communicator_t comm );
void sctk_window_release( sctk_window_t win );

/* Refcounter interactions */
void sctk_win_acquire( sctk_window_t win_id );
int sctk_win_relax( sctk_window_t win_id );

/* Remote window mapping */

struct sctk_window_map_request
{
	int source_rank;
	int remote_rank;
	sctk_window_t win_id;
};

static inline void sctk_window_map_request_init( struct sctk_window_map_request * mr, int remote_rank, sctk_window_t win_id )
{
	mr->source_rank = sctk_get_task_rank();
	mr->remote_rank = remote_rank;
	mr->win_id = win_id;
}

int sctk_window_map_remote( int remote_rank, sctk_window_t win_id );


/************************************************************************/
/* Window Operations                                                    */
/************************************************************************/

struct sctk_window_emulated_RDMA
{
	int source_rank;
	int remote_rank;
	size_t offset;
	size_t size;
	sctk_window_t win_id;
};

static inline void sctk_window_emulated_RDMA_init( struct sctk_window_emulated_RDMA * erma, int remote_rank, size_t offset, size_t size, sctk_window_t win_id )
{
	erma->source_rank = sctk_get_task_rank();
	erma->remote_rank = remote_rank;
	erma->offset = offset;
	erma->size = size;
	erma->win_id = win_id;
}


struct sctk_window_emulated_fetch_and_op_RDMA
{
	struct sctk_window_emulated_RDMA rdma;
	RDMA_type type;
	RDMA_op op;
	char add[16];
};


static inline void sctk_window_emulated_fetch_and_op_RDMA_init( struct sctk_window_emulated_fetch_and_op_RDMA * fop, int remote_rank, size_t offset, sctk_window_t win_id, RDMA_op op, RDMA_type type, void * add )
{
	sctk_window_emulated_RDMA_init( &fop->rdma, remote_rank, offset, RDMA_type_size( type ), win_id );
	
	fop->type = type;
	fop->op = op;
	
	assume( fop->rdma.size <= 16 );

	if( add )
	{
		memcpy( fop->add, add, fop->rdma.size );
	}
}



void sctk_window_RDMA_write( sctk_window_t win_id, void * src_addr, size_t size, size_t dest_offset, sctk_request_t  * req  );
void sctk_window_RDMA_write_win( sctk_window_t src_win_id, size_t src_offset, size_t size,  sctk_window_t dest_win_id, size_t dest_offset, sctk_request_t  * req  );

void sctk_window_RDMA_read( sctk_window_t win_id, void * dest_addr, size_t size, size_t src_offset, sctk_request_t  * req  );
void sctk_window_RDMA_read_win( sctk_window_t src_win_id, size_t src_offset, size_t size, sctk_window_t dest_win_id, size_t dest_offset, sctk_request_t  * req );

void sctk_window_RDMA_fetch_and_op( sctk_window_t remote_win_id, size_t remote_offset, void * fetch_addr, void * add, RDMA_op op,  RDMA_type type, sctk_request_t  * req );
void sctk_window_RDMA_fetch_and_op_win( sctk_window_t remote_win_id, size_t remote_offset, sctk_window_t local_win_id, size_t fetch_offset, void * add, RDMA_op op,  RDMA_type type, sctk_request_t  * req );
void sctk_window_RDMA_fetch_and_op_ctrl_msg_handler( struct sctk_window_emulated_fetch_and_op_RDMA *fop );

void sctk_window_RDMA_fence( sctk_window_t win_id );


/************************************************************************/
/* Control Messages Handlers                                            */
/************************************************************************/

enum
{
	TAG_RDMA_READ,
	TAG_RDMA_WRITE,
	TAG_RDMA_FETCH_AND_OP,
	TAG_RDMA_MAP
};

void sctk_window_map_remote_ctrl_msg_handler( struct sctk_window_map_request * mr );
void sctk_window_relax_ctrl_msg_handler( sctk_window_t win_id );
void sctk_window_RDMA_emulated_write_ctrl_msg_handler( struct sctk_window_emulated_RDMA *erma );
void sctk_window_RDMA_emulated_read_ctrl_msg_handler( struct sctk_window_emulated_RDMA *erma );


#endif /* SCTK_WINDOW_H */
