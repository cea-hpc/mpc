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
/* # following URL "http://www.cecill.info".                              # */
/* #                                                                      # */
/* # The fact that you are presently reading this means that you have     # */
/* # had knowledge of the CeCILL-C license and that you accept its        # */
/* # terms.                                                               # */
/* #                                                                      # */
/* # Authors:                                                             # */
/* #   - PERACHE Marc marc.perache@cea.fr                                 # */
/* #   - POUGET Kevin pougetk@ocre.cea.fr                                 # */
/* #                                                                      # */
/* ######################################################################## */

#if defined (TDB_REMOTE_STATIC) && defined (MPC_Thread_db)

#include "tdb_remote.h"
#include <thread_db.h>

volatile int rtdb_lib_state = TDB_LIB_TO_START ;
/* static structure for enabled event, always reachable */
volatile struct td_thr_events rtdb_ta_events ;
volatile tdb_thread_debug_t *rtdb_thread_list = NULL ;

void rtdb_bp_creation (void) {}
void rtdb_bp_death (void) {}

#else
#ifndef MPC_Thread_db
void rtdb_bp_creation (void) {}
void rtdb_bp_death (void) {}
#endif
#endif
