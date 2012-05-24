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
/* #   - DIDELOT Sylvain didelot.sylvain@gmail.com                        # */
/* #                                                                      # */
/* ######################################################################## */

#ifdef MPC_USE_INFINIBAND
#ifndef __SCTK__IB_CM_H_
#define __SCTK__IB_CM_H_

#include "sctk_route.h"
#include "sctk_inter_thread_comm.h"
/*-----------------------------------------------------------
 *  MACROS
 *----------------------------------------------------------*/
#define CM_MASK_TAG (1<<8)
#define CM_OD_REQ_TAG (2)
#define CM_OD_ACK_TAG (3)
#define CM_OD_DONE_TAG (4)

#define ONDEMAND_DECO_REQ_TAG (5)
#define ONDEMAND_DECO_ACK_TAG (6)
#define ONDEMAND_DECO_DONE_REQ_TAG (7)
#define ONDEMAND_DECO_DONE_ACK_TAG (8)

struct sctk_thread_ptp_message_body_s;

/*-----------------------------------------------------------
 *  STRUCTURES
 *----------------------------------------------------------*/
/* QP */
typedef struct {

} sctk_ib_cm_qp_connection;

typedef struct {

} sctk_ib_cm_qp_deconnection;


/* RDMA */
typedef struct {

} sctk_ib_cm_rdma_connection;

typedef struct {

} sctk_ib_cm_rdma_resizing;

typedef struct {

} sctk_ib_cm_rdma_deconnection;

/*-----------------------------------------------------------
 *  FUNCTIONS
 *----------------------------------------------------------*/
/* Ring connexion */
void sctk_ib_cm_connect_ring (sctk_rail_info_t* rail,
			       int (*route)(int , sctk_rail_info_t* ),
			       void(*route_init)(sctk_rail_info_t*));

/* Fully-connected */
void sctk_ib_cm_connect_to(int from, int to, struct sctk_rail_info_s* rail);
void sctk_ib_cm_connect_from(int from, int to,sctk_rail_info_t* rail);

/* On-demand connexions */
int sctk_ib_cm_on_demand_recv_check(sctk_thread_ptp_message_body_t *msg);
int sctk_ib_cm_on_demand_recv(struct sctk_rail_info_s *rail,
    sctk_thread_ptp_message_t *msg, struct sctk_ibuf_s* ibuf, int recopy);
sctk_route_table_t *sctk_ib_cm_on_demand_request(int dest,sctk_rail_info_t* rail);

void sctk_ib_cm_deco_ack(sctk_rail_info_t* rail,
    sctk_route_table_t* route_table, int ack);


/*-----------------------------------------------------------
 *  On demand QP deconnexion
 *----------------------------------------------------------*/
/* Recv */
void sctk_ib_cm_deco_request_recv(sctk_rail_info_t *rail, void* payload, int src);
void sctk_ib_cm_deco_ack_recv(sctk_rail_info_t *rail, void* ack, int src);
void sctk_ib_cm_deco_done_ack_recv(sctk_rail_info_t *rail, void* ack, int src);
void sctk_ib_cm_deco_done_request_recv(sctk_rail_info_t *rail, void* ack, int src);

/* Send */
void sctk_ib_cm_deco_request_send(sctk_rail_info_t* rail, sctk_route_table_t* route_table);
void sctk_ib_cm_deco_done_request_send(sctk_rail_info_t* rail, sctk_route_table_t* route_table);
void sctk_ib_cm_deco_ack_send(sctk_rail_info_t* rail, sctk_route_table_t* route_table, int ack);
void sctk_ib_cm_deco_done_ack_send(sctk_rail_info_t* rail, sctk_route_table_t* route_table, int ack);

#endif
#endif
