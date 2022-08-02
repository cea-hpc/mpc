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
/* #   - BESNARD Jean-Baptiste jbbesnard@paratools.fr                     # */
/* #                                                                      # */
/* ######################################################################## */
#include "multirail.h"
#include "mpc_common_datastructure.h"
#include "mpc_topology.h"

#ifdef MPC_USE_DMTCP
#include "sctk_ft_iface.h"
#endif

#include <sctk_alloc.h>
#include <mpc_lowcomm_monitor.h>

/************************************************************************/
/* Rail Gates                                                           */
/************************************************************************/

/* HERE ARE DEFAULT GATES */

static inline int __gate_boolean(__UNUSED__ sctk_rail_info_t *rail, __UNUSED__ mpc_lowcomm_ptp_message_t *message, void *gate_config)
{
	struct _mpc_lowcomm_config_struct_gate_boolean *conf = (struct _mpc_lowcomm_config_struct_gate_boolean *)gate_config;

	return conf->value;
}

static inline int __gate_probabilistic(__UNUSED__ sctk_rail_info_t *rail, __UNUSED__ mpc_lowcomm_ptp_message_t *message, void *gate_config)
{
	struct _mpc_lowcomm_config_struct_gate_probabilistic *conf = (struct _mpc_lowcomm_config_struct_gate_probabilistic *)gate_config;

	int num = (rand() % 100);

	return num < conf->probability;
}

static inline int __gate_minsize(__UNUSED__ sctk_rail_info_t *rail, mpc_lowcomm_ptp_message_t *message, void *gate_config)
{
	struct _mpc_lowcomm_config_struct_gate_min_size *conf = (struct _mpc_lowcomm_config_struct_gate_min_size *)gate_config;

	size_t message_size = SCTK_MSG_SIZE(message);

	return conf->value < message_size;
}

static inline int __gate_maxsize(__UNUSED__ sctk_rail_info_t *rail, mpc_lowcomm_ptp_message_t *message, void *gate_config)
{
	struct _mpc_lowcomm_config_struct_gate_max_size *conf = (struct _mpc_lowcomm_config_struct_gate_max_size *)gate_config;

	size_t message_size = SCTK_MSG_SIZE(message);

	return message_size < conf->value;
}

static inline int __gate_msgtype(__UNUSED__ sctk_rail_info_t *rail, mpc_lowcomm_ptp_message_t *message, void *gate_config)
{
	struct _mpc_lowcomm_config_struct_gate_message_type *conf = (struct _mpc_lowcomm_config_struct_gate_message_type *)gate_config;

	int is_process_specific = sctk_is_process_specific_message(SCTK_MSG_HEADER(message) );
	int tag = SCTK_MSG_TAG(message);

	mpc_lowcomm_ptp_message_class_t class =
		(SCTK_MSG_HEADER(message) )->message_type;

	/* It is a emulated RMA and it is not allowed */
	if( (class == MPC_LOWCOMM_RDMA_MESSAGE) && !conf->emulated_rma)
	{
		return 0;
	}

	/* It is a process specific message
	 * and it is allowed */
	if(is_process_specific && conf->process)
	{
		return 1;
	}

	/* It is a common message and it is allowed */
	if(!is_process_specific && conf->common)
	{
		return 1;
	}

	return 0;
}

struct __gate_ctx
{
	int   (*func)(sctk_rail_info_t *rail, mpc_lowcomm_ptp_message_t *message, void *gate_config);
	void *params;
};

union ptrconv
{
	void *ptr;
	int (*fp)(sctk_rail_info_t *, mpc_lowcomm_ptp_message_t *, void *);
};


static inline void __gate_ctx_get(struct _mpc_lowcomm_config_struct_net_gate *gate,
                                  struct __gate_ctx *ctx)
{
	ctx->func   = NULL;
	ctx->params = NULL;

	if(!gate)
	{
		return;
	}

	union ptrconv pconv;

	switch(gate->type)
	{
		case MPC_CONF_RAIL_GATE_BOOLEAN:
			ctx->func   = __gate_boolean;
			ctx->params = (void *)&gate->value.boolean;
			break;

		case MPC_CONF_RAIL_GATE_PROBABILISTIC:
			ctx->func   = __gate_probabilistic;
			ctx->params = (void *)&gate->value.probabilistic;
			break;

		case MPC_CONF_RAIL_GATE_MINSIZE:
			ctx->func   = __gate_minsize;
			ctx->params = (void *)&gate->value.minsize;
			break;

		case MPC_CONF_RAIL_GATE_MAXSIZE:
			ctx->func   = __gate_maxsize;
			ctx->params = (void *)&gate->value.maxsize;
			break;

		case MPC_CONF_RAIL_GATE_USER:
			pconv.ptr = gate->value.user.gatefunc.value;
			ctx->func = pconv.fp;
			if(!ctx->func)
			{
				bad_parameter("Could not resolve user-defined rail gate function %s == %p", gate->value.user.gatefunc.name, gate->value.user.gatefunc.value);
			}
			ctx->params = (void *)&gate->value.user;
			break;

		case MPC_CONF_RAIL_GATE_MSGTYPE:
			ctx->func   = __gate_msgtype;
			ctx->params = (void *)&gate->value.msgtype;
			break;

		case MPC_CONF_RAIL_GATE_NONE:
		default:
			mpc_common_debug_fatal("No such gate type");
	}
}

/***********************
* ENDPOINT LIST ENTRY *
***********************/

static inline int _mpc_lowcomm_multirail_endpoint_list_entry_init(_mpc_lowcomm_multirail_endpoint_list_entry_t *entry,
                                                                  _mpc_lowcomm_endpoint_t *endpoint)
{
	if(!endpoint)
	{
		mpc_common_debug_error("No endpoint provided.. ignoring");
		return 1;
	}

	if(!endpoint->rail)
	{
		mpc_common_debug_error("No rail found in endpoint.. ignoring");
		return 1;
	}

	entry->in_use = 1;

	entry->priority   = endpoint->rail->runtime_config_rail->priority;
	entry->gates      = endpoint->rail->runtime_config_rail->gates;
	entry->gate_count = endpoint->rail->runtime_config_rail->gates_size;

	entry->endpoint = endpoint;

	return 0;
}

static inline int _mpc_lowcomm_multirail_endpoint_list_entry_release(_mpc_lowcomm_multirail_endpoint_list_entry_t *entry)
{
	if(!entry)
	{
		return 1;
	}

	memset(entry, 0, sizeof(_mpc_lowcomm_multirail_endpoint_list_entry_t) );

	return 0;
}

/*****************
* ENDPOINT LIST *
*****************/

int _mpc_lowcomm_multirail_endpoint_list_init(_mpc_lowcomm_multirail_endpoint_list_t *list)
{
	list->size    = 128;
	list->entries = sctk_calloc(list->size, sizeof(_mpc_lowcomm_multirail_endpoint_list_entry_t) );
	assume(list->entries != NULL);
	list->elem_count = 0;

	return 0;
}

int _mpc_lowcomm_multirail_endpoint_list_release(_mpc_lowcomm_multirail_endpoint_list_t *list)
{
	sctk_free(list->entries);
	memset(list, 0, list->size * sizeof(_mpc_lowcomm_multirail_endpoint_list_t) );

	return 0;
}

int _mpc_lowcomm_multirail_endpoint_list_push(_mpc_lowcomm_multirail_endpoint_list_t *list,
                                              _mpc_lowcomm_endpoint_t *endpoint)
{
	int new_prio = endpoint->rail->priority;
	unsigned int i;

	/* Are we at the last element ? */
	if(list->elem_count == (list->size - 1) )
	{
		/* We need to realloc */
		list->entries = sctk_realloc(list->entries, list->size * 2 * sizeof(_mpc_lowcomm_multirail_endpoint_list_entry_t) );
		assume(list->entries);
		memset(list->entries + list->size, 0, list->size * sizeof(_mpc_lowcomm_multirail_endpoint_list_entry_t) );
		/* Now actually dup list size */
		list->size *= 2;
	}

	/* We should always have a free elem */
	assume(list->elem_count != (list->size - 1) );

	/* Now scan to find where we should put our slot
	 * priorities are in decreasing order */
	int insert_offset = -1;

	for(i = 0; i < list->elem_count; i++)
	{
		_mpc_lowcomm_multirail_endpoint_list_entry_t *entry = &list->entries[i];
		assert(entry->in_use);

		if(entry->priority < new_prio)
		{
			/* We found an existing endpoint with a lower prio */
			insert_offset = i;
			break;
		}
	}

	if(insert_offset != -1)
	{
		int to_move = list->elem_count - insert_offset - 1;
		assert(0 <= to_move);
		/* At the point where we want to insert we want to shift right the following */
		memmove(list->entries + insert_offset + 1,
		        list->entries + insert_offset,
		        to_move * sizeof(_mpc_lowcomm_multirail_endpoint_list_entry_t) );
	}
	else
	{
		/* All were higher prio insert at the end */
		insert_offset = list->elem_count;
	}

	/* At this point we have one new elem */
	list->elem_count++;
	assume(list->elem_count <= list->size);
	assume(insert_offset != -1);

	/* Now clear the new entry */
	_mpc_lowcomm_multirail_endpoint_list_entry_t *new_entry = &list->entries[insert_offset];
	_mpc_lowcomm_multirail_endpoint_list_entry_release(new_entry);
	/* And insert the new entry */
	_mpc_lowcomm_multirail_endpoint_list_entry_init(new_entry, endpoint);

	return 0;
}

static inline int __endpoint_list_pop(_mpc_lowcomm_multirail_endpoint_list_t *list,
                                      int entry_offet)
{
	assume(0 <= entry_offet);
	assume((unsigned int)entry_offet <= list->elem_count);

	/* Free the elem */
	_mpc_lowcomm_multirail_endpoint_list_entry_release(&list->entries[entry_offet]);

	/* Memmove the following */
	int to_move_cnt = list->elem_count - entry_offet - 1;
	assert(0 <= to_move_cnt);

	memmove(&list->entries[entry_offet], &list->entries[entry_offet + 1], to_move_cnt * sizeof(_mpc_lowcomm_multirail_endpoint_list_entry_t) );

	list->elem_count--;

	return 0;
}

int _mpc_lowcomm_multirail_endpoint_list_pop_endpoint(_mpc_lowcomm_multirail_endpoint_list_t *list,
                                                      _mpc_lowcomm_endpoint_t *topop)
{
	int topop_offset = -1;
	unsigned int i;

	for(i = 0; i < list->elem_count; i++)
	{
		if(list->entries[i].endpoint == topop)
		{
			topop_offset = i;
			break;
		}
	}

	if(topop_offset == -1)
	{
		mpc_common_debug_warning("Could not find this entry in table");
		return 1;
	}

	return __endpoint_list_pop(list, topop_offset);
}

void _mpc_lowcomm_multirail_endpoint_list_prune(_mpc_lowcomm_multirail_endpoint_list_t *list)
{
	unsigned int i;
	int did_free = 0;

	do
	{
		for(i = 0; i < list->elem_count; i++)
		{
			if(list->entries[i].endpoint->rail->state == SCTK_RAIL_ST_DISABLED)
			{
				__endpoint_list_pop(list, i);
				did_free = 1;
				break;
			}
		}
	} while(did_free);
}

/************************************************************************/
/* _mpc_lowcomm_multirail_table_entry                               */
/************************************************************************/

void _mpc_lowcomm_multirail_table_entry_init(_mpc_lowcomm_multirail_table_entry_t *entry, mpc_lowcomm_peer_uid_t destination)
{
	_mpc_lowcomm_multirail_endpoint_list_init(&entry->endpoints);
	mpc_common_rwlock_t lckinit = MPC_COMMON_SPIN_RWLOCK_INITIALIZER;
	entry->endpoints_lock = lckinit;
	entry->destination    = destination;
}

void _mpc_lowcomm_multirail_table_entry_release(_mpc_lowcomm_multirail_table_entry_t *entry)
{
	if(!entry)
	{
		return;
	}

	/* Make sure that we are not acquired */
	mpc_common_spinlock_write_lock(&entry->endpoints_lock);

	_mpc_lowcomm_multirail_endpoint_list_release(&entry->endpoints);
	entry->destination = -1;
}

void _mpc_lowcomm_multirail_table_entry_free(_mpc_lowcomm_multirail_table_entry_t *entry)
{
	_mpc_lowcomm_multirail_table_entry_release(entry);
	sctk_free(entry);
}

void _mpc_lowcomm_multirail_table_entry_prune(_mpc_lowcomm_multirail_table_entry_t *entry)
{
	mpc_common_spinlock_write_lock(&entry->endpoints_lock);
	_mpc_lowcomm_multirail_endpoint_list_prune(&entry->endpoints);
	mpc_common_spinlock_write_unlock(&entry->endpoints_lock);
}

_mpc_lowcomm_multirail_table_entry_t *_mpc_lowcomm_multirail_table_entry_new(mpc_lowcomm_peer_uid_t destination)
{
	_mpc_lowcomm_multirail_table_entry_t *ret = sctk_malloc(sizeof(_mpc_lowcomm_multirail_table_entry_t) );

	assume(ret != NULL);

	_mpc_lowcomm_multirail_table_entry_init(ret, destination);

	return ret;
}

void _mpc_lowcomm_multirail_table_entry_push_endpoint(_mpc_lowcomm_multirail_table_entry_t *entry, _mpc_lowcomm_endpoint_t *endpoint)
{
	mpc_common_spinlock_write_lock(&entry->endpoints_lock);
	_mpc_lowcomm_multirail_endpoint_list_push(&entry->endpoints, endpoint);
	mpc_common_spinlock_write_unlock(&entry->endpoints_lock);
}

void _mpc_lowcomm_multirail_table_entry_pop_endpoint(_mpc_lowcomm_multirail_table_entry_t *entry, _mpc_lowcomm_endpoint_t *endpoint)
{
	mpc_common_spinlock_write_lock(&entry->endpoints_lock);
	_mpc_lowcomm_multirail_endpoint_list_pop_endpoint(&entry->endpoints, endpoint);
	mpc_common_spinlock_write_unlock(&entry->endpoints_lock);
}

/************************************************************************/
/* _mpc_lowcomm_multirail_table                                     */
/************************************************************************/


static struct _mpc_lowcomm_multirail_table ____mpc_lowcomm_multirail_table;

static inline struct _mpc_lowcomm_multirail_table *_mpc_lowcomm_multirail_table_get()
{
	return &____mpc_lowcomm_multirail_table;
}

/************************************************************************/
/* Endpoint Election Logic                                              */
/************************************************************************/

/**
 * Choose the endpoint to use for sending the message.
 * This function will try to find a route to send the message. It it does not find
 * a direct link, NULL is returned.
 * \param[in] msg the message to send
 * \param[in] destination_process the remote process
 * \param[in] is_process_specific is the message targeting the process (not an MPI message) ?
 * \param[in] is_for_on_demand is a on-demand request ?
 * \param[in] ext_routes route array to iterate with (will be locked).
 * \return a pointer to the matching route, NULL otherwise (the route lock will NOT be hold in case of match)
 */
static inline _mpc_lowcomm_endpoint_t *__elect_endpoint(mpc_lowcomm_ptp_message_t *msg,
                                                        mpc_lowcomm_peer_uid_t destination_process,
                                                        int is_process_specific,
                                                        int is_for_on_demand,
                                                        _mpc_lowcomm_multirail_table_entry_t **ext_routes)
{
	assert( mpc_lowcomm_peer_get_set(destination_process) != 0);

	_mpc_lowcomm_multirail_table_entry_t *routes = _mpc_lowcomm_multirail_table_acquire_routes(destination_process);

	*ext_routes = routes;

	_mpc_lowcomm_multirail_endpoint_list_entry_t *cur = NULL;

	unsigned int current_route = 0;

	if(routes)
	{
		if(0 < routes->endpoints.elem_count)
		{
			cur = &routes->endpoints.entries[current_route];
		}
	}
	else
	{
		/* Not route to host */
		return NULL;
	}

	/* Note that in the case of process specific messages
	 * we return the first matching route which is the
	 * one of Highest priority */
	if(is_process_specific)
	{
		return (cur) ? (cur->endpoint) : NULL;
	}

	for(current_route = 0; current_route < routes->endpoints.elem_count; current_route++)
	{
		cur = &routes->endpoints.entries[current_route];

		int this_rail_has_been_elected = 1;

		/* First we want only to check on-demand rails */
		if(!cur->endpoint->rail->connect_on_demand && is_for_on_demand)
		{
			/* No on-demand in this rail */
			this_rail_has_been_elected = 0;
		}
		else
		{
			/* If it is connecting just wait for
			 * the connection completion */
			int wait_connecting;

			do
			{
				if(_mpc_lowcomm_endpoint_get_state(cur->endpoint) == _MPC_LOWCOMM_ENDPOINT_CONNECTING)
				{
					wait_connecting = 1;
					_mpc_lowcomm_multirail_notify_idle();
					sched_yield();
				}
				else
				{
					wait_connecting = 0;
				}
			} while(wait_connecting);


			/* Now that we wait for the connection only try an endpoint if it is connected */
			if(_mpc_lowcomm_endpoint_get_state(cur->endpoint) == _MPC_LOWCOMM_ENDPOINT_CONNECTED)
			{
				int cur_gate;
				/* Note that no gate is equivalent to being elected */
				for(cur_gate = 0; cur_gate < cur->gate_count; cur_gate++)
				{
					struct __gate_ctx gate_ctx;
					__gate_ctx_get(&cur->gates[cur_gate], &gate_ctx);

					if( (gate_ctx.func)(cur->endpoint->rail, msg, gate_ctx.params) == 0)
					{
						this_rail_has_been_elected = 0;
						break;
					}
				}
			}
			else
			{
				/* If the rail is not connected it
				 * cannot be elected yet */
				this_rail_has_been_elected = 0;
			}
		}

		if(this_rail_has_been_elected)
		{
			break;
		}
	}

	if(!cur)
	{
		return NULL;
	}


	return cur->endpoint;
}

#define ON_DEMMAND_CONN_LOCK_COUNT 32
static mpc_common_spinlock_t on_demand_connection_lock[ON_DEMMAND_CONN_LOCK_COUNT];

/* Per VP pending on-demand connection list */
static __thread sctk_pending_on_demand_t *__pending_on_demand = NULL;

void sctk_pending_on_demand_push(sctk_rail_info_t *rail, mpc_lowcomm_peer_uid_t dest)
{
	/* Is the rail on demand ? */
	if(!rail->connect_on_demand)
	{
		/* No then nothing to do */
		return;
	}

	sctk_pending_on_demand_t *new = sctk_malloc(sizeof(struct sctk_pending_on_demand_s) );
	assume(new != NULL);

	new->next = (struct sctk_pending_on_demand_s *)__pending_on_demand;

	new->dest = dest;
	assert( mpc_lowcomm_peer_get_set(dest) != 0);

	new->rail = rail;

	__pending_on_demand = new;
}

static inline void __pending_on_demand_release(sctk_pending_on_demand_t *pod)
{
	sctk_free(pod);
}

static inline sctk_pending_on_demand_t *__pending_on_demand_get()
{
	sctk_pending_on_demand_t *ret = __pending_on_demand;

	if(ret)
	{
		__pending_on_demand = ret->next;
	}

	return ret;
}

static inline void __pending_on_demand_process()
{
	sctk_pending_on_demand_t *pod = __pending_on_demand_get();

	while(pod)
	{
		/* Check if the endpoint exist in the rail */
		_mpc_lowcomm_endpoint_t *previous_endpoint = sctk_rail_get_any_route_to_process(pod->rail, pod->dest);

		/* No endpoint ? then its worth locking */
		if(!previous_endpoint)
		{
			mpc_common_spinlock_lock(&on_demand_connection_lock[pod->dest % ON_DEMMAND_CONN_LOCK_COUNT]);

			/* Check again to avoid RC */
			previous_endpoint = sctk_rail_get_any_route_to_process(pod->rail, pod->dest);

			/* We can create it */
			if(!previous_endpoint)
			{
				(pod->rail->connect_on_demand)(pod->rail, pod->dest);
			}

			mpc_common_spinlock_unlock(&on_demand_connection_lock[pod->dest % ON_DEMMAND_CONN_LOCK_COUNT]);
		}

		sctk_pending_on_demand_t *to_free = pod;
		/* Get next */
		pod = __pending_on_demand_get();
		/* Free previous */
		__pending_on_demand_release(to_free);
	}
}

/**
 * Called to create a new route for the sending the given message.
 * \param[in] msg the message to send.
 */
static inline void __on_demand_connection(mpc_lowcomm_ptp_message_t *msg)
{
	int count = sctk_rail_count();
	int i;

	int tab[64];

	assume(count < 64);

	memset(tab, 0, 64 * sizeof(int) );

	/* Lets now test each rail to find the one accepting this
	 * message while being able to setup on demand-connections */
	for(i = 0; i < count; i++)
	{
		sctk_rail_info_t *rail = sctk_rail_get_by_id(i);


		/* First we want only to check on-demand rails */
		if(!rail->connect_on_demand)
		{
			/* No on-demand in this rail */
			continue;
		}

		/* ###############################################################################
		 * Now we test gate functions to detect if this message is elligible on this rail
		 * ############################################################################### */

		/* Retrieve data from CTX */
		struct _mpc_lowcomm_config_struct_net_gate *gates = rail->runtime_config_rail->gates;
		int gate_count = rail->runtime_config_rail->gates_size;

		int priority = rail->runtime_config_rail->priority;

		int this_rail_has_been_elected = 1;

		/* Test all gates on this rail */
		int j;
		for(j = 0; j < gate_count; j++)
		{
			struct __gate_ctx gate_ctx;
			__gate_ctx_get(&gates[j], &gate_ctx);

			if( (gate_ctx.func)(rail, msg, gate_ctx.params) == 0)
			{
				/* This gate does not pass */
				this_rail_has_been_elected = 0;
				break;
			}
		}

		/* ############################################################################### */

		/* If we are here the rail is ellected */

		if(this_rail_has_been_elected)
		{
			/* This rail passed the test save its priority
			 * to prepare the next phase of prority selection */
			tab[i] = priority;
		}
	}

	/* Lets now connect to the rails passing the test with the highest priority */
	int current_max = 0;
	int max_offset  = -1;

	for(i = 0; i < count; i++)
	{
		if(!tab[i])
		{
			continue;
		}

		if(current_max <= tab[i])
		{
			/* This is the new BEST */
			current_max = tab[i];
			max_offset  = i;
		}
	}


	if(max_offset < 0)
	{
		/* No rail found */
		mpc_common_debug_fatal("No route to host == Make sure you have at least one on-demand rail able to satify any type of message");
	}

	mpc_lowcomm_peer_uid_t dest_process = SCTK_MSG_DEST_PROCESS_UID(msg);

	/* Enter the critical section to guanrantee the uniqueness of the
	 * newly created rail by first checking if it is not already present */
	mpc_common_spinlock_lock(&on_demand_connection_lock[dest_process % ON_DEMMAND_CONN_LOCK_COUNT]);


	/* First retry to acquire a route for on-demand
	 * in order to avoid double connections */
	_mpc_lowcomm_multirail_table_entry_t *routes = NULL;
	_mpc_lowcomm_endpoint_t *previous_endpoint   = __elect_endpoint(msg, dest_process, 0 /* Not Process Specific */, 1 /* For On-Demand */, &routes);

	/* We need to relax the routes before pushing the endpoint as the new entry
	 * will end in the same endpoint list */
	_mpc_lowcomm_multirail_table_relax_routes(routes);


	/* Check if no previous */
	if(!previous_endpoint)
	{
		/* If we are here we have elected the on-demand rail with the highest priority and matching this message
		 * initiate the on-demand connection */
		sctk_rail_info_t *elected_rail = sctk_rail_get_by_id(max_offset);
		(elected_rail->connect_on_demand)(elected_rail, dest_process);
	}

	mpc_common_spinlock_unlock(&on_demand_connection_lock[dest_process % ON_DEMMAND_CONN_LOCK_COUNT]);
}

/************************************************************************/
/* _mpc_lowcomm_multirail_hooks                                                 */
/************************************************************************/


/**
 * Compute the closest neighbor to the final destination.
 * This function is called when an on-demand route cannot be crated or a CM message
 * is sent but no direct routes exist. It assumes that at least one rail has a connected (no singleton)
 * and that the topology is at least based on a ring (be aware of potential deadlocks if not)
 *
 * The distance between the destination and the new destination is computed by substracting
 * the final destination rank by the best intermediate one. The destination with the lowest result will
 * be elected.
 *
 * \param[in] the final destination process to reach
 * \param[out] new_destination the next process to target (can be the final destination)
 */
static inline void __route_to_process(mpc_lowcomm_peer_uid_t destination, mpc_lowcomm_peer_uid_t *new_destination)
{
	struct _mpc_lowcomm_multirail_table * table = _mpc_lowcomm_multirail_table_get();
	_mpc_lowcomm_multirail_table_entry_t *entry;

	int distance = -1;

	mpc_common_spinlock_read_lock(&table->table_lock);


	MPC_HT_ITER(&table->destination_table, entry)
	{
		/* Only test connected endpoint as some networks
		 * might create the endpoint before sending
		 * the control message to the target process.
		 * In such case it would be selected (dist == 0)
		 * but would not be connected ! */


		if(entry->endpoints.elem_count)
		{
			mpc_common_nodebug("STATE %d == %d", entry->destination, _mpc_lowcomm_endpoint_get_state(entry->endpoints->endpoint) );

			if(_mpc_lowcomm_endpoint_get_state(entry->endpoints.entries[0].endpoint) == _MPC_LOWCOMM_ENDPOINT_CONNECTED)
			{
				mpc_common_nodebug(" %d --- %d", destination, entry->destination);
				if(destination == entry->destination)
				{
					distance         = 0;
					*new_destination = destination;
					break;
				}

				int cdistance = entry->destination - destination;

				if(cdistance < 0)
				{
					cdistance = -cdistance;
				}

				if( (distance < 0) || (cdistance < distance) )
				{
					distance         = cdistance;
					*new_destination = entry->destination;
				}
			}
		}
	}
	MPC_HT_ITER_END(&table->destination_table)

	mpc_common_spinlock_read_unlock(&table->table_lock);

	if(distance == -1)
	{
		mpc_common_debug_fatal("No route to host %d ", destination);
	}
}

void topology_simulation_sleep3(int src, int dst, int size) {

  hwloc_topology_t topology = mpc_topology_global_get();

  struct hwloc_distances_s * latency_matrix;
  struct hwloc_distances_s * bandwidth_matrix;

  unsigned int nr1 = 1, nr2 = 1;
  float sleep_time = 0;

  if(mpc_topology_is_latency_factors()) {
    hwloc_distances_get(topology, &nr1, &latency_matrix  , HWLOC_DISTANCES_KIND_FROM_USER | HWLOC_DISTANCES_KIND_MEANS_LATENCY  , 0);
    if(nr1) {
      unsigned long latency = latency_matrix->values[src + dst * latency_matrix->nbobjs];
      hwloc_distances_release(topology, latency_matrix  );

      sleep_time += latency;
    }
  }

  if(mpc_topology_is_bandwidth_factors()) {
    hwloc_distances_get(topology, &nr2, &bandwidth_matrix, HWLOC_DISTANCES_KIND_FROM_USER | HWLOC_DISTANCES_KIND_MEANS_BANDWIDTH, 0);
    if(nr2) {
      unsigned long bandwidth = bandwidth_matrix->values[src + dst * bandwidth_matrix->nbobjs];
      hwloc_distances_release(topology, bandwidth_matrix);

      if(bandwidth)
        sleep_time += size / bandwidth;
    }
  }

  if(sleep_time)
    usleep(sleep_time);
}

/**
 * Main entry point in low_level_comm for sending a message.
 * \param[in] msg the message to send.
 */
void _mpc_lowcomm_multirail_send_message(mpc_lowcomm_ptp_message_t *msg)
{
	assert( mpc_lowcomm_peer_get_set(SCTK_MSG_DEST_PROCESS_UID(msg)) != 0);
	int retry;
	mpc_lowcomm_peer_uid_t destination_process;

	int is_process_specific = sctk_is_process_specific_message(SCTK_MSG_HEADER(msg) );

	/* If the message is based on signalization we directly rely on routing */
	if(is_process_specific && !_mpc_lowcomm_message_is_for_universe(SCTK_MSG_HEADER(msg)))
	{
		/* Find the process to which to route to */
		__route_to_process(SCTK_MSG_DEST_PROCESS_UID(msg), &destination_process);
	}
	else
	{
		/* We want to reach the desitination of the Message */
		destination_process = SCTK_MSG_DEST_PROCESS_UID(msg);
	}

	int no_existing_route_matched = 0;

	do
	{
		retry = 0;

		if(no_existing_route_matched)
		{
			/* We have to do an on-demand connection */
			__on_demand_connection(msg);
		}

		/* We need to retrieve the route entry in order to be
		 * able to relax it after use */
		_mpc_lowcomm_multirail_table_entry_t *routes = NULL;

		_mpc_lowcomm_endpoint_t *endpoint = __elect_endpoint(msg, destination_process, is_process_specific, 0 /* Not for On-Demand */, &routes);

		if(endpoint)
		{
			sctk_rail_info_t *target_rail = endpoint->rail;

			/* Override by parent if present (topological rail) */
			if(endpoint->parent_rail != NULL)
			{
				target_rail = endpoint->parent_rail;
			}

			mpc_common_nodebug("RAIL %d", target_rail->rail_number);

			/* Prepare reordering */
			_mpc_lowcomm_reorder_msg_register(msg);

      topology_simulation_sleep3(
          mpc_lowcomm_peer_get_rank(mpc_lowcomm_monitor_get_uid()), 
          mpc_lowcomm_peer_get_rank(endpoint->dest), 
          SCTK_MSG_SIZE(msg));

			/* Send the message */
			(target_rail->send_message_endpoint)(msg, endpoint);
		}
		else
		{
			/* Here we found no route to the process or
			 * none mathed this message, we have to retry
			 * while creating a new connection */
			no_existing_route_matched = 1;
			retry = 1;
		}

		/* Relax Routes */
		_mpc_lowcomm_multirail_table_relax_routes(routes);
	} while(retry);

	/* Before Leaving Make sure that there are no pending on-demand
	 * connections (addeb by the topological rail while holding routes) */
	__pending_on_demand_process();
}

/**
 * Main entry-point for notifying rails a new RECV has been posted.
 * ALL RAILS WILL BE NOTIFIED (not only the one that may be the real recever).
 * \param[in] msg the locally-posted RECV.
 */
void _mpc_lowcomm_multirail_notify_receive(mpc_lowcomm_ptp_message_t *msg)
{
	int count = sctk_rail_count();
	int i;

	for(i = 0; i < count; i++)
	{
		sctk_rail_info_t *rail = sctk_rail_get_by_id(i);

		if(rail->notify_recv_message)
		{
			(rail->notify_recv_message)(msg, rail);
		}
	}
}

void _mpc_lowcomm_multirail_notify_matching(mpc_lowcomm_ptp_message_t *msg)
{
	int count = sctk_rail_count();
	int i;

	for(i = 0; i < count; i++)
	{
		sctk_rail_info_t *rail = sctk_rail_get_by_id(i);

		if(rail->notify_matching_message)
		{
			(rail->notify_matching_message)(msg, rail);
		}
	}
}

void _mpc_lowcomm_multirail_notify_perform(mpc_lowcomm_peer_uid_t remote, int remote_task_id, int polling_task_id, int blocking)
{
#ifdef MPC_USE_DMTCP
	if(sctk_ft_no_suspend_start() )
	{
#endif
	int count = sctk_rail_count();
	int i;

	for(i = 0; i < count; i++)
	{
		sctk_rail_info_t *rail = sctk_rail_get_by_id(i);

		if(rail->notify_perform_message)
		{
			(rail->notify_perform_message)(remote, remote_task_id, polling_task_id, blocking, rail);
		}
	}
#ifdef MPC_USE_DMTCP
	sctk_ft_no_suspend_end();
}
#endif
}

void _mpc_lowcomm_multirail_notify_probe(mpc_lowcomm_ptp_message_header_t *hdr, int *status)
{
#ifdef MPC_USE_DMTCP
	if(sctk_ft_no_suspend_start() )
	{
#endif
	int count = sctk_rail_count();
	int i;
	int ret = 0, tmp_ret = -1;
	static int no_rail_support = 0;

	if(no_rail_support)         /* shortcut when no rail support probing at all */
	{
		*status = -1;
		return;
	}

	for(i = 0; i < count; i++)
	{
		sctk_rail_info_t *rail = sctk_rail_get_by_id(i);
		tmp_ret = -1;         /* re-init for next rail */

		/* if current driver can handle a probing procedure... */
		if(rail->notify_probe_message)
		{
			/* possible values of tmp_ret:
			 * -1 -> driver-specific probing not handled (either because no function pointer set or the function returns -1)
			 * 0 -> No matching message found
			 * 1 -> At least one message found based on requirements
			 */
			(rail->notify_probe_message)(rail, hdr, &tmp_ret);
		}

		assert(tmp_ret == -1 || tmp_ret == 1 || tmp_ret == 0);

		/* three scenarios based on that :
		 * - If a rail found a matching message -> returns 1
		 * - If all rails support probing, and returns 0 -> returns 0
		 * - If at least one rail returns -1 AND probing failed -> returns -1, meaning that the previous behavior (logical header lookup through perform()) should be run.
		 */
		if(tmp_ret == 1)         /* found */
		{
			*status = 1;
			return;
		}
		ret += tmp_ret;
	}
	*status = ret;
	if(ret == -count)         /* not a single rail support probing: stop running this function */
	{
		no_rail_support = 1;
	}
#ifdef MPC_USE_DMTCP
	sctk_ft_no_suspend_end();
}
#endif
}

void _mpc_lowcomm_multirail_notify_idle()
{
	/* Fault Tolerance mechanism cannot allow any driver to be modified during a pending checkpoint.
	 * This is our way to maintain consistency for data to be saved.
	 */
#ifdef MPC_USE_DMTCP
	if(sctk_ft_no_suspend_start() )
	{
#endif
	int count = sctk_rail_count();
	int i;

	for(i = 0; i < count; i++)
	{
		sctk_rail_info_t *rail = sctk_rail_get_by_id(i);

		if(rail->state == SCTK_RAIL_ST_ENABLED && rail->notify_idle_message)
		{
			(rail->notify_idle_message)(rail);
		}
	}
#ifdef MPC_USE_DMTCP
	sctk_ft_no_suspend_end();
}
#endif
}

struct sctk_anysource_polling_ctx
{
	int               polling_task_id;
	int               blocking;
	sctk_rail_info_t *rail;
};


void notify_anysource_trampoline(void *pctx)
{
	struct sctk_anysource_polling_ctx *ctx = (struct sctk_anysource_polling_ctx *)pctx;

	(ctx->rail->notify_any_source_message)(ctx->polling_task_id, ctx->blocking, ctx->rail);
}

void _mpc_lowcomm_multirail_notify_anysource(int polling_task_id, int blocking)
{
	int count = sctk_rail_count();
	int i;

	for(i = 0; i < count; i++)
	{
		sctk_rail_info_t *rail = sctk_rail_get_by_id(i);

		if(rail->notify_any_source_message)
		{
			struct sctk_anysource_polling_ctx ctx;

			ctx.polling_task_id = polling_task_id;
			ctx.blocking        = blocking;
			ctx.rail            = rail;

			sctk_topological_polling_tree_poll(&rail->any_source_polling_tree, notify_anysource_trampoline, (void *)&ctx);
		}
	}
}

void _mpc_lowcomm_multirail_notify_new_comm(mpc_lowcomm_communicator_id_t comm_idx, size_t size)
{
	int count = sctk_rail_count();
	int i;

	for(i = 0; i < count; i++)
	{
		sctk_rail_info_t *rail = sctk_rail_get_by_id(i);

		if(rail->notify_new_comm)
		{
			rail->notify_new_comm(rail, comm_idx, size);
		}
	}
}

void _mpc_lowcomm_multirail_notify_delete_comm(mpc_lowcomm_communicator_id_t comm_idx, size_t size)
{
	int count = sctk_rail_count();
	int i;

	for(i = 0; i < count; i++)
	{
		sctk_rail_info_t *rail = sctk_rail_get_by_id(i);

		if(rail->notify_del_comm)
		{
			rail->notify_del_comm(rail, comm_idx, size);
		}
	}
}


/************************************************************************/
/* _mpc_lowcomm_multirail_table                                     */
/************************************************************************/


void _mpc_lowcomm_multirail_table_init()
{
	struct _mpc_lowcomm_multirail_table *table = _mpc_lowcomm_multirail_table_get();

	mpc_common_hashtable_init(&table->destination_table, 1024);
	mpc_common_rwlock_t lckinit = MPC_COMMON_SPIN_RWLOCK_INITIALIZER;
	table->table_lock = lckinit;

	int i;

	for(i = 0 ; i < ON_DEMMAND_CONN_LOCK_COUNT ; i++)
	{
		mpc_common_spinlock_init(&on_demand_connection_lock[i], 0);
	}
}

void _mpc_lowcomm_multirail_table_release()
{
		mpc_common_debug_error("TABLE FRE");

	struct _mpc_lowcomm_multirail_table * table = _mpc_lowcomm_multirail_table_get();
	_mpc_lowcomm_multirail_table_entry_t *entry;

	/* Make sure the table is never used again */
	mpc_common_spinlock_write_lock(&table->table_lock);

	MPC_HT_ITER(&table->destination_table, entry)
	{
		_mpc_lowcomm_multirail_table_entry_free(entry);
	}
	MPC_HT_ITER_END(&table->destination_table)

	mpc_common_hashtable_release(&table->destination_table);
}

void _mpc_lowcomm_multirail_table_prune(void)
{
	struct _mpc_lowcomm_multirail_table * table = _mpc_lowcomm_multirail_table_get();
	_mpc_lowcomm_multirail_table_entry_t *entry = NULL;

	mpc_common_spinlock_write_lock(&table->table_lock);

	MPC_HT_ITER(&table->destination_table, entry)
	{
		_mpc_lowcomm_multirail_table_entry_prune(entry);
	}
	MPC_HT_ITER_END(&table->destination_table)

	mpc_common_spinlock_write_unlock(&table->table_lock);
}

_mpc_lowcomm_multirail_table_entry_t *_mpc_lowcomm_multirail_table_acquire_routes(mpc_lowcomm_peer_uid_t destination)
{
	struct _mpc_lowcomm_multirail_table * table      = _mpc_lowcomm_multirail_table_get();
	_mpc_lowcomm_multirail_table_entry_t *dest_entry = NULL;

	mpc_common_spinlock_read_lock(&table->table_lock);

	//mpc_common_debug_warning("GET endpoint to %d", destination );

	dest_entry = mpc_common_hashtable_get(&table->destination_table, destination);

	if(dest_entry)
	{
		/* Acquire entries in read */
		mpc_common_spinlock_read_lock(&dest_entry->endpoints_lock);
	}

	mpc_common_spinlock_read_unlock(&table->table_lock);

	return dest_entry;
}


mpc_lowcomm_peer_uid_t _mpc_lowcomm_multirail_table_get_closest_peer(mpc_lowcomm_peer_uid_t dest,
													int (*distance_fn)(mpc_lowcomm_peer_uid_t dest, 
																 mpc_lowcomm_peer_uid_t candidate),
													int * minimal_distance)
{
	struct _mpc_lowcomm_multirail_table * table      = _mpc_lowcomm_multirail_table_get();

	mpc_lowcomm_peer_uid_t ret = 0;
	int cur_distance = -1;

	_mpc_lowcomm_multirail_table_entry_t *dest_entry = NULL;

	MPC_HT_ITER(&table->destination_table, dest_entry)
	{
		int distance = (distance_fn)(dest, dest_entry->destination);

		if(distance == 0)
		{
			/* Exact match */
			ret = dest_entry->destination;
			*minimal_distance = 0;
			MPC_HT_ITER_BREAK(&table->destination_table);
		}

		if( (cur_distance < 0) || (distance < cur_distance))
		{
			*minimal_distance = distance;
			ret = dest_entry->destination;
		}

	}
	MPC_HT_ITER_END(&table->destination_table)

	return ret;
}


void _mpc_lowcomm_multirail_table_relax_routes(_mpc_lowcomm_multirail_table_entry_t *entry)
{
	if(!entry)
	{
		return;
	}

	/* Just relax the read lock inside a previously acquired entry */
	mpc_common_spinlock_read_unlock(&entry->endpoints_lock);
}

void _mpc_lowcomm_multirail_table_push_endpoint(_mpc_lowcomm_endpoint_t *endpoint)
{
	struct _mpc_lowcomm_multirail_table * table      = _mpc_lowcomm_multirail_table_get();
	_mpc_lowcomm_multirail_table_entry_t *dest_entry = NULL;

	mpc_common_spinlock_write_lock(&table->table_lock);

	dest_entry = mpc_common_hashtable_get(&table->destination_table, endpoint->dest);

	if(!dest_entry)
	{
		/* There is no previous destination_table_entry */
		dest_entry = _mpc_lowcomm_multirail_table_entry_new(endpoint->dest);
		mpc_common_hashtable_set(&table->destination_table, endpoint->dest, dest_entry);
	}

	_mpc_lowcomm_multirail_table_entry_push_endpoint(dest_entry, endpoint);

	mpc_common_spinlock_write_unlock(&table->table_lock);
}

void _mpc_lowcomm_multirail_table_pop_endpoint(_mpc_lowcomm_endpoint_t *topop)
{
	struct _mpc_lowcomm_multirail_table * table      = _mpc_lowcomm_multirail_table_get();
	_mpc_lowcomm_multirail_table_entry_t *dest_entry = NULL;

	mpc_common_spinlock_write_lock(&table->table_lock);

	dest_entry = mpc_common_hashtable_get(&table->destination_table, topop->dest);

	if(dest_entry)
	{
		_mpc_lowcomm_multirail_table_entry_pop_endpoint(dest_entry, topop);
	}

	mpc_common_spinlock_write_unlock(&table->table_lock);
}
