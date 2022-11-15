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

#include <limits.h>
#include "mpc_common_debug.h"
#include "sctk_alloc.h"
#include "mpc_common_helper.h"
#include "mpc_common_spinlock.h"
#include "sctk_ptl_iface.h"
#include "sctk_ptl_types.h"
#include "mpc_common_datastructure.h"
#include "sctk_ptl_offcoll.h"
#include "sctk_low_level_comm.h"
#include "mpc_common_rank.h"
#include "mpc_launch_pmi.h"

static OPA_int_t nb_mds = OPA_INT_T_INITIALIZER(0);

/**
 * Will print the Portals structure.
 * \param[in] srail the Portals rail to dump
 */
void sctk_ptl_print_structure(__UNUSED__ sctk_ptl_rail_info_t* srail)
{
#ifdef MPC_ENABLE_DEBUG_MESSAGES
	sctk_ptl_limits_t l = srail->max_limits;
	mpc_common_debug(
	"\n======== PORTALS STRUCTURE ========\n"
	"\n"
	" PORTALS ENTRIES       : \n"
	"  - PTE flags          : 0x%x\n"
	"  - Comm. nb entries   : %d\n"
	"  - HIDDEN nb entries  : %d\n"
	"  - each EQ size       : %d\n"
	"  - Max msg size       : %llu\n"
	"\n"
	" ME management         : \n"
	"  - Nb OVERFLOW slots  : %d\n"
	"  - OVERFLOW slot size : %d\n"
	"  - ME PUT flags       : 0x%x\n"
	"  - ME GET flags       : 0x%x\n"
	"  - ME OVERFLOW FLAGS  : 0x%x\n"
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
	"  - MAX Msg Size (MB)  : %d\n"
	"\n"
	" OFFLOADING\n"
	"  - Collectives        : %s\n"
	"  - On-demand          : %s\n"
	"\n===================================",
	SCTK_PTL_PTE_FLAGS,
	srail->nb_entries,
	SCTK_PTL_PTE_HIDDEN,
	SCTK_PTL_EQ_PTE_SIZE,
	srail->max_mr,

	SCTK_PTL_ME_OVERFLOW_NB,
	srail->eager_limit,
	SCTK_PTL_ME_PUT_FLAGS,
	SCTK_PTL_ME_GET_FLAGS,
	SCTK_PTL_ME_OVERFLOW_FLAGS,

	SCTK_PTL_EQ_MDS_SIZE,
	SCTK_PTL_MD_PUT_FLAGS,
	SCTK_PTL_MD_GET_FLAGS,

	l.max_pt_index,
	l.max_unexpected_headers,
	l.max_mds,
	l.max_entries,
	l.max_cts,
	l.max_eqs,
	l.max_iovecs,
	l.max_list_size,
	l.max_triggered_ops,
	((size_t)l.max_msg_size / (1024 * 1024)),
	(SCTK_PTL_IS_OFFLOAD_COLL(srail->offload_support) ? "True" : "False"),
	(SCTK_PTL_IS_OFFLOAD_OD(srail->offload_support) ? "True" : "False")
	);
	#endif

}

/**
 * Init max values for driver config limits.
 * \param[in] l the limits to set
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
 * \return the Portals rail object
 */
sctk_ptl_rail_info_t sctk_ptl_hardware_init(sctk_ptl_interface_t iface)
{

	/* BXI specific We first need to make sure we do not exhaust command queues on the BXI
	   by settign a few internal variables for CQs sharing */
	int local_proc_count = mpc_common_get_local_process_count();

	if(local_proc_count >= 62)
	{
		int cqmaxpid = (local_proc_count / 62) + 1;
		if(cqmaxpid > 8)
		{
			cqmaxpid = 8;
		}
		char val[16];
		snprintf(val, 16, "%d", cqmaxpid);
		/* Set not overwriting */
		setenv("PORTALS4_CQ_MAXPIDS", val, 0);

		uint64_t job_id = 1111;
		mpc_launch_pmi_get_job_id(&job_id);
		snprintf(val, 16, "%lu", job_id);

		setenv("PORTALS4_CQ_GROUP", val, 0);
	}

	sctk_ptl_rail_info_t res;
	memset(&res, 0, sizeof(sctk_ptl_rail_info_t));
	sctk_ptl_limits_t l;

	/* pre-actions */
	assert(sizeof(sctk_ptl_std_content_t)     <= sizeof(ptl_match_bits_t));
	assert(sizeof(sctk_ptl_offload_content_t) <= sizeof(ptl_match_bits_t));
	assert(sizeof(sctk_ptl_matchbits_t)       <= sizeof(ptl_match_bits_t));
	assert(sizeof(sctk_ptl_std_data_t)        <= sizeof(ptl_hdr_data_t));
	assert(sizeof(sctk_ptl_cm_data_t)         <= sizeof(ptl_hdr_data_t));
	assert(sizeof(sctk_ptl_offload_data_t)    <= sizeof(ptl_hdr_data_t));
	assert(sizeof(sctk_ptl_imm_data_t)        <= sizeof(ptl_hdr_data_t));
	
	/* init the driver */
	sctk_ptl_chk(PtlInit());

	/* set max values */
	__sctk_ptl_set_limits(&l);

	/* init one physical interface */
	sctk_ptl_chk(PtlNIInit(
		iface,                             /* Type of interface */
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
void sctk_ptl_hardware_fini(sctk_ptl_rail_info_t* srail)
{
	/* tear down the interface */
	sctk_ptl_chk(PtlNIFini(srail->iface));

	/* tear down the driver */
	PtlFini();
}

void lcr_ptl_software_init(sctk_ptl_rail_info_t *srail, size_t comm_dims)
{
        int i;
        sctk_ptl_md_t md; 
	sctk_ptl_pte_t * table = NULL;
	srail->nb_entries = 0;

	comm_dims = SCTK_PTL_PTE_HIDDEN + comm_dims;
        /* create the EQ */
        sctk_ptl_chk(PtlEQAlloc(srail->iface,         /* the NI handler */
                                SCTK_PTL_EQ_PTE_SIZE, /* the number of slots in the EQ */
                                &srail->mes_eq        /* the returned handler */
                               ));

	table = sctk_malloc(sizeof(sctk_ptl_pte_t) * comm_dims); /* one CM, one recovery, one RDMA */
	memset(table, 0, comm_dims * sizeof(sctk_ptl_pte_t));
	
	mpc_common_hashtable_init(&srail->pt_table, (comm_dims < 64) ? 64 : comm_dims);
	mpc_common_hashtable_init(&srail->reverse_pt_table, (comm_dims < 64) ? 64 : comm_dims);

	for (i = 0; i < SCTK_PTL_PTE_HIDDEN; i++)
	{
		/* register the PT */
		sctk_ptl_chk(PtlPTAlloc(
			srail->iface,       /* the NI handler */
			SCTK_PTL_PTE_FLAGS, /* PT entry specific flags */
			srail->mes_eq,      /* the EQ for this entry */
			i,                  /* the desired index value */
			&table[i].idx       /* the effective index value */
		));

                mpc_common_hashtable_set(&srail->pt_table, i, &table[i]);
	}

        md.start     = 0;
        md.length    = PTL_SIZE_MAX;
        md.options   = 0;
        md.ct_handle = PTL_CT_NONE;
        md.eq_handle = srail->mes_eq;

	sctk_ptl_chk(PtlMDBind(
		srail->iface,     /* the NI handler */
		&md,              /* the MD to bind with memory region */
		&srail->mdh       /* out: the MD handler */
	));
	
	OPA_store_int(&srail->rdma_cpt, 0);
	if(srail->max_mr > srail->max_limits.max_msg_size)
		srail->max_mr = srail->max_limits.max_msg_size;
	
	if(sctk_ptl_offcoll_enabled(srail))
		sctk_ptl_offcoll_init(srail);

	//FIXME: nb_entries was not set but used in fini function which
	//       segfaulted
	srail->nb_entries = comm_dims;

	sctk_ptl_print_structure(srail);
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
void sctk_ptl_software_init(sctk_ptl_rail_info_t* srail, size_t comm_dims)
{
	size_t i;
	//size_t eager_size = srail->eager_limit;

	sctk_ptl_pte_t * table = NULL;
	srail->nb_entries = 0;
	comm_dims = SCTK_PTL_PTE_HIDDEN + comm_dims;

	table = sctk_malloc(sizeof(sctk_ptl_pte_t) * comm_dims); /* one CM, one recovery, one RDMA */
	memset(table, 0, comm_dims * sizeof(sctk_ptl_pte_t));
	
	mpc_common_hashtable_init(&srail->pt_table, (comm_dims < 64) ? 64 : comm_dims);
	mpc_common_hashtable_init(&srail->reverse_pt_table, (comm_dims < 64) ? 64 : comm_dims);


	for (i = 0; i < SCTK_PTL_PTE_HIDDEN; i++)
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
			i,           /* the desired index value */
			&table[i].idx       /* the effective index value */
		));

                mpc_common_hashtable_set(&srail->pt_table, i, &table[i]);
		/*table[i].taglocks = sctk_malloc(sizeof(mpc_common_spinlock_t) * SCTK_PTL_PTE_NB_LOCKS);*/
		/*int j;*/
		/*for (j = 0; j < SCTK_PTL_PTE_NB_LOCKS; ++j) */
		/*{*/
			/*table[i].taglocks[j] = MPC_COMMON_SPINLOCK_INITIALIZER;*/
		/*}*/
	}

	/* fill the CM queue with preallocated buffers
	 * This is the only situation where ME-PUT w/ IGNORE_ALL should be
	 * pushed into the priority list !
	 */
	//NOTE: this leaked it is already allocated in previous loop 
	//sctk_ptl_me_feed(srail, table + SCTK_PTL_PTE_CM, eager_size, SCTK_PTL_ME_OVERFLOW_NB, SCTK_PTL_PRIORITY_LIST, SCTK_PTL_TYPE_CM, SCTK_PTL_PROT_NONE);

	//NOTE: initially allocated to avoid dynamical porte allocation
	//	but notify_new_comm called each time new comm created.
	//for (i = SCTK_PTL_PTE_HIDDEN; i < comm_dims; ++i)
	//{
	//	sctk_ptl_pte_create(srail, &table[i], i, i);
	//}

	/* create the global EQ, shared by pending MDs */
	sctk_ptl_chk(PtlEQAlloc(
		srail->iface,        /* The NI handler */
		SCTK_PTL_EQ_MDS_SIZE, /* number of slots available for events */
		&srail->mds_eq        /* out: the EQ handler */
	));

	OPA_store_int(&srail->rdma_cpt, 0);
	if(srail->max_mr > srail->max_limits.max_msg_size)
		srail->max_mr = srail->max_limits.max_msg_size;
	
	if(sctk_ptl_offcoll_enabled(srail))
		sctk_ptl_offcoll_init(srail);

	//FIXME: nb_entries was not set but used in fini function which
	//       segfaulted
	srail->nb_entries = comm_dims;

	sctk_ptl_print_structure(srail);
}

static inline void __pte_idx_register(sctk_ptl_rail_info_t* srail, ptl_pt_index_t idx, mpc_lowcomm_communicator_id_t comm_id)
{
	mpc_lowcomm_communicator_id_t * new = sctk_malloc(sizeof(mpc_lowcomm_communicator_id_t));
	*new = comm_id - SCTK_PTL_PTE_HIDDEN;
	mpc_common_nodebug("REVERSE REG %d maps to %llu", idx, *new );
	mpc_common_hashtable_set(&srail->reverse_pt_table, idx, new);
}


mpc_lowcomm_communicator_id_t sctk_ptl_pte_idx_to_comm_id(sctk_ptl_rail_info_t* srail, ptl_pt_index_t idx)
{
	mpc_lowcomm_communicator_id_t* ret = mpc_common_hashtable_get(&srail->reverse_pt_table, idx);
	assume(ret != NULL);
	return *ret;
}


/**
 * Dynamically create a new Portals entry.
 * \param[in] srail the Portals rail
 * \param[out] pte Portals entry pointer, to init
 * \param[in] key the Portals desired ID
 */
void sctk_ptl_pte_create(sctk_ptl_rail_info_t* srail, sctk_ptl_pte_t* pte, ptl_pt_index_t requested_index, size_t key)
{
	size_t eager_size = srail->eager_limit;
#ifdef MPC_LOWCOMM_PROTOCOL
	/* register the PT */
	sctk_ptl_chk(PtlPTAlloc(
		srail->iface,       /* the NI handler */
		SCTK_PTL_PTE_FLAGS, /* PT entry specific flags */
		srail->mes_eq,        /* the EQ for this entry */
		requested_index,  /* the desired index value */
		&pte->idx       /* the effective index value */
	));
#else 
	/* create the EQ for this PT */
	sctk_ptl_chk(PtlEQAlloc(
		srail->iface,         /* the NI handler */
		SCTK_PTL_EQ_PTE_SIZE, /* the number of slots in the EQ */
		&pte->eq          /* the returned handler */
	));

	/* register the PT */
	sctk_ptl_chk(PtlPTAlloc(
		srail->iface,       /* the NI handler */
		SCTK_PTL_PTE_FLAGS, /* PT entry specific flags */
		pte->eq,        /* the EQ for this entry */
		requested_index,  /* the desired index value */
		&pte->idx       /* the effective index value */
	));
#endif

	if(sctk_ptl_offcoll_enabled(srail))
	{
		sctk_ptl_offcoll_pte_init(srail, pte);
	}

	sctk_ptl_me_feed(srail, pte, eager_size, SCTK_PTL_ME_OVERFLOW_NB, SCTK_PTL_OVERFLOW_LIST, SCTK_PTL_TYPE_STD,
	 SCTK_PTL_PROT_NONE);

	mpc_common_hashtable_set(&srail->pt_table, key, pte);

	mpc_common_nodebug("REG %llu maps to %d", key, pte->idx );

	__pte_idx_register(srail, pte->idx, key);

	srail->nb_entries++;
	//mpc_common_debug_error("CREATING comm %d", srail->nb_entries);

}

static inline void __pte_release(sctk_ptl_rail_info_t* srail, sctk_ptl_pte_t* pte)
{
	if(sctk_ptl_offcoll_enabled(srail))
		sctk_ptl_offcoll_pte_fini(srail, pte);
	
#ifndef MPC_LOWCOMM_PROTOCOL
	sctk_ptl_chk(PtlEQFree(
		pte->eq       /* the EQ handler */
	));
#endif

	sctk_ptl_chk(PtlPTFree(
		srail->iface,     /* the NI handler */
		pte->idx      /* the PTE to destroy */
	));	

	srail->nb_entries--;
}

void sctk_ptl_pte_release(sctk_ptl_rail_info_t* srail, ptl_pt_index_t requested_index)
{
	sctk_ptl_pte_t* cur = mpc_common_hashtable_get(&srail->pt_table, requested_index);

	if(!cur)
	{
		return;
	}

	mpc_common_hashtable_delete(&srail->pt_table, requested_index);

	/* Remove the PTE */
	__pte_release(srail, cur);
}



/**
 * Release Portals structure from our program.
 *
 * After here, no communication should be made on these resources
 * \param[in] srail the Portals rail
 */
void sctk_ptl_software_fini(sctk_ptl_rail_info_t* srail)
{
	int table_dims = srail->nb_entries;
	assert(table_dims > 0);
	int i;
	void* base_ptr = NULL;

	/* don't want to hang the NIC */
	//return;

	for(i = 0; i < table_dims; i++)
	{
		sctk_ptl_pte_t* cur = mpc_common_hashtable_get(&srail->pt_table, i);


		if(i==0)
			base_ptr = cur;

		if (cur)
			__pte_release(srail, cur);
	}

	sctk_ptl_chk(PtlEQFree(
		srail->mds_eq             /* the EQ handler */
	));

	/* write 'NULL' to be sure */
	sctk_free(base_ptr);
	mpc_common_hashtable_release(&srail->pt_table);
	mpc_common_hashtable_release(&srail->reverse_pt_table);

	srail->nb_entries = 0;

	if(sctk_ptl_offcoll_enabled(srail))
		sctk_ptl_offcoll_fini(srail);
}

static inline void __sctk_ptl_ct_alloc(sctk_ptl_rail_info_t* srail, sctk_ptl_cnth_t* cnth_ptr)
{
	sctk_ptl_chk(PtlCTAlloc(
		srail->iface,
		cnth_ptr
	));
}
void sctk_ptl_ct_alloc(sctk_ptl_rail_info_t* srail, sctk_ptl_cnth_t* cnth_ptr)
{
	return __sctk_ptl_ct_alloc(srail, cnth_ptr);
}

/**
 * Free the counting event allocated with sctk_ptl_me_create_with_cnt.
 * \param[in] cth the counting event handler.
 */
void sctk_ptl_ct_free(sctk_ptl_cnth_t cth)
{
	sctk_ptl_chk(PtlCTFree(
		cth
	));
}

size_t sctk_ptl_ct_get(sctk_ptl_cnth_t cth)
{
	sctk_ptl_cnt_t dummy = (sctk_ptl_cnt_t){.success = 0, .failure = 0};
	sctk_ptl_chk(PtlCTGet(cth, &dummy));
	return (size_t)dummy.success;
}

void sctk_ptl_ct_wait_thrs(sctk_ptl_cnth_t cth, size_t thrs, sctk_ptl_cnt_t* ev)
{
	ev->success = 0;
	ev->failure = 0;
	int i = 1;

	/* choose one */
#if 0 
	PtlCTWait(cth, thrs, ev);
#else
	/* by benching these two methods, the BXI Wait() 
	 * is longer than a while(){Get();} but is probably also
	 * less stressful. Also the solution below let the program
	 * works even if -c=1, as polling is required to progress those
	 * who are calling this ct_wait_thrs() function (offloaded colls mainly)
	 */
	sctk_ptl_chk(PtlCTGet(cth, ev));
	while(ev->success < thrs && !ev->failure)
	{
		sctk_ptl_chk(PtlCTGet(cth, ev));
		/*if(!(i % 1000))*/
		_mpc_lowcomm_multirail_notify_idle();
		i++;
	}
#endif
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
		.type = SCTK_PTL_TYPE_NONE,
		.prot = SCTK_PTL_PROT_NONE,
		.cnt_frag = OPA_INT_T_INITIALIZER(0),
		.match = SCTK_PTL_MATCH_INIT,
		.msg_seq_nb = -1,
		.req_sz = 0,
		.msg = NULL /* later filled */
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
sctk_ptl_local_data_t* sctk_ptl_me_create_with_cnt(sctk_ptl_rail_info_t* srail, void * start, size_t size, sctk_ptl_id_t remote, sctk_ptl_matchbits_t match, sctk_ptl_matchbits_t ign, int flags )
{
	sctk_ptl_local_data_t* ret; 
	
	ret = sctk_ptl_me_create(start, size, remote, match, ign, (flags | PTL_ME_EVENT_CT_COMM | PTL_ME_EVENT_CT_OVERFLOW));

	__sctk_ptl_ct_alloc(srail, &ret->slot.me.ct_handle);

	return ret;
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
	assert((NULL != user_ptr->slot.me.start) || (0 == user_ptr->slot.me.length));
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
 * The unlink will generate an event that will be in charge of freeing memory.
 * This function should not be used if ME can be set with PTL_ME_USE_ONCE flag.
 * That's why we have \see sctk_ptl_me_free.
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
}

/**
 * Free the memory associated with a Portals request and free the request depending on parameters.
 * \param[in] request the request to free
 * \param[in] free_buffer is the 'start' buffer to be freed too ?
 */
void sctk_ptl_me_free(sctk_ptl_local_data_t* request, int free_buffer)
{
	return;
	if(free_buffer)
	{
		if(request->list == SCTK_PTL_PRIORITY_LIST)
			sctk_free(request->slot.me.start);
		else
			sctk_free((char*)request->slot.me.start - sizeof(OPA_ptr_t));
	}
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
		.cnt_frag = OPA_INT_T_INITIALIZER(0),
		.match = SCTK_PTL_MATCH_INIT,
		.msg_seq_nb = -1,
		.msg = NULL /* later filled */
	};

	return user;
}

sctk_ptl_local_data_t* sctk_ptl_md_create_with_cnt(sctk_ptl_rail_info_t* srail, void* start, size_t length, int flags)
{
	sctk_ptl_local_data_t* user = NULL;
	user = sctk_ptl_md_create(srail, start, length, flags);
	
	__sctk_ptl_ct_alloc(srail, &user->slot.md.ct_handle);

	return user;
}

#ifdef MPC_LOWCOMM_PROTOCOL
/**
 * Register the MD inside Portals architecture.
 * \param[in] md the MD to register
 * \return a pointer to the MD handler
 */
int lcr_ptl_md_register(sctk_ptl_rail_info_t* srail, sctk_ptl_local_data_t* user)
{
	assert(user && srail);
	int max = srail->max_limits.max_mds;

	if (OPA_fetch_and_incr_int(&nb_mds) >= max)
	{
		OPA_decr_int(&nb_mds);
		return MPC_LOWCOMM_NO_RESOURCE;
	}

	sctk_ptl_chk(PtlMDBind(
		srail->iface,     /* the NI handler */
		&user->slot.md,   /* the MD to bind with memory region */
		&user->slot_h.mdh /* out: the MD handler */
	));
	
	return MPC_LOWCOMM_SUCCESS;
}
#endif

/**
 * Register the MD inside Portals architecture.
 * \param[in] md the MD to register
 * \return a pointer to the MD handler
 */
void sctk_ptl_md_register(sctk_ptl_rail_info_t* srail, sctk_ptl_local_data_t* user)
{
	assert(user && srail);
	int max = srail->max_limits.max_mds;
	int i = 0;

	while(OPA_fetch_and_incr_int(&nb_mds) >= max)
	{
		OPA_decr_int(&nb_mds);
		if((i++)%20000 == 0)
			_mpc_lowcomm_multirail_notify_idle();
		else
			mpc_thread_yield();
	}

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
	OPA_decr_int(&nb_mds);
	sctk_free(request);
}

/**
 * Get the current process Portals ID.
 * \param[in] srail the Portals-specific info struct
 * \return the current sctk_ptl_id_t
 */
sctk_ptl_id_t sctk_ptl_self(sctk_ptl_rail_info_t* srail)
{
	return srail->id;
}

/**
 * Append some OVERFLOW slots to the requested PT entry.
 * \param[in] srail the Portals-specific info struct
 * \param[in] pte the PT to enlarge with OVERFLOW ME
 * \param[in] size the ME size
 * \param[in] nb numberof ME to add
 */
void sctk_ptl_me_feed(sctk_ptl_rail_info_t* srail, sctk_ptl_pte_t* pte, size_t me_size, int nb, int list, char type, char protocol)
{
	int j;
	assert(list == SCTK_PTL_PRIORITY_LIST || list == SCTK_PTL_OVERFLOW_LIST);

	for (j = 0; j < nb; j++)
	{
		sctk_ptl_local_data_t* user;
		void* buf = sctk_malloc(me_size);
		user = sctk_ptl_me_create(
				buf, /* keep it explicit... */
				me_size, /* buffer size */
				SCTK_PTL_ANY_PROCESS, /* targetable by any process */
				SCTK_PTL_MATCH_INIT, /* we don't care the match_bits */ 
				SCTK_PTL_IGN_ALL, /* triggers all requestss */
				((list == SCTK_PTL_PRIORITY_LIST) ? SCTK_PTL_ME_PUT_FLAGS : SCTK_PTL_ME_OVERFLOW_FLAGS) | SCTK_PTL_ONCE
		);
		user->msg = NULL;
		user->list = list;
		user->type = type;
		user->prot = protocol;

		sctk_ptl_me_register(srail, user, pte);
	}
}

int sctk_ptl_me_emit_probe(sctk_ptl_rail_info_t* srail, sctk_ptl_pte_t* pte, sctk_ptl_local_data_t* user, int probe_level )
{
	user->type = SCTK_PTL_TYPE_PROBE;
	sctk_ptl_chk(PtlMESearch(
		srail->iface,
		pte->idx,
		&user->slot.me,
		probe_level,
		user
	));

	return PTL_OK;
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
int sctk_ptl_emit_get(sctk_ptl_local_data_t* user, size_t size, sctk_ptl_id_t remote, sctk_ptl_pte_t* pte, sctk_ptl_matchbits_t match, size_t local_off, size_t remote_off, void* user_ptr)
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
int sctk_ptl_emit_put(sctk_ptl_local_data_t* user, size_t size, sctk_ptl_id_t remote, sctk_ptl_pte_t* pte, sctk_ptl_matchbits_t match, size_t local_off, size_t remote_off, size_t extra, void* user_ptr)
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

/**
 * Emit a PTL Swap() request.
 * \param[in] get_user the preset request, where data will be stored
 * \param[in] get_user the preset request, data to send
 * \param[in] size the length (in bytes) targeted by the request
 * \param[in] remote the remote ID
 * \param[in] pte the PT entry to target remotely (all processes init the table in the same order)
 * \param[in] match the match_bits
 * \param[in] local_getoff the offset in the local GET buffer (MD)
 * \param[in] local_putoff the offset in the local PUT buffer (MD)
 * \param[in] remote_off the offset in the remote buffer (ME)
 * \param[in] cmp the compare element
 * \param[in] type the Portals RDMA type
 * \param[in] user_ptr the request to attach to the Portals command.
 * \return PTL_OK, abort() otherwise
 */
int sctk_ptl_emit_swap(sctk_ptl_local_data_t* get_user, sctk_ptl_local_data_t* put_user, size_t size, sctk_ptl_id_t remote, sctk_ptl_pte_t* pte, sctk_ptl_matchbits_t match, size_t local_getoff, size_t local_putoff, size_t remote_off, const void* cmp, sctk_ptl_rdma_type_t type, void* user_ptr)
{
	sctk_ptl_rdma_op_t op = PTL_CSWAP;

	sctk_ptl_chk(PtlSwap(
		get_user->slot_h.mdh, /* Where data will be copied locally */
		local_getoff,         /* local offset */
		put_user->slot_h.mdh, /* Data to send */
		local_putoff,         /* local offset */
		size,                 /* size to be sent/received */
		remote,               /* the target */
		pte->idx,             /* Portals index */
		match.raw,            /* match bits */
		remote_off,           /* remote offset */
		user_ptr,             /* attached user_ptr */
		0,                    /* TBD */
		cmp,                  /* The value used to compare */
		op,                   /* RDMA operation */
		type                  /* RDMA type */

	));

	return PTL_OK;
}

int sctk_ptl_emit_atomic(sctk_ptl_local_data_t* put_user, size_t size, sctk_ptl_id_t remote, sctk_ptl_pte_t* pte, sctk_ptl_matchbits_t match, size_t local_off, size_t remote_off, sctk_ptl_rdma_op_t op, sctk_ptl_rdma_type_t type, void* user_ptr)
{
	sctk_ptl_chk(PtlAtomic(
		put_user->slot_h.mdh, /* Data to send */
		local_off,            /* local offset */
		size,                 /* size to be sent/received */
		PTL_ACK_REQ,          /* want to receive _ACK event */
		remote,               /* the target */
		pte->idx,             /* Portals index */
		match.raw,            /* match bits */
		remote_off,           /* remote offset */
		user_ptr,             /* attached user_ptr */
		0,                    /* Not needed here */
		op,                   /* RDMA operation */
		type                  /* RDMA type */

	));

	return PTL_OK;
}

/**
 * Emit a PTL FetchAtomic() request.
 * \param[in] get_user the preset request, where data will be stored
 * \param[in] get_user the preset request, data to send
 * \param[in] size the length (in bytes) targeted by the request
 * \param[in] remote the remote ID
 * \param[in] pte the PT entry to target remotely (all processes init the table in the same order)
 * \param[in] match the match_bits
 * \param[in] local_getoff the offset in the local GET buffer (MD)
 * \param[in] local_putoff the offset in the local PUT buffer (MD)
 * \param[in] remote_off the offset in the remote buffer (ME)
 * \param[in] op the Portals RDMA operation
 * \param[in] type the Portals RDMA type
 * \param[in] user_ptr the request to attach to the Portals command.
 * \return PTL_OK, abort() otherwise
 */
int sctk_ptl_emit_fetch_atomic(sctk_ptl_local_data_t* get_user, sctk_ptl_local_data_t* put_user, size_t size, sctk_ptl_id_t remote, sctk_ptl_pte_t* pte, sctk_ptl_matchbits_t match, size_t local_getoff, size_t local_putoff, size_t remote_off, sctk_ptl_rdma_op_t op, sctk_ptl_rdma_type_t type, void* user_ptr)
{
	sctk_ptl_chk(PtlFetchAtomic(
		get_user->slot_h.mdh, /* Where data will be copied locally */
		local_getoff,         /* local offset */
		put_user->slot_h.mdh, /* Data to send */
		local_putoff,         /* local offset */
		size,                 /* size to be sent/received */
		remote,               /* the target */
		pte->idx,             /* Portals index */
		match.raw,            /* match bits */
		remote_off,           /* remote offset */
		user_ptr,             /* attached user_ptr */
		0,                    /* Not needed here */
		op,                   /* RDMA operation */
		type                  /* RDMA type */

	));

	return PTL_OK;
}

int sctk_ptl_emit_cnt_incr(sctk_ptl_cnth_t target_cnt, size_t incr)
{
	sctk_ptl_chk(PtlCTInc(
		target_cnt,
		(sctk_ptl_cnt_t){.success=incr, .failure=0}
	));
	return PTL_OK;
}

int sctk_ptl_emit_cnt_set(sctk_ptl_cnth_t target_cnt, size_t val)
{
	sctk_ptl_chk(PtlCTSet(
		target_cnt,
		(sctk_ptl_cnt_t){.success=val, .failure=0}
	));
	return PTL_OK;
}
/**
 * Same as Get(), but will be hardware-triggered when 'cnt' reaches the set threshold.
 *
 * \param[in] user the preset request, associated to a MD slot
 * \param[in] size the length (in bytes) targeted by the request
 * \param[in] remote the remote ID
 * \param[in] pte the PT entry to target remotely (all processes init the table in the same order)
 * \param[in] match the match_bits
 * \param[in] local_off the offset in the local buffer (MD)
 * \param[in] remote_off the offset in the remote buffer (ME)
 * \param[in] user_ptr the data to associate with the request
 * \param[in] cnt the counter triggering the Get()
 * \param[in] threshold the value triggering the Get()
 * \return PTL_OK, abort() otherwise.
 */
int sctk_ptl_emit_trig_get(sctk_ptl_local_data_t* user, size_t size, sctk_ptl_id_t remote, sctk_ptl_pte_t* pte, sctk_ptl_matchbits_t match, size_t local_off, size_t remote_off, void* user_ptr, sctk_ptl_cnth_t cnt, size_t threshold)
{
	/** WARNING: the API provide a prototype where user_ptr & remote_off are inverted.
	 *
	 * It is probably a typo error in the API  but be careful if some implementation chose to 
	 * be strict with the standard. The BXI implementation inverts them to be consistent
	 * with Get() initial syntax
	 */
	sctk_ptl_chk(PtlTriggeredGet(
		user->slot_h.mdh,
		local_off,
		size,
		remote,
		pte->idx,
		match.raw,
		remote_off,
		user_ptr,
		cnt,
		threshold
	));

	return PTL_OK;
}

int sctk_ptl_emit_trig_put(sctk_ptl_local_data_t* user, size_t size, sctk_ptl_id_t remote, sctk_ptl_pte_t* pte, sctk_ptl_matchbits_t match, size_t local_off, size_t remote_off, size_t extra, void* user_ptr, sctk_ptl_cnth_t cnt, size_t threshold)
{
	/** TODO: Be sure the case w/ triggeredGet does not occur
	 * here -> inverted parameters
	 */
	sctk_ptl_chk(PtlTriggeredPut(
		user->slot_h.mdh,
		local_off,
		size,
		PTL_ACK_REQ,
		remote,
		pte->idx,
		match.raw,
		remote_off,
		user_ptr,
		extra,
		cnt,
		threshold
	));

	return PTL_OK;
}

int sctk_ptl_emit_trig_atomic(sctk_ptl_local_data_t* put_user, size_t size, sctk_ptl_id_t remote, sctk_ptl_pte_t* pte, sctk_ptl_matchbits_t match, size_t local_off, size_t remote_off, sctk_ptl_rdma_op_t op, sctk_ptl_rdma_type_t type, void* user_ptr, sctk_ptl_cnth_t cnt, size_t threshold)
{
	/** TODO: Be sure the case w/ triggeredGet does not occur
	 * here -> inverted parameters
	 */
	sctk_ptl_chk(PtlTriggeredAtomic(
		put_user->slot_h.mdh, /* Data to send */
		local_off,            /* local offset */
		size,                 /* size to be sent/received */
		PTL_ACK_REQ,          /* want to receive _ACK event */
		remote,               /* the target */
		pte->idx,             /* Portals index */
		match.raw,            /* match bits */
		remote_off,           /* remote offset */
		user_ptr,             /* attached user_ptr */
		0,                    /* Not needed here */
		op,                   /* RDMA operation */
		type,                 /* RDMA type */
		cnt,
		threshold
	));

	return PTL_OK;
}

int sctk_ptl_emit_trig_fetch_atomic(sctk_ptl_local_data_t* get_user, sctk_ptl_local_data_t* put_user, size_t size, sctk_ptl_id_t remote, sctk_ptl_pte_t* pte, sctk_ptl_matchbits_t match, size_t local_getoff, size_t local_putoff, size_t remote_off, sctk_ptl_rdma_op_t op, sctk_ptl_rdma_type_t type, void* user_ptr, sctk_ptl_cnth_t cnt, size_t threshold)
{
	/** TODO: Be sure the case w/ triggeredGet does not occur
	 * here -> inverted parameters
	 */
	sctk_ptl_chk(PtlTriggeredFetchAtomic(
		get_user->slot_h.mdh, /* Where data will be copied locally */
		local_getoff,         /* local offset */
		put_user->slot_h.mdh, /* Data to send */
		local_putoff,         /* local offset */
		size,                 /* size to be sent/received */
		remote,               /* the target */
		pte->idx,             /* Portals index */
		match.raw,            /* match bits */
		remote_off,           /* remote offset */
		user_ptr,             /* attached user_ptr */
		0,                    /* Not needed here */
		op,                   /* RDMA operation */
		type,                 /* RDMA type */
		cnt,
		threshold
	));

	return PTL_OK;
}

int sctk_ptl_emit_trig_swap(sctk_ptl_local_data_t* get_user, sctk_ptl_local_data_t* put_user, size_t size, sctk_ptl_id_t remote, sctk_ptl_pte_t* pte, sctk_ptl_matchbits_t match, size_t local_getoff, size_t local_putoff, size_t remote_off, const void* cmp, sctk_ptl_rdma_type_t type, void* user_ptr, sctk_ptl_cnth_t cnt, size_t threshold)
{
	sctk_ptl_rdma_op_t op = PTL_CSWAP;

	sctk_ptl_chk(PtlTriggeredSwap(
		get_user->slot_h.mdh, /* Where data will be copied locally */
		local_getoff,         /* local offset */
		put_user->slot_h.mdh, /* Data to send */
		local_putoff,         /* local offset */
		size,                 /* size to be sent/received */
		remote,               /* the target */
		pte->idx,             /* Portals index */
		match.raw,            /* match bits */
		remote_off,           /* remote offset */
		user_ptr,             /* attached user_ptr */
		0,                    /* TBD */
		cmp,                  /* The value used to compare */
		op,                   /* RDMA operation */
		type,                 /* RDMA type */
		cnt,
		threshold

	));

	return PTL_OK;
}

int sctk_ptl_emit_trig_cnt_incr(sctk_ptl_cnth_t target_cnt, size_t incr, sctk_ptl_cnth_t tracked, size_t threshold)
{
	/* we only consider success increments here... */
	sctk_ptl_cnt_t cnt = (sctk_ptl_cnt_t){.success = incr, .failure = 0};
	sctk_ptl_chk(PtlTriggeredCTInc(
		target_cnt,  /* the counter to increment */
		cnt,         /* the increment value */
		tracked,     /* which counter used to trigger */
		threshold    /* threshold to reach by the cnt above */
	));

	return PTL_OK;
}

int sctk_ptl_emit_trig_cnt_set(sctk_ptl_cnth_t target_cnt, size_t val, sctk_ptl_cnth_t tracked, size_t threshold)
{
	sctk_ptl_chk(PtlTriggeredCTSet(
		target_cnt,  /* the counter to increment */
		(sctk_ptl_cnt_t){.success=val, .failure=0},         /* the new value */
		tracked,     /* which counter used to trigger */
		threshold    /* threshold to reach by the cnt above */
	));

	return PTL_OK;
}
