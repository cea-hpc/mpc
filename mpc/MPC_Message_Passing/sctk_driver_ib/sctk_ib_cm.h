/* ############################# MPC License ############################## */
/* # Wed Nov 19 15:19:19 CET 2008                                         # */
/* # Copyright or (C) or Copr. Commissariat a l'Energie Atomique          # */
/* # Copyright or (C) or Copr. 2010-2012 Université de Versailles         # */
/* # St-Quentin-en-Yvelines                                               # */
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
/* #   - DIDELOT Sylvain sylvain.didelot@exascale-computing.eu            # */
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
#define CM_MASK_TAG (1<<8) /* Up to 128 */
#define CM_OD_REQ_TAG (2)
#define CM_OD_ACK_TAG (3)
#define CM_OD_DONE_TAG (4)
#define CM_OD_STATIC_TAG (5)

#define ONDEMAND_DECO_REQ_TAG (6)
#define ONDEMAND_DECO_ACK_TAG (7)
#define ONDEMAND_DECO_DONE_REQ_TAG (8)
#define ONDEMAND_DECO_DONE_ACK_TAG (9)

#define CM_OD_RDMA_REQ_TAG (10)
#define CM_OD_RDMA_ACK_TAG (11)
#define CM_OD_RDMA_DONE_TAG (12)

#define CM_RESIZING_RDMA_REQ_TAG (13)
#define CM_RESIZING_RDMA_ACK_TAG (14)
#define CM_RESIZING_RDMA_DONE_TAG (15)

struct sctk_thread_ptp_message_body_s;

/*-----------------------------------------------------------
 *  STRUCTURES
 *----------------------------------------------------------*/

#define CM_SET_REQUEST(r,x) (r->request_id = x)
#define CM_GET_REQUEST(r) (r->request_id)

/* All the following structures are requests
 * for the Connection Manager. */
typedef enum {
  cm_request_qp_connection,       /* Connection to a remote QP */
  cm_request_qp_deconnection,     /* Deconnection from a remote QP */
  cm_request_rdma_connection,     /* Establishment of a RDMA connection*/
  cm_request_rdma_deconnection,   /* Remove a RDMA connection */
  cm_request_rdma_resizing,       /* Resize a RDMA connection */
} sctk_ib_cm_request_t;

/* ACK */
typedef struct {
  int ack;
} sctk_ib_cm_ack_t;

/* DONE */
typedef struct {
  int done;
} sctk_ib_cm_done_t;

/* QP */
typedef struct {
  sctk_ib_cm_request_t request_id;  /* Request id. *MUST* be the first field */
  sctk_uint16_t lid;
  sctk_uint32_t qp_num;
  sctk_uint32_t psn;
} sctk_ib_cm_qp_connection_t;

typedef struct {
  sctk_ib_cm_request_t request_id;  /* Request id. *MUST* be the first field */
} sctk_ib_cm_qp_deconnection_t;

/* RDMA connection and resizing */
typedef struct {
  sctk_ib_cm_request_t request_id;  /* Request id. *MUST* be the first field */
  int connected;
  int size;   /* Size of a slot */
  int nb;     /* Number of slots */
  sctk_uint32_t rkey;
  void *addr;
} sctk_ib_cm_rdma_connection_t;

typedef struct {
  sctk_ib_cm_request_t request_id;  /* Request id. *MUST* be the first field */

} sctk_ib_cm_rdma_deconnection_t;

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

/* RDMA resizing */
int sctk_ib_cm_resizing_rdma_request(sctk_rail_info_t* rail_targ, struct sctk_ib_qp_s *remote,
    int entry_size, int entry_nb);
void sctk_ib_cm_resizing_rdma_ack(sctk_rail_info_t* rail_targ,  struct sctk_ib_qp_s *remote,
    sctk_ib_cm_rdma_connection_t* send_keys);

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

int sctk_ib_cm_on_demand_rdma_check_request(
    sctk_rail_info_t* rail_targ, struct sctk_ib_qp_s *remote);

int sctk_ib_cm_on_demand_rdma_request(
    sctk_rail_info_t* rail_targ, struct sctk_ib_qp_s *remote,
    int entry_size, int entry_nb);
#endif
#endif
