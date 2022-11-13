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

#include "mpc_lowcomm_types.h"
#include "endpoint.h"
#include "sctk_ptl_rdma.h"
#include "sctk_ptl_iface.h"
#include "mpc_common_asm.h"

/**
 * Global pointer to the RDMA PT entry.
 */
static sctk_ptl_pte_t* rdma_pte = NULL;

/**
 * Convert a MPC type to Portals one.
 * \param[in] type the MPC type
 * \return Portals type
 */
static inline ptl_datatype_t __sctk_ptl_convert_type(RDMA_type type)
{
	switch(type)
	{
		case RDMA_TYPE_CHAR               : return PTL_INT8_T      ; break ;
		case RDMA_TYPE_DOUBLE             : return PTL_DOUBLE      ; break ;
		case RDMA_TYPE_FLOAT              : return PTL_FLOAT       ; break ;
		case RDMA_TYPE_INT                : return PTL_INT32_T     ; break ;
		case RDMA_TYPE_LONG_DOUBLE        : return PTL_LONG_DOUBLE ; break ;
		case RDMA_TYPE_LONG               : return PTL_INT64_T     ; break ;
		case RDMA_TYPE_LONG_LONG          : return PTL_INT64_T     ; break ;
		case RDMA_TYPE_LONG_LONG_INT      : return PTL_INT64_T     ; break ;
		case RDMA_TYPE_SHORT              : return PTL_INT16_T     ; break ;
		case RDMA_TYPE_SIGNED_CHAR        : return PTL_INT8_T      ; break ;
		case RDMA_TYPE_UNSIGNED_CHAR      : return PTL_UINT8_T     ; break ;
		case RDMA_TYPE_UNSIGNED           : return PTL_UINT32_T    ; break ;
		case RDMA_TYPE_UNSIGNED_LONG      : return PTL_UINT64_T    ; break ;
		case RDMA_TYPE_UNSIGNED_LONG_LONG : return PTL_UINT64_T    ; break ;
		case RDMA_TYPE_UNSIGNED_SHORT     : return PTL_UINT16_T    ; break ;
		case RDMA_TYPE_WCHAR              : return PTL_INT16_T     ; break ;
		default: 
			mpc_common_debug_fatal("Type not handled by Portals: %d", type);
	}
	return 0;
}

/**
 * Convert an MPC operation to Portals one.
 * \param[in] op the MPC op
 * \return the Portals op
 */
static inline ptl_op_t __sctk_ptl_convert_op(RDMA_op op)
{
	switch(op)
	{
		case RDMA_SUM  : return PTL_SUM  ; break ;
		case RDMA_MIN  : return PTL_MIN  ; break ;
		case RDMA_MAX  : return PTL_MAX  ; break ;
		case RDMA_PROD : return PTL_PROD ; break ;
		case RDMA_LAND : return PTL_LAND ; break ;
		case RDMA_BAND : return PTL_BAND ; break ;
		case RDMA_LOR  : return PTL_LOR  ; break ;
		case RDMA_BOR  : return PTL_BOR  ; break ;
		case RDMA_LXOR : return PTL_LXOR ; break ;
		case RDMA_BXOR : return PTL_BXOR ; break ;
		default:
			mpc_common_debug_fatal("Operation not supported by Portals %d", op);
	}
	return 0;
}

/**
 * Check if the given operation is unary (INC | DEC).
 * If so, prepare a buffer with the adequat increment/decrement.
 * \param[in] op the MPC RDMA operation
 * \param[in] type the MPC RDMA type
 * \param[out] buf the buffer to store the increment/decrement.
 * \param[out] size buf size.
 */
static inline short __sctk_ptl_is_unary_op(RDMA_op op, RDMA_type type, char* buf, size_t size)
{
	if(op != RDMA_INC && op != RDMA_DEC)
	{
		return 0;
	}

	assert(size >= RDMA_type_size(type));
	
	memset(buf, 0, size);
	if(op == RDMA_INC)
	{
		switch(type)
		{
			case RDMA_TYPE_CHAR:
			case RDMA_TYPE_UNSIGNED_CHAR:
			case RDMA_TYPE_SIGNED_CHAR:
			{
				int8_t x = 1;
				memcpy(buf, &x, sizeof(int8_t));
				break;
			}
			case RDMA_TYPE_SHORT:
			case RDMA_TYPE_WCHAR:
			case RDMA_TYPE_UNSIGNED_SHORT:
			{
				int16_t x = 1;
				memcpy(buf, &x, sizeof(int16_t));
				break;
			}
			case RDMA_TYPE_DOUBLE:
			{
				double x = 1.0;
				memcpy(buf, &x, sizeof(double));
				break;
			}
			case RDMA_TYPE_FLOAT:
			{
				float x = 1.0;
				memcpy(buf, &x, sizeof(float));
				break;
			}
			case RDMA_TYPE_INT:
			case RDMA_TYPE_UNSIGNED:
			{
				int32_t x = 1;
				memcpy(buf, &x, sizeof(int32_t));
				break;
			}
			case RDMA_TYPE_LONG_LONG:
			case RDMA_TYPE_LONG:
			case RDMA_TYPE_LONG_LONG_INT:
			case RDMA_TYPE_UNSIGNED_LONG:
			case RDMA_TYPE_UNSIGNED_LONG_LONG:
			{
				int64_t x = 1;
				memcpy(buf, &x, sizeof(int64_t));
				break;
			}
			case RDMA_TYPE_LONG_DOUBLE:
			{
				long double x = 1.0;
				memcpy(buf, &x, sizeof(long double));
				break;
			}
			default:
				not_reachable();
		}
	}
	else if( op == RDMA_DEC)
	{
		switch(type)
		{
			case RDMA_TYPE_CHAR:
			case RDMA_TYPE_UNSIGNED_CHAR:
			case RDMA_TYPE_SIGNED_CHAR:
			{
				int8_t x = -1;
				memcpy(buf, &x, sizeof(int8_t));
				break;
			}
			case RDMA_TYPE_SHORT:
			case RDMA_TYPE_WCHAR:
			case RDMA_TYPE_UNSIGNED_SHORT:
			{
				int16_t x = -1;
				memcpy(buf, &x, sizeof(int16_t));
				break;
			}
			case RDMA_TYPE_DOUBLE:
			{
				double x = -1.0;
				memcpy(buf, &x, sizeof(double));
				break;
			}
			case RDMA_TYPE_FLOAT:
			{
				float x = -1.0;
				memcpy(buf, &x, sizeof(float));
				break;
			}
			case RDMA_TYPE_INT:
			case RDMA_TYPE_UNSIGNED:
			{
				int32_t x = -1;
				memcpy(buf, &x, sizeof(int32_t));
				break;
			}
			case RDMA_TYPE_LONG_LONG:
			case RDMA_TYPE_LONG:
			case RDMA_TYPE_LONG_LONG_INT:
			case RDMA_TYPE_UNSIGNED_LONG:
			case RDMA_TYPE_UNSIGNED_LONG_LONG:
			{
				int64_t x = -1;
				memcpy(buf, &x, sizeof(int64_t));
				break;
			}
			case RDMA_TYPE_LONG_DOUBLE:
			{
				long double x = -1.0;
				memcpy(buf, &x, sizeof(long double));
				break;
			}
			default:
				not_reachable();
		}
	}

	return 1;
}

/** boolean to check if Portals support 'fetch_and_op', which it supports */
int sctk_ptl_rdma_fetch_and_op_gate( sctk_rail_info_t *rail, size_t size, RDMA_op op, RDMA_type type )
{
	UNUSED(rail); 
	UNUSED(size);
	UNUSED(type);
	/* trouble with PtlFetchAtomic() & the BXI, disabling the call, fallback w/ CMs */
	return 0;
	/* don't support directly INC & DEC w/ Portals... */
	return (op != RDMA_INC && op != RDMA_DEC);
}

/**
 * Implement the fetch_and_op method for Portals.
 *
 * \param[in] rail the Portals rail
 * \param[in] msg the built msg for this request
 * \param[in] fetch_addr the addr to store the previous data (remote)
 * \param[in] local_key the local RDMA struct, associated to fetch_addr
 * \param[in] remote_addr the addr to update
 * \param[in] remote_key the remote RDMA struct, associated to remote_addr
 * \param[in] addr the data to add to remote_addr
 * \param[in] op the operation to apply (remote_addr op add)
 * \param[in] type the type of fetch_addr, remote_addr & add
 */
#define BUF_SIZE 32
void sctk_ptl_rdma_fetch_and_op(  sctk_rail_info_t *rail,
		mpc_lowcomm_ptp_message_t *msg,
		void * fetch_addr,
		struct  sctk_rail_pin_ctx_list * local_key,
		void * remote_addr,
		struct  sctk_rail_pin_ctx_list * remote_key,
		void * add,
		RDMA_op op,
		RDMA_type type )
{
	sctk_ptl_rail_info_t* srail = &rail->network.ptl;
	sctk_ptl_id_t remote = remote_key->pin.ptl.origin;
	void* remote_start   = remote_key->pin.ptl.start;
	void* local_start    = local_key->pin.ptl.start;
	size_t local_off, remote_off, add_off, size;
	sctk_ptl_local_data_t* add_md = NULL;
	sctk_ptl_local_data_t* copy = NULL;
	char buf[BUF_SIZE];

	size = RDMA_type_size(type);

	if(__sctk_ptl_is_unary_op(op, type, buf, BUF_SIZE))
	{
		add = (void*)(&buf);
	}

	/* sanity checks */
	assert(local_start == local_key->pin.ptl.md_data->slot.md.start);
	assert(fetch_addr  >= local_start);
	assert(fetch_addr + size <= local_start + local_key->pin.ptl.md_data->slot.md.length);
	assert(remote_addr >= remote_start);
	
	local_off  = fetch_addr  - local_start;
	remote_off = remote_addr - remote_start;

	add_md = sctk_ptl_md_create(srail, add, size, SCTK_PTL_MD_ATOMICS_FLAGS);
	add_off = 0;
	sctk_ptl_md_register(srail, add_md);

	add_md->type = SCTK_PTL_TYPE_RDMA;
	add_md->msg = NULL;
	
	/* this is dirty: see comments in sctk_ptl_pin_region() */
	copy                   = sctk_malloc(sizeof(sctk_ptl_local_data_t));
	*copy                  = *local_key->pin.ptl.md_data;
	copy->msg              = msg;
	msg->tail.ptl.user_ptr = add_md;
	copy->type = SCTK_PTL_TYPE_RDMA;
	
	sctk_ptl_emit_fetch_atomic(
		local_key->pin.ptl.md_data,   /* GET MD --> fetch_addr */
		add_md,                       /* PUT MD  --> add */
		size,                         /* request size */
		remote,                       /* target */
		rdma_pte,                     /* PT entry */
		remote_key->pin.ptl.match,    /* unique match ID */
		local_off,                    /* local window offset */
		add_off,                      /* because GET MD is locally created */
		remote_off,                   /* remote window offset */
		__sctk_ptl_convert_op(op),    /* element used for comparison (same type as others) */
		__sctk_ptl_convert_type(type),/* Portals-converted RDMA type */
		copy

	);
	mpc_common_debug("PORTALS: SEND-FETCH-AND-OP (match=%s, loff=%llu, roff=%llu, op=%d, add=%p, sz=%llu)", __sctk_ptl_match_str(sctk_malloc(32), 32, remote_key->pin.ptl.match.raw), local_off, remote_off, op, add_md, size);
}

/** boolean to check if Portals support 'compare_and_swap', which it supports */
int sctk_ptl_rdma_cas_gate( sctk_rail_info_t *rail, size_t size, RDMA_type type )
{
	UNUSED(rail);
	UNUSED(size);
	UNUSED(type);
	return 1;
}

/**
 * Implement the compare_and_swap metho for Portals.
 *
 * Please be aware that the new value can be outside of any previously-pinned region.
 * Thus, we first have to create a pinned segment (=MD) for this small piece of data.
 * One idea could be to maintain a global MD segment for this purpose, and just memcpy() the data
 * when the request completes.
 *
 * \param[in] rail the Portail rail
 * \param[in] msg the MPC built msg
 * \param[in] res_addr the data where the result will be stored
 * \param[in] local_key the local RDMA struct, associated to res_addr
 * \param[in] remote_addr the data to access to if to compare & swap
 * \param[in] remote_key the remote RDMA struct, associated to remote_addr
 * \param[in] comp the 'comp' attribute
 * \param[in] new the value to update if the compare matched
 * \param[in] type the type of res_addr, remote_addr, comp & new
 */
void sctk_ptl_rdma_cas(sctk_rail_info_t *rail,
		mpc_lowcomm_ptp_message_t *msg,
		void *  res_addr,
		struct  sctk_rail_pin_ctx_list * local_key,
		void * remote_addr,
		struct  sctk_rail_pin_ctx_list * remote_key,
		void * comp,
		void * new,
		RDMA_type type )
{
	sctk_ptl_rail_info_t* srail = &rail->network.ptl;
	sctk_ptl_id_t remote = remote_key->pin.ptl.origin;
	void* remote_start   = remote_key->pin.ptl.start;
	void* local_start    = local_key->pin.ptl.start;
	size_t local_off, remote_off, new_off, size;
	sctk_ptl_local_data_t* new_md = NULL;
	sctk_ptl_local_data_t* copy = NULL;

	size = RDMA_type_size(type);
	/* sanity checks */
	assert(local_start == local_key->pin.ptl.md_data->slot.md.start);
	assert(res_addr  >= local_start);
	assert(res_addr + size <= local_start + local_key->pin.ptl.md_data->slot.md.length);
	assert(remote_addr >= remote_start);
	
	local_off  = res_addr  - local_start;
	remote_off = remote_addr - remote_start;

	new_md = sctk_ptl_md_create(srail, new, size, SCTK_PTL_MD_ATOMICS_FLAGS);
	new_off = 0;
	sctk_ptl_md_register(srail, new_md);

	new_md->type = SCTK_PTL_TYPE_RDMA;
	new_md->msg = NULL;
	
	/* this is dirty: see comments in sctk_ptl_pin_region() */
	copy                   = sctk_malloc(sizeof(sctk_ptl_local_data_t));
	*copy                  = *local_key->pin.ptl.md_data;
	copy->msg              = msg;
	msg->tail.ptl.user_ptr = new_md;
	copy->type = SCTK_PTL_TYPE_RDMA;

	sctk_ptl_emit_swap(
		local_key->pin.ptl.md_data,   /* GET MD --> res_addr */
		new_md,                       /* PUT MD  --> new */
		size,                         /* request size */
		remote,                       /* target */
		rdma_pte,                     /* PT entry */
		remote_key->pin.ptl.match,    /* unique match ID */
		local_off,                    /* local window offset */
		new_off,                      /* because GET MD is locally created */
		remote_off,                   /* remote window offset */
		comp,                         /* element used for comparison (same type as others) */
		__sctk_ptl_convert_type(type),/* Portals-converted RDMA type */
		copy

	);
	mpc_common_nodebug("PORTALS: SEND-CAS (loff=%llu, roff=%llu, comp=%p, new=%p)", local_off, remote_off, comp, new);
}

/**
 * Implement the one-sided Write() method.
 *
 * Note that because we don't store the effective size of the remote region, 
 * we can't check that dest_addr is inside the pinned segment.
 *
 * Please see also notes in \see sctk_ptl_pin_region.
 * 
 *
 * \param[in] rail the Portals rail
 * \param[in] msg the built msg
 * \param[in] src_addr for WRITE, the local addr where data will be picked up
 * \param[in] local_key the local RDMA struct associated to src_addr
 * \param[in] dest_addr for WRITE, the remote addr where data will be written
 * \param[in] remote_key the remote RDMA struct associated to dest_addr
 * \param[in] size the amount of bytes to send
 */
void sctk_ptl_rdma_write(sctk_rail_info_t *rail, mpc_lowcomm_ptp_message_t *msg,
		void * src_addr, struct sctk_rail_pin_ctx_list * local_key,
		void * dest_addr, struct  sctk_rail_pin_ctx_list * remote_key,
		size_t size)
{
	UNUSED(rail);
	sctk_ptl_id_t remote = remote_key->pin.ptl.origin;
	void* remote_start   = remote_key->pin.ptl.start;
	void* local_start    = local_key->pin.ptl.start;
	size_t local_off, remote_off;
	sctk_ptl_local_data_t* copy = NULL;

	/* sanity checks */
	assert(local_start == local_key->pin.ptl.md_data->slot.md.start);
	assert(src_addr  >= local_start);
	assert(src_addr + size <= local_start + local_key->pin.ptl.md_data->slot.md.length);
	assert(dest_addr >= remote_start);
	
	/* compute offsets, WRITE --> src = local, dest = remote */
	local_off  = src_addr  - local_start;
	remote_off = dest_addr - remote_start;

	/* this is dirty: see comments in sctk_ptl_pin_region() */
	copy                   = sctk_malloc(sizeof(sctk_ptl_local_data_t));
	*copy                  = *local_key->pin.ptl.md_data;
	copy->msg              = msg;
	msg->tail.ptl.user_ptr = NULL; /* 'no extra allocated data' */
	msg->tail.ptl.copy     = 0;
	copy->type = SCTK_PTL_TYPE_RDMA;
	
	sctk_ptl_emit_put(
		local_key->pin.ptl.md_data, /* The base MD */
		size,                      /* request size */
		remote,                    /* target process */
		rdma_pte,                  /* Portals entry */
		remote_key->pin.ptl.match, /* match bits */
		local_off, remote_off,     /* offsets */
		0,                         /* Number of bytes sent */
		copy
	);
}

/**
 * Implement the one-sided Read() method.
 *
 * Note that because we don't store the effective size of the remote region, 
 * we can't check that src_addr is inside the pinned segment.
 *
 * Please see also notes in \see sctk_ptl_pin_region.
 * 
 *
 * \param[in] rail the Portals rail
 * \param[in] msg the built msg
 * \param[in] src_addr for READ, the remote addr where data will be read
 * \param[in] remote_key the remote RDMA struct associated to src_addr
 * \param[in] dest_addr for READ, the local addr where data will be written
 * \param[in] remote_key the local RDMA struct associated to dest_addr
 * \param[in] size the amount of bytes to send
 */
void sctk_ptl_rdma_read(sctk_rail_info_t *rail, mpc_lowcomm_ptp_message_t *msg,
		void * src_addr,  struct  sctk_rail_pin_ctx_list * remote_key,
		void * dest_addr, struct  sctk_rail_pin_ctx_list * local_key,
		size_t size)
{

	UNUSED(rail);
	sctk_ptl_id_t remote        = remote_key->pin.ptl.origin;
	void* remote_start          = remote_key->pin.ptl.start;
	void* local_start           = local_key->pin.ptl.start;
	sctk_ptl_local_data_t* copy = NULL;
	size_t local_off, remote_off;

	/* sanity checks */
	assert(local_start == local_key->pin.ptl.md_data->slot.md.start);
	assert(dest_addr + size <= local_start + local_key->pin.ptl.md_data->slot.md.length);
	assert(src_addr  >= remote_start);
	assert(dest_addr >= local_start);
	
	/* compute offsets, READ --> src = remote, dest = local */
	local_off  = dest_addr - local_start;
	remote_off = src_addr  - remote_start;

	/* this is dirty: please read comments in sctk_ptl_pin_region() */
	copy                   = sctk_malloc(sizeof(sctk_ptl_local_data_t));
	*copy                  = *local_key->pin.ptl.md_data;
	copy->msg              = msg;
	msg->tail.ptl.user_ptr = NULL; /* NULL in RDMA ctx means: 'no extra allocated data' */
	msg->tail.ptl.copy     = 0;


	sctk_ptl_emit_get(
		local_key->pin.ptl.md_data, /* the base MD */
		size,                       /* request size */
		remote,                     /* target Process */
		rdma_pte,                   /* Portals entry */
		remote_key->pin.ptl.match,  /* match_bits */
		local_off, remote_off,      /* offsets */
		copy
	);
}

/**
 * Creat a new pinned segment, reachable for RDMA operations.
 *
 * NOTES:
 *  Commonly, one request creates one MD to be processed. In case of RDMA, we use
 *  a shared MD to handle multiple requests (Get,Put,CAS...). The point is that when polling
 *  events, we expect the MD to handle the associated request (=user_ptr). In case of
 *  RDMA this is not possible, because of concurrency (THREAD_MULTIPLE).
 *
 *  As a work-around for now, each request is attached to a copied 'user_ptr' pointing to the same
 *  MD. This means that, when polling RDMA msg, sctk_ptl_md_release should not be invoked to free the msg
 *  (like standard msg). This will be done when unlinking the pinned segment.
 *
 *  The code in sctk_ptl_toolkit.c (sctk_ptl_mds_poll) takes care of it by just complete_and_free the msg.
 *
 * \param[in] rail the Portals rail
 * \param[out] list the RDMA-specific ctx to fill
 * \param[in] addr the start address
 * \param[in] size the region length
 */
void sctk_ptl_pin_region( struct sctk_rail_info_s * rail, struct sctk_rail_pin_ctx_list * list, void * addr, size_t size )
{
	sctk_ptl_rail_info_t* srail    = &rail->network.ptl;
	sctk_ptl_local_data_t *md_request, *me_request;
	int md_flags, me_flags;
	sctk_ptl_matchbits_t match, ign;
	sctk_ptl_pte_t *pte;
	sctk_ptl_id_t remote;

	if(rdma_pte==NULL)
		rdma_pte = mpc_common_hashtable_get(&srail->pt_table, SCTK_PTL_PTE_RDMA);

	md_request = me_request = NULL;
	match      = ign        = SCTK_PTL_MATCH_INIT;
	md_flags   = me_flags   = 0;
	pte        = NULL;
	remote     = SCTK_PTL_ANY_PROCESS;

	/* Configure the MD */
	md_flags   = SCTK_PTL_MD_PUT_FLAGS | SCTK_PTL_MD_GET_FLAGS;
	md_request = sctk_ptl_md_create(srail, addr, size, md_flags);
	md_request->type = SCTK_PTL_TYPE_RDMA;
#ifdef MPC_LOWCOMM_PROTOCOL
        lcr_ptl_md_register(srail, md_request);
#else
	sctk_ptl_md_register(srail, md_request);
#endif

	/* configure the ME */
	me_flags       = SCTK_PTL_ME_PUT_FLAGS | SCTK_PTL_ME_GET_FLAGS;
	match.data.tag = OPA_fetch_and_incr_int(&rail->network.ptl.rdma_cpt);
	ign.data.tag   = SCTK_PTL_MATCH_TAG;
	ign.data.rank  = SCTK_PTL_IGN_RANK;
	ign.data.uid   = SCTK_PTL_IGN_UID;
	pte            = rdma_pte;
	remote         = SCTK_PTL_ANY_PROCESS;
	me_request     = sctk_ptl_me_create(addr, size, remote, match, ign, me_flags);
	me_request->type = SCTK_PTL_TYPE_RDMA;
	sctk_ptl_me_register(srail, me_request, pte);

	/* Register infos in the RDMA ctx.
	 * Note that this ctx will be sent to the remote
	 */
	list->rail_id         = rail->rail_number;
	list->pin.ptl.me_data = me_request;
	list->pin.ptl.md_data = md_request;
	list->pin.ptl.origin  = srail->id;
	list->pin.ptl.start   = addr;
	list->pin.ptl.match   = match;

	mpc_common_debug_info("REGISTER RDMA %p->%p (match=%s) '%llu', origin=%llu, pte idx=%d, size=%llu", addr, addr + size,  __sctk_ptl_match_str(sctk_malloc(32), 32, match.raw), (unsigned long long int)addr, list->pin.ptl.origin, rdma_pte->idx, size);
}

/**
 * Free (from Portals point of view) the pinned segment.
 *
 * Note that all requests must have been completed before calling this function.
 * If not, some hardware implementation can hang.
 *
 * \param[in] rail the Portals rail
 * \param[in] list the ctx filled when segment was pinned
 */
void sctk_ptl_unpin_region( struct sctk_rail_info_s * rail, struct sctk_rail_pin_ctx_list * list )
{
	UNUSED(rail);
	assert(list);
	sctk_ptl_md_release(list->pin.ptl.md_data);
	sctk_ptl_me_release(list->pin.ptl.me_data);
	mpc_common_debug_info("RELEASE RDMA %p->%p %s", list->pin.ptl.me_data->slot.me.start, list->pin.ptl.me_data->slot.me.start + list->pin.ptl.me_data->slot.me.length, __sctk_ptl_match_str(sctk_malloc(32), 32, list->pin.ptl.me_data->slot.me.match_bits));
}

void lcr_ptl_handle_rdma_ev_me(sctk_rail_info_t *rail, sctk_ptl_event_t *ev)
{
        sctk_ptl_local_data_t *user_ptr = ev->user_ptr;
        lcr_completion_t *comp           = user_ptr->msg;

	UNUSED(rail);
        switch(ev->type)
        {
        case PTL_EVENT_PUT:                   /* a Put() reached the local process */
        case PTL_EVENT_GET:                   /* a Get() reached the local process */
                if (comp != NULL) {
                        comp->sent = ev->mlength;
                        comp->comp_cb(comp);
                }
                break;

        case PTL_EVENT_ATOMIC:                /* an Atomic() reached the local process */
        case PTL_EVENT_FETCH_ATOMIC:          /* a FetchAtomic() reached the local process */
                break;

        case PTL_EVENT_PUT_OVERFLOW:          /* a previous received PUT matched a just appended ME */
        case PTL_EVENT_GET_OVERFLOW:          /* a previous received GET matched a just appended ME */
        case PTL_EVENT_FETCH_ATOMIC_OVERFLOW: /* a previously received FETCH-ATOMIC matched a just appended one */
        case PTL_EVENT_ATOMIC_OVERFLOW:       /* a previously received ATOMIC matched a just appended one */
        case PTL_EVENT_PT_DISABLED:           /* ERROR: The local PTE is disabled (FLOW_CTRL) */
        case PTL_EVENT_SEARCH:                /* a PtlMESearch completed */
                /* probably nothing to do here */
        case PTL_EVENT_LINK:                  /* MISC: A new ME has been linked, (maybe not useful) */
        case PTL_EVENT_AUTO_UNLINK:           /* an USE_ONCE ME has been automatically unlinked */
        case PTL_EVENT_AUTO_FREE:             /* an USE_ONCE ME can be now reused */
                not_reachable();              /* have been disabled */
                break;
        default:
                mpc_common_debug_fatal("Portals ME event not recognized: %d", ev->type);
                break;
        }

}

/**
 * Notification that a remote RDMA request reached the local process.
 * By definition of RDMA, there are nothing to do from this side.
 *
 * \param[in] rail the Portals rail
 * \param[in] ev the generated event
 */
void sctk_ptl_rdma_event_me(sctk_rail_info_t* rail, sctk_ptl_event_t ev)
{
	UNUSED(rail);
	switch(ev.type)
	{
		case PTL_EVENT_PUT:                   /* a Put() reached the local process */
		case PTL_EVENT_GET:                   /* a Get() reached the local process */
		case PTL_EVENT_ATOMIC:                /* an Atomic() reached the local process */
		case PTL_EVENT_FETCH_ATOMIC:          /* a FetchAtomic() reached the local process */
			break;

		case PTL_EVENT_PUT_OVERFLOW:          /* a previous received PUT matched a just appended ME */
		case PTL_EVENT_GET_OVERFLOW:          /* a previous received GET matched a just appended ME */
		case PTL_EVENT_FETCH_ATOMIC_OVERFLOW: /* a previously received FETCH-ATOMIC matched a just appended one */
		case PTL_EVENT_ATOMIC_OVERFLOW:       /* a previously received ATOMIC matched a just appended one */
		case PTL_EVENT_PT_DISABLED:           /* ERROR: The local PTE is disabled (FLOW_CTRL) */
		case PTL_EVENT_SEARCH:                /* a PtlMESearch completed */
			                              /* probably nothing to do here */
		case PTL_EVENT_LINK:                  /* MISC: A new ME has been linked, (maybe not useful) */
		case PTL_EVENT_AUTO_UNLINK:           /* an USE_ONCE ME has been automatically unlinked */
		case PTL_EVENT_AUTO_FREE:             /* an USE_ONCE ME can be now reused */
			not_reachable();              /* have been disabled */
			break;
		default:
			mpc_common_debug_fatal("Portals ME event not recognized: %d", ev.type);
			break;
	}

}

void lcr_ptl_handle_rdma_ev_md(sctk_rail_info_t *rail, sctk_ptl_event_t *ev)
{
	UNUSED(rail);
        sctk_ptl_local_data_t *user_ptr = ev->user_ptr;
        lcr_completion_t *comp           = user_ptr->msg;

        switch (ev->type) {
        case PTL_EVENT_SEND:   /* not supported */
                mpc_common_debug_fatal("Unrecognized MD event: %d", ev->type);
                break;
        case PTL_EVENT_REPLY:  /* read  */
        case PTL_EVENT_ACK:    /* write */
                comp->sent = ev->mlength;
                comp->comp_cb(comp);
                break;
        default:
                mpc_common_debug_fatal("Unrecognized MD event: %d", ev->type);
                break;
        }
}

/**
 * Notification that a locally-emited request reached the remote process.
 *
 * \param[in] rail the Portals rail
 * \param[in] ev the generated event.
 */
void sctk_ptl_rdma_event_md(sctk_rail_info_t* rail, sctk_ptl_event_t ev)
{
	sctk_ptl_local_data_t* ptr = (sctk_ptl_local_data_t*)ev.user_ptr;
	mpc_lowcomm_ptp_message_t* msg = (mpc_lowcomm_ptp_message_t*)ptr->msg;
	sctk_ptl_local_data_t* atomic_ptr = (sctk_ptl_local_data_t*)msg->tail.ptl.user_ptr;

	UNUSED(rail);
	switch(ev.type)
	{
		case PTL_EVENT_ACK:    /* write  || CAS || FETCH_AND_OP */
			if(atomic_ptr != NULL)
			{
				sctk_free(ptr); /* just for the temp MD buffer */
				break;
			}
			/* FALLTHROUGH */
			/* else, complete_and_free_message */
		case PTL_EVENT_REPLY:  /* READ || CAS || FETCH_AND_OP */
			mpc_lowcomm_ptp_message_complete_and_free(msg);
			sctk_free(ptr);
			break;
		case PTL_EVENT_SEND:   /* special case, here will fall extra-allocated MD for atomic ops */
			assert(atomic_ptr != NULL);
			sctk_ptl_md_release(atomic_ptr);
			break;
		default:
			mpc_common_debug_fatal("Unrecognized MD event: %d", ev.type);
			break;
	}
}
