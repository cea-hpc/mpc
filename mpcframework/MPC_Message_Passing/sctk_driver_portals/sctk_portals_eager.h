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
/* # had knowledge of the CeCILL-C license and tt you accept its          # */
/* # terms.                                                               # */
/* #                                                                      # */
/* # Authors:                                                             # */
/* #   - ADAM Julien adamj@paratools.fr                                   # */
/* #                                                                      # */
/* ######################################################################## */

#ifndef __SCTK_PORTALS_EAGER_H_
#define __SCTK_PORTALS_EAGER_H_

#ifdef __cplusplus
extern "C"
{
#endif

#ifdef MPC_USE_PORTALS
#include <sctk_portals_helper.h>
struct sctk_endpoint_s;
struct sctk_rail_info_s;
struct sctk_thread_ptp_message_s;


void sctk_portals_eager_message_copy ( struct sctk_message_to_copy_s *tmp );
void sctk_portals_eager_free (void* msg);
void sctk_portals_eager_send_put ( struct sctk_endpoint_s *endpoint,struct sctk_thread_ptp_message_s *msg);
void sctk_portals_eager_recv_put (struct sctk_rail_info_s* rail,  unsigned int indice, ptl_event_t* event);

#endif /* MPC_USE_PORTALS */

#ifdef __cplusplus
}
#endif

#endif /* __SCTK_PORTALS_EAGER_H_ */
