
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
/* #   - CAPRA Antoine capra@paratools.com                                # */
/* #                                                                      # */
/* ######################################################################## */

#ifndef __MPCOMP_OMPT_TYPES_EVENT_H__
#define __MPCOMP_OMPT_TYPES_EVENT_H__

typedef enum ompt_event_e{

	/*--- Mandatory Events ---*/
	ompt_event_thread_begin 		= 1,
	ompt_event_thread_end   		= 2,
	ompt_event_parallel_begin = 3,
	ompt_event_parallel_end   = 4,
	ompt_event_task_create   = 5,
	ompt_event_task_schedule  = 6,
	ompt_event_implicite_task  = 7,
	ompt_event_target = 8,
	ompt_event_target_data = 9,
	ompt_event_target_update = 10,
	ompt_event_control = 11,
	ompt_event_runtime_shutdown = 12,

	/*--- Optional Events for Blame Shifting ---*/
	ompt_event_idle = 13,
	ompt_event_sync_region_wait = 14,
	ompt_event_mutex_realese = 15,

	/*--- Optional Events for Instrumentation-based Tools ---*/
	ompt_event_task_dependences = 16,
	ompt_event_task_dependences_pair = 17,
	ompt_event_worksharing = 18,
	ompt_event_master = 19,
	ompt_event_target_data_map = 20,
	ompt_event_sync_region = 21,
	ompt_event_init_lock = 22,
	ompt_event_destroy_lock = 23,
	ompt_event_mutex_acquire = 24,
	ompt_event_mutex_acquired = 25,
	ompt_event_nested_lock = 26,
	ompt_event_flush = 27
	ompt_event_cancel = 28
} ompt_event_t;

#endif /* __MPCOMP_OMPT_TYPES_EVENT_H__ */
