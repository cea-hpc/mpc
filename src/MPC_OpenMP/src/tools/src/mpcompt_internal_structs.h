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
/* #   - CAPRA Antoine capra@paratools.com                                # */
/* #   - CARRIBAULT Patrick patrick.carribault@cea.fr                     # */
/* #                                                                      # */
/* ######################################################################## */

#ifndef __MPCOMPT_INTERNAL_STRUCT_H__
#define __MPCOMPT_INTERNAL_STRUCT_H__

#if OMPT_SUPPORT

typedef struct mpcompt_enumerate_infos_s {
  const char* name;
  unsigned long id;
} mpcompt_enumerate_infos_t;

typedef struct mpcompt_callback_infos_s {
  const char *name;
  ompt_callbacks_t id;
  ompt_set_result_t status;
} mpcompt_callback_infos_t;

// compare two string ignoring case
#ifndef OMPT_STR_MATCH
#define OMPT_STR_MATCH( string0, string1 ) \
  !strcasecmp( string0, string1 )
#endif /* OMPT_STR_MATCH */

#endif /* OMPT_SUPPORT */
#endif /* __MPCOMPT_INTERNAL_STRUCT_H__ */
