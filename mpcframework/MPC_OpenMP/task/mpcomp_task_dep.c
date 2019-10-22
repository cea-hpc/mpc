#include <mpc_common_types.h>
#include <mpc_common_types.h>
#include <mpc_common_asm.h>
#include "mpcomp_types.h"
#include "sctk_runtime_config_struct.h"

#include "mpcomp_core.h"
#include "mpcomp_task.h"
#include "mpcomp_task_tree.h"
#include "mpcomp_task_macros.h"
#include "mpcomp_task_list.h"
#include "mpcomp_task_utils.h"
#include "mpcomp_tree_utils.h"
#include "mpcomp_task_stealing.h"

#include "mpcomp_task_dep.h"

#include "mpc_common_asm.h"

#include "sctk_debug.h"
#include "mpcomp_task_utils.h"

#include "ompt.h"
extern ompt_callback_t* OMPT_Callbacks;

#ifdef MPCOMP_USE_TASKDEP

static int __mpcomp_task_process_deps(mpcomp_task_dep_node_t *task_node,
                                      mpcomp_task_dep_ht_table_t *htable,
                                      void **depend) {
  sctk_assert(task_node);
  sctk_assert(htable);
  sctk_assert(depend);

  size_t i, j;
  int predecessors_num;
  mpcomp_task_dep_ht_entry_t *entry;
  mpcomp_task_dep_node_t *last_out;

  const size_t tot_deps_num = (uintptr_t)depend[0];
  const size_t out_deps_num = (uintptr_t)depend[1];

  if (!tot_deps_num)
    return 0;

  // Filter redundant value
#if OMPT_SUPPORT
  uintptr_t task_already_process_num = 0;

  ompt_dependence_t* task_deps =
    (ompt_dependence_t*) sctk_malloc( sizeof( ompt_dependence_t) * tot_deps_num );
  sctk_assert( task_deps );
  memset( task_deps, 0, sizeof( ompt_dependence_t) * tot_deps_num );
#else
  size_t task_already_process_num = 0;

  uintptr_t *task_already_process_list =
     (uintptr_t*) sctk_malloc(sizeof(uintptr_t) * tot_deps_num);
  sctk_assert(task_already_process_list);
#endif /* OMPT_SUPPORT */

  predecessors_num = 0;

  for (i = 0; i < tot_deps_num; i++) {
    int redundant = 0;

    /* FIND HASH IN HTABLE */
    const uintptr_t addr = (uintptr_t)depend[2 + i];
    const int type =
        (i < out_deps_num) ? MPCOMP_TASK_DEP_OUT : MPCOMP_TASK_DEP_IN;
    sctk_assert(task_already_process_num < tot_deps_num);

#if OMPT_SUPPORT
    ompt_dependence_type_t ompt_type = (i < out_deps_num) ?
      ompt_dependence_type_out : ompt_dependence_type_in;
#endif /* OMPT_SUPPORT */

    for (j = 0; j < task_already_process_num; j++) {
#if OMPT_SUPPORT
      if( (uintptr_t)task_deps[j].variable.ptr == addr ) {
        if( task_deps[j].dependence_type != ompt_dependence_type_inout
              && ompt_type != task_deps[j].dependence_type )
          task_deps[j].dependence_type = ompt_dependence_type_inout;
        redundant = 1;
        break;
      }
#else
      if (task_already_process_list[j] == addr) {
        redundant = 1;
        break;
      }
#endif /* OMPT_SUPPORT */
    }

    sctk_nodebug("task: %p deps: %p redundant : %d \n", task_node, addr,
                 redundant);
    /** OUT are in first position en OUT > IN deps */
    if (redundant)
      continue;

#if OMPT_SUPPORT
    task_deps[task_already_process_num].variable.ptr = (void*) addr;
    task_deps[task_already_process_num].dependence_type = ompt_type;
    task_already_process_num++;
#else
    task_already_process_list[task_already_process_num++] = addr;
#endif /* OMPT_SUPPORT */

    entry = mpcomp_task_dep_add_entry(htable, addr);
    sctk_assert(entry);

    last_out = entry->last_out;

    // NEW [out] dep must be after all [in] deps
    if (type == MPCOMP_TASK_DEP_OUT && entry->last_in != NULL) {
      mpcomp_task_dep_node_list_t *node_list;
      for (node_list = entry->last_in; node_list; node_list = node_list->next) {
        mpcomp_task_dep_node_t *node = node_list->node;
        if (OPA_load_int(&(node->status)) <
            MPCOMP_TASK_DEP_TASK_FINALIZED)
        // if( node->task )
        {
          MPCOMP_TASK_DEP_LOCK_NODE(node);
          if (OPA_load_int(&(node->status)) <
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
      if (last_out && (OPA_load_int(&(last_out->status)) <
                       MPCOMP_TASK_DEP_TASK_FINALIZED))
      // if( last_out && last_out->task )
      {
        MPCOMP_TASK_DEP_LOCK_NODE(last_out);
        if (OPA_load_int(&(last_out->status)) <
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

#if OMPT_SUPPORT
  task_node->ompt_task_deps = task_deps;
  depend[0] = (void*) task_already_process_num;
#else
  sctk_free(task_already_process_list);
#endif /* OMPT_SUPPORT */

  return predecessors_num;
}

void __mpcomp_task_finalize_deps(mpcomp_task_t *task) {
  mpcomp_task_dep_node_t *task_node, *succ_node;
  mpcomp_task_dep_node_list_t *list_elt;

  sctk_assert(task);

  if(!(task->task_dep_infos))
    return;


  // if (!(task->task_dep_infos->htable)) {
    /* remove all elements from task dep hash table */
    //(void) mpcomp_task_dep_free_task_htable( task->task_dep_infos->htable );
    // task->task_dep_infos->htable = NULL;
  // }

  task_node = task->task_dep_infos->node;

  /* No dependers */
  if (!(task_node)) {
    return;
  }

  /* Release Task Deps */
  MPCOMP_TASK_DEP_LOCK_NODE(task_node);
  OPA_store_int( &( task_node->status ), MPCOMP_TASK_DEP_TASK_FINALIZED );
  MPCOMP_TASK_DEP_UNLOCK_NODE(task_node);

  /* Unref my successors */
  while ((list_elt = task_node->successors)) {
    succ_node = list_elt->node;
    const int prev =
        OPA_fetch_and_decr_int(&(succ_node->predecessors)) - 1;

    if( !prev && succ_node->if_clause)
    {
      if( OPA_load_int( &( succ_node->status ) ) != MPCOMP_TASK_DEP_TASK_FINALIZED )
       if( OPA_cas_int( &( succ_node->status ), MPCOMP_TASK_DEP_TASK_NOT_EXECUTE, MPCOMP_TASK_DEP_TASK_RELEASED )
           == MPCOMP_TASK_DEP_TASK_NOT_EXECUTE )
         __mpcomp_task_process(succ_node->task, 1);
    }

    task_node->successors = list_elt->next;
    mpcomp_task_dep_node_unref(succ_node);
    sctk_free(list_elt);
  }

#if OMPT_SUPPORT
  if( task_node->ompt_task_deps ) {
    sctk_free( task_node->ompt_task_deps );
    task_node->ompt_task_deps = NULL;
  }
#endif /* OMPT_SUPPORT */

  mpcomp_task_dep_node_unref(task_node);
  return;
}

void mpcomp_task_with_deps(void (*fn)(void *), void *data,
                           void (*cpyfn)(void *, void *), long arg_size,
                           long arg_align, bool if_clause, unsigned flags,
                           void **depend, bool intel_alloc, mpcomp_task_t *intel_task) {
  int predecessors_num;
  mpcomp_thread_t *thread;
  mpcomp_task_dep_node_t *task_node;
  mpcomp_task_t *current_task, *new_task;
 
  __mpcomp_init();

  thread = (mpcomp_thread_t *)sctk_openmp_thread_tls;

  if ( thread->info.num_threads == 1 ||
              !(mpcomp_task_dep_is_flag_with_deps(flags))) {
      if(!intel_alloc){
          __mpcomp_task(fn, data, cpyfn, arg_size, arg_align, if_clause, flags);
      }
      else{
          __mpcomp_task_process(intel_task, if_clause);
      }
      return;
  }

  current_task = (mpcomp_task_t *)MPCOMP_TASK_THREAD_GET_CURRENT_TASK(thread);
  /* Is it possible ?! See GOMP source code	*/
  sctk_assert((uintptr_t)depend[0] > (uintptr_t)0);

  if (!(current_task->task_dep_infos->htable)) {
    current_task->task_dep_infos->htable =
        mpcomp_task_dep_alloc_task_htable(mpcomp_task_dep_mpc_hash_func);
    sctk_assert(current_task->task_dep_infos->htable);
  }

  task_node = mpcomp_task_dep_new_node();
  sctk_assert(task_node);

  /* TODO: pass real number of dep instead of 0 ? (for OMPT) */
  if(!intel_alloc){
      new_task = __mpcomp_task_alloc(fn, data, cpyfn, arg_size, arg_align,
              if_clause, flags, 1);
  }
  else{
      new_task = intel_task;
  }

  sctk_assert(new_task);

  new_task->task_dep_infos = sctk_malloc(sizeof(mpcomp_task_dep_task_infos_t));
  sctk_assert(new_task->task_dep_infos);
  memset(new_task->task_dep_infos, 0, sizeof(mpcomp_task_dep_task_infos_t));
  /* TODO remove redundant assignement (see mpcomp_task_dep_new_node) */
  task_node->task = NULL;

  /* Can't be execute by release func */
  OPA_store_int(&(task_node->predecessors), 0);
  OPA_store_int(&(task_node->status),
                         MPCOMP_TASK_DEP_TASK_PROCESS_DEP);

  predecessors_num = __mpcomp_task_process_deps(
      task_node, current_task->task_dep_infos->htable, depend);

  task_node->if_clause = if_clause;
  task_node->task = new_task;
  new_task->task_dep_infos->node = task_node;
  /* Should be remove TOTEST */
  OPA_read_write_barrier();

#if OMPT_SUPPORT
  if( mpcomp_ompt_is_enabled() && OMPT_Callbacks ) {
    ompt_callback_dependences_t callback;
    callback = (ompt_callback_dependences_t) OMPT_Callbacks[ompt_callback_dependences];

    if( callback ) {
      int tot_deps_num = (int) depend[0];

      callback( &( new_task->ompt_task_data ), new_task->task_dep_infos->node->ompt_task_deps, tot_deps_num );
    }
  }
#endif

  /* task_node->predecessors can be update by release task */
  OPA_add_int(&(task_node->predecessors), predecessors_num);
  OPA_store_int(&(task_node->status),
                         MPCOMP_TASK_DEP_TASK_NOT_EXECUTE);

  if (!if_clause) {
        while (OPA_load_int(&(task_node->predecessors))) {
            mpcomp_task_schedule(0); /* schedule thread doing if0 with dependances until deps resolution 
                                        mpcomp_task_schedule(0) because refcount will remain >= 2 with if0 
                                        mpcomp_task_schedule(1) would not stop looping */
        }
        if(intel_alloc){
            /* Because with intel compiler task code is between begin_if0 and 
             * complet_if0 call, so we don't call it in __mpcomp_task_process */
            return;
        }
  }

  if (OPA_load_int(&(task_node->predecessors)) == 0) {
      if( OPA_load_int( &( task_node->status ) ) != MPCOMP_TASK_DEP_TASK_FINALIZED )
       if( OPA_cas_int( &( task_node->status ), MPCOMP_TASK_DEP_TASK_NOT_EXECUTE, MPCOMP_TASK_DEP_TASK_RELEASED )
           == MPCOMP_TASK_DEP_TASK_NOT_EXECUTE )
         __mpcomp_task_process(new_task, if_clause);
  }
}

#endif /* MPCOMP_USE_TASKDEP */
