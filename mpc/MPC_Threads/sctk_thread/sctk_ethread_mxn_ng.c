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
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include "sctk_pthread.h"
#include "sctk_debug.h"
#include "sctk_thread.h"
#include "sctk_tls.h"
#include "sctk_internal_thread.h"
#include "sctk_posix_pthread.h"
#include "sctk_config_pthread.h"
#include "sctk_kernel_thread.h"
#include <semaphore.h>


void
sctk_ethread_mxn_ng_thread_init (void){
  sctk_only_once ();

  sctk_multithreading_initialised = 1;

  sctk_thread_data_init ();
}
