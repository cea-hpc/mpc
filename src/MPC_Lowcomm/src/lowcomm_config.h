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
/* #   - PERACHE Marc    marc.perache@cea.fr                              # */
/* #                                                                      # */
/* ######################################################################## */
#ifndef LOWCOMM_CONFIG_H_
#define LOWCOMM_CONFIG_H_

#include <mpc_lowcomm_types.h>


/*******************
 * COLLECTIVE CONF *
 *******************/

struct _mpc_lowcomm_coll_conf
{
    char * algorithm;
    void ( *mpc_lowcomm_coll_init_hook )( mpc_lowcomm_communicator_t id );

    /* Barrier */
    int barrier_arity;

    /* Bcast */
    size_t bcast_max_size;
    int bcast_max_arity;
    size_t bcast_check_threshold;

    /* Allreduce */
    size_t allreduce_max_size;
    int allreduce_max_arity;
    size_t allreduce_check_threshold;
    int allreduce_max_slots;
};

struct _mpc_lowcomm_coll_conf * _mpc_lowcomm_coll_conf_get(void);

void mpc_lowcomm_coll_init_hook( mpc_lowcomm_communicator_t id );

/************************************
 * GLOBAL CONFIGURATION FOR LOWCOMM *
 ************************************/


void _mpc_lowcomm_config_register(void);
void _mpc_lowcomm_config_validate(void);


#endif /* LOWCOMM_CONFIG_H_ */