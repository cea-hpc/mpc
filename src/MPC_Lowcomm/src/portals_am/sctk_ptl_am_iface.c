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

#ifdef MPC_USE_PORTALS

#include <limits.h>
#include <mpc_lowcomm.h>
#include "mpc_common_debug.h"
#include "sctk_alloc.h"
#include "mpc_common_helper.h"
#include "sctk_ptl_am_iface.h"
#include "sctk_ptl_am_types.h"
#include "arpc_weak.h"

#include "mpc_common_asm.h"
#include "mpc_launch_pmi.h"
#include "mpc_common_rank.h"
#include "mpc_lowcomm_arpc.h"

/** threshold for large ARPC request */
static size_t sctk_ptl_am_req_max_small_size = 0;
/** threshold for large ARPC response */
static size_t sctk_ptl_am_rep_max_small_size = 0;
/** load threhold to put blocs to handle next requests */
static size_t sctk_ptl_am_req_trsh_new = 0;

/**
 * Will print the Portals structure.
 * \param[in] srail the Portals rail to dump
 */
void sctk_ptl_am_print_structure( sctk_ptl_am_rail_info_t *srail )
{
	__UNUSED__ sctk_ptl_limits_t l = srail->max_limits;
	mpc_common_debug(
		"\n======== PORTALS AM STRUCTURE ========\n"
		"\n"
		" PORTALS ENTRIES       : \n"
		"  - PTE flags          : 0x%x\n"
		"  - Comm. nb entries   : %d\n"
		"  - each EQ size       : %d\n"
		"\n"
		" ME management         : \n"
		"  - ME PUT flags       : 0x%x\n"
		"  - ME GET flags       : 0x%x\n"
		"\n"
		" MD management         : \n"
		"  - shared EQ size     : %d\n"
		"  - MD PUT flags       : 0x%x\n"
		"  - MD GET flags       : 0x%x\n"
		"\n"
		" PORTALS LIMITS        : \n"
		"  - MAX PT entries     : %d\n"
		"  - MAX unexpected     : %d\n"
		"  - MAX MDs            : %d\n"
		"  - MAX MEs            : %d\n"
		"  - MAX CTs            : %d\n"
		"  - MAX EQs            : %d\n"
		"  - MAX IOVECs         : %d\n"
		"  - MAX MEs/PTE        : %d\n"
		"  - MAX TriggeredOps   : %d\n"
		"\n===================================",
		SCTK_PTL_AM_PTE_FLAGS,
		srail->nb_entries,
		SCTK_PTL_AM_EQ_PTE_SIZE,

		SCTK_PTL_AM_ME_PUT_FLAGS,
		SCTK_PTL_AM_ME_GET_FLAGS,

		SCTK_PTL_AM_EQ_MDS_SIZE,
		SCTK_PTL_AM_MD_PUT_FLAGS,
		SCTK_PTL_AM_MD_GET_FLAGS,

		l.max_pt_index,
		l.max_unexpected_headers,
		l.max_mds,
		l.max_entries,
		l.max_cts,
		l.max_eqs,
		l.max_iovecs,
		l.max_list_size,
		l.max_triggered_ops );
}

/**
 * Init max values for driver config limits.
 * \param[in] l the limits to set
 */
static inline void __sctk_ptl_set_limits( sctk_ptl_limits_t *l )
{
	*l = ( ptl_ni_limits_t ){
		.max_entries = INT_MAX,				   /* Max number of entries allocated at any time */
		.max_unexpected_headers = INT_MAX,	 /* max number of headers at any time */
		.max_mds = INT_MAX,					   /* Max number of MD allocated at any time */
		.max_cts = INT_MAX,					   /* Max number of CT allocated at any time */
		.max_eqs = INT_MAX,					   /* Max number of EQ allocated at any time */
		.max_pt_index = INT_MAX,			   /* Max PT index */
		.max_iovecs = INT_MAX,				   /* max number of iovecs for a single MD */
		.max_list_size = INT_MAX,			   /* Max number of entrie for one PT entry */
		.max_triggered_ops = INT_MAX,		   /* Max number of triggered ops */
		.max_msg_size = PTL_SIZE_MAX,		   /* max message's size */
		.max_atomic_size = PTL_SIZE_MAX,	   /* max size writable atomically */
		.max_fetch_atomic_size = PTL_SIZE_MAX, /* Max size for a fetch op */
		.max_waw_ordered_size = PTL_SIZE_MAX,  /* Max length for ordering-guarantee */
		.max_war_ordered_size = PTL_SIZE_MAX,  /* max length for ordering guarantee */
		.max_volatile_size = PTL_SIZE_MAX,	 /* Max length for MD w/ VOLATILE flag */
		.features = 0						   /* could be a combination or TARGET_BIND_INACCESSIBLE | TOTAL_DATA_ORDERING | COHERENT_ATOMIC */
	};
}

/**
 * Create the link between our program and the real driver.
 * \return the Portals rail object
 */
sctk_ptl_am_rail_info_t sctk_ptl_am_hardware_init()
{
	sctk_ptl_am_rail_info_t res;
	sctk_ptl_limits_t l;

	/* init the driver */
	sctk_ptl_chk( PtlInit() );

	/* set max values */
	__sctk_ptl_set_limits( &l );

	/* init one physical interface */
	sctk_ptl_chk( PtlNIInit(
		PTL_IFACE_DEFAULT,				   /* Type of interface */
		PTL_NI_MATCHING | PTL_NI_PHYSICAL, /* physical NI, w/ matching enabled */
		PTL_PID_ANY,					   /* Let Portals decide process's PID */
		&l,								   /* desired Portals boundaries */
		&res.max_limits,				   /* effective Portals limits */
		&res.iface						   /* THE handler */
		) );

	/* retrieve the process identifier */
	sctk_ptl_chk( PtlGetPhysId(
		res.iface, /* The NI handler */
		&res.id	/* the structure to store it */
		) );

	/* Set the max request size to not be considered as large.
	 * If the request will fill more than 40% of the remote memory, a dedicated ME will be created
	 */
	sctk_ptl_am_req_max_small_size = (int) ( SCTK_PTL_AM_CHUNK_SZ * 0.40 );
	/* Set the max response size to not be considered as large.
	 * By default, it represents one cell. It can be less than this value
	 * but never greater.
	 */
	sctk_ptl_am_rep_max_small_size = SCTK_PTL_AM_REP_CELL_SZ;

	/*  By default, when the targeted ME is filled up to 50 %, a new slot is appended */
	sctk_ptl_am_req_trsh_new = (int) ( SCTK_PTL_AM_CHUNK_SZ / 2 );

	/* Some checks:
	   1. A cell cannot be larger than a chunk
	   2. A chunk should be a multiple of the number of cells, to manage reponse slots properly.
	   These values could be provided by the MPC configuration, if desired
	*/
	assert( SCTK_PTL_AM_CHUNK_SZ > SCTK_PTL_AM_REP_CELL_SZ );
	if ( SCTK_PTL_AM_CHUNK_SZ % SCTK_PTL_AM_REP_CELL_SZ != 0 )
		mpc_common_debug_fatal( "Please make sure the max chunk size is a multiple of the number of cells !" );

	return res;
}

/**
 * Shut down the link between our program and the driver.
 * \param[in] srail the Portals rail, created by sctk_ptl_hardware_init
 */
void sctk_ptl_am_hardware_fini( sctk_ptl_am_rail_info_t *srail )
{
	/* tear down the interface */
	sctk_ptl_chk( PtlNIFini( srail->iface ) );

	/* tear down the driver */
	PtlFini();
}

/**
 * @brief Create a list of chunks to append to the given PT entry.
 *
 * Chunk type will be given by the me_type parameter. This function can be used to append ME for both
 * REQUEST of RESPONSE slots (but not large).
 *
 * @param srail the AM Portals rail
 * @param srv_idx The index to fill ME with
 * @param me_type types of slots to create (REQ/REP)
 * @param nb_elts number of elements to append
 * @param me_flags flags to set for each appended MEs
 */
static void sctk_ptl_am_pte_populate( sctk_ptl_am_rail_info_t *srail, int srv_idx, int me_type, size_t nb_elts, int me_flags )
{
	size_t j = 0;
	sctk_ptl_am_pte_t *pte = srail->pte_table[srv_idx];

	assert( srail );
	assert( srv_idx >= 0 && nb_elts >= 1 );
	assert( pte );
	assert( me_type == SCTK_PTL_AM_REQ_TYPE || me_type == SCTK_PTL_AM_REP_TYPE );

	sctk_ptl_am_matchbits_t match = ( sctk_ptl_am_matchbits_t ){
		.data.srvcode = srv_idx,			  /* the SRV code */
		.data.rpcode = 0,					  /* for ME created here, the RPC is not useful (will be ignored) */
		.data.tag = 0,						  /* the tag (useless for REQ, mandatory for REP) */
		.data.inc_data = 0,					  /* Don't care for these blocs */
		.data.is_req = me_type,				  /* Type of chunk: REQ of REP ? */
		.data.is_large = SCTK_PTL_AM_OP_SMALL /* All these blocs are considered to receive small messages */
	};

	sctk_ptl_am_matchbits_t ign = ( me_type == SCTK_PTL_AM_REQ_TYPE ) ? SCTK_PTL_IGN_FOR_REQ : SCTK_PTL_IGN_FOR_REP;

	sctk_ptl_am_chunk_t *last = NULL, *first = NULL, *prev = NULL;

	/* create a linked list of nb_elts chunks */
	for ( j = 0; j < nb_elts; ++j )
	{
		prev = last;
		sctk_ptl_am_local_data_t *block;
		last = (sctk_ptl_am_chunk_t *) sctk_calloc( 1, sizeof( sctk_ptl_am_chunk_t ) );

		OPA_store_int( &last->refcnt, 0 );
		OPA_store_int( &last->noff, 0 );
		OPA_store_int( &last->up, 1 );
		last->tag = 0;

		/* in case of response, the tag will have to match */
		if ( me_type == SCTK_PTL_AM_REP_TYPE )
		{
			match.data.tag = OPA_fetch_and_incr_int( &pte->next_tag );
			last->tag = match.data.tag;
		}

		/* create the ME */
		block = sctk_ptl_am_me_create(
			last->buf,			  /* the buffer, sized to SCTK_PTL_AM_CHUNK_SZ */
			SCTK_PTL_AM_CHUNK_SZ, /* size of buf */
			SCTK_PTL_ANY_PROCESS, /* process allowed to trigger this bloc : ALL */
			match,				  /* the match_bits for this chunk */
			ign,				  /* the ign_bits for this chunk */
			me_flags );			  /* ME flags (REQ should have the extra PTL_MANAGE_LOCAL flag) */

		/* register the ME into the table */
		sctk_ptl_am_me_register( srail, block, pte );

		/* linked list registration */
		block->block = last;
		last->uptr = block;
		last->prev = prev;
		last->next = NULL;

		if ( !first )
			first = last;
		if ( prev )
			prev->next = last;

		mpc_common_nodebug( "POPULATE %d %s %p - %p (%llu)", last->tag, ( ( me_type == SCTK_PTL_AM_REQ_TYPE ) ? "REQ" : "REP" ), last->buf, last->buf + SCTK_PTL_AM_CHUNK_SZ, SCTK_PTL_AM_CHUNK_SZ );
	}

	/* store the list  */
	if ( first )
	{
		if ( me_type == SCTK_PTL_AM_REQ_TYPE )
		{
			if(pte->req_tail)
			{
				pte->req_tail->next = first;
				first->prev = pte->req_tail;
			}
			pte->req_tail = last;

			if ( !pte->req_head )
				pte->req_head = first;
		}
		else
		{
			if(pte->rep_tail)
			{
				pte->rep_tail->next = first;
				first->prev = pte->rep_tail;
			}

			pte->rep_tail = last;

			if ( !pte->rep_head )
				pte->rep_head = first;
		}
	}
}

/**
 * Map the Portals structure in user-space to be ready for communication.
 *
 * After this functions, Portals entries should be ready to use. This function
 * is different from hardware init beceause it is possible to have multiple
 * software implementation relying on the same NI (why not?).
 * \param[in] srail the Portals rail
 * \param[in] dims PT dimensions
 */
void sctk_ptl_am_software_init( sctk_ptl_am_rail_info_t *srail, int nb_srv )
{
	ssize_t i = 0;
	srail->nb_entries = 0;

	/* create PT table & EQ table (one for each PTE) */
	srail->pte_table = (sctk_ptl_am_pte_t **) sctk_calloc( nb_srv, sizeof( sctk_ptl_am_pte_t * ) );
	srail->meqs_table = (sctk_ptl_eq_t *) sctk_calloc( nb_srv, sizeof( sctk_ptl_eq_t ) );

	/* Now initialize each PTE */
	for ( i = 0; i < nb_srv; ++i )
	{
		sctk_ptl_am_pte_create( srail, i );
	}

	/* create the global EQ for the global MD */
	sctk_ptl_chk( PtlEQAlloc(
		srail->iface,			 /* The NI handler */
		SCTK_PTL_AM_EQ_MDS_SIZE, /* number of slots available for events */
		&srail->mds_eq			 /* out: the EQ handler */
		) );

	/* create the MD, mapping the whole address space */
	srail->md_slot.slot.md = ( sctk_ptl_md_t ){
		.start = NULL,					 /* first usable address */
		.length = PTL_SIZE_MAX,			 /* should represent the whole address space */
		.options = SCTK_PTL_AM_MD_FLAGS, /* MD flags */
		.eq_handle = srail->mds_eq,		 /* the EQ inited above */
		.ct_handle = PTL_CT_NONE		 /* no CT event required */
	};

	sctk_ptl_chk( PtlMDBind(
		srail->iface,			   /* the NI handler */
		&srail->md_slot.slot.md,   /* the MD to bind with memory region */
		&srail->md_slot.slot_h.mdh /* out: the MD handler */
		) );

	sctk_ptl_am_print_structure( srail );
}

/**
 * Release Portals structure from our program.
 *
 * After here, no communication should be made on these resources
 * \param[in] srail the Portals rail
 */
void sctk_ptl_am_software_fini( sctk_ptl_am_rail_info_t *srail )
{
	int table_dims = srail->nb_entries;
	assert( table_dims > 0 );
	int i;
	void *base_ptr = NULL;

	/* don't want to hang the NIC */
	return;

	for ( i = 0; i < table_dims; i++ )
	{
		sctk_ptl_am_pte_t *cur = srail->pte_table[i];
		if ( i == 0 )
			base_ptr = cur;

		sctk_ptl_chk( PtlEQFree(
			srail->meqs_table[i] /* the EQ handler */
			) );

		sctk_ptl_chk( PtlPTFree(
			srail->iface, /* the NI handler */
			cur->idx	  /* the PTE to destroy */
			) );
	}

	sctk_ptl_chk( PtlEQFree(
		srail->mds_eq /* the EQ handler */
		) );

	/* write 'NULL' to be sure */
	sctk_free( base_ptr );
	srail->nb_entries = 0;
}

/**
 * Dynamically create a new Portals entry.
 * \param[in] srail the Portals rail
 * \param[out] pte Portals entry pointer, to init
 */
void sctk_ptl_am_pte_create( sctk_ptl_am_rail_info_t *srail, size_t key )
{
	sctk_ptl_am_pte_t *pte = srail->pte_table[key];

	if ( pte )
		return;

	pte = (sctk_ptl_am_pte_t *) sctk_malloc( sizeof( sctk_ptl_am_pte_t ) );
	srail->pte_table[key] = pte;

	/* create the EQ for this PT */
	sctk_ptl_chk( PtlEQAlloc(
		srail->iface,			 /* the NI handler */
		SCTK_PTL_AM_EQ_PTE_SIZE, /* the number of slots in the EQ */
		srail->meqs_table + key  /* the returned handler */
		) );

	/* register the PT */
	sctk_ptl_chk( PtlPTAlloc(
		srail->iface,			/* the NI handler */
		SCTK_PTL_AM_PTE_FLAGS,  /* PT entry specific flags */
		srail->meqs_table[key], /* the EQ for this entry */
		key,					/* the desired index value */
		&pte->idx				/* the effective index value */
		) );

	/* just a warn to detect the day where our ids won't match pt_index anymore.
	 * This saves us from a mapping table.
	 */
	if ( key != pte->idx )
		mpc_common_debug_warning( "A Portals entry does not match the computed index !" );

	mpc_common_spinlock_init(&pte->pte_lock, 0);
	OPA_store_int( &pte->next_tag, 0 );
	pte->req_head = NULL;
	pte->req_tail = NULL;
	pte->rep_head = NULL;
	pte->rep_tail = NULL;

	/* populate each entry with REQ/REP MEs
	 * Note the PTL_ME_MANAGE_LOCAL for REQ ME slots, because RPC reception in buffers is handled
	 * locally by the target process.
	*/
	sctk_ptl_am_pte_populate( srail, key, SCTK_PTL_AM_REQ_TYPE, SCTK_PTL_AM_REQ_NB_DEF, SCTK_PTL_AM_ME_PUT_FLAGS | PTL_ME_MANAGE_LOCAL );
	sctk_ptl_am_pte_populate( srail, key, SCTK_PTL_AM_REP_TYPE, SCTK_PTL_AM_REP_NB_DEF, SCTK_PTL_AM_ME_PUT_FLAGS );

	srail->nb_entries++;
}

/**
 * @brief Emit an RPC request to a remote process described by the sctk_ptl_am_msg_t parameter.
 *
 * This function will block until RPC completion.
 *
 * @param srail the AM portals rail
 * @param srv the SRV code as emitted by the ARPC wrapper
 * @param rpc the RPC code as emitted by the ARPC wrapper
 * @param start_in the start address where the ARPC input argument resides.
 * @param sz_in Size of the argument to send
 * @param start_out Pointer of pointer, to return the address where data are stored
 * @param sz_out pointer to size, to return the effective response size.
 * @param msg An internal msg, to communicate extra info for the current request.
 * @return sctk_ptl_am_msg_t* the real RPC msg associated to the response.
 */
sctk_ptl_am_msg_t *sctk_ptl_am_send_request( sctk_ptl_am_rail_info_t *srail, int srv, int rpc, const void *start_in, size_t sz_in, void **start_out, __UNUSED__ size_t *sz_out, sctk_ptl_am_msg_t *msg )
{
	assert( srail );
	assert( srv >= 0 && rpc >= 0 );
	assert( msg );


	sctk_ptl_am_pte_t *pte = srail->pte_table[srv];
	sctk_ptl_am_msg_t *msg_resp = NULL;
	int tag = -1, dedicated_me = 0;
	sctk_ptl_am_imm_data_t req_imm;
	sctk_ptl_am_chunk_t *rep_me = NULL;
	sctk_ptl_id_t remote = msg->remote;
	sctk_ptl_am_matchbits_t md_match;

	assert( pte );

	/* 1. Look for a place to store the response, if needed */
	if ( start_out )
	{
		int found_space;
		do
		{
			found_space = 1;
			rep_me = pte->rep_head;
			while ( rep_me ) /* iterate of available ME-RESP chunks */
			{
				/* if the current one is currently mapped */
				if ( OPA_load_int( &rep_me->up ) )
				{
					/* Is a cell available ? */
					req_imm.data.offset = OPA_fetch_and_add_int( &rep_me->noff, SCTK_PTL_AM_REP_CELL_SZ );

					/* YES and it is the last one for this chunk */
					if ( req_imm.data.offset == ( SCTK_PTL_AM_CHUNK_SZ - SCTK_PTL_AM_REP_CELL_SZ ) ) /* last cell */
					{
						__UNUSED__ int cas_val = OPA_cas_int( &rep_me->up, 1, 0 );
						assert( cas_val == 1 );
						break;
					}

					/* YES and it remains space in the chunk */
					else if ( req_imm.data.offset < SCTK_PTL_AM_CHUNK_SZ )
					{
						break;
					}

					/* NO */
					OPA_fetch_and_add_int( &rep_me->noff, ( -1 ) * (int) ( SCTK_PTL_AM_REP_CELL_SZ ) );
				}
				/* next chunk */
				rep_me = rep_me->next;
			}

			/* if no valid chunk has been found, append a new one and retry */
			if ( !rep_me )
			{
				sctk_ptl_am_pte_populate( srail, srv, SCTK_PTL_AM_REP_TYPE, 1, SCTK_PTL_AM_ME_PUT_FLAGS );
				found_space = 0;
			}
		} while ( !found_space );

		assert( rep_me );

		assert( req_imm.data.offset <= SCTK_PTL_AM_CHUNK_SZ - SCTK_PTL_AM_REP_CELL_SZ );

		/* save the tag of the found ME-REP */
		tag = rep_me->tag;
		OPA_incr_int( &rep_me->refcnt );

		/* as each response cell contains an header, save the header addr and
		 * the space to put back the response. The req_imm.data.offset is the 64-bit extra info
		 * injected into the Portals PUT() request. It should be cell_offset + header_sz.
		 */
		msg_resp = (sctk_ptl_am_msg_t *) ( rep_me->buf + req_imm.data.offset );
		msg_resp->completed = 0;
		msg_resp->remote = msg->remote;
		msg_resp->size = 0;
		msg_resp->chunk_addr = rep_me;
		req_imm.data.offset += SCTK_PTL_AM_REP_HDR_SZ;
		msg_resp->msg_type.rep.addr = msg_resp->data;
	}
	else /* no response expected */
	{
		msg_resp = NULL;
	}

	/* 2. If the data to send are too large --> dedicate a GET ME */
	if ( sz_in >= sctk_ptl_am_req_max_small_size )
	{
		/* Inform the remote process of the request size.
	 	* This is only used when message is large, to know the malloc size to prepare
	 	*/
		req_imm.data.size = sz_in;

		sctk_ptl_am_local_data_t *user_ptr = NULL;
		sctk_ptl_am_matchbits_t match = SCTK_PTL_MATCH_BUILD( srv, rpc, tag, 1, SCTK_PTL_AM_REQ_TYPE, SCTK_PTL_AM_OP_LARGE );
		sctk_ptl_am_matchbits_t ign = SCTK_PTL_IGN_FOR_LARGE;

		/* register the ME */
		user_ptr = sctk_ptl_am_me_create( (void *) start_in, sz_in, remote, match, ign, SCTK_PTL_AM_ME_GET_FLAGS );
		sctk_ptl_am_me_register( srail, user_ptr, pte );

		/* the number of byte to send with the following PUT() is zero */
		sz_in = 0;
		dedicated_me = 1;
	}

	/* Set flags to match a generic request slot */
	md_match = SCTK_PTL_MATCH_BUILD( srv, rpc, tag, !dedicated_me, SCTK_PTL_AM_REQ_TYPE, SCTK_PTL_AM_OP_SMALL );


	sctk_ptl_am_emit_put(
		&srail->md_slot,		  /* the MD struct */
		sz_in,					  /* size of the request to send */
		remote,					  /* the target */
		pte,					  /* the targeted PT entry */
		md_match,				  /* the match bits */
		(size_t) start_in,		  /* the offset into the current MD where data is located = absolute address */
		SCTK_PTL_AM_ME_NO_OFFSET, /* no remote offset (because handled with MANAGE_LOCAL) */
		req_imm,				  /* imm_data to request: the offset for the response and the size of the request (32-bit) */
		NULL					  /* user_ptr is not relevant here */
	);
	return msg_resp;
}

/**
 * @brief Emit the response after executing the RPC.
 *
 * This function will not block until completion (once the response is emited, the function returns).
 *
 * @param srail the AM portals rail
 * @param srv the SRV code as emitted by the ARPC wrapper
 * @param rpc the RPC code as emitted by the ARPC wrapper
 * @param start the start address where the data to send are located
 * @param sz the data size
 * @param remote_id the remote process who originated the RPC
 * @param msg extra info, specific to the response
 */
void sctk_ptl_am_send_response( sctk_ptl_am_rail_info_t *srail, int srv, int rpc, void *start, size_t sz, __UNUSED__ int remote_id, sctk_ptl_am_msg_t *msg )
{
	assert( srail );
	assert( srv >= 0 && rpc >= 0 );
	assert( msg );

	sctk_ptl_am_pte_t *pte = srail->pte_table[srv];
	sctk_ptl_id_t remote = msg->remote;
	sctk_ptl_am_imm_data_t rep_imm;
	sctk_ptl_am_matchbits_t md_match;
	int dedicated_me = 0;

	rep_imm.data.offset = -1; /* not used for response */
	rep_imm.data.size = sz;

	/* 1. If the data to send are too large --> dedicate a GET ME */
	if ( sz >= sctk_ptl_am_rep_max_small_size )
	{
		sctk_ptl_am_local_data_t *user_ptr = NULL;
		sctk_ptl_am_matchbits_t match = SCTK_PTL_MATCH_BUILD(
			srv,				   /* the SRV code */
			rpc,				   /* the RPC code */
			msg->msg_type.req.tag, /* the tag provided when receiving the request, to set the response */
			1,					   /* is the message contain direct data ? YES (because LARGE) */
			SCTK_PTL_AM_REP_TYPE,  /* type RESPONSE */
			SCTK_PTL_AM_OP_LARGE   /* is a LARGE msg ? YES */
		);
		sctk_ptl_am_matchbits_t ign = SCTK_PTL_IGN_FOR_LARGE;

		/* register the ME */
		user_ptr = sctk_ptl_am_me_create( (void *) start, sz, remote, match, ign, SCTK_PTL_AM_ME_GET_FLAGS );
		sctk_ptl_am_me_register( srail, user_ptr, pte );

		/* as large, no data are required to be sent in the following PUT() */
		sz = 0;
		dedicated_me = 1;
	}

	/* Set flags to match a pre-reserved response slot */
	md_match = SCTK_PTL_MATCH_BUILD(
		srv,				   /* the SRV code */
		rpc,				   /* the RPC code */
		msg->msg_type.req.tag, /* the tag provided when receiving the request */
		!dedicated_me,		   /* are data included in the PUT response ? */
		SCTK_PTL_AM_REP_TYPE,  /* it is a RESPONSE */
		SCTK_PTL_AM_OP_SMALL   /* This is a small msg */
	);


	sctk_ptl_am_emit_put(
		&srail->md_slot,		  /* the MD */
		sz,						  /* the data size to send */
		remote,					  /* the initiator of the RPC request */
		pte,					  /* the PT entry to target (same as for the request ) */
		md_match,				  /* the match_bits */
		(size_t) start,			  /* the offset in the MD where data is = absolute address */
		msg->msg_type.req.offset, /* the offset in the remote ME : provided in the incoming request */
		rep_imm,				  /* imm_data = size of the response, useful to large reponse */
		NULL					  /* user_ptr not relevant here */
	);
}

/**
 * @brief Wait for the response to arrive.
 *
 * This is relevant to use on the am_msg_t returned by send_request call.
 *
 * @param srail the AM portals rail
 * @param msg the response msg.
 */
void sctk_ptl_am_wait_response( sctk_ptl_am_rail_info_t *srail, sctk_ptl_am_msg_t *msg )
{
	while ( !msg->completed )
	{
		sctk_ptl_am_incoming_lookup( srail );
		sctk_ptl_am_outgoing_lookup( srail );
	}
}

/**
 * @brief Free a chunk (REP/REQ, large or not) after being consumed.
 *
 * @param c the local_data_t attached to the ME.
 */
static inline void __sctk_ptl_am_free_chunk( sctk_ptl_am_local_data_t *c )
{
	assert( c );

	/* if a bloc is associated, it is a 'small' chunk (REQ/REP, but not large) */
	if ( c->block )
	{
		/* in that case, check that all cells have been released by their owners */
		assert( OPA_load_int( &c->block->refcnt ) <= 0 );
		/* check also that the chunk has been detached from the Portals device */
		assert( OPA_load_int( &c->block->up ) == 0 );
		sctk_free( (void *) c->block );
	}
	sctk_free( (void *) c );
}

/**
 * @brief Free the response resources.
 *
 * Should be called after each RPC call from upper layer to notify Portals that the cell
 * used to return data has be conusmed and can be recycled. With the protobuf interface,
 * this is done just before de-serializing data info the C++ object.
 *
 * We know that the cell is built with a header, directly preceding the actual payload.
 * Thus, from the data pointer, we can retrieve the cell header (through container_of).
 *
 * @param addr the data address
 * @return 0 if this call to free() really freed its attached chunk, 1 otherwise.
 */
int sctk_ptl_am_free_response( void *addr )
{
	if ( !addr ) /* not response was expected */
		return 1;

	sctk_ptl_am_msg_t *msg = container_of( addr, sctk_ptl_am_msg_t, data );
	int last_val = OPA_fetch_and_decr_int( &msg->chunk_addr->refcnt );
	int cas_val = OPA_load_int( &msg->chunk_addr->up );

	/* if this is the last free from the chunk */
	if ( last_val - 1 == 0 && cas_val == 0 )
	{
		sctk_ptl_am_me_release( msg->chunk_addr->uptr );

		/* TODO: Reuse buffer */
		/*__sctk_ptl_am_free_chunk( msg->chunk_addr->uptr );*/
		return 0;
	}
	return 1;
}

/**
 * @brief Static inlined function, to deal with incoming put event.
 *
 * @param srail the AM portals rail
 * @param ev the polled put event
 * @param pte the PT where this event has been found.
 */
static inline void __sctk_ptl_am_event_handle_put( sctk_ptl_am_rail_info_t *srail, sctk_ptl_event_t *ev, sctk_ptl_am_pte_t *pte )
{
	sctk_ptl_am_matchbits_t m;
	sctk_ptl_am_msg_t msg;
	sctk_arpc_context_t ctx;
	sctk_ptl_am_imm_data_t rep_imm;
	void *req_buf = NULL, *resp_buf = NULL;
	size_t req_sz = 0, resp_sz = 0;

	/* get the match_bits for the received msg */
	m.raw = ev->match_bits;

	/* build the ctx forwarded to upper layer */
	ctx = ( sctk_arpc_context_t ){
		.dest = -1,
		.srvcode = ev->pt_index,
		.rpcode = m.data.rpcode
	};

	rep_imm.raw = ev->hdr_data;

	msg = ( sctk_ptl_am_msg_t ){
		.remote = ev->initiator,
		.msg_type.req.offset = rep_imm.data.offset,
		.msg_type.req.tag = m.data.tag,
		.size = rep_imm.data.size,
		.completed = 0
	};

	/******** IF REQUEST *********/
	if ( ( m.data.is_req & 0x1 ) == SCTK_PTL_AM_REQ_TYPE )
	{
		/* repopulate a new slot if needed */
		sctk_ptl_am_local_data_t *chunk = (sctk_ptl_am_local_data_t *) ev->user_ptr;
		unsigned long long start_offset = (unsigned long long) ev->start - (unsigned long long) chunk->slot.me.start;
		if ( start_offset <= sctk_ptl_am_req_trsh_new && start_offset + ev->mlength > sctk_ptl_am_req_trsh_new )
		{
			sctk_ptl_am_pte_populate( srail, ev->pt_index, SCTK_PTL_AM_REQ_TYPE, 1, SCTK_PTL_AM_ME_PUT_FLAGS | PTL_ME_MANAGE_LOCAL );
		}
		req_buf = ev->start;
		req_sz = ev->mlength;
		OPA_incr_int( &chunk->block->refcnt );

		if ( !m.data.inc_data )
		{
			assert( req_sz == 0 );
			req_sz = rep_imm.data.size;
			/* data to GET() are large and embedded in the message */
			m.data.is_large = 1;
			m.data.inc_data = 1;
			assert( ( m.data.is_req & 0x1 ) == SCTK_PTL_AM_REQ_TYPE );
			req_buf = sctk_malloc( req_sz );
			sctk_ptl_am_emit_get( &srail->md_slot, req_sz, ev->initiator, pte, m, (size_t) req_buf, 0, &msg.completed );

			/* Wait upon completion (ct_event ?) */
			while ( !msg.completed )
			{
				sctk_ptl_am_outgoing_lookup( srail );
			}
		}
		/* now process the RPC call */
		mpc_arpc_recv_call_ptl( &ctx, req_buf, req_sz, &resp_buf, &resp_sz, &msg );

		/* flag the memory in the ME as freeable */
		int last_value = OPA_fetch_and_decr_int( &chunk->block->refcnt );
		int cas_value = OPA_load_int( &chunk->block->up );

		/* if this slot is the last one used in the memory block AND the ME has been unlinked -> free everything */
		if ( last_value - 1 == 0 && cas_value == 0 )
		{
			__sctk_ptl_am_free_chunk( chunk );
		}

		/* just free the temp buffer used to GET() locally */
		if ( req_buf != ev->start )
		{
			sctk_free( (void *) req_buf );
		}
	}
	else /********** IF RESPONSE ************/
	{
		assert( ( m.data.is_req & 0x1 ) == SCTK_PTL_AM_REP_TYPE );
		sctk_ptl_am_msg_t *init_msg = container_of( ev->start, sctk_ptl_am_msg_t, data );

		init_msg->msg_type.rep.addr = ev->start;
		init_msg->size = ev->mlength;

		if ( !m.data.inc_data )
		{
			assert( req_sz == 0 );
			resp_sz = rep_imm.data.size;
			m.data.is_large = 1;
			m.data.inc_data = 1;
			resp_buf = sctk_malloc( req_sz );

			sctk_ptl_am_emit_get( &srail->md_slot, resp_sz, ev->initiator, pte, m, (size_t) resp_buf, 0, &msg.completed );

			/* Wait upon completion (ct_event ?) */
			while ( !msg.completed )
			{
				sctk_ptl_am_outgoing_lookup( srail );
			}
			init_msg->msg_type.rep.addr = resp_buf;
			init_msg->size = resp_sz;
		}

		init_msg->completed = 1;
	}
}

/**
 * @brief Static inlined function to deal with UNLINK event.
 *
 * @param srail the AM portals rail
 * @param ev the polled event
 * @param pte the PTE where the event has been found
 */
static inline void __sctk_ptl_am_event_handle_unlink(__UNUSED__ sctk_ptl_am_rail_info_t *srail,
						     sctk_ptl_event_t *ev,
						     __UNUSED__ sctk_ptl_am_pte_t *pte )
{
	sctk_ptl_am_matchbits_t m;
	m.raw = ev->match_bits;
	sctk_ptl_am_local_data_t *chunk = (sctk_ptl_am_local_data_t *) ev->user_ptr;
	/* special case, for this event, ev.match_bits is not set */
	m.raw = chunk->slot.me.match_bits;
	if ( m.data.is_large )
	{
		/* if this is a large buffer, two cases (event generated because LARGE = USE_ONCE):
					 *  1. type REQUEST = LARGE buffer from sender side -> just free the ptl object (user buffer)
					 *  2. type RESPONSE = LARGE buffer allocated by us -> free it
					 */
		if ( m.data.is_req /* == SCTK_PTL_AM_REP_TYPE */ )
			sctk_free( (void *) chunk->slot.me.start );

		sctk_free( (void *) chunk );
	}
	else
	{
		/* if this is not a large buffer, two cases:
		 *  1. type REQUEST: MANAGE_LOCAL flag trigers this event
		 *  2. type RESPONSE: ME blocks are manually handled and this event should
		 *  	not be triggered (check w/ an assert())
		 */

		assert( m.data.is_req == SCTK_PTL_AM_REQ_TYPE );
		//int cas_res = OPA_cas_int( &chunk->block->up, 1, 0 ); /* currenT ME is unlinked */
		int cnt_res = OPA_load_int( &chunk->block->refcnt );
		/* Are all slots unused at the unlinking time ? */
		if ( cnt_res == 0 )
		{
			/* Free the local_data AND the CHUNK_T object */
			__sctk_ptl_am_free_chunk( chunk );
		}
	}
}

/**
 * @brief Main polling loop, for handling the next incoming event frmo the Portals card.
 *
 * This function will call sctk_ptl_am_eq_poll_me, calling itself PtlEQPoll.
 *
 * @param srail AM Portals rail
 * @return 0 if one event has been polled, 1 otherwise
 */
int sctk_ptl_am_incoming_lookup( sctk_ptl_am_rail_info_t *srail )
{
	sctk_ptl_event_t ev;
	int ret = -1;
	unsigned int id = -1;

	/* Main purpose: Attempt to poll a event among all EQ */
	ret = sctk_ptl_am_eq_poll_me( srail, &ev, &id );

	/* if an event has been polled */
	if ( ret == PTL_OK )
	{
		sctk_ptl_am_pte_t *pte = srail->pte_table[id];
		if ( ev.ni_fail_type != PTL_NI_OK )
		{
			mpc_common_debug_error( "ME: Failed event %s: %d", sctk_ptl_am_event_decode( ev ), ev.ni_fail_type );
			MPC_CRASH();
		}

		switch ( ev.type )
		{
			case PTL_EVENT_PUT:
			{
				__sctk_ptl_am_event_handle_put( srail, &ev, pte );
				break;
			}
			case PTL_EVENT_GET:
			{
				/* nothing to do, The ME is USE_ONCE */
				break;
			}
			case PTL_EVENT_AUTO_UNLINK:
			{
				__sctk_ptl_am_event_handle_unlink( srail, &ev, pte );
				break;
			}
			default:
				mpc_common_debug_error( "Not handled ME event: %s", sctk_ptl_am_event_decode( ev ) );
		}
		return 0;
	}
	else
	{
		if ( ret != PTL_EQ_EMPTY )
			mpc_common_debug_fatal( "NOK ME: %s", sctk_ptl_am_rc_decode( ret ) );
	}

	/* no event found in any valid EQ */
	return 1;
}

/**
 * @brief Main polling loop, handling the next outgoing event (from MD-EQ).
 *
 * @param srail the AM portals rail
 * @return 0 if an event has been polled, 1 otherwise.
 */
int sctk_ptl_am_outgoing_lookup( sctk_ptl_am_rail_info_t *srail )
{
	int ret;
	sctk_ptl_event_t ev;

	ret = sctk_ptl_am_eq_poll_md( srail, &ev );

	if ( ret == PTL_OK )
	{
		assert( ev.ni_fail_type == PTL_NI_OK );
		switch ( ev.type )
		{
			case PTL_EVENT_REPLY: /* When a GET() completes, unlocks the waiting request */
			{
				char *c = (char *) ev.user_ptr;
				assert( c );
				( *c ) = 1;
				break;
			}
			default:
				mpc_common_debug( "Not handled MD event: %s", sctk_ptl_am_event_decode( ev ) );
		}
	}
	else
	{
		if ( ret != PTL_EQ_EMPTY )
			mpc_common_debug_fatal( "NOK MD: %s", sctk_ptl_am_rc_decode( ret ) );
	}

	return ret;
}

/**
 * This function creates a new memory entry.
 *
 * \param[in] start first buffer address
 * \param[in] size number of bytes to include into the ME
 * \param[in] remote the target process id
 * \param[in] match the matching bits for this slot
 * \param[in] ign the ignoring bits from the above parameter
 * \param[in] flags ME-specific flags (PUT,GET,...)
 * \return the allocated request
 */
sctk_ptl_am_local_data_t *sctk_ptl_am_me_create( void *start, size_t size, sctk_ptl_id_t remote, sctk_ptl_am_matchbits_t match, sctk_ptl_am_matchbits_t ign, int flags )
{
	sctk_ptl_am_local_data_t *user_ptr = (sctk_ptl_am_local_data_t *) sctk_malloc( sizeof( sctk_ptl_am_local_data_t ) );

	*user_ptr = ( sctk_ptl_am_local_data_t ){
		.slot.me = ( sctk_ptl_me_t ){
			.start = start,
			.length = (ptl_size_t) size,
			.ct_handle = PTL_CT_NONE,
			.uid = PTL_UID_ANY,
			.match_id = remote,
			.match_bits = match.raw,
			.ignore_bits = ign.raw,
			.options = flags,
			.min_free = SCTK_PTL_AM_REQ_MIN_FREE,
		},
		.slot_h.meh = PTL_INVALID_HANDLE,
		.block = NULL};

	return user_ptr;
}

/**
 * Register a memory entry to the specific PTE.
 *
 * \param[in] slot the already created ME
 * \param[in] pte  the PT index where the ME will be attached to
 * \param[in] list in which list this ME has to be registed ? PRIORITY || OVERFLOW
 */
void sctk_ptl_am_me_register( sctk_ptl_am_rail_info_t *srail, sctk_ptl_am_local_data_t *user_ptr, sctk_ptl_am_pte_t *pte )
{
	assert( user_ptr );
	sctk_ptl_chk( PtlMEAppend(
		srail->iface,			/* the NI handler */
		pte->idx,				/* the targeted PT entry */
		&user_ptr->slot.me,		/* the ME to register in the table */
		SCTK_PTL_PRIORITY_LIST, /* in which list the ME has to be appended */
		user_ptr,				/* usr_ptr: forwarded when polling the event */
		&user_ptr->slot_h.meh   /* out: the ME handler */
		) );
}

/**
 * Remove the ME from the Portals table.
 *
 * The unlink will generate an event that will be in charge of freeing memory.
 * This function should not be used if ME can be set with PTL_ME_USE_ONCE flag.
 * That's why we have \see sctk_ptl_me_free.
 *
 *  \param[in] meh the ME handler.
 */
void sctk_ptl_am_me_release( sctk_ptl_am_local_data_t *request )
{
	/* WARN: This function can return
	 * PTL_IN_USE if the targeted ME is in OVERFLOW_LIST
	 * and at least one unexpected header exists for it
	 */
	sctk_ptl_chk( PtlMEUnlink(
		request->slot_h.meh ) );
}

/**
 * Free the memory associated with a Portals request and free the request depending on parameters.
 * \param[in] request the request to free
 * \param[in] free_buffer is the 'start' buffer to be freed too ?
 */
void sctk_ptl_am_me_free( sctk_ptl_am_local_data_t *request, int free_buffer )
{
	if ( free_buffer )
	{
		sctk_free( request->slot.me.start );
	}
	sctk_free( request );
}

/**
 * Get the current process Portals ID.
 * \param[in] srail the Portals-specific info struct
 * \return the current sctk_ptl_id_t
 */
sctk_ptl_id_t sctk_ptl_am_self( sctk_ptl_am_rail_info_t *srail )
{
	return srail->id;
}

/**
 * Send a PTL Get() request.
 *
 * \param[in] user the preset request, associated to a MD slot
 * \param[in] size the length (in bytes) targeted by the request
 * \param[in] remote the remote ID
 * \param[in] pte the PT entry to target remotely (all processes init the table in the same order)
 * \param[in] match the match_bits
 * \param[in] local_off the offset in the local buffer (MD)
 * \param[in] remote_off the offset in the remote buffer (ME)
 * \param[in] user_ptr the request to attach to the Portals command.
 * \return PTL_OK, abort() otherwise.
 */
int sctk_ptl_am_emit_get( sctk_ptl_am_local_data_t *user, size_t size, sctk_ptl_id_t remote, sctk_ptl_am_pte_t *pte, sctk_ptl_am_matchbits_t match, size_t local_off, size_t remote_off, void *user_ptr )
{
	sctk_ptl_chk( PtlGet(
		user->slot_h.mdh,
		local_off,
		size,
		remote,
		pte->idx,
		match.raw,
		remote_off,
		user_ptr ) );

	return PTL_OK;
}

/**
 * Emit a PTL Put() request.
 * \param[in] user the preset request, associated to a MD slot
 * \param[in] size the length (in bytes) targeted by the request
 * \param[in] remote the remote ID
 * \param[in] pte the PT entry to target remotely (all processes init the table in the same order)
 * \param[in] match the match_bits
 * \param[in] local_off the offset in the local buffer (MD)
 * \param[in] remote_off the offset in the remote buffer (ME)
 * \param[in] user_ptr the request to attach to the Portals command.
 * \return PTL_OK, abort() otherwise.
 */
int sctk_ptl_am_emit_put( sctk_ptl_am_local_data_t *user, size_t size, sctk_ptl_id_t remote, sctk_ptl_am_pte_t *pte, sctk_ptl_am_matchbits_t match, size_t local_off, size_t remote_off, sctk_ptl_am_imm_data_t extra, void *user_ptr )
{
	assert( size <= user->slot.md.length );

	//mpc_common_debug_warning("PUT-%d %init_msg+%llu -> %llu REMOTE=%d/%d MATCH=%s (EXTRA=%llu)", pte->idx, user->slot.md.start+local_off, size, remote_off, remote.phys.nid, remote.phys.pid, __sctk_ptl_am_match_str((char*)sctk_malloc(70), 70, match.raw), extra.raw);

	sctk_ptl_chk( PtlPut(
		user->slot_h.mdh,
		local_off, /* offset */
		size,
		PTL_ACK_REQ,
		remote,
		pte->idx,
		match.raw,
		remote_off, /* offset */
		user_ptr,
		extra.raw ) );

	return PTL_OK;
}

/**
 * Create the initial Portals topology: the ring.
 * \param[in] rail the just-initialized Portals rail
 */
void sctk_ptl_am_register_process( sctk_ptl_am_rail_info_t *srail )
{
	int tmp_ret;

	/* serialize the id_t to a string, compliant with PMI handling */
	srail->connection_infos_size = sctk_ptl_am_data_serialize(
		&srail->id,				 /* the process it to serialize */
		sizeof( sctk_ptl_id_t ), /* size of the Portals ID struct */
		srail->connection_infos, /* the string to store the serialization */
		MPC_COMMON_MAX_STRING_SIZE			 /* max allowed string's size */
	);
	assert( srail->connection_infos_size > 0 );

	/* register the serialized id into the PMI */
	tmp_ret = mpc_launch_pmi_put_as_rank(
		srail->connection_infos,	  /* the string to publish */
		SCTK_PTL_AM_PMI_TAG,			  /* rail ID: PMI tag */
		0 /* Not local */
	);
	assert( tmp_ret == 0 );

	//Wait for all processes to complete the ring topology init */
 mpc_launch_pmi_barrier();
}

/**
 * Retrieve the ptl_process_id object, associated with a given MPC process rank.
 * \param[in] rail the Portals rail
 * \param[in] dest the MPC process rank
 * \return the Portals process id
 */
sctk_ptl_id_t sctk_ptl_am_map_id( __UNUSED__ sctk_ptl_am_rail_info_t *srail, int dest )
{
	int tmp_ret;
	char connection_infos[MPC_COMMON_MAX_STRING_SIZE];
	sctk_ptl_id_t id = SCTK_PTL_ANY_PROCESS;

	/* retrieve the right neighbour id struct */
	tmp_ret = mpc_launch_pmi_get_as_rank(
		connection_infos,	/* the recv buffer */
		MPC_COMMON_MAX_STRING_SIZE,	 /* the recv buffer max size */
		SCTK_PTL_AM_PMI_TAG, /* rail IB: PMI tag */
		dest				 /* which process we are targeting */
	);

	assert( tmp_ret == 0 );

	sctk_ptl_am_data_deserialize(
		connection_infos, /* the buffer containing raw data */
		&id,			  /* the target struct */
		sizeof( id )	  /* target struct size */
	);
	return id;
}

#endif /* MPC_USE_PORTALS */
