#include <sctk_bool.h>
#include <sctk_int.h>
#include <sctk_asm.h>
#include "mpcomp_types.h"
#include "sctk_runtime_config_struct.h"

#include "mpcomp_task.h"
#include "mpcomp_task_tree.h"
#include "mpcomp_task_macros.h"
#include "mpcomp_task_list.h"
#include "mpcomp_task_utils.h"
#include "mpcomp_tree_utils.h"
#include "mpcomp_task_stealing.h"

#include "mpcomp_task_dep.h"
#include "MurmurHash_64.h"
#include "sctk_atomics.h"

#include "sctk_debug.h"
#include "mpcomp_task_utils.h"

#include "ompt.h"
extern ompt_callback_t* OMPT_Callbacks;

#ifdef MPCOMP_USE_TASKDEP

static int __mpcomp_task_wait_deps(mpcomp_task_dep_node_t *task_node) {
  while (sctk_atomics_load_int(&(task_node->predecessors))) {
    mpcomp_task_schedule();
  }
}

static int __mpcomp_task_process_deps(mpcomp_task_dep_node_t *task_node,
                                      mpcomp_task_dep_ht_table_t *htable,
                                      void **depend) {

  int i, j, predecessors_num;
  mpcomp_task_dep_ht_entry_t *entry;
  mpcomp_task_dep_node_t *last_out;

  const size_t tot_deps_num = (uintptr_t)depend[0];
  const size_t out_deps_num = (uintptr_t)depend[1];

  if (!tot_deps_num)
    return 0;

  // Filter redundant value
  int task_already_process_num = 0;
  uintptr_t *task_already_process_list =
      sctk_malloc(sizeof(uintptr_t) * tot_deps_num);
  sctk_assert(task_already_process_list);

  predecessors_num = 0;

  for (i = 0; i < tot_deps_num; i++) {
    int redundant = 0;

    /* FIND HASH IN HTABLE */
    const uintptr_t addr = (uintptr_t)depend[2 + i];
    const int type =
        (i < out_deps_num) ? MPCOMP_TASK_DEP_OUT : MPCOMP_TASK_DEP_IN;
    sctk_assert(task_already_process_num < tot_deps_num);

    for (j = 0; j < task_already_process_num; j++) {
      if (task_already_process_list[j] == addr) {
        redundant = 1;
        break;
      }
    }
		
    sctk_nodebug("task: %p deps: %p redundant : %d \n", task_node, addr,
                 redundant);
    /** OUT are in first position en OUT > IN deps */
    if (redundant)
      continue;

    task_already_process_list[task_already_process_num++] = addr;

    entry = mpcomp_task_dep_add_entry(htable, addr);
    sctk_assert(entry);

    last_out = entry->last_out;

    // NEW [out] dep must be after all [in] deps
    if (type == MPCOMP_TASK_DEP_OUT && entry->last_in != NULL) {
      mpcomp_task_dep_node_list_t *node_list;
      for (node_list = entry->last_in; node_list; node_list = node_list->next) {
        mpcomp_task_dep_node_t *node = node_list->node;
        if (sctk_atomics_load_int(&(node->status)) <
            MPCOMP_TASK_DEP_TASK_FINALIZED)
        // if( node->task )
        {
          MPCOMP_TASK_DEP_LOCK_NODE(node);
          if (sctk_atomics_load_int(&(node->status)) <
              MPCOMP_TASK_DEP_TASK_FINALIZED)
          // if( node->task )
          {
            node->successors = mpcomp_task_dep_alloc_node_list_elt(
                node->successors, task_node);
            predecessors_num++;
            sctk_nodebug("IN predecessors");
          }
          MPCOMP_TASK_DEP_UNLOCK_NODE(node);
        }
      }
      mpcomp_task_dep_free_node_list_elt(entry->last_in);
      entry->last_in = NULL;
    } else {
      /** Non executed OUT dependency**/
      if (last_out && (sctk_atomics_load_int(&(last_out->status)) <
                       MPCOMP_TASK_DEP_TASK_FINALIZED))
      // if( last_out && last_out->task )
      {
        MPCOMP_TASK_DEP_LOCK_NODE(last_out);
        if (sctk_atomics_load_int(&(last_out->status)) <
            MPCOMP_TASK_DEP_TASK_FINALIZED)
        // if( last_out->task )
        {
          last_out->successors = mpcomp_task_dep_alloc_node_list_elt(
              last_out->successors, task_node);
          predecessors_num++;
          sctk_nodebug("OUT predecessors");
        }
        MPCOMP_TASK_DEP_UNLOCK_NODE(last_out);
      }
    }

    if (type == MPCOMP_TASK_DEP_OUT) {
      mpcomp_task_dep_node_unref(last_out);
      entry->last_out = mpcomp_task_dep_node_ref(task_node);
      sctk_nodebug("last_out : %p -- %p", last_out, addr);
    } else {
      sctk_assert(type == MPCOMP_TASK_DEP_IN);
      entry->last_in =
          mpcomp_task_dep_alloc_node_list_elt(entry->last_in, task_node);
    }
  }

  sctk_free(task_already_process_list);
  return predecessors_num;
}

void __mpcomp_task_finalize_deps(mpcomp_task_t *task) {
  mpcomp_task_dep_node_t *task_node, *succ_node;
  mpcomp_task_dep_node_list_t *list_elt;

  sctk_assert(task);

  if (!(task->task_dep_infos->htable)) {
    /* remove all elements from task dep hash table */
    //(void) mpcomp_task_dep_free_task_htable( task->task_dep_infos->htable );
    // task->task_dep_infos->htable = NULL;
  }

  task_node = task->task_dep_infos->node;

  /* No dependers */
  if (!(task_node)) {
    return;
  }

  /* Release Task Deps */
  MPCOMP_TASK_DEP_LOCK_NODE(task_node);
  sctk_atomics_store_int(&(task_node->status), MPCOMP_TASK_DEP_TASK_FINALIZED);
  // task_node->task = NULL;
  MPCOMP_TASK_DEP_UNLOCK_NODE(task_node);

  /* Unref my successors */
  while ((list_elt = task_node->successors)) {
    succ_node = list_elt->node;
    const int prev =
        sctk_atomics_fetch_and_decr_int(&(succ_node->predecessors)) - 1;
    sctk_nodebug("release sucessors ...");
    task_node->successors = list_elt->next;

    if (!prev &&
        sctk_atomics_cas_int(&(succ_node->status),
                             MPCOMP_TASK_DEP_TASK_NOT_EXECUTE,
                             MPCOMP_TASK_DEP_TASK_RELEASED) ==
            MPCOMP_TASK_DEP_TASK_NOT_EXECUTE) {
      // TODO MANAGE if_clause
      sctk_atomics_read_write_barrier();
      if( succ_node->task )
         __mpcomp_task_process(succ_node->task, 0);
    }

    mpcomp_task_dep_node_unref(succ_node);
    sctk_free(list_elt);
  }

  mpcomp_task_dep_node_unref(task_node);
  return;
}

void mpcomp_task_with_deps(void (*fn)(void *), void *data,
                           void (*cpyfn)(void *, void *), long arg_size,
                           long arg_align, bool if_clause, unsigned flags,
                           void **depend) {
  int predecessors_num;
  mpcomp_thread_t *thread;
  mpcomp_task_dep_node_t *task_node;
  mpcomp_task_t *current_task, *new_task;

  __mpcomp_init();

  thread = (mpcomp_thread_t *)sctk_openmp_thread_tls;
  current_task = (mpcomp_task_t *)MPCOMP_TASK_THREAD_GET_CURRENT_TASK(thread);

  if (!(mpcomp_task_dep_is_flag_with_deps(flags))) {
    __mpcomp_task(fn, data, cpyfn, arg_size, arg_align, if_clause, flags);
    return;
  }

  /* Is it possible ?! See GOMP source code	*/
  sctk_assert((uintptr_t)depend[0] > (uintptr_t)0);

  if (!(current_task->task_dep_infos->htable)) {
    current_task->task_dep_infos->htable =
        mpcomp_task_dep_alloc_task_htable(&mpcomp_task_dep_mpc_hash_func);
    sctk_assert(current_task->task_dep_infos->htable);
  }

  task_node = mpcomp_task_dep_new_node();
  sctk_assert(task_node);

  new_task = __mpcomp_task_alloc(fn, data, cpyfn, arg_size, arg_align,
                                 if_clause, flags, 0);
  sctk_assert(new_task);

  task_node->task = NULL;

#if 1 //OMPT_SUPPORT
	if( mpcomp_ompt_is_enabled() )
	{
   	if( OMPT_Callbacks )
   	{
			ompt_callback_task_dependences_t callback; 
			callback = (ompt_callback_task_dependences_t) OMPT_Callbacks[ompt_callback_task_dependences];
			if( callback )
			{
				uintptr_t i, tot_deps_num, out_deps_num;
				ompt_task_dependence_t* ompt_task_deps;

  				tot_deps_num = (uintptr_t)depend[0];
  				out_deps_num = (uintptr_t)depend[1];
				ompt_task_deps = (ompt_task_dependence_t*) sctk_malloc(sizeof( ompt_task_dependence_t) * tot_deps_num );
				sctk_assert( ompt_task_deps );

				for( i = 0; i < tot_deps_num; i++ )
			 	{	
					ompt_task_dependence_flag_t dep_flag = 0;
					dep_flag = ( i < out_deps_num) ? ompt_task_dependence_type_out : ompt_task_dependence_type_in;
					ompt_task_deps[i].variable_addr = (void*) depend[i+2]; 
					ompt_task_deps[i].dependence_flags = dep_flag; 
				} 

				new_task->ompt_task_data = ompt_data_none;
				callback( &( new_task->ompt_task_data ), ompt_task_deps, tot_deps_num );
			}
		}
	}
#endif /* OMPT_SUPPORT */
	
  /* Can't be execute by release func */
  sctk_atomics_store_int(&(task_node->predecessors), 0);
  sctk_atomics_store_int(&(task_node->status),
                         MPCOMP_TASK_DEP_TASK_PROCESS_DEP);

  predecessors_num = __mpcomp_task_process_deps(
      task_node, current_task->task_dep_infos->htable, depend);

  task_node->task = new_task;
  new_task->task_dep_infos->node = task_node;
  sctk_atomics_read_write_barrier();

  /* task_node->predecessors can be update by release task */
  sctk_atomics_add_int(&(task_node->predecessors), predecessors_num);
  sctk_atomics_store_int(&(task_node->status),
                         MPCOMP_TASK_DEP_TASK_NOT_EXECUTE);

  if (if_clause) {
    sctk_atomics_store_int(&(task_node->status),
                           MPCOMP_TASK_DEP_TASK_FINALIZED);
    __mpcomp_task_wait_deps(task_node);
  }

  if (sctk_atomics_load_int(&(task_node->predecessors)) == 0) {
    sctk_nodebug("%s: Direct run ...", __func__);
    __mpcomp_task_process(new_task, if_clause);
  }
}

#endif /* MPCOMP_USE_TASKDEP */
