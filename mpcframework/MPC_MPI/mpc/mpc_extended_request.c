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
#include "mpc_extended_request.h"
#include <stdlib.h>
#include "sctk_alloc.h"

void GRequest_context_init( struct GRequest_context * ctx )
{
	ctx->classes = NULL;
	ctx->current_id = 0;
}

void GRequest_context_release( struct GRequest_context * ctx )
{
	MPCX_GRequest_class_t *current, *tmp;	
	
	HASH_ITER(hh, ctx->classes, current, tmp) {
		HASH_DEL(ctx->classes, current);  
		sctk_free(current);
	}
	
	GRequest_context_init( ctx );
}

int GRequest_context_add_class( struct GRequest_context *ctx,
				     MPC_Grequest_query_function * query_fn,
				     MPC_Grequest_cancel_function * cancel_fn,
				     MPC_Grequest_free_function * free_fn,
				     MPCX_Grequest_poll_fn * poll_fn,
				     MPCX_Grequest_wait_fn * wait_fn,
				     MPCX_Request_class * new_class )
{
	/* Allocate a new entry */
	MPCX_GRequest_class_t * pnew_class = sctk_calloc( 1, sizeof(  MPCX_GRequest_class_t ) );
	
	/* Fill its content */
	pnew_class->query_fn = query_fn;
	pnew_class->cancel_fn = cancel_fn;
	pnew_class->free_fn = free_fn;
	pnew_class->poll_fn = poll_fn;
	pnew_class->wait_fn = wait_fn;
	
	pnew_class->class_id = ctx->current_id;
	
	/* Add it to the hash table */
	HASH_ADD_INT( ctx->classes, class_id, pnew_class );
	
	/* Set the new_class id */
	*new_class =  (MPCX_Request_class)pnew_class->class_id;
	
	/* Increment Id */
	ctx->current_id++;
	
	return MPC_SUCCESS;
}


MPCX_GRequest_class_t  * GRequest_context_get_class( struct GRequest_context *ctx, MPCX_Request_class requested_class )
{
	MPCX_GRequest_class_t * pclass;

	HASH_FIND_INT(ctx->classes, &requested_class, pclass); 
		
	return pclass;
}
