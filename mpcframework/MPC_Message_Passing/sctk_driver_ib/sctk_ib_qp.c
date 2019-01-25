/* ############################# MPC License ############################## */
/* # Wed Nov 19 15:19:19 CET 2008                                         # */
/* # Copyright or (C) or Copr. Commissariat a l'Energie Atomique          # */
/* # Copyright or (C) or Copr. 2010-2012 Universit�� de Versailles         # */
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
#include "sctk_ib.h"
#include "sctk_ib_config.h"
#include "sctk_ibufs.h"
#include "sctk_ibufs_rdma.h"
#include "sctk_ib_qp.h"
#include "sctk_ib_prof.h"
#include "sctk_ib_polling.h"
#include "sctk_pmi.h"
#include "sctk_asm.h"
#include "utlist.h"
#include "sctk_ib_mpi.h"
#include <sctk_spinlock.h>
#include <errno.h>
#include <string.h>

/* IB debug macros */
#if defined SCTK_IB_MODULE_NAME
#error "SCTK_IB_MODULE already defined"
#endif
//#define SCTK_IB_MODULE_DEBUG
#define SCTK_IB_MODULE_NAME "QP"
#include "sctk_ib_toolkit.h"

/*-----------------------------------------------------------
 *  HT of remote peers.
 *  Used to determine the remote which has generated an event
 *  to the CQ of the SRQ.
 *----------------------------------------------------------*/
struct sctk_ib_qp_ht_s
{
	int key;
	UT_hash_handle hh;
	struct sctk_ib_qp_s *remote;
};

static sctk_spin_rwlock_t __qp_ht_lock = SCTK_SPIN_RWLOCK_INITIALIZER;

sctk_ib_qp_t  *sctk_ib_qp_ht_find ( struct sctk_ib_rail_info_s *rail_ib, int key )
{
	struct sctk_ib_qp_ht_s *entry = NULL;

	if ( rail_ib->remotes == NULL )
		return NULL;

	sctk_spinlock_read_lock ( &__qp_ht_lock );
	HASH_FIND_INT ( rail_ib->remotes, &key, entry );
	sctk_spinlock_read_unlock ( &__qp_ht_lock );

	if ( entry != NULL )
		return entry->remote;

	return NULL;
}

static inline void sctk_ib_qp_ht_add ( struct sctk_ib_rail_info_s *rail_ib, struct sctk_ib_qp_s *remote, int key )
{
	struct sctk_ib_qp_ht_s *entry = NULL;

#ifdef IB_DEBUG
	struct sctk_ib_qp_s *rem;
	rem = sctk_ib_qp_ht_find ( rail_ib, key );
	ib_assume ( rem == NULL );
#endif

	entry = sctk_malloc ( sizeof ( struct sctk_ib_qp_ht_s ) );
	ib_assume ( entry );
	entry->key = key;
	entry->remote = remote;

	sctk_spinlock_write_lock ( &__qp_ht_lock );
	HASH_ADD_INT ( rail_ib->remotes, key, entry );
	sctk_spinlock_write_unlock ( &__qp_ht_lock );
}

/*-----------------------------------------------------------
 *  Completion queue
 *----------------------------------------------------------*/

char *sctk_ib_cq_print_status ( enum ibv_wc_status status )
{
	switch ( status )
	{
		case IBV_WC_SUCCESS:
				return ( "IBV_WC_SUCCESS: success" );
			break;

		case IBV_WC_LOC_LEN_ERR:
			return ( "IBV_WC_LOC_LEN_ERR: local length error" );
			break;

		case IBV_WC_LOC_QP_OP_ERR:
			return ( "IBV_WC_LOC_QP_OP_ERR: local QP op error" );
			break;

		case IBV_WC_LOC_EEC_OP_ERR:
			return ( "IBV_WC_LOC_EEC_OP_ERR: local EEC op error" );
			break;

		case IBV_WC_LOC_PROT_ERR:
			return ( "IBV_WC_LOC_PROT_ERR: local protection error" );
			break;

		case IBV_WC_WR_FLUSH_ERR:
			return ( "IBV_WC_WR_FLUSH_ERR: write flush error" );
			break;

		case IBV_WC_MW_BIND_ERR:
			return ( "IBV_WC_MW_BIND_ERR: MW bind error" );
			break;

		case IBV_WC_BAD_RESP_ERR:
			return ( "IBV_WC_BAD_RESP_ERR: bad response error" );
			break;

		case IBV_WC_LOC_ACCESS_ERR:
			return ( "IBV_WC_LOC_ACCESS_ERR: local access error" );
			break;

		case IBV_WC_REM_INV_REQ_ERR:
			return ( "IBV_WC_REM_INV_REQ_ERR: remote invalid request error" );
			break;

		case IBV_WC_REM_ACCESS_ERR:
			return ( "IBV_WC_REM_ACCESS_ERR: remote access error" );
			break;

		case IBV_WC_REM_OP_ERR:
			return ( "IBV_WC_REM_OP_ERR: remote op error" );
			break;

		case IBV_WC_RETRY_EXC_ERR:
			return ( "IBV_WC_RETRY_EXC_ERR: retry exceded error" );
			break;

		case IBV_WC_RNR_RETRY_EXC_ERR:
			return ( "IBV_WC_RNR_RETRY_EXC_ERR: RNR retry exceded error" );
			break;

		case IBV_WC_LOC_RDD_VIOL_ERR:
			return ( "IBV_WC_LOC_RDD_VIOL_ERR: local RDD violation error" );
			break;

		case IBV_WC_REM_INV_RD_REQ_ERR:
			return ( "IBV_WC_REM_INV_RD_REQ_ERR: remote invalid read request error" );
			break;

		case IBV_WC_REM_ABORT_ERR:
			return ( "IBV_WC_REM_ABORT_ERR: remote abort error" );
			break;

		case IBV_WC_INV_EECN_ERR:
			return ( "IBV_WC_INV_EECN_ERR: invalid EECN error" );
			break;

		case IBV_WC_INV_EEC_STATE_ERR:
			return ( "IBV_WC_INV_EEC_STATE_ERR: invalid EEC state error" );
			break;

		case IBV_WC_FATAL_ERR:
			return ( "IBV_WC_FATAL_ERR: fatal error" );
			break;

		case IBV_WC_RESP_TIMEOUT_ERR:
			return ( "IBV_WC_RESP_TIMEOUT_ERR: response timeout error" );
			break;

		case IBV_WC_GENERAL_ERR:
			return ( "IBV_WC_GENERAL_ERR: general error" );
			break;
	}

	return ( "ERROR NOT KNOWN" );
}


/*-----------------------------------------------------------
 *  Exchange keys
 *----------------------------------------------------------*/
void sctk_ib_qp_key_print ( __UNUSED__ sctk_ib_cm_qp_connection_t *keys )
{
	sctk_nodebug ( "LID=%lu psn=%lu qp_num=%lu", keys->lid,
	               keys->psn,
	               keys->qp_num );
}

void sctk_ib_qp_key_fill ( sctk_ib_cm_qp_connection_t *keys, sctk_uint16_t lid, sctk_uint32_t qp_num, sctk_uint32_t psn )
{
	keys->lid = lid;
	keys->qp_num = qp_num;
	keys->psn = psn;
}


void sctk_ib_qp_key_create_value ( char *msg, size_t size, sctk_ib_cm_qp_connection_t *keys )
{
	int ret;

	ret = snprintf ( msg, size, "%08x:%08x:%08x", keys->lid,
	                 keys->qp_num,
	                 keys->psn );
	/* We assume the value doest not overflow with the buffer */
	ib_assume ( ret < size );

	sctk_ib_qp_key_print ( keys );
}

void sctk_ib_qp_key_create_key ( char *msg, size_t size, int rail_id, int src, int dest )
{
	int ret;
	/* We create the key with the number of the rail */
	ret = snprintf ( msg, size, "IB-%02d|%06d:%06d", rail_id, src, dest );
	sctk_nodebug ( "key: %s", msg );
	/* We assume the value doest not overflow with the buffer */
	ib_assume ( ret < size );
}

sctk_ib_cm_qp_connection_t sctk_ib_qp_keys_convert ( char *msg )
{
	sctk_ib_cm_qp_connection_t keys;
	sscanf ( msg, "%08x:%08x:%08x", ( unsigned int * ) &keys.lid,
	         ( unsigned int * ) &keys.qp_num,
	         ( unsigned int * ) &keys.psn );
	sctk_ib_qp_key_print ( &keys );

	return keys;
}

/*-----------------------------------------------------------
 *  Queue Pair Keys: Used for the ring connection
 *----------------------------------------------------------*/
void sctk_ib_qp_keys_send ( struct sctk_ib_rail_info_s *rail_ib, sctk_ib_qp_t *remote )
{
	LOAD_DEVICE ( rail_ib );
	int key_max = sctk_pmi_get_max_key_len();
	int val_max = sctk_pmi_get_max_val_len();
	
	char * key = sctk_malloc( sizeof(char) * key_max + 1 );
	assume( key != NULL );
	
	char * val = sctk_malloc( sizeof(char) * val_max + 1 );
	assume( val != NULL );
	
	int ret;

	sctk_ib_cm_qp_connection_t qp_keys =
	{
		.lid = device->port_attr.lid,
		.qp_num = remote->qp->qp_num,
		.psn = remote->psn,
	};

	sctk_ib_qp_key_create_key ( key, key_max, rail_ib->rail->rail_number, sctk_process_rank, remote->rank );
	sctk_ib_qp_key_create_value ( val, val_max, &qp_keys );
	ret = sctk_pmi_put_connection_info_str ( val, key );
	ib_assume ( ret == SCTK_PMI_SUCCESS );
	
	sctk_free( key );
	sctk_free( val );
}

sctk_ib_cm_qp_connection_t sctk_ib_qp_keys_recv ( struct sctk_ib_rail_info_s *rail_ib,
                                                  int dest_process )
{
	sctk_ib_cm_qp_connection_t qp_keys;
	int key_max = sctk_pmi_get_max_key_len();
	int val_max = sctk_pmi_get_max_val_len();
	
	char * key = sctk_malloc( sizeof(char) * key_max  + 1);
	assume( key != NULL );
	
	char * val = sctk_malloc( sizeof(char) * val_max + 1);
	assume( val != NULL );

	int ret;

	sctk_ib_qp_key_create_key ( key, key_max, rail_ib->rail->rail_number, dest_process, sctk_process_rank );
	ret = sctk_pmi_get_connection_info_str ( val, val_max, key );
	ib_assume ( ret == SCTK_PMI_SUCCESS );
	qp_keys = sctk_ib_qp_keys_convert ( val );

	sctk_free( key );
	sctk_free( val );

	return qp_keys;
}

/*-----------------------------------------------------------
 *  Queue Pair Keys
 *----------------------------------------------------------*/
sctk_ib_qp_t *sctk_ib_qp_new()
{
	sctk_ib_qp_t *remote;

	remote = sctk_malloc ( sizeof ( sctk_ib_qp_t ) );
	ib_assume ( remote );
	memset ( remote, 0, sizeof ( sctk_ib_qp_t ) );

	return remote;
}

void sctk_ib_qp_destroy ( sctk_ib_qp_t *remote )
{
	if ( !remote )
	{
		SCTK_IB_ABORT ( "Trying to free a remote entry which is not initialized" );
	}

	/* destroy the QP */
	int ret = ibv_destroy_qp ( remote->qp );
	if(ret)
		sctk_fatal("Failure to destroy QP: %s", strerror(ret));
	/* We do not remove the entry. */
	 free(remote); 
}

struct ibv_qp *sctk_ib_qp_init ( struct sctk_ib_rail_info_s *rail_ib, sctk_ib_qp_t *remote, struct ibv_qp_init_attr *attr, int rank )
{
	LOAD_DEVICE ( rail_ib );

	remote->qp = ibv_create_qp ( device->pd, attr );
	PROF_INC ( rail_ib->rail, ib_qp_created );
        if (!remote->qp) {
          sctk_warning("IB issue: try to reduce cap.max_send_wr %d -> %d",
                       attr->cap.max_send_wr, attr->cap.max_send_wr / 3);
          attr->cap.max_send_wr = attr->cap.max_send_wr / 3;
          remote->qp = ibv_create_qp(device->pd, attr);
        }

        if (!remote->qp) {
          SCTK_IB_ABORT("Cannot create QP for rank %d", rank);
        }

        sctk_nodebug("QP Initialized for rank %d %p", remote->rank, remote->qp);

        /* Add QP to HT */
        struct sctk_ib_qp_s *rem;
        rem = sctk_ib_qp_ht_find(rail_ib, remote->qp->qp_num);

        if (rem == NULL) {
          sctk_ib_qp_ht_add(rail_ib, remote, remote->qp->qp_num);
        }

        return remote->qp;
}

void sctk_ib_qp_free_all(struct sctk_ib_rail_info_s *rail_ib)
{
	struct sctk_ib_qp_ht_s* head = rail_ib->remotes, *tofree, *tmp;

	HASH_ITER(hh, head, tofree, tmp)
	{
		HASH_DELETE(hh, head, tofree);
		sctk_ib_qp_destroy(tofree->remote);
		tofree->remote = NULL;
	}
	assert(HASH_CNT(hh, head) == 0);
}

struct ibv_qp_init_attr sctk_ib_qp_init_attr ( struct sctk_ib_rail_info_s *rail_ib )
{
	LOAD_DEVICE ( rail_ib );
	LOAD_CONFIG ( rail_ib );
	struct ibv_qp_init_attr attr;
	memset ( &attr, 0, sizeof ( struct ibv_qp_init_attr ) );

	attr.send_cq  = device->send_cq;
	attr.recv_cq  = device->recv_cq;
	attr.srq      = device->srq;
	attr.cap.max_send_wr  = config->qp_tx_depth;
        if (attr.cap.max_send_wr > (unsigned int)device->dev_attr.max_qp_wr) {
          attr.cap.max_send_wr = device->dev_attr.max_qp_wr;
        }
        attr.cap.max_recv_wr = config->qp_rx_depth;
        if (attr.cap.max_recv_wr > (unsigned int)device->dev_attr.max_qp_wr) {
          attr.cap.max_recv_wr = device->dev_attr.max_qp_wr;
        }
        attr.cap.max_send_sge = config->max_sg_sq;
        attr.cap.max_recv_sge = config->max_sg_rq;
        attr.cap.max_inline_data = config->max_inline;
        /* RC Transport by default */
        attr.qp_type = IBV_QPT_RC;
        /* if this value is set to 1, all work requests (WR) will
        * generate completion queue events (CQE). If this value is set to 0,
        * only WRs that are flagged will generate CQE's*/
        attr.sq_sig_all = 0;
        return attr;
}

struct ibv_qp_attr sctk_ib_qp_state_init_attr ( struct sctk_ib_rail_info_s *rail_ib, int *flags )
{
	LOAD_CONFIG ( rail_ib );
	LOAD_DEVICE ( rail_ib );
	struct ibv_qp_attr attr;
	memset ( &attr, 0, sizeof ( struct ibv_qp_attr ) );

	/*init state */
	attr.qp_state = IBV_QPS_INIT;
	/* pkey index, normally 0 */
	attr.pkey_index = device->index_pkey;
	/* physical port number (1 .. n) */
	attr.port_num = config->adm_port;
	attr.qp_access_flags = IBV_ACCESS_REMOTE_WRITE
	                       | IBV_ACCESS_LOCAL_WRITE
	                       | IBV_ACCESS_REMOTE_READ
	                       | IBV_ACCESS_REMOTE_ATOMIC;

	*flags = IBV_QP_STATE
	         | IBV_QP_PKEY_INDEX
	         | IBV_QP_PORT
	         | IBV_QP_ACCESS_FLAGS;

	return attr;
}

struct ibv_qp_attr sctk_ib_qp_state_rtr_attr ( struct sctk_ib_rail_info_s *rail_ib, sctk_ib_cm_qp_connection_t *keys, int *flags )
{
	LOAD_CONFIG ( rail_ib );
	struct ibv_qp_attr attr;
	memset ( &attr, 0, sizeof ( struct ibv_qp_attr ) );

	attr.qp_state = IBV_QPS_RTR;
	/* 512 is the recommended value */
	attr.path_mtu = IBV_MTU_2048;
	/* QP number of remote QP */
	/* maximul number if resiyrces for incoming RDMA request */
	attr.max_dest_rd_atomic = config->rdma_depth;
	
	/* number or outstanding RDMA reads and atomic operations allowed */
	attr.max_rd_atomic = config->rdma_depth;

	/* maximum RNR NAK timer (recommanded value: 12) */
	attr.min_rnr_timer = 12;

	/* an address handle (AH) needs to be created and filled in as appropriate. */
	attr.ah_attr.is_global = 0;
	/* destination LID */
	attr.ah_attr.dlid = keys->lid;
	/* QP number of remote QP */
	attr.dest_qp_num = keys->qp_num;
	attr.rq_psn = keys->psn;
	/* service level */
	attr.ah_attr.sl = 0;
	/* source path bits */
	attr.ah_attr.src_path_bits = 0;
	attr.ah_attr.port_num = config->adm_port;

	*flags = IBV_QP_STATE
	         | IBV_QP_AV
	         | IBV_QP_PATH_MTU
	         | IBV_QP_DEST_QPN
	         | IBV_QP_RQ_PSN
	         | IBV_QP_MAX_DEST_RD_ATOMIC
	         | IBV_QP_MIN_RNR_TIMER;

	return attr;
}

struct ibv_qp_attr sctk_ib_qp_state_rts_attr ( struct sctk_ib_rail_info_s *rail_ib, sctk_uint32_t psn, int *flags )
{
	LOAD_CONFIG ( rail_ib );
	struct ibv_qp_attr attr;
	memset ( &attr, 0, sizeof ( struct ibv_qp_attr ) );

	attr.qp_state = IBV_QPS_RTS;
	/* local ACK timeout (recommanted value: 14) */
	attr.timeout = 14;
	/* retry count (recommended value: 7) */
	attr.retry_cnt = 7;
	/* RNR retry count (recommended value: 7) */
	attr.rnr_retry = 7;
	/* packet sequence number */
	attr.sq_psn = psn;

	/* maximul number if resiyrces for incoming RDMA request */
	attr.max_dest_rd_atomic = config->rdma_depth;
	
	/* number or outstanding RDMA reads and atomic operations allowed */
	attr.max_rd_atomic = config->rdma_depth;

	*flags = IBV_QP_STATE
	         | IBV_QP_TIMEOUT
	         | IBV_QP_RETRY_CNT
	         | IBV_QP_RNR_RETRY
	         | IBV_QP_SQ_PSN
	         | IBV_QP_MAX_QP_RD_ATOMIC;

	return attr;
}

struct ibv_qp_attr sctk_ib_qp_STATE_RESET_attr ( int *flags )
{
	struct ibv_qp_attr attr;
	memset ( &attr, 0, sizeof ( struct ibv_qp_attr ) );

	/* Reset the state of the QP */
	attr.qp_state = IBV_QPS_RESET;

	*flags = IBV_QP_STATE;

	return attr;
}


void sctk_ib_qp_modify ( sctk_ib_qp_t *remote, struct ibv_qp_attr *attr, int flags )
{
	sctk_nodebug ( "Modify QP for remote %p to state %d", remote, attr->qp_state );

	if ( ibv_modify_qp ( remote->qp, attr, flags ) != 0 )
	{
		SCTK_IB_ABORT_WITH_ERRNO ( "Cannot modify Queue Pair" );
	}
}

/*-----------------------------------------------------------
 *  Shared Receive queue
 *----------------------------------------------------------*/
struct ibv_srq *sctk_ib_srq_init ( struct sctk_ib_rail_info_s *rail_ib, struct ibv_srq_init_attr *attr )
{
	LOAD_DEVICE ( rail_ib );
	LOAD_CONFIG ( rail_ib );
	device->srq = ibv_create_srq ( device->pd, attr );

	if ( !device->srq )
	{
		SCTK_IB_ABORT_WITH_ERRNO ( "Cannot create Shared Received Queue" );
		sctk_abort();
	}

	//  config->max_srq_ibufs_posted = attr->attr.max_wr;
	sctk_ib_debug ( "Initializing SRQ with %d entries (max:%d)",
	                attr->attr.max_wr, sctk_ib_srq_get_max_srq_wr ( rail_ib ) );
	config->srq_credit_limit = config->max_srq_ibufs_posted / 2;

	return device->srq;
}

void sctk_ib_srq_free(sctk_ib_rail_info_t *rail_ib)
{
	LOAD_DEVICE (rail_ib);
	int ret = ibv_destroy_srq(device->srq);
	if(ret)
		sctk_fatal("Failure to destroy the SRQ: %s", strerror(ret));
	device->srq = NULL;
}

struct ibv_srq_init_attr sctk_ib_srq_init_attr ( struct sctk_ib_rail_info_s *rail_ib )
{
	LOAD_CONFIG ( rail_ib );
	struct ibv_srq_init_attr attr;

	memset ( &attr, 0, sizeof ( struct ibv_srq_init_attr ) );

	attr.attr.srq_limit = config->srq_credit_thread_limit;
	attr.attr.max_wr = sctk_ib_srq_get_max_srq_wr ( rail_ib );
	attr.attr.max_sge = config->max_sg_rq;

	return attr;
}

TODO ( "Check the max WR !!" )
int sctk_ib_srq_get_max_srq_wr ( struct sctk_ib_rail_info_s *rail_ib )
{
	LOAD_DEVICE ( rail_ib );
	return device->dev_attr.max_srq_wr;
}

int sctk_ib_qp_get_cap_flags ( struct sctk_ib_rail_info_s *rail_ib )
{
	LOAD_DEVICE ( rail_ib );
	return device->dev_attr.device_cap_flags;
}

char* qp_states[] = { "RESET", "INIT", "RTR", "RTS", "SQD", "SQE", "ERR", "UNK"};

char * sctk_ib_qp_print_state( struct ibv_qp *qp)
{
	struct ibv_qp_attr attr;
	struct ibv_qp_init_attr init_attr;
	int ret = ibv_query_qp(qp, &attr , IBV_QP_STATE, &init_attr);
	if(!ret)
	{
		switch(attr.qp_state)
		{
			case IBV_QPS_RESET: return qp_states[0]; break;
			case IBV_QPS_INIT: return qp_states[1]; break;
			case IBV_QPS_RTR: return qp_states[2]; break;
			case IBV_QPS_RTS: return qp_states[3]; break;
			case IBV_QPS_SQD: return qp_states[4]; break;
			case IBV_QPS_SQE: return qp_states[5]; break;
			case IBV_QPS_ERR: return qp_states[6]; break;
			case IBV_QPS_UNKNOWN: return qp_states[7]; break;
		}
	}
	sctk_fatal("Unable to query the QP !");
	return NULL;
}
	

/*-----------------------------------------------------------
 *  ALLOCATION
 *----------------------------------------------------------*/
void sctk_ib_qp_allocate_init ( struct sctk_ib_rail_info_s *rail_ib, int rank, sctk_ib_qp_t *remote, int ondemand, sctk_endpoint_t *endpoint )
{
	LOAD_CONFIG ( rail_ib );
	LOAD_DEVICE ( rail_ib );
	sctk_ib_qp_ondemand_t *od = &device->ondemand;
	struct ibv_qp_init_attr  init_attr;
	struct ibv_qp_attr       attr;
	int flags;

	sctk_nodebug ( "QP reinited for rank %d", rank );

	remote->endpoint = endpoint;
	remote->psn = lrand48 () & 0xffffff;
	remote->rank = rank;
	remote->free_nb = config->qp_tx_depth;
	remote->post_lock = SCTK_SPINLOCK_INITIALIZER;
	/* For buffered eager */
	remote->ib_buffered.entries = NULL;
	remote->ib_buffered.lock = SCTK_SPINLOCK_INITIALIZER;
	remote->unsignaled_counter = 0;
	OPA_store_int ( &remote->ib_buffered.number, 0 );

	/* Add it to the Cicrular Linked List if the QP is created from
	* an ondemand request */
	if ( ondemand )
	{
		remote->R = 1;
		remote->ondemand = 1;
		sctk_spinlock_lock ( &od->lock );
		sctk_nodebug ( "[%d] Add QP to rank %d %p", rail_ib->rail->rail_number, remote->rank, remote );
		CDL_PREPEND ( od->qp_list, remote );

		if ( od->qp_list_ptr == NULL )
		{
			od->qp_list_ptr = remote;
		}

		sctk_spinlock_unlock ( &od->lock );
	}

	init_attr = sctk_ib_qp_init_attr ( rail_ib );
	sctk_ib_qp_init ( rail_ib, remote, &init_attr, rank );
	sctk_ib_qp_allocate_set_rtr ( remote, 0 );
	sctk_ib_qp_allocate_set_rts ( remote, 0 );
	sctk_ib_qp_set_pending_data ( remote, 0 );
	remote->rdma.previous_max_pending_data = 0;
	sctk_ib_qp_set_deco_canceled ( remote, ACK_OK );

	sctk_ib_qp_set_local_flush_ack ( remote, ACK_UNSET );
	sctk_ib_qp_set_remote_flush_ack ( remote, ACK_UNSET );

	remote->lock_rtr = SCTK_SPINLOCK_INITIALIZER;
	remote->lock_rts = SCTK_SPINLOCK_INITIALIZER;
	/* Lock for sending messages */
	remote->lock_send = SCTK_SPINLOCK_INITIALIZER;
	remote->flushing_lock = SCTK_SPINLOCK_INITIALIZER;
	/* RDMA */
	sctk_ibuf_rdma_remote_init ( remote );

	attr = sctk_ib_qp_state_init_attr ( rail_ib, &flags );
	sctk_ib_qp_modify ( remote, &attr, flags );
}

void sctk_ib_qp_allocate_rtr ( struct sctk_ib_rail_info_s *rail_ib, sctk_ib_qp_t *remote, sctk_ib_cm_qp_connection_t *keys )
{
	struct ibv_qp_attr       attr;
	int flags;

	attr = sctk_ib_qp_state_rtr_attr ( rail_ib, keys, &flags );
	sctk_nodebug ( "Modify QR RTR for rank %d", remote->rank );
	sctk_ib_qp_modify ( remote, &attr, flags );
	sctk_ib_qp_allocate_set_rtr ( remote, 1 );
}

void sctk_ib_qp_allocate_rts ( struct sctk_ib_rail_info_s *rail_ib,  sctk_ib_qp_t *remote )
{
	struct ibv_qp_attr       attr;
	int flags;

	attr = sctk_ib_qp_state_rts_attr ( rail_ib, remote->psn, &flags );
	sctk_nodebug ( "Modify QR RTS for rank %d", remote->rank );
	sctk_ib_qp_modify ( remote, &attr, flags );
	sctk_ib_qp_allocate_set_rts ( remote, 1 );
}

void sctk_ib_qp_allocate_reset ( __UNUSED__ struct sctk_ib_rail_info_s *rail_ib, sctk_ib_qp_t *remote )
{
	struct ibv_qp_attr       attr;
	sctk_endpoint_t *endpoint = remote->endpoint;
	int flags;

	attr = sctk_ib_qp_STATE_RESET_attr ( &flags );
	sctk_nodebug ( "Modify QR RESET for rank %d", remote->rank );
	sctk_ib_qp_modify ( remote, &attr, flags );

	sctk_ib_qp_allocate_set_rtr ( remote, 0 );
	sctk_ib_qp_allocate_set_rts ( remote, 0 );
}


/*-----------------------------------------------------------
 *  Send a message to a remote QP
 *----------------------------------------------------------*/
/*
 * Returns if the counter has to be reset because a signaled message needs to be send
 */
static int check_signaled ( struct sctk_ib_rail_info_s *rail_ib, sctk_ib_qp_t *remote, sctk_ibuf_t *ibuf )
{
	LOAD_CONFIG ( rail_ib );
	char is_signaled = ( ibuf->desc.wr.send.send_flags & IBV_SEND_SIGNALED ) ? 1 : 0;

	if ( ! is_signaled )
	{
		if ( remote->unsignaled_counter + 1 > ( config->qp_tx_depth >> 1 ) )
		{
			/*ibuf->desc.wr.send.send_flags | IBV_SEND_SIGNALED;*/
			return 1;
		}
		else
		{
			return 0;
		}
	}
	else
	{
		return 1;
	}

	return 0;
}

static void inc_signaled ( struct sctk_ib_rail_info_s *rail_ib, sctk_ib_qp_t *remote, int need_reset )
{
	LOAD_CONFIG ( rail_ib );

	if ( need_reset )
	{
		remote->unsignaled_counter = 0;
	}
	else
	{
		remote->unsignaled_counter ++;
	}
}


struct wait_send_s
{
	struct sctk_ib_rail_info_s *rail_ib;
	int flag;
	sctk_ib_qp_t *remote;
	sctk_ibuf_t *ibuf;
};

void sctk_ib_qp_release_entry ( __UNUSED__ struct sctk_ib_rail_info_s *rail_ib,  __UNUSED__ sctk_ib_qp_t *remote )
{
	not_implemented();
}

extern void sctk_network_notify_idle_message_multirail_ib_wait_send ();

static void *wait_send ( void *arg )
{
	struct wait_send_s *a = ( struct wait_send_s * ) arg;
	int rc;

	PROF_TIME_START ( a->rail_ib->rail, ib_post_send );

	sctk_spinlock_lock ( &a->remote->lock_send );

	int need_reset = check_signaled ( a->rail_ib, a->remote, a->ibuf );

	rc = ibv_post_send ( a->remote->qp, & ( a->ibuf->desc.wr.send ), & ( a->ibuf->desc.bad_wr.send ) );

	if ( rc == 0 )
		inc_signaled ( a->rail_ib, a->remote, need_reset );

	sctk_spinlock_unlock ( &a->remote->lock_send );

	PROF_TIME_END ( a->rail_ib->rail, ib_post_send );

	if ( rc == 0 )
	{
		a->flag = 1;
	}
	else
	{
		sctk_network_notify_idle_message_multirail_ib_wait_send();
		/*     sctk_notify_idle_message (); */
	}

	return NULL;
}



/* Send a message without locks. Messages which are sent are not blocked by locks.
 * This is useful for QP deconnexion */
static inline void __send_ibuf ( struct sctk_ib_rail_info_s *rail_ib, sctk_ib_qp_t *remote, sctk_ibuf_t *ibuf )
{
	int rc;

	sctk_nodebug ( "[%d] Send no-lock message to process %d %p %d", rail_ib->rail_nb, remote->rank, remote->qp, rail_ib->rail->rail_number );

	ibuf->remote = remote;

	/* We lock here because ibv_post_send uses mutexes which provide really bad performances.
	* Instead we encapsulate the call between spinlocks */
	PROF_TIME_START ( rail_ib->rail, ib_post_send_lock );
	sctk_spinlock_lock ( &remote->lock_send );
	PROF_TIME_END ( rail_ib->rail, ib_post_send_lock );

	PROF_TIME_START ( rail_ib->rail, ib_post_send );

	int need_reset = check_signaled ( rail_ib, remote, ibuf );

	rc = ibv_post_send ( remote->qp, & ( ibuf->desc.wr.send ), & ( ibuf->desc.bad_wr.send ) );

	PROF_TIME_END ( rail_ib->rail, ib_post_send );

	if ( rc == 0 )
		inc_signaled ( rail_ib, remote, need_reset );

	sctk_spinlock_unlock ( &remote->lock_send );

	if ( rc != 0 )
	{
		struct wait_send_s wait_send_arg;
		wait_send_arg.flag = 0;
		wait_send_arg.remote = remote;
		wait_send_arg.ibuf = ibuf;
		wait_send_arg.rail_ib = rail_ib;

		//#warning "We should remove these sctk_error and use a counter instead"
		sctk_warning ( "[%d] NO LOCK QP full for remote %d, waiting for posting message... (pending: %d)", rail_ib->rail->rail_number,
		               remote->rank, sctk_ib_qp_get_pending_data ( remote ) );
		sctk_thread_wait_for_value_and_poll ( &wait_send_arg.flag, 1,
		                                      ( void ( * ) ( void * ) ) wait_send, &wait_send_arg );

		sctk_warning ( "[%d] NO LOCK QP full for remote %d, waiting for posting message... (pending: %d) DONE", rail_ib->rail->rail_number,
		               remote->rank, sctk_ib_qp_get_pending_data ( remote ) );
		sctk_nodebug ( "[%d] NO LOCK QP message sent to remote %d", rail_ib->rail->rail_number, remote->rank );
	}

	sctk_nodebug ( "SENT no-lock message to process %d %p", remote->rank, remote->qp );
}

/* Send an ibuf to a remote.
 */
int sctk_ib_qp_send_ibuf ( struct sctk_ib_rail_info_s *rail_ib, sctk_ib_qp_t *remote, sctk_ibuf_t *ibuf )
{
#ifdef IB_DEBUG

	if ( IBUF_GET_PROTOCOL ( ibuf->buffer )  ==  SCTK_IB_NULL_PROTOCOL )
	{
		sctk_ib_toolkit_print_backtrace();
		not_reachable();
	}

#endif


	/* We inc the number of pending requests */
	sctk_ib_qp_add_pending_data ( remote, ibuf );

	__send_ibuf ( rail_ib, remote, ibuf );

	/* We release the buffer if it has been inlined & if it
	* do not generate an event once the transmission done */
		
	if ( ( ibuf->flag == SEND_INLINE_IBUF_FLAG
	|| ibuf->flag == RDMA_WRITE_INLINE_IBUF_FLAG )
	&& ( ( ( ibuf->desc.wr.send.send_flags & IBV_SEND_SIGNALED ) == 0 ) ) )
	{

		struct sctk_ib_polling_s poll;
		POLL_INIT ( &poll );

		/* Decrease the number of pending requests */
		int current_pending;
		current_pending = sctk_ib_qp_fetch_and_sub_pending_data ( remote, ibuf );
		sctk_ibuf_rdma_update_max_pending_data ( remote, current_pending );
		sctk_network_poll_send_ibuf ( rail_ib->rail, ibuf);
		return 0;
	}


	return 1;
}

#endif
