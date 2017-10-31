#include <limits.h>
#include "sctk_ptl_iface.h"
#include "sctk_debug.h"


static sctk_ptl_nih_t iface_handler;  /* *< The NI handler, scoped only in this file */
static sctk_ptl_eq_t mds_eq;          /* *< the global EQ, shared by registerd MDs */
static ptl_ni_limits_t driver_limits; /* *< global max values allowed by the NIC */
static sctk_ptl_id_t process_id;      /* *< local process id */
static int table_dims;                /* *< number of Portals entries */

/**
 * Init max values for driver config limits.
 */
static inline void __sctk_ptl_set_limits()
{
	driver_limits = (ptl_ni_limits_t){
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
void sctk_ptl_hardware_init()
{
	/* init the driver */
	sctk_ptl_chk(PtlInit());

	/* set max values */
	__sctk_ptl_set_limits();

	/* init one physical interface */
	sctk_ptl_chk(PtlNIInit(
		PTL_IFACE_DEFAULT,                 /* Type of interface */
		PTL_NI_MATCHING | PTL_NI_PHYSICAL, /* physical NI, w/ matching enabled */
		PTL_PID_ANY,                       /* Let Portals decide process's PID */
		&driver_limits,                    /* desired Portals boundaries */
		&driver_limits,                    /* effective Portals limits */
		&iface_handler                     /* THE handler */
	));

	/* retrieve the process identifier */
	sctk_ptl_chk(PtlGetPhysId(
		iface_handler,     /* The NI handler */
		&process_id        /* the structure to store it */
	));
}

/**
 * Shut down the link between our program and the driver.
 */
void sctk_ptl_hardware_fini()
{
	/* tear down the interface */
	sctk_ptl_chk(PtlNIFini(iface_handler));

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
sctk_ptl_pte_t* sctk_ptl_software_init(int dims)
{
	sctk_ptl_pte_t * table = NULL;
	int i;

	assert(dims > 0);
	table = sctk_malloc(sizeof(sctk_ptl_pte_t) * dims);
	table_dims = dims;
	for (i = 0; i < dims; ++i) 
	{
		/* create the EQ for this PT */
		sctk_ptl_chk(PtlEQAlloc(
			iface_handler,        /* the NI handler */
			SCTK_PTL_EQ_PTE_SIZE, /* the number of slots in the EQ */
			&table[i].eq          /* the returned handler */
		));

		/* register the PT */
		sctk_ptl_chk(PtlPTAlloc(
			iface_handler,      /* the NI handler */
			SCTK_PTL_PTE_FLAGS, /* PT entry specific flags */
			table[i].eq,        /* the EQ for this entry */
			i,                  /* the desired index value */
			&table[i].idx       /* the effective index value */
		));

		/* Populate the unique & generic ME in the overflow_list, to detect late-send patterns
		 */
		sctk_ptl_me_t* me = sctk_malloc(sizeof(sctk_ptl_me_t));
		*me = (sctk_ptl_me_t) {
			.start  = me,                          /* don't care here */
			.length = (ptl_size_t)0,               /* important: 0 will be seen as a truncated message and it is what we want */
			.uid = PTL_UID_ANY,
			.match_id.phys.nid = PTL_NID_ANY,
			.match_id.phys.pid = PTL_PID_ANY,
			.match_bits = 0 ,                      /* don't care here */
			.ignore_bits = SCTK_PTL_IGNORE_ALL.raw,/* we will match any request */
			.options = SCTK_PTL_ME_OVERFLOW_FLAGS, /* only GET events */
			.ct_handle = PTL_CT_NONE               /* no need of this counting event */
		};

		sctk_ptl_chk(PtlMEAppend(
			iface_handler,      /* the NI handler */
			table[i].idx,       /* the targeted PTE */
			me,                 /* the ME to append */
			PTL_OVERFLOW_LIST,  /* in which list to append it */
			&me,                /* user pointer to add at event-polling step */
			&table[i].uniq_meh  /* returned handler */
		));
	}

	/* create the global EQ, shared by pending MDs */
	sctk_ptl_chk(PtlEQAlloc(
		iface_handler,        /* The NI handler */
		SCTK_PTL_EQ_MDS_SIZE, /* number of slots available for events */
		&mds_eq               /* out: the EQ handler */
	));

	return table;
}

/**
 * Release Portals structure from our program.
 *
 * After here, no communication should be made on these resources
 */
void sctk_ptl_software_fini(sctk_ptl_pte_t* table)
{
	assert(table_dims > 0);
	int i;
	for(i = 0; i < table_dims; i++)
	{
		sctk_ptl_chk(PtlMEUnlink(
			table[i].uniq_meh /* The ME handler */
		));

		sctk_ptl_chk(PtlEQFree(
			table[i].eq        /* the EQ handler */
		));

		sctk_ptl_chk(PtlPTFree(
			iface_handler,     /* the NI handler */
			table[i].idx       /* the PTE to destroy */
		));
	}

	sctk_ptl_chk(PtlEQFree(
		mds_eq                     /* the EQ handler */
	));
	/* write 'NULL' to be sure */
	memset(table, 0, sizeof(sctk_ptl_pte_t) * table_dims);
	sctk_free(table);
	table_dims = 0;
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
 * \return a pointer to the ME
 */
sctk_ptl_me_t* sctk_ptl_me_create(void * start, size_t size, sctk_ptl_id_t remote, sctk_ptl_matchbits_t match, sctk_ptl_matchbits_t ign, int flags )
{
	sctk_ptl_me_t* slot = sctk_malloc(sizeof(sctk_ptl_me_t));

	*slot = (sctk_ptl_me_t){
		.start = start,
		.length = (ptl_size_t) size,
		.ct_handle = PTL_CT_NONE,
		.uid = PTL_UID_ANY,
		.match_id.phys.nid = remote.phys.nid,
		.match_id.phys.pid = remote.phys.pid,
		.match_bits = match.raw,
		.ignore_bits = ign.raw,
		.options = flags,
		.min_free = 0
	};
	return slot;
}

/**
 * Register a memory entry to the specific PTE.
 *
 * \param[in] slot the already created ME
 * \param[in] pte  the PT index where the ME will be attached to
 * \param[in] list in which list this ME has to be registed ? PRIORITY || OVERFLOW
 * \return a pointer to the ME handler
 */
sctk_ptl_meh_t* sctk_ptl_me_register(sctk_ptl_me_t* slot, sctk_ptl_pte_t* pte, ptl_list_t list)
{
	sctk_ptl_meh_t* slot_h = sctk_malloc(sizeof(sctk_ptl_meh_t));
	sctk_ptl_chk(PtlMEAppend(
		iface_handler, /* the NI handler */
		pte->idx,      /* the targeted PT entry */
		slot,          /* the ME to register in the table */
		list,          /* in which list the ME has to be appended */
		slot,          /* usr_ptr: forwarded when polling the event */
		slot_h         /* out: the ME handler */
	));

	return slot_h;
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
void sctk_ptl_me_release(sctk_ptl_meh_t* meh)
{
	/* WARN: This function can return
	 * PTL_IN_USE if the targeted ME is in OVERFLOW_LIST
	 * and at least one unexpected header exists for it
	 */
	sctk_ptl_chk(PtlMEUnlink(
		*meh
	));
	sctk_free(meh);
}

/**
 * Create a local memory region to be registered to Portals.
 * \param[in] start buffer first address
 * \param[in] length buffer length
 * \param[in] flags MD-specific flags (PUT, GET...)
 * \return a pointer to the MD
 */
sctk_ptl_md_t* sctk_ptl_md_create(void* start, size_t length, int flags)
{
	sctk_ptl_md_t* md = sctk_malloc(sizeof(sctk_ptl_md_t));

	*md = (sctk_ptl_md_t){
		.start = start,
		.length = (ptl_size_t)length,
		.options = flags,
		.ct_handle = PTL_CT_NONE,
		.eq_handle = mds_eq
	};

	return md;
}

/**
 * Register the MD inside Portals architecture.
 * \param[in] md the MD to register
 * \return a pointer to the MD handler
 */
sctk_ptl_mdh_t* sctk_ptl_md_register(const sctk_ptl_md_t* md)
{
	assert(md);
	sctk_ptl_mdh_t* mdh = sctk_malloc(sizeof(sctk_ptl_mdh_t));
	sctk_ptl_chk(PtlMDBind(
		iface_handler, /* the NI handler */
		md,            /* the MD to bind with memory region */
		mdh            /* out: the MD handler */
	));

	return mdh;
}

/**
 * Release a previously registered MD.
 * This function will not free memory. Portals will ensure an event to be
 * published. The handling of this event will free any structure.
 *
 * \param[in] mdh the MD handler to release
 */
void sctk_ptl_md_release(sctk_ptl_mdh_t* mdh)
{
	assert(mdh);
	sctk_ptl_chk(PtlMDRelease(
		*mdh
	));
	sctk_free(mdh);
}

/**
 * Attempt to progress pending requests by getting the next MD-related event.
 *
 * \param[out] ev the event to fill
 * \return <ul>
 * <li><b>PTL_OK</b> if an event has been found with no error</li>
 * <li><b>PTL_EQ_EMPTY</b> if not event are present in the queue </li>
 * <li><b>PTL_EQ_DROPPED</b> if a event has been polled but at least one event has been dropped due to a lack of space in the queue</lib>
 * <li><b>any other code</b> is an error</li>
 * </ul>
 */
int sctk_ptl_eq_poll_md(sctk_ptl_event_t* ev)
{
	int ret;
	
	assert(ev);
	ret = PtlEQGet(mds_eq, ev);

	if (ret == PTL_EQ_EMPTY || ret == PTL_EQ_DROPPED)
	{
			memset(ev, 0, sizeof(sctk_ptl_event_t));
			ev->type = -1;
			return ret;
	}
	
	sctk_ptl_chk(ret);
	
	switch(ev->type)
	{
		case PTL_EVENT_SEND:  /* in case of Put/Atomic, data left the local process */
		case PTL_EVENT_ACK:   /* in case of Put/Atomic, data reached the remote process */
		case PTL_EVENT_REPLY: /* in case of Get/Atomic, data returned to the local process */
			break;
		default:
			sctk_fatal("Portals MD event not recognized: %d", ev->type);
	}
	return PTL_OK;
}

/**
 * Poll asynchronous notification from the NIC, about registered memory regions.
 *
 * \param[in] pte the Portals entry which contains the EQ to process
 * \param[out] ev the event to fill
 * \return <ul>
 * <li><b>PTL_OK</b> if an event has been found with no error</li>
 * <li><b>PTL_EQ_EMPTY</b> if not event are present in the queue </li>
 * <li><b>PTL_EQ_DROPPED</b> if a event has been polled but at least one event has been dropped due to a lack of space in the queue</lib>
 * <li><b>any other code</b> is an error</li>
 * </ul>
 */
int sctk_ptl_eq_poll_me(sctk_ptl_pte_t* pte, sctk_ptl_event_t* ev)
{
	int ret;
	
	assert(ev);
	ret = PtlEQGet(pte->eq, ev);

	if (ret == PTL_EQ_EMPTY || ret == PTL_EQ_DROPPED)
	{
			memset(ev, 0, sizeof(sctk_ptl_event_t));
			ev->type = -1;
			return ret;
	}


	sctk_ptl_chk(ret);

	switch(ev->type)
	{
		case PTL_EVENT_GET:                   /* The target received a matching Get() request */
		case PTL_EVENT_GET_OVERFLOW:          /* the initiator did not match the data (late-send) */

		case PTL_EVENT_PUT:                   /* the remote pushed back the data after we failed to get at first place */
		case PTL_EVENT_PUT_OVERFLOW:          /* Control_message (it is never considered as an expected message) */

		case PTL_EVENT_ATOMIC:                /* Deal with Atomic operation, and matched it with an already posted request */
		case PTL_EVENT_ATOMIC_OVERFLOW:       /* received an unexpected incoming atomic request */

		case PTL_EVENT_FETCH_ATOMIC:          /* same as the one above but with FETCH_ATOMIC op */
		case PTL_EVENT_FETCH_ATOMIC_OVERFLOW: /* same as the one above but with FETCH_ATOMIC op */

		case PTL_EVENT_PT_DISABLED:           /* Error: this event is emitted when a request has been posted to a disabled PT */
		case PTL_EVENT_LINK:                  /* a new memory entry has been posted */
		case PTL_EVENT_AUTO_UNLINK:           /* the memory entry has been automatically unlinked (PTL_*E_USE_ONCE) */
		case PTL_EVENT_AUTO_FREE:             /* the previously automatic unlinked memory entry can now be reused */
		case PTL_EVENT_SEARCH:                /* A memory entry search has completed (but did not necessary succeeded) */
			break;
		default:
			sctk_fatal("Portals ME event not recognized: %d", ev->type);
	}

	return PTL_OK;
}

sctk_ptl_id_t sctk_ptl_self()
{
	return process_id;
}

int sctk_ptl_emit_get(sctk_ptl_mdh_t* mdh, size_t size, sctk_ptl_id_t remote, sctk_ptl_pte_t* pte, sctk_ptl_matchbits_t match)
{
	sctk_ptl_chk(PtlGet(
		*mdh,
		0,
		size,
		remote,
		pte->idx,
		match.raw,
		0,
		NULL
	));

	return PTL_OK;
}

int sctk_ptl_emit_put()
{
	/*sctk_ptl_chk(PtlPut(*/
	/*));*/

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
