/* ############################# MPI License ############################## */
/* # Wed Nov 19 15:19:19 CET 2008                                         # */
/* # Copyright or (C) or Copr. Commissariat a l'Energie Atomique          # */
/* #                                                                      # */
/* # IDDN.FR.001.230040.000.S.P.2007.000.10000                            # */
/* # This file is part of the MPI Runtime.                                # */
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
/* #   - BESNARD Jean-Baptiste   jbbesnard@paratools.fr                   # */
/* #                                                                      # */
/* ######################################################################## */
#ifndef MPI_HALO_CONTEXT_H
#define MPI_HALO_CONTEXT_H

#include <mpc_mpi.h>
#include <mpc_common_spinlock.h>
#include <stdlib.h>

/************************************************************************/
/* Halo context (storage and integer resolution)                        */
/************************************************************************/

struct sctk_mpi_halo_s;
struct sctk_mpi_halo_exchange_s;

struct sctk_mpi_halo_context
{
	struct sctk_mpi_halo_s ** halo_cells;
	int halo_counter;
	int halo_size;

	struct sctk_mpi_halo_exchange_s ** exchange_cells;
	int exchange_counter;
	int exchange_size;

	mpc_common_spinlock_t lock;
};

void sctk_mpi_halo_context_init( struct sctk_mpi_halo_context * ctx );
void sctk_mpi_halo_context_relase( struct sctk_mpi_halo_context * ctx );

struct sctk_mpi_halo_s* sctk_mpi_halo_context_get( struct sctk_mpi_halo_context * ctx, int id );
int sctk_mpi_halo_context_set( struct sctk_mpi_halo_context * ctx,  int id, struct sctk_mpi_halo_s * halo );
struct sctk_mpi_halo_s * sctk_mpi_halo_context_new( struct sctk_mpi_halo_context * ctx,  int  * id );

struct sctk_mpi_halo_exchange_s * sctk_mpi_halo_context_exchange_get( struct sctk_mpi_halo_context * ctx,  int id );
int sctk_mpi_halo_context_exchange_set( struct sctk_mpi_halo_context * ctx,  int id, struct sctk_mpi_halo_exchange_s * halo );
struct sctk_mpi_halo_exchange_s *  sctk_mpi_halo_context_exchange_new( struct sctk_mpi_halo_context * ctx,  int  * id );

/************************************************************************/
/* Halo Cells                                                           */
/************************************************************************/


/* MPI Halo */
typedef enum
{
	MPI_HALO_TYPE_NONE,
	MPI_HALO_CELL_LOCAL,
	MPI_HALO_CELL_REMOTE
}sctk_mpi_halo_type_t;

typedef enum
{
	MPI_HALO_BUFFER_NONE,
	MPI_HALO_BUFFER_ALIAS,
	MPI_HALO_BUFFER_ALLOCATED
}sctk_mpi_halo_buffer_type_t;


struct sctk_mpi_halo_s
{
	sctk_mpi_halo_type_t type;
	char * label;
	int is_committed;

	/* Message info */
	void * halo_buffer;
	sctk_mpi_halo_buffer_type_t halo_buffer_type;
	int halo_remote;
	int halo_tag;
	char * remote_label;
	MPI_Datatype halo_type;
	int halo_count;
	size_t halo_extent;
};

int sctk_mpi_halo_init( struct sctk_mpi_halo_s * halo , char * label , MPI_Datatype type, int count );
int sctk_mpi_halo_release( struct sctk_mpi_halo_s * halo );
int sctk_mpi_halo_set(  struct sctk_mpi_halo_s * halo, void * ptr );
int sctk_mpi_halo_get(  struct sctk_mpi_halo_s * halo, void ** ptr );

/************************************************************************/
/* Halo Exchange                                                        */
/************************************************************************/

/*
 * Exchange Action
 */
typedef enum
{
	MPI_HALO_EXCHANGE_NONE,
	MPI_HALO_EXCHANGE_SEND,
	MPI_HALO_EXCHANGE_RECV,
	MPI_HALO_EXCHANGE_SEND_LOCAL,
	MPI_HALO_EXCHANGE_RECV_LOCAL
}sctk_mpi_halo_exchange_action_t;


struct sctk_mpi_halo_exchange_action_s
{
	MPI_Request request;
	int process;
	int tag;
	sctk_mpi_halo_exchange_action_t action;
	struct sctk_mpi_halo_s * halo;
	struct sctk_mpi_halo_exchange_action_s * prev;
};

struct sctk_mpi_halo_exchange_action_s * sctk_mpi_halo_exchange_new_action( struct sctk_mpi_halo_s * halo, sctk_mpi_halo_exchange_action_t action, int process, int tag, struct sctk_mpi_halo_exchange_action_s * prev);
int sctk_mpi_halo_exchange_action_free( struct sctk_mpi_halo_exchange_action_s * action );

/*
 * Exchange Request
 */
struct sctk_mpi_halo_exchange_request
{
	int source_process;
	int dest_process;
	int tag;
	char source_label[512];
	char remote_label[512];
	size_t extent;
};

int sctk_mpi_halo_exchange_request_init( struct sctk_mpi_halo_exchange_request * req , struct sctk_mpi_halo_s * halo );
int sctk_mpi_halo_exchange_request_relase( struct sctk_mpi_halo_exchange_request * req  );
void sctk_mpi_halo_exchange_request_print( struct sctk_mpi_halo_exchange_request * req );


struct sctk_mpi_halo_exchange_s
{
	 struct sctk_mpi_halo_s ** halo;
	 int halo_count;
	 int halo_size;

	struct sctk_mpi_halo_exchange_action_s * halo_actions;

};

/* Internal halo storage */
int sctk_mpi_halo_exchange_push_halo( struct sctk_mpi_halo_exchange_s *ex,  struct sctk_mpi_halo_s * halo );
struct sctk_mpi_halo_s * sctk_mpi_halo_exchange_get_halo( struct sctk_mpi_halo_exchange_s *ex, char *label );

/* Actual interface */
int sctk_mpi_halo_exchange_init( struct sctk_mpi_halo_exchange_s *ex );
int sctk_mpi_halo_exchange_release( struct sctk_mpi_halo_exchange_s *ex );

int sctk_mpi_halo_bind_remote( struct sctk_mpi_halo_exchange_s *ex, struct sctk_mpi_halo_s * halo, int remote, char * remote_label );
int sctk_mpi_halo_bind_local( struct sctk_mpi_halo_exchange_s *ex, struct sctk_mpi_halo_s * halo );
int sctk_mpi_halo_exchange_commit( struct sctk_mpi_halo_exchange_s * ex );

int sctk_mpi_halo_iexchange( struct sctk_mpi_halo_exchange_s *ex );
int sctk_mpi_halo_exchange_wait( struct sctk_mpi_halo_exchange_s *ex );
int sctk_mpi_halo_exchange_blocking( struct sctk_mpi_halo_exchange_s *ex );

#endif /* MPI_HALO_CONTEXT_H */
