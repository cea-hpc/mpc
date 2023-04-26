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

#ifndef __SCTK__IB_CM_H_
#define __SCTK__IB_CM_H_

#include "endpoint.h"
#include "comm.h"
/*-----------------------------------------------------------
 *  MACROS
 *----------------------------------------------------------*/

typedef enum
{
	CM_OD_REQ_TAG,
	CM_OD_ACK_TAG,
	CM_OD_DONE_TAG,
	
	CM_OD_STATIC_TAG,
	
	ONDEMAND_DECO_REQ_TAG,
	ONDEMAND_DECO_ACK_TAG,
	ONDEMAND_DECO_DONE_REQ_TAG,
	ONDEMAND_DECO_DONE_ACK_TAG,
	
	CM_OD_RDMA_REQ_TAG,
	CM_OD_RDMA_ACK_TAG,
	CM_OD_RDMA_DONE_TAG,
	
	CM_RESIZING_RDMA_REQ_TAG,
	CM_RESIZING_RDMA_ACK_TAG,
	CM_RESIZING_RDMA_DONE_TAG,
	
	CM_RESIZING_RDMA_DECO_REQ_TAG
	
}_mpc_lowcomm_ib_Control_message_t;


struct sctk_thread_ptp_message_body_s;

/*-----------------------------------------------------------
 *  STRUCTURES
 *----------------------------------------------------------*/

#define CM_SET_REQUEST(r,x) (r->request_id = x)
#define CM_GET_REQUEST(r) (r->request_id)
enum _mpc_lowcomm_ib_cm_change_state_type_e
{
    CONNECTION = 111,
    RESIZING = 222,
};

/* ACK */
typedef struct
{
	int rail_id;  /* rail id. *MUST* be the first field */
	int ack;
} _mpc_lowcomm_ib_cm_ack_t;

/* DONE */
typedef struct
{
	int rail_id;  /* rail id. *MUST* be the first field */
	int done;
} _mpc_lowcomm_ib_cm_done_t;

/* QP */
typedef struct
{
	int rail_id;  /* rail id. *MUST* be the first field */
	uint16_t lid;
	uint32_t qp_num;
	uint32_t psn;
} _mpc_lowcomm_ib_cm_qp_connection_t;

typedef struct
{
	int rail_id;  /* rail id. *MUST* be the first field */
} _mpc_lowcomm_ib_cm_qp_deconnection_t;

/* RDMA connection and resizing */
typedef struct
{
	int rail_id;  /* rail id. *MUST* be the first field */
	int connected;
	int size;   /* Size of a slot */
	int nb;     /* Number of slots */
	uint32_t rkey;
	void *addr;
} _mpc_lowcomm_ib_cm_rdma_connection_t;

typedef struct
{
	int rail_id;  /* rail id. *MUST* be the first field */
} _mpc_lowcomm_ib_cm_rdma_deconnection_t;

typedef struct {
	_mpc_lowcomm_ib_Control_message_t action; /* What to be performed */
	_mpc_lowcomm_ib_cm_rdma_connection_t conn;
}_mpc_lowcomm_ib_cm_rdma_control_message_t;


/*-----------------------------------------------------------
 *  FUNCTIONS
 *----------------------------------------------------------*/
/* Ring connexion */
void _mpc_lowcomm_ib_cm_connect_ring ( sctk_rail_info_t *rail );

/* Fully-connected */
void _mpc_lowcomm_ib_cm_connect_to ( int from, int to, struct sctk_rail_info_s *rail );
void _mpc_lowcomm_ib_cm_connect_from ( int from, int to, sctk_rail_info_t *rail );

/* On-demand connexions */
int _mpc_lowcomm_ib_cm_on_demand_recv_check ( mpc_lowcomm_ptp_message_body_t *msg );
 
_mpc_lowcomm_endpoint_t *_mpc_lowcomm_ib_cm_on_demand_request ( int dest, sctk_rail_info_t *rail );

_mpc_lowcomm_endpoint_t * _mpc_lowcomm_ib_cm_on_demand_request_monitor(sctk_rail_info_t *rail, mpc_lowcomm_peer_uid_t dest);
void _mpc_lowcomm_ib_cm_monitor_register_callbacks(sctk_rail_info_t * rail);

void _mpc_lowcomm_ib_cm_deco_ack ( sctk_rail_info_t *rail,
                           _mpc_lowcomm_endpoint_t *route_table, int ack );

/* RDMA resizing */
int _mpc_lowcomm_ib_cm_resizing_rdma_request ( sctk_rail_info_t *rail_targ, struct _mpc_lowcomm_ib_qp_s *remote,
                                       int entry_size, int entry_nb );
void _mpc_lowcomm_ib_cm_resizing_rdma_ack ( sctk_rail_info_t *rail_targ,  struct _mpc_lowcomm_ib_qp_s *remote,
                                    _mpc_lowcomm_ib_cm_rdma_connection_t *send_keys );

/*-----------------------------------------------------------
 *  On demand QP deconnexion
 *----------------------------------------------------------*/
/* Recv */
//void _mpc_lowcomm_ib_cm_deco_request_recv ( sctk_rail_info_t *rail, void *payload, int src );
//void _mpc_lowcomm_ib_cm_deco_ack_recv ( sctk_rail_info_t *rail, void *ack, int src );
//void _mpc_lowcomm_ib_cm_deco_done_ack_recv ( sctk_rail_info_t *rail, void *ack, int src );
//void _mpc_lowcomm_ib_cm_deco_done_request_recv ( sctk_rail_info_t *rail, void *ack, int src );

/* Send */
//void _mpc_lowcomm_ib_cm_deco_request_send ( sctk_rail_info_t *rail, _mpc_lowcomm_endpoint_t *route_table );
//void _mpc_lowcomm_ib_cm_deco_done_request_send ( sctk_rail_info_t *rail, _mpc_lowcomm_endpoint_t *route_table );
//void _mpc_lowcomm_ib_cm_deco_ack_send ( sctk_rail_info_t *rail, _mpc_lowcomm_endpoint_t *route_table, int ack );
//void _mpc_lowcomm_ib_cm_deco_done_ack_send ( sctk_rail_info_t *rail, _mpc_lowcomm_endpoint_t *route_table, int ack );

int _mpc_lowcomm_ib_cm_on_demand_rdma_check_request (
    sctk_rail_info_t *rail_targ, struct _mpc_lowcomm_ib_qp_s *remote );

int _mpc_lowcomm_ib_cm_on_demand_rdma_request (
    sctk_rail_info_t *rail_targ, struct _mpc_lowcomm_ib_qp_s *remote,
    int entry_size, int entry_nb );
#endif
