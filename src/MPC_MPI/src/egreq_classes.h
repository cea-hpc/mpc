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
#ifndef MPC_MPI_EGREQ_CLASSES_H_
#define MPC_MPI_EGREQ_CLASSES_H_

#include <mpc_lowcomm_types.h>
#include "uthash.h"

typedef struct MPCX_GRequest_class_s
{
	int class_id; /**< Unique identifier of this request class */

	/* Request context */
	sctk_Grequest_query_function * query_fn;
	sctk_Grequest_cancel_function * cancel_fn;
	sctk_Grequest_free_function * free_fn;
	sctk_Grequest_poll_fn * poll_fn;
	sctk_Grequest_wait_fn * wait_fn;

	UT_hash_handle hh; /**< This dummy data structure is required by UTHash is order to make this data structure hashable */
}MPCX_GRequest_class_t;



struct _mpc_egreq_classes_storage
{
	MPCX_GRequest_class_t * classes;
	int current_id;
};



void _mpc_egreq_classes_storage_init( struct _mpc_egreq_classes_storage * ctx );
void _mpc_egreq_classes_storage_release( struct _mpc_egreq_classes_storage * ctx );

int _mpc_egreq_classes_storage_add_class( struct _mpc_egreq_classes_storage *ctx,
				     sctk_Grequest_query_function * query_fn,
				     sctk_Grequest_cancel_function * cancel_fn,
				     sctk_Grequest_free_function * free_fn,
				     sctk_Grequest_poll_fn * poll_fn,
				     sctk_Grequest_wait_fn * wait_fn,
				     sctk_Request_class * new_class );


MPCX_GRequest_class_t * _mpc_egreq_classes_storage_get_class( struct _mpc_egreq_classes_storage *ctx, sctk_Request_class requested_class );


#endif /* MPC_MPI_EGREQ_CLASSES_H_ */
