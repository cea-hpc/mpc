#include <limits.h>
#include "sctk_debug.h"
#include "sctk_alloc.h"
#include "sctk_io_helper.h"
#include "sctk_ptl_iface.h"
#include "sctk_ptl_types.h"

void sctk_ptl_print_structure(sctk_ptl_rail_info_t* srail)
{
	sctk_warning(
	"\n======== PORTALS STRUCTURE ========\n"
	"\n"
	" PORTALS ENTRIES            : \n"
	"  - PTE flags               : 0x%x\n"
	"  - Comm. nb entries        : %d\n"
	"  - HIDDEN nb entries       : %d\n"
	"  - First Comm entry addr   : %p\n"
	"  - First HIDDEN entry addr : %p\n"
	"  - each EQ size            : %d\n"
	"\n"
	" ME management              : \n"
	"  - Nb OVERFLOW slots       : %d\n"
	"  - OVERFLOW slot size      : %d\n"
	"  - ME PUT flags            : 0x%x\n"
	"  - ME GET flags            : 0x%x\n"
	"  - ME OVERFLOW FLAGS       : 0x%x\n"
	"\n"
	" MD management              : \n"
	"  - shared EQ size          : %d\n"
	"  - MD PUT flags            : 0x%x\n"
	"  - MD GET flags            : 0x%x\n"
	"\n===================================",
	SCTK_PTL_PTE_FLAGS,
	srail->nb_entries,
	SCTK_PTL_PTE_HIDDEN,
	srail->pt_entries,
	srail->pt_entries - SCTK_PTL_PTE_HIDDEN,
	SCTK_PTL_EQ_PTE_SIZE,

	SCTK_PTL_ME_OVERFLOW_NB,
	srail->eager_limit,
	SCTK_PTL_ME_PUT_FLAGS,
	SCTK_PTL_ME_GET_FLAGS,
	SCTK_PTL_ME_OVERFLOW_FLAGS,

	SCTK_PTL_EQ_MDS_SIZE,
	SCTK_PTL_MD_PUT_FLAGS,
	SCTK_PTL_MD_GET_FLAGS
	);
	
}

/**
 * Init max values for driver config limits.
 */
static inline void __sctk_ptl_set_limits(sctk_ptl_limits_t* l)
{
	*l = (ptl_ni_limits_t){
		.max_entries = INT_MAX,                /* Max number of entries allocated at any time */
		.max_unexpected_headers = INT_MAX,     /* max number of headers at any time */
		.max_mds = INT_MAX,                    /* Max number of MD allocated at any time */
		.max_cts = INT_MAX,                    /* Max number of CT allocated at any time */
		.max_eqs = INT_MAX,                    /* Max number of EQ allocated at any time */
		.max_pt_index = INT_MAX,               /* Max PT index */
		.max_iovecs = INT_MAX,                 /* max number of iovecs for a single MD */
		.max_list_size = INT_MAX,              /* Max number of entrie for one PT entry */
		.max_triggered_ops = INT_MAX,          /* Max number of triggered ops */
		/*.max_cids = INT_MAX,                   [> max number of CID's (?) <]*/
		.max_msg_size = PTL_SIZE_MAX,          /* max message's size */
		.max_atomic_size = PTL_SIZE_MAX,       /* max size writable atomically */
		.max_fetch_atomic_size = PTL_SIZE_MAX, /* Max size for a fetch op */
		.max_waw_ordered_size = PTL_SIZE_MAX,  /* Max length for ordering-guarantee */
		.max_war_ordered_size = PTL_SIZE_MAX,  /* max length for ordering guarantee */
		.max_volatile_size = PTL_SIZE_MAX,     /* Max length for MD w/ VOLATILE flag */
		.features = 0                          /* could be a combination or TARGET_BIND_INACCESSIBLE | TOTAL_DATA_ORDERING | COHERENT_ATOMIC */
	};
}

/**
 * Create the link between our program and the real driver.
 */
sctk_ptl_rail_info_t sctk_ptl_hardware_init()
{
	sctk_ptl_rail_info_t res;
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
 */
void sctk_ptl_hardware_fini(sctk_ptl_rail_info_t* srail)
{
	/* tear down the interface */
	sctk_ptl_chk(PtlNIFini(srail->iface));

	/* tear down the driver */
	PtlFini();
}

/**
 * Map the Portals structure in user-space to be ready for communication.
 *
 * After this functions, Portals entries should be ready to use. This function
 * is different from hardware init beceause it is possible to have multiple 
 * software implementation relying on the same NI (why not?).
 * \param[in] dims PT dimensions
 * \return the pointer to the PT
 */
sctk_ptl_pte_t* sctk_ptl_software_init(sctk_ptl_rail_info_t* srail)
{
	sctk_ptl_pte_t * table = NULL;
	size_t i, dims, eager_size;

	dims = srail->nb_entries + SCTK_PTL_PTE_HIDDEN;
	eager_size = srail->eager_limit;

	table = sctk_malloc(sizeof(sctk_ptl_pte_t) * dims); /* one CM, one recovery, one RDMA */
	for (i = 0; i < dims; ++i)
	{
		/* create the EQ for this PT */
		sctk_ptl_chk(PtlEQAlloc(
			srail->iface,         /* the NI handler */
			SCTK_PTL_EQ_PTE_SIZE, /* the number of slots in the EQ */
			&table[i].eq          /* the returned handler */
		));

		/* register the PT */
		sctk_ptl_chk(PtlPTAlloc(
			srail->iface,       /* the NI handler */
			SCTK_PTL_PTE_FLAGS, /* PT entry specific flags */
			table[i].eq,        /* the EQ for this entry */
			i,                  /* the desired index value */
			&table[i].idx       /* the effective index value */
		));

		sctk_ptl_me_feed_overflow(srail, table + i, eager_size, SCTK_PTL_ME_OVERFLOW_NB);
	}



	/* create the global EQ, shared by pending MDs */
	sctk_ptl_chk(PtlEQAlloc(
		srail->iface,        /* The NI handler */
		SCTK_PTL_EQ_MDS_SIZE, /* number of slots available for events */
		&srail->mds_eq        /* out: the EQ handler */
	));

	sctk_ptl_print_structure(srail);

	srail->pt_entries = (sctk_ptl_pte_t*)table + SCTK_PTL_PTE_HIDDEN;
	sctk_atomics_store_int(&srail->rdma_cpt, 0);
	return srail->pt_entries;
}

/**
 * Release Portals structure from our program.
 *
 * After here, no communication should be made on these resources
 */
void sctk_ptl_software_fini(sctk_ptl_rail_info_t* srail)
{
	int table_dims = srail->nb_entries;
	sctk_ptl_pte_t* table = srail->pt_entries;
	assert(table_dims > 0);
	int i;
	for(i = 0; i < table_dims; i++)
	{
		sctk_ptl_chk(PtlEQFree(
			table[i].eq       /* the EQ handler */
		));

		sctk_ptl_chk(PtlPTFree(
			srail->iface,     /* the NI handler */
			table[i].idx      /* the PTE to destroy */
		));
	}

	sctk_ptl_chk(PtlEQFree(
		srail->mds_eq             /* the EQ handler */
	));
	/* write 'NULL' to be sure */
	void * base_ptr = table - (sizeof(sctk_ptl_pte_t) * SCTK_PTL_PTE_HIDDEN);
	memset(base_ptr, 0, sizeof(sctk_ptl_pte_t) * (table_dims + SCTK_PTL_PTE_HIDDEN));
	sctk_free(base_ptr);
	srail->nb_entries = 0;
}


/**
 * create a new PTE, to append to the Portals table.
 *
 * \param[in,out] srail the Portals-specific rial
 * \param[in] comm_id the communicator id
 * \return a pointer to the (potentially newly-allocated) portals table.
 */
sctk_ptl_pte_t* sctk_ptl_entry_add(sctk_ptl_rail_info_t* srail, int comm_id)
{
	not_implemented();
	return NULL;
}

/**
 * This function creates a new memory entry and register it in the table.
 *
 * \param[in] start first buffer address
 * \param[in] size number of bytes to include into the ME
 * \param[in] remote the target process id
 * \param[in] match the matching bits for this slot
 * \param[in] ign the ignoring bits from the above parameter
 * \param[in] flags ME-specific flags (PUT,GET,...)
 */
sctk_ptl_local_data_t* sctk_ptl_me_create(void * start, size_t size, sctk_ptl_id_t remote, sctk_ptl_matchbits_t match, sctk_ptl_matchbits_t ign, int flags )
{
	sctk_ptl_local_data_t* user_ptr = sctk_malloc(sizeof(sctk_ptl_local_data_t));

	*user_ptr = (sctk_ptl_local_data_t){
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
		.list = SCTK_PTL_PRIORITY_LIST,
		.msg = NULL /* later filled */
	};

	return user_ptr;
}

/**
 * Register a memory entry to the specific PTE.
 *
 * \param[in] slot the already created ME
 * \param[in] pte  the PT index where the ME will be attached to
 * \param[in] list in which list this ME has to be registed ? PRIORITY || OVERFLOW
 */
void sctk_ptl_me_register(sctk_ptl_rail_info_t* srail, sctk_ptl_local_data_t* user_ptr, sctk_ptl_pte_t* pte)
{
	assert(user_ptr);
	sctk_ptl_chk(PtlMEAppend(
		srail->iface,         /* the NI handler */
		pte->idx,             /* the targeted PT entry */
		&user_ptr->slot.me,   /* the ME to register in the table */
		user_ptr->list,       /* in which list the ME has to be appended */
		user_ptr,             /* usr_ptr: forwarded when polling the event */
		&user_ptr->slot_h.meh /* out: the ME handler */
	));

}

/**
 * Remove the ME from the Portals table.
 *
 * This function WILL NOT free the ME. The unlink will generate
 * an event that will be in charge of freeing memory.
 * This function should not be used if ME can be set with PTL_ME_USE_ONCE flag.
 *
 *  \param[in] meh the ME handler.
 */
void sctk_ptl_me_release(sctk_ptl_local_data_t* request)
{
	/* WARN: This function can return
	 * PTL_IN_USE if the targeted ME is in OVERFLOW_LIST
	 * and at least one unexpected header exists for it
	 */
	sctk_ptl_chk(PtlMEUnlink(
		request->slot_h.meh
	));
	sctk_free(request);
}

/**
 * Create a local memory region to be registered to Portals.
 * \param[in] start buffer first address
 * \param[in] length buffer length
 * \param[in] flags MD-specific flags (PUT, GET...)
 * \return a pointer to the MD
 */
sctk_ptl_local_data_t* sctk_ptl_md_create(sctk_ptl_rail_info_t* srail, void* start, size_t length, int flags)
{
	sctk_ptl_local_data_t* user = sctk_malloc(sizeof(sctk_ptl_local_data_t));
	
	*user = (sctk_ptl_local_data_t){
		.slot.md = (sctk_ptl_md_t){
			.start = start,
			.length = (ptl_size_t)length,
			.options = flags,
			.ct_handle = PTL_CT_NONE,
			.eq_handle = srail->mds_eq
		},
		.slot_h.mdh = PTL_INVALID_HANDLE,
		.list = SCTK_PTL_PRIORITY_LIST,
		.msg = NULL /* later filled */
	};

	return user;
}

/**
 * Register the MD inside Portals architecture.
 * \param[in] md the MD to register
 * \return a pointer to the MD handler
 */
void sctk_ptl_md_register(sctk_ptl_rail_info_t* srail, sctk_ptl_local_data_t* user)
{
	assert(user && srail);
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
void sctk_ptl_md_release(sctk_ptl_local_data_t* request)
{
	assert(request);
	sctk_ptl_chk(PtlMDRelease(
		request->slot_h.mdh
	));
	sctk_free(request);
}

sctk_ptl_id_t sctk_ptl_self(sctk_ptl_rail_info_t* srail)
{
	return srail->id;
}

void sctk_ptl_me_feed_overflow(sctk_ptl_rail_info_t* srail, sctk_ptl_pte_t* pte, size_t me_size, int nb)
{
	int j;
	for (j = 0; j < nb; j++)
	{
		void* buf = sctk_malloc(me_size);
		sctk_ptl_local_data_t* user;

		user = sctk_ptl_me_create(
				buf, /* temporary buffer to pin */
				me_size, /* buffer size */
				SCTK_PTL_ANY_PROCESS, /* targetable by any process */
				SCTK_PTL_MATCH_INIT, /* we don't care the match_bits */ 
				SCTK_PTL_IGN_ALL, /* triggers all requestss */
				SCTK_PTL_ME_OVERFLOW_FLAGS /* OVERFLOW-specifics flags */
				);
		user->msg = NULL;
		user->list = SCTK_PTL_OVERFLOW_LIST;

		sctk_ptl_me_register(srail, user, pte);
	}
}

int sctk_ptl_emit_get(sctk_ptl_local_data_t* user, size_t size, sctk_ptl_id_t remote, sctk_ptl_pte_t* pte, sctk_ptl_matchbits_t match)
{
	sctk_ptl_chk(PtlGet(
		user->slot_h.mdh,
		0,
		size,
		remote,
		pte->idx,
		match.raw,
		0,
		user
	));

	return PTL_OK;
}

int sctk_ptl_emit_put(sctk_ptl_local_data_t* user, size_t size, sctk_ptl_id_t remote, sctk_ptl_pte_t* pte, sctk_ptl_matchbits_t match)
{
	assert (size <= user->slot.md.length);
	sctk_ptl_chk(PtlPut(
		user->slot_h.mdh,
		0, /* offset */
		size,
		PTL_ACK_REQ,
		remote,
		pte->idx,
		match.raw,
		0, /* offset */
		user,
		0 /* TBD */
	));

	return PTL_OK;
}

int sctk_ptl_emit_atomic()
{
	/*sctk_ptl_chk(PtlAtomic(*/
	/*));*/

	return PTL_OK;
}

int sctk_ptl_emit_fetch_atomic()
{
	/*sctk_ptl_chk(PtlFetchAtomic(*/
	/*));*/

	return PTL_OK;
}
