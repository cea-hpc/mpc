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

#if defined(MPC_USE_PORTALS) && defined(MPC_Active_Message)
#include <limits.h>
#include "sctk_debug.h"
#include "sctk_alloc.h"
#include "sctk_io_helper.h"
#include "sctk_ptl_am_iface.h"
#include "sctk_ptl_am_types.h"
#include "sctk_atomics.h"


/**
 * Will print the Portals structure.
 * \param[in] srail the Portals rail to dump
 */
void sctk_ptl_am_print_structure(sctk_ptl_am_rail_info_t* srail)
{
	sctk_ptl_limits_t l = srail->max_limits;
	sctk_info(
	"\n======== PORTALS AM STRUCTURE ========\n"
	"\n"
	" PORTALS ENTRIES       : \n"
	"  - PTE flags          : 0x%x\n"
	"  - Comm. nb entries   : %d\n"
	"  - HIDDEN nb entries  : %d\n"
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
	SCTK_PTL_AM_PTE_HIDDEN,
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
	l.max_triggered_ops
	);
	
}

/**
 * Init max values for driver config limits.
 * \param[in] l the limits to set
 */
static inline void __sctk_ptl_set_limits(sctk_ptl_limits_t* l)
{
	*l = (ptl_ni_limits_t){
		.max_entries            = INT_MAX,      /* Max number of entries allocated at any time */
		.max_unexpected_headers = INT_MAX,      /* max number of headers at any time */
		.max_mds                = INT_MAX,      /* Max number of MD allocated at any time */
		.max_cts                = INT_MAX,      /* Max number of CT allocated at any time */
		.max_eqs                = INT_MAX,      /* Max number of EQ allocated at any time */
		.max_pt_index           = INT_MAX,      /* Max PT index */
		.max_iovecs             = INT_MAX,      /* max number of iovecs for a single MD */
		.max_list_size          = INT_MAX,      /* Max number of entrie for one PT entry */
		.max_triggered_ops      = INT_MAX,      /* Max number of triggered ops */
		.max_msg_size           = PTL_SIZE_MAX, /* max message's size */
		.max_atomic_size        = PTL_SIZE_MAX, /* max size writable atomically */
		.max_fetch_atomic_size  = PTL_SIZE_MAX, /* Max size for a fetch op */
		.max_waw_ordered_size   = PTL_SIZE_MAX, /* Max length for ordering-guarantee */
		.max_war_ordered_size   = PTL_SIZE_MAX, /* max length for ordering guarantee */
		.max_volatile_size      = PTL_SIZE_MAX, /* Max length for MD w/ VOLATILE flag */
		.features               = 0             /* could be a combination or TARGET_BIND_INACCESSIBLE | TOTAL_DATA_ORDERING | COHERENT_ATOMIC */
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
	sctk_ptl_chk(PtlInit());

	/* set max values */
	__sctk_ptl_set_limits(&l);

	/* init one physical interface */
	sctk_ptl_chk(PtlNIInit(
		PTL_IFACE_DEFAULT,                 /* Type of interface */
		PTL_NI_MATCHING | PTL_NI_PHYSICAL, /* physical NI, w/ matching enabled */
		PTL_PID_ANY,                       /* Let Portals decide process's PID */
		&l,                                /* desired Portals boundaries */
		&res.max_limits,                   /* effective Portals limits */
		&res.iface                         /* THE handler */
	));

	/* retrieve the process identifier */
	sctk_ptl_chk(PtlGetPhysId(
		res.iface, /* The NI handler */
		&res.id    /* the structure to store it */
	));

	return res;
}

/**
 * Shut down the link between our program and the driver.
 * \param[in] srail the Portals rail, created by sctk_ptl_hardware_init
 */
void sctk_ptl_am_hardware_fini(sctk_ptl_am_rail_info_t* srail)
{
	/* tear down the interface */
	sctk_ptl_chk(PtlNIFini(srail->iface));

	/* tear down the driver */
	PtlFini();
}

static void sctk_ptl_am_pte_populate(sctk_ptl_am_rail_info_t* srail, sctk_ptl_am_pte_t* pte, int me_type, size_t nb_elts, int me_flags, int srvcode)
{
	size_t j = 0;
	sctk_ptl_matchbits_t match = (sctk_ptl_matchbits_t){
		.data.srvcode  = srvcode,
		.data.rpcode   = 0,
		.data.tag      = 0,
		.data.inc_data = 0,
		.data.is_req   = me_type,
		.data.is_large = SCTK_PTL_AM_OP_SMALL
	};

	sctk_ptl_matchbits_t ign = (sctk_ptl_matchbits_t)
	{
		.data.srvcode  = SCTK_PTL_AM_MATCH_SRV,  /* fixed SRV code for a given PTE */
		.data.rpcode   = SCTK_PTL_AM_IGN_RPC,    /* RPCs are gathered in the same ME */
		.data.tag      = (me_type == SCTK_PTL_AM_REQ_TYPE) ? SCTK_PTL_AM_IGN_TAG : SCTK_PTL_AM_MATCH_TAG,    /* REQ accepts any tags */
		.data.inc_data = SCTK_PTL_AM_IGN_DATA,   /* Define whether data are in the request */
		.data.is_req   = SCTK_PTL_AM_MATCH_TYPE, /* Type should be REQUEST */
		.data.is_large = SCTK_PTL_AM_MATCH_LARGE /* the REQ should NEVER be large */
	};

	sctk_ptl_am_chunk_t* c = NULL, *first = NULL, *prev = NULL;

	for (j = 0; j < nb_elts; ++j) 
	{
		prev = c;
		sctk_ptl_am_local_data_t* block;
		c = sctk_calloc(1, sizeof(sctk_ptl_am_chunk_t));
		sctk_atomics_store_int(&c->refcnt, 0);
		sctk_atomics_store_int(&c->noff, 0);

		int tag = (me_type == SCTK_PTL_AM_REQ_TYPE) ? 0 : sctk_atomics_fetch_and_incr_int(&pte->next_tag);
		match.data.tag = tag;
		c->tag = tag;

		block = sctk_ptl_am_me_create(
				c->buf,
				SCTK_PTL_AM_CHUNK_SZ,
				SCTK_PTL_ANY_PROCESS,
				match,
				ign,
				me_flags
				);
		sctk_ptl_am_me_register(srail, block, pte);
		c->uptr = block;
		c->next = pte->req_head;
		
		/* remember the first block, to append into our list */
		if(!prev)
			first = c;
		else
			prev->next = c;
	}

	/* store the new start */
	if(first)
		pte->req_head = first;
}

void sctk_ptl_am_md_populate(sctk_ptl_am_rail_info_t* srail, size_t nb_elts, int md_flags)
{
	int j = 0;
	sctk_ptl_am_chunk_t* c = NULL, *prev = NULL, *first = NULL;

	for (j = 0; j < nb_elts; ++j)
	{
		prev = c;
		sctk_ptl_am_local_data_t* block;
		sctk_ptl_am_chunk_t* c = sctk_calloc(1, sizeof(sctk_ptl_am_chunk_t));
		block = sctk_ptl_am_md_create(srail, c->buf, SCTK_PTL_AM_CHUNK_SZ, md_flags);

		c->tag = -1;
		sctk_ptl_am_md_register(srail, block);
		
		c->uptr = block;
		c->next = NULL;
		sctk_atomics_store_int(&c->refcnt, 0);
		sctk_atomics_store_int(&c->noff, 0);

		if(!prev)
			first = c;
		else
			prev->next = c;
	}
	if(first)
		srail->md_head = first;

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
void sctk_ptl_am_software_init(sctk_ptl_am_rail_info_t* srail, int nb_srv)
{
	size_t i, j;
	size_t eager_size = srail->eager_limit;

	sctk_ptl_am_pte_t * table = NULL;
	srail->nb_entries = nb_srv;
	nb_srv += SCTK_PTL_AM_PTE_HIDDEN;

	table = sctk_malloc(sizeof(sctk_ptl_am_pte_t) * nb_srv);
	MPCHT_init(&srail->pt_table, 256);

	for (i = 0; i < SCTK_PTL_AM_PTE_HIDDEN; i++)
	{
		/* nothing to do for now */
	}

	/* Not initialize each PTE */
	for (i = SCTK_PTL_AM_PTE_HIDDEN; i < nb_srv; ++i)
	{
		sctk_ptl_am_pte_create(srail, table + i, i);
		sctk_ptl_am_pte_populate(srail, table+i, SCTK_PTL_AM_REQ_TYPE, SCTK_PTL_AM_REQ_NB_DEF, SCTK_PTL_AM_ME_PUT_FLAGS | PTL_ME_MANAGE_LOCAL, i);
		sctk_ptl_am_pte_populate(srail, table+i, SCTK_PTL_AM_REP_TYPE, SCTK_PTL_AM_REP_NB_DEF, SCTK_PTL_AM_ME_PUT_FLAGS, i);

	}

	/* create the global EQ, shared by pending MDs */
	sctk_ptl_chk(PtlEQAlloc(
		srail->iface,        /* The NI handler */
		SCTK_PTL_AM_EQ_MDS_SIZE, /* number of slots available for events */
		&srail->mds_eq        /* out: the EQ handler */
	));

	sctk_ptl_am_md_populate(srail, SCTK_PTL_AM_MD_NB_DEF, SCTK_PTL_AM_MD_FLAGS);
	srail->md_lock = SCTK_SPINLOCK_INITIALIZER;

	sctk_ptl_am_print_structure(srail);
}

/**
 * Dynamically create a new Portals entry.
 * \param[in] srail the Portals rail
 * \param[out] pte Portals entry pointer, to init
 * \param[in] key the Portals desired ID
 */
void sctk_ptl_am_pte_create(sctk_ptl_am_rail_info_t* srail, sctk_ptl_am_pte_t* pte, size_t key)
{
	size_t eager_size = srail->eager_limit;
	/* create the EQ for this PT */
	sctk_ptl_chk(PtlEQAlloc(
		srail->iface,            /* the NI handler */
		SCTK_PTL_AM_EQ_PTE_SIZE, /* the number of slots in the EQ */
		&pte->eq                 /* the returned handler */
	));

	/* register the PT */
	sctk_ptl_chk(PtlPTAlloc(
		srail->iface,          /* the NI handler */
		SCTK_PTL_AM_PTE_FLAGS, /* PT entry specific flags */
		pte->eq,               /* the EQ for this entry */
		key,                   /* the desired index value */
		&pte->idx              /* the effective index value */
	));

	pte->pte_lock = SCTK_SPINLOCK_INITIALIZER;
	sctk_atomics_store_int(&pte->next_tag, 0);
	pte->req_head = NULL;
	pte->rep_head = NULL;


	/* sctk_ptl_me_feed(srail, pte, eager_size, SCTK_PTL_ME_OVERFLOW_NB, SCTK_PTL_OVERFLOW_LIST, SCTK_PTL_TYPE_STD, SCTK_PTL_PROT_NONE); */
	
	MPCHT_set(&srail->pt_table, key, pte);
	/* suppose that comm_idx always increase
	 * We add 1, because key is an idx */
	srail->nb_entries++;
}

void sctk_ptl_am_send_request(sctk_ptl_am_rail_info_t* srail, int srv, int rpc, void* start_in, size_t sz_in, void** start_out, size_t* sz_out, sctk_ptl_id_t remote)
{
	/*
	 * 1. if large, prepare a dedicated ME
	 * 2. if no more MD space, create a new chunk
	 * 3. Save somewhere the offset where the request has been put into the MD (to be provided when calling Put())
	 */
	sctk_assert(sz_in >= 0);
	sctk_ptl_am_pte_t* pte = MPCHT_get(&srail->pt_table, srv);
	int tag = -1;
	/* find the MD where data could be memcpy'd */
	size_t space = 0;
	size_t md_off = 0, me_off = 0;
	sctk_ptl_am_chunk_t* c_md;
	sctk_ptl_am_chunk_t* c_me;
	
	sctk_ptl_matchbits_t md_match, md_ign;


	/******************************************/
	/******************************************/
	/******************************************/
	
	if(start_out)
	{
		int found_space = 1;
		do
		{
			c_me = pte->rep_head;
			while(c_me)
			{
				me_off = sctk_atomics_fetch_and_add_int(&c_me->noff, SCTK_PTL_AM_REP_CELL_SZ);
				if(me_off <= SCTK_PTL_AM_CHUNK_SZ) /* last cell */
					break;

				sctk_atomics_fetch_and_add_int(&c_me->noff, (-1) * (SCTK_PTL_AM_REP_CELL_SZ));
				c_me = c_me->next;
			}
			if(!c_me)
			{
				sctk_ptl_am_pte_populate(srail, pte, SCTK_PTL_AM_REP_TYPE, 1, SCTK_PTL_AM_ME_PUT_FLAGS, srv );
				found_space = 0;
			}
		} while (!found_space);
		tag = c_me->tag;
	}
	
	/******************************************/
	/******************************************/
	/******************************************/
	if(sz_in >= SCTK_PTL_AM_CHUNK_SZ)
	{
		/* dedicated ME */
		sctk_ptl_am_local_data_t* user_ptr;
		sctk_ptl_matchbits_t match = SCTK_PTL_MATCH_BUILD(srv, rpc, tag, 1, SCTK_PTL_AM_REQ_TYPE, SCTK_PTL_AM_OP_LARGE);
		sctk_ptl_matchbits_t ign = SCTK_PTL_IGN_FOR_LARGE;
		
		user_ptr = sctk_ptl_am_me_create(start_in, sz_in, remote , match, ign, SCTK_PTL_AM_ME_GET_FLAGS);
		sctk_ptl_am_me_register(srail, user_ptr, pte);
		sz_in = 0;
	}
	
	c_md = srail->md_head;

	while(c_md)
	{
		md_off = sctk_atomics_fetch_and_add_int(&c_md->noff, sz_in);
		space = SCTK_PTL_AM_CHUNK_SZ - md_off;

		if(space >= sz_in)
			break;

		sctk_atomics_fetch_and_add_int(&c_md->noff, (-1) * sz_in);
		c_md = c_md->next;
	}
	if(c_md && space >= sz_in)  /* Found a valid MD */
	{
		sctk_assert(md_off > 0 && SCTK_PTL_AM_CHUNK_SZ - md_off == space);
		memcpy(c_md->buf + md_off, start_in, sz_in);
	}
	else /* no available MDs : allocate a new one and retry */
	{
		sctk_ptl_am_md_populate(srail, 1, SCTK_PTL_AM_MD_FLAGS);
		sctk_ptl_am_send_request(srail, srv, rpc, start_in, sz_in, start_out, sz_out, remote);
	}

	md_match = SCTK_PTL_MATCH_BUILD(srv, rpc, tag , (sz_in == 0), SCTK_PTL_AM_REQ_TYPE, SCTK_PTL_AM_OP_SMALL);
	md_ign = SCTK_PTL_IGN_FOR_REQ;
	sctk_ptl_am_emit_put(c_md->uptr, sz_in, remote, pte, md_match, md_off, -1, me_off, c_md->uptr);
	
}

void sctk_ptl_am_send_response(sctk_ptl_am_rail_info_t* srail, int srv, int rpc, void* start, size_t sz, sctk_ptl_id_t remote)
{
	/*
	 * 1. if no more ME space, create a new chunk
	 * 2. Save somewhere the offset where the response is expected (to be provided into imm_data when calling Put())
	 * 3. 
	 */
	if(sz >= SCTK_PTL_AM_CHUNK_SZ)
	{
		sz = 0;
	}

	size_t space = 0;

}


/**
 * Release Portals structure from our program.
 *
 * After here, no communication should be made on these resources
 * \param[in] srail the Portals rail
 */
void sctk_ptl_am_software_fini(sctk_ptl_am_rail_info_t* srail)
{
	int table_dims = srail->nb_entries;
	assert(table_dims > 0);
	int i;
	void* base_ptr = NULL;

	/* don't want to hang the NIC */
	return;

	for(i = 0; i < table_dims; i++)
	{
		sctk_ptl_am_pte_t* cur = MPCHT_get(&srail->pt_table, i);
		if(i==0)
			base_ptr = cur;

		sctk_ptl_chk(PtlEQFree(
			cur->eq       /* the EQ handler */
		));

		sctk_ptl_chk(PtlPTFree(
			srail->iface,     /* the NI handler */
			cur->idx      /* the PTE to destroy */
		));
	}

	sctk_ptl_chk(PtlEQFree(
		srail->mds_eq             /* the EQ handler */
	));

	/* write 'NULL' to be sure */
	sctk_free(base_ptr);
	MPCHT_release(&srail->pt_table);
	srail->nb_entries = 0;
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
sctk_ptl_am_local_data_t* sctk_ptl_am_me_create(void * start, size_t size, sctk_ptl_id_t remote, sctk_ptl_matchbits_t match, sctk_ptl_matchbits_t ign, int flags )
{
	sctk_ptl_am_local_data_t* user_ptr = sctk_malloc(sizeof(sctk_ptl_am_local_data_t));

	*user_ptr = (sctk_ptl_am_local_data_t){
		.slot.me = (sctk_ptl_me_t){
			.start = start,
			.length = (ptl_size_t) size,
			.ct_handle = PTL_CT_NONE,
			.uid = PTL_UID_ANY,
			.match_id = remote,
			.match_bits = match.raw,
			.ignore_bits = ign.raw,
			.options = flags,
			.min_free = 0
		},
		.slot_h.meh = PTL_INVALID_HANDLE,
	};

	return user_ptr;
}

/**
 * This function will create an ME with an attached counting event.
 * This function calls sctk_ptl_me_create.
 * \param[in] srail the Portals rail
 * \param[in] start first buffer address
 * \param[in] size number of bytes to include into the ME
 * \param[in] remote the target process id
 * \param[in] match the matching bits for this slot
 * \param[in] ign the ignoring bits from the above parameter
 * \param[in] flags ME-specific flags (PUT,GET,...)
 * \return the allocated request
 */
sctk_ptl_am_local_data_t* sctk_ptl_am_me_create_with_cnt(sctk_ptl_am_rail_info_t* srail, void * start, size_t size, sctk_ptl_id_t remote, sctk_ptl_matchbits_t match, sctk_ptl_matchbits_t ign, int flags )
{
	sctk_ptl_am_local_data_t* ret; 
	
	ret = sctk_ptl_am_me_create(start, size, remote, match, ign, flags | PTL_ME_EVENT_CT_COMM);

	sctk_ptl_chk(PtlCTAlloc(
		srail->iface,
		&ret->slot.me.ct_handle
	));

	return ret;
}

/**
 * Register a memory entry to the specific PTE.
 *
 * \param[in] slot the already created ME
 * \param[in] pte  the PT index where the ME will be attached to
 * \param[in] list in which list this ME has to be registed ? PRIORITY || OVERFLOW
 */
void sctk_ptl_am_me_register(sctk_ptl_am_rail_info_t* srail, sctk_ptl_am_local_data_t* user_ptr, sctk_ptl_am_pte_t* pte)
{
	assert(user_ptr);
	sctk_ptl_chk(PtlMEAppend(
		srail->iface,         /* the NI handler */
		pte->idx,             /* the targeted PT entry */
		&user_ptr->slot.me,   /* the ME to register in the table */
		SCTK_PTL_PRIORITY_LIST, /* in which list the ME has to be appended */
		user_ptr,             /* usr_ptr: forwarded when polling the event */
		&user_ptr->slot_h.meh /* out: the ME handler */
	));
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
void sctk_ptl_am_me_release(sctk_ptl_am_local_data_t* request)
{
	/* WARN: This function can return
	 * PTL_IN_USE if the targeted ME is in OVERFLOW_LIST
	 * and at least one unexpected header exists for it
	 */
	sctk_ptl_chk(PtlMEUnlink(
		request->slot_h.meh
	));
}

/**
 * Free the memory associated with a Portals request and free the request depending on parameters.
 * \param[in] request the request to free
 * \param[in] free_buffer is the 'start' buffer to be freed too ?
 */
void sctk_ptl_am_me_free(sctk_ptl_am_local_data_t* request, int free_buffer)
{
	if(free_buffer)
	{
		sctk_free(request->slot.me.start);
	}
	sctk_free(request);
}

/**
 * Free the counting event allocated with sctk_ptl_me_create_with_cnt.
 * \param[in] cth the counting event handler.
 */
void sctk_ptl_am_ct_free(sctk_ptl_cnth_t cth)
{
	sctk_ptl_chk(PtlCTFree(
		cth
	));
}

/**
 * Create a local memory region to be registered to Portals.
 * \param[in] start buffer first address
 * \param[in] length buffer length
 * \param[in] flags MD-specific flags (PUT, GET...)
 * \return a pointer to the MD
 */
sctk_ptl_am_local_data_t* sctk_ptl_am_md_create(sctk_ptl_am_rail_info_t* srail, void* start, size_t length, int flags)
{
	sctk_ptl_am_local_data_t* user = sctk_malloc(sizeof(sctk_ptl_am_local_data_t));
	
	*user = (sctk_ptl_am_local_data_t){
		.slot.md = (sctk_ptl_md_t){
			.start = start,
			.length = (ptl_size_t)length,
			.options = flags,
			.ct_handle = PTL_CT_NONE,
			.eq_handle = srail->mds_eq
		},
		.slot_h.mdh = PTL_INVALID_HANDLE,
	};

	return user;
}

/**
 * Register the MD inside Portals architecture.
 * \param[in] md the MD to register
 * \return a pointer to the MD handler
 */
void sctk_ptl_am_md_register(sctk_ptl_am_rail_info_t* srail, sctk_ptl_am_local_data_t* user)
{
	assert(user && srail);
	size_t max = srail->max_limits.max_mds;
	int i = 0;

	sctk_ptl_chk(PtlMDBind(
		srail->iface,     /* the NI handler */
		&user->slot.md,   /* the MD to bind with memory region */
		&user->slot_h.mdh /* out: the MD handler */
	));
}

/**
 * Release a previously registered MD.
 * This function will not free memory. Portals will ensure an event to be
 * published. The handling of this event will free any structure.
 *
 * \param[in] mdh the MD handler to release
 */
void sctk_ptl_am_md_release(sctk_ptl_am_local_data_t* request)
{
	assert(request);
	sctk_ptl_chk(PtlMDRelease(
		request->slot_h.mdh
	));
	sctk_free(request);
}

/**
 * Get the current process Portals ID.
 * \param[in] srail the Portals-specific info struct
 * \return the current sctk_ptl_id_t
 */
sctk_ptl_id_t sctk_ptl_am_self(sctk_ptl_am_rail_info_t* srail)
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
int sctk_ptl_am_emit_get(sctk_ptl_am_local_data_t* user, size_t size, sctk_ptl_id_t remote, sctk_ptl_am_pte_t* pte, sctk_ptl_matchbits_t match, size_t local_off, size_t remote_off, void* user_ptr)
{
	sctk_ptl_chk(PtlGet(
		user->slot_h.mdh,
		local_off,
		size,
		remote,
		pte->idx,
		match.raw,
		remote_off,
		user_ptr
	));

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
int sctk_ptl_am_emit_put(sctk_ptl_am_local_data_t* user, size_t size, sctk_ptl_id_t remote, sctk_ptl_am_pte_t* pte, sctk_ptl_matchbits_t match, size_t local_off, size_t remote_off, size_t extra, void* user_ptr)
{
	assert (size <= user->slot.md.length);
	sctk_ptl_chk(PtlPut(
		user->slot_h.mdh,
		local_off, /* offset */
		size,
		PTL_ACK_REQ,
		remote,
		pte->idx,
		match.raw,
		remote_off, /* offset */
		user_ptr,
		extra
	));

	return PTL_OK;
}
#endif
