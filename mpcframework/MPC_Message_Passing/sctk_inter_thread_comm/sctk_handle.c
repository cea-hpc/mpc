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
#include "sctk_handle.h"
#include "sctk_spinlock.h"
#include "sctk_atomics.h"
#include "sctk_ht.h"
#include "sctk_thread.h"
#include "sctk_debug.h"

/************************************************************************/
/* Storage for this whole file                                          */
/************************************************************************/

static int init_done = 0;
static sctk_spinlock_t init_lock = 0;


/* Error handlers */
static sctk_atomics_int current_errhandler;
static struct MPCHT error_handlers;

/* Handles */
static sctk_atomics_int current_handle;
static struct MPCHT handle_context;

/* Error codes */
static sctk_atomics_int current_error_class;
static sctk_atomics_int current_error_code;
static struct MPCHT error_strings;


static void _mpc_mpi_err_init()
{
	/* Error handlers */
	sctk_atomics_store_int(&current_errhandler, 100);
	MPCHT_init(&error_handlers, 8);

	/* Handles */
	sctk_atomics_store_int(&current_handle, SCTK_BOOKED_HANDLES);
	MPCHT_init(&handle_context, 64);

	/* Error codes */
	sctk_atomics_store_int( &current_error_class , 1024 );
	sctk_atomics_store_int( &current_error_code , 100 );
	MPCHT_init(&error_strings, 8);
}

static void mpc_mpi_err_init_once()
{
	sctk_spinlock_lock( &init_lock );

	if( !init_done )
	{
		_mpc_mpi_err_init();
		init_done = 1;
	}

	sctk_spinlock_unlock( &init_lock );
}


/************************************************************************/
/* Error Handler                                                        */
/************************************************************************/

int sctk_errhandler_register(sctk_generic_handler eh,  sctk_errhandler_t * errh )
{
	mpc_mpi_err_init_once();
	/* Create an unique id */
	sctk_errhandler_t new_id = sctk_atomics_fetch_and_add_int(&current_errhandler, 1);
	/* Give it to the iface */
	*errh = (sctk_errhandler_t)new_id;
	/* Save in the HT */
	MPCHT_set( &error_handlers, new_id, (void *)eh );
	/* All ok */
	return 0;
}

sctk_generic_handler sctk_errhandler_resolve( sctk_errhandler_t errh )
{
	mpc_mpi_err_init_once();
	/* Direct HT lookup */
	return MPCHT_get( &error_handlers, errh );
}

int sctk_errhandler_free( sctk_errhandler_t errh )
{
	mpc_mpi_err_init_once();
	/* Does it exists  ? */
	sctk_generic_handler eh =  sctk_errhandler_resolve( errh );

	if( !eh )
	{
		/* No then error */
		return 1;
	}

	/* If present delete */
	MPCHT_delete( &error_handlers, errh );

	/* All done */
	return 0;
}

/************************************************************************/
/* MPI Handles                                                          */
/************************************************************************/

static inline sctk_uint64_t sctk_handle_compute(sctk_handle id, sctk_handle_type type )
{
	return (0 & ((id) << 4 ))||type;
}




struct sctk_handle_context * sctk_handle_context_new(sctk_handle id)
{
	struct sctk_handle_context *ret = sctk_malloc( sizeof( struct sctk_handle_context ));
	assume( ret != NULL );

	ret->id = id;
	ret->handler = SCTK_ERRHANDLER_NULL;

	return ret;
}


static sctk_spinlock_t handle_mod_lock = 0;


struct sctk_handle_context * sctk_handle_context_no_lock( sctk_handle id, sctk_handle_type type )
{
	return (struct sctk_handle_context *)MPCHT_get(&handle_context, sctk_handle_compute(id, type) );
}



struct sctk_handle_context * sctk_handle_context( sctk_handle id, sctk_handle_type type )
{
	struct sctk_handle_context * ret = NULL;

	sctk_spinlock_lock_yield( &handle_mod_lock );
	ret = sctk_handle_context_no_lock( id, type );
	sctk_spinlock_unlock( &handle_mod_lock );

	return ret;
}




sctk_handle sctk_handle_new_from_id(int previous_id , sctk_handle_type type)
{
	sctk_handle new_handle_id;

	mpc_mpi_err_init_once();

	sctk_spinlock_lock_yield( &handle_mod_lock );

	if( sctk_handle_context_no_lock( previous_id, type ) == NULL )
	{
		/* Create an unique id */
		new_handle_id = previous_id; 
	
		/* Create the handler payload */
		struct sctk_handle_context * ctx =  sctk_handle_context_new(new_handle_id);
	
		/* Save in the HT */
		MPCHT_set( &handle_context, sctk_handle_compute(new_handle_id, type), (void *)ctx );
	}

	sctk_spinlock_unlock( &handle_mod_lock );

	/* All ok return the handle */
	return new_handle_id;
}

sctk_handle sctk_handle_new(sctk_handle_type type)
{
	sctk_handle new_handle_id;

	mpc_mpi_err_init_once();

	/* Create an unique id */
	new_handle_id = sctk_atomics_fetch_and_add_int(&current_handle, 1);
	
	return sctk_handle_new_from_id( new_handle_id, type );
}


int sctk_handle_free( sctk_handle id, sctk_handle_type type )
{

	sctk_spinlock_lock_yield( &handle_mod_lock );
	
	struct sctk_handle_context *hctx = sctk_handle_context_no_lock( id, type );

	if( !hctx )
	{
		sctk_spinlock_unlock( &handle_mod_lock );
		return MPC_ERR_ARG;
	}

	MPCHT_delete(&handle_context, sctk_handle_compute(id, type));

	sctk_spinlock_unlock( &handle_mod_lock );

	sctk_handle_context_release(hctx);

	
	return 0;
}

int sctk_handle_context_release(struct sctk_handle_context *hctx)
{
	memset(hctx, 0, sizeof(sctk_handle_context));
	sctk_free( hctx );

	return 0;
}



sctk_errhandler_t sctk_handle_get_errhandler( sctk_handle id, sctk_handle_type type )
{
	struct sctk_handle_context *hctx = sctk_handle_context( id, type );

	if( !hctx )
	{
		return SCTK_ERRHANDLER_NULL;
	}

	return hctx->handler;
}


int sctk_handle_set_errhandler( sctk_handle id, sctk_handle_type type,  sctk_errhandler_t errh )
{
	struct sctk_handle_context *hctx = sctk_handle_context( id, type );

	if( !hctx )
	{
		return 1;
	}

	hctx->handler = errh;
}
