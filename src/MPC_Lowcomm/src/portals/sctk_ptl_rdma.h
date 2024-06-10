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
/* #   - ADAM Julien julien.adam@paratools.com                            # */
/* #   - BESNARD Jean-Baptiste jean-baptiste.besnard@paratools.com        # */
/* #                                                                      # */
/* ######################################################################## */

#ifndef __SCTK_PTL_RDMA_H_
#define __SCTK_PTL_RDMA_H_
#ifdef MPC_USE_PORTALS

#include "rail.h"

int sctk_ptl_rdma_fetch_and_op_gate( sctk_rail_info_t *rail, size_t size, RDMA_op op, RDMA_type type );
void sctk_ptl_rdma_fetch_and_op(  sctk_rail_info_t *rail,
		mpc_lowcomm_ptp_message_t *msg,
		void * fetch_addr,
		struct  sctk_rail_pin_ctx_list * local_key,
		void * remote_addr,
		struct  sctk_rail_pin_ctx_list * remote_key,
		void * add,
		RDMA_op op,
		RDMA_type type );
int sctk_ptl_rdma_cas_gate( sctk_rail_info_t *rail, size_t size, RDMA_type type );
void sctk_ptl_rdma_cas(   sctk_rail_info_t *rail,
		mpc_lowcomm_ptp_message_t *msg,
		void *  res_addr,
		struct  sctk_rail_pin_ctx_list * local_key,
		void * remote_addr,
		struct  sctk_rail_pin_ctx_list * remote_key,
		void * comp,
		void * new,
		RDMA_type type );
void sctk_ptl_rdma_write(  sctk_rail_info_t *rail, mpc_lowcomm_ptp_message_t *msg,
		void * src_addr, struct sctk_rail_pin_ctx_list * local_key,
		void * dest_addr, struct  sctk_rail_pin_ctx_list * remote_key,
		size_t size );
void sctk_ptl_rdma_read(  sctk_rail_info_t *rail, mpc_lowcomm_ptp_message_t *msg,
		void * src_addr,  struct  sctk_rail_pin_ctx_list * remote_key,
		void * dest_addr, struct  sctk_rail_pin_ctx_list * local_key,
		size_t size );
/* Pinning */
void sctk_ptl_pin_region( struct sctk_rail_info_s * rail, struct sctk_rail_pin_ctx_list * list, void * addr, size_t size );
void sctk_ptl_unpin_region( struct sctk_rail_info_s * rail, struct sctk_rail_pin_ctx_list * list );
void sctk_ptl_rdma_event_me(sctk_rail_info_t* rail, sctk_ptl_event_t ev);
void sctk_ptl_rdma_event_md(sctk_rail_info_t* rail, sctk_ptl_event_t ev);
void lcr_ptl_handle_rdma_ev_md(sctk_rail_info_t *rail, sctk_ptl_event_t *ev);
void lcr_ptl_handle_rdma_ev_me(sctk_rail_info_t *rail, sctk_ptl_event_t *ev);
#endif /* MPC_USE_PORTALS */
#endif /* ifndef __SCTK_PTL_RDMA_H_ */
