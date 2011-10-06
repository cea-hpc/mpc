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
#ifndef __SCTK_INTER_THREAD_COMMUNICATIONS_H_
#define __SCTK_INTER_THREAD_COMMUNICATIONS_H_

#include <sctk_config.h>
#include <sctk_debug.h>

#ifdef __cplusplus
extern "C"
{
#endif
  struct sctk_request_s;

  typedef struct sctk_thread_message_header_s
  {
    int src;
  } sctk_thread_message_header_t;

  typedef unsigned int sctk_pack_indexes_t;
  typedef unsigned long sctk_pack_absolute_indexes_t;
  typedef int sctk_count_t;

  typedef struct
  {
    sctk_count_t count;
    sctk_pack_indexes_t *begins;
    sctk_pack_indexes_t *ends;
    sctk_pack_absolute_indexes_t *begins_absolute;
    sctk_pack_absolute_indexes_t *ends_absolute;
  } sctk_default_pack_t;

  typedef struct sctk_thread_ptp_message_s{
    volatile int* completion_flag;  
  }sctk_thread_ptp_message_t;

  typedef struct sctk_request_s{
    volatile int completion_flag;
    sctk_thread_ptp_message_t *msg;
    sctk_thread_message_header_t header;
    volatile size_t msg_size;
    sctk_default_pack_t default_pack;
    int is_null;
    /*     void *ptr; */
  } sctk_request_t;

#ifdef __cplusplus
}
#endif

#endif
