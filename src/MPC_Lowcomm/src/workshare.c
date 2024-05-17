#include <math.h>
#include <assert.h>
#include "lowcomm_config.h"
#include <mpc_common_rank.h>

#ifdef MPC_Threads
#include "thread.h"
#else
#include "mpc_lowcomm_workshare.h"
#endif

#define MPC_WORKSHARE_SCHEDULE_STATIC 0
#define MPC_WORKSHARE_SCHEDULE_DYNAMIC 1
#define MPC_WORKSHARE_SCHEDULE_GUIDED 2
#define MPC_WORKSHARE_SCHEDULE_RUNTIME 4

static inline unsigned long long __get_chunk_and_execute_steal_from_end(mpc_workshare*, int, int);
static inline unsigned long long __get_chunk_and_execute_steal_from_start(mpc_workshare*, int, int);
static unsigned long long (*__get_chunk_and_execute)(mpc_workshare*, int, int);

void mpc_omp_init();
int MPCX_Disguise(mpc_lowcomm_communicator_t,int);
int MPCX_Undisguise();

typedef struct worker_workshare
{
  void* workshare;
  int mpi_rank;
} worker_workshare;

static int __choose_target_random(struct mpc_workshare*,int rank);
static int __choose_target_roundrobin(struct mpc_workshare*, int rank);
static int __choose_target_producer(struct mpc_workshare*, int rank);
static int __choose_target_less_stealers(struct mpc_workshare*, int rank);
static int __choose_target_less_stealers_producer(struct mpc_workshare*, int rank);
static int __choose_target_topological(struct mpc_workshare*, int rank);
static int __choose_target_strictly_topological(struct mpc_workshare*, int rank);
static int (*__choose_target)(struct mpc_workshare*, int rank);

void mpc_lowcomm_workshare_stop_stealing()
{
#ifdef MPC_Threads
  /* Forbid other processes to steal : is the entry function for #pragma ws stopsteal */
  mpc_workshare *ws_infos;
  int rank = mpc_common_get_local_task_rank();
  ws_infos = &mpc_thread_data_get()->workshare[rank];
  ws_infos->is_allowed_to_steal = 0;
#endif
}

void mpc_lowcomm_workshare_resteal()
{
  /* Authorize other processes to steal : entry function for #pragma ws resteal */
#ifdef MPC_Threads
  mpc_workshare *ws_infos;
  int rank = mpc_common_get_local_task_rank();
  ws_infos = &mpc_thread_data_get()->workshare[rank];
  ws_infos->is_allowed_to_steal = 1;
#endif

}

void mpc_lowcomm_workshare_atomic_start()
{
  /* Entry function for #pragma ws atomic */
#ifdef MPC_Threads
  mpc_workshare *ws_infos;
  int rank = mpc_common_get_local_task_rank();
  ws_infos = &mpc_thread_data_get()->workshare[rank];
  mpc_common_spinlock_lock(&ws_infos->atomic_lock);
#endif
}

void mpc_lowcomm_workshare_atomic_end()
{
#ifdef MPC_Threads
  mpc_workshare *ws_infos;
  int rank = mpc_common_get_local_task_rank();
  ws_infos = &mpc_thread_data_get()->workshare[rank];
  mpc_common_spinlock_unlock(&ws_infos->atomic_lock);
#endif
}

void mpc_lowcomm_workshare_critical_start()
{
  /* Entry function for #pragma ws critical */
#ifdef MPC_Threads
  mpc_workshare *ws_infos;
  int rank = mpc_common_get_local_task_rank();
  ws_infos = &mpc_thread_data_get()->workshare[rank];
  mpc_common_spinlock_lock(&ws_infos->critical_lock);
#endif
}

void mpc_lowcomm_workshare_critical_end()
{
#ifdef MPC_Threads
  mpc_workshare *ws_infos;
  int rank = mpc_common_get_local_task_rank();
  ws_infos = &mpc_thread_data_get()->workshare[rank];
  mpc_common_spinlock_unlock(&ws_infos->critical_lock);
#endif
}


void mpc_lowcomm_workshare_init_func_pointers()
{

  int steal_from_end = _mpc_lowcomm_conf_get()->workshare.steal_from_end;
  int steal_mode = _mpc_lowcomm_conf_get()->workshare.steal_mode;
  if(steal_from_end == 1)
    __get_chunk_and_execute = __get_chunk_and_execute_steal_from_end;
  else
    __get_chunk_and_execute = __get_chunk_and_execute_steal_from_start;

  switch(steal_mode)
  {
    case WS_STEAL_MODE_ROUNDROBIN:
      __choose_target = __choose_target_roundrobin;
      break;
    case WS_STEAL_MODE_RANDOM:
      __choose_target = __choose_target_random;
      break;
    case WS_STEAL_MODE_PRODUCER:
      __choose_target = __choose_target_producer;
      break;
    case WS_STEAL_MODE_LESS_STEALERS:
      __choose_target = __choose_target_less_stealers;
      break;
    case WS_STEAL_MODE_LESS_STEALERS_PRODUCER:
      __choose_target = __choose_target_less_stealers_producer;
      break;
    case WS_STEAL_MODE_TOPOLOGICAL:
      __choose_target = __choose_target_topological;
      break;
    case WS_STEAL_MODE_STRICTLY_TOPOLOGICAL:
      __choose_target = __choose_target_strictly_topological;
      break;
    default:
      fprintf(stderr,"Unknown workshare steal mode, defaulting to roundrobin\n");
      __choose_target = __choose_target_roundrobin;
      break;
  }
}

void mpc_lowcomm_workshare_start(void (*func) (void*,long long,long long) , void* shareds,long long lb, long long ub, long long incr, int chunk_size, int steal_chunk_size, int scheduling_types, int nowait)
{
  /* Entry function for #pragma ws for */
#ifdef MPC_Threads
  mpc_omp_init();

  if((ub >= lb && incr < 0) || (ub <= lb && incr > 0)) return;
  int rank = mpc_common_get_local_task_rank();
  mpc_workshare *ws_infos;
  ws_infos = &mpc_thread_data_get()->workshare[rank];
  ws_infos->lb = lb;
  ws_infos->ub = ub;
  ws_infos->incr = incr;
  ws_infos->chunk_size = chunk_size;
  ws_infos->steal_chunk_size = steal_chunk_size;
  /* 3 last bits for scheduling, 3 bits before for steal scheduling */
  ws_infos->scheduling_type = scheduling_types & 7;
  ws_infos->steal_scheduling_type = scheduling_types >> 3;
  ws_infos->func = func;
  ws_infos->shareds = shareds;
  ws_infos->nowait = nowait;
  if(ws_infos->scheduling_type == MPC_WORKSHARE_SCHEDULE_RUNTIME)
  {
    int schedule_type = _mpc_lowcomm_conf_get()->workshare.schedule;
    int chunksize = _mpc_lowcomm_conf_get()->workshare.chunk_size;
    ws_infos->scheduling_type = schedule_type;
    ws_infos->chunk_size = chunksize;
  }
  if(ws_infos->steal_scheduling_type == MPC_WORKSHARE_SCHEDULE_RUNTIME)
  {
    int schedule_type = _mpc_lowcomm_conf_get()->workshare.steal_schedule;
    int chunksize = _mpc_lowcomm_conf_get()->workshare.steal_chunk_size;
    ws_infos->steal_scheduling_type = schedule_type;
    ws_infos->steal_chunk_size = chunksize;
  }

  OPA_store_ptr(&(ws_infos->cur_index),(void*) lb);
  OPA_store_ptr(&(ws_infos->reverse_index), (void*)ub);
  int num_tasks = mpc_topology_get_pu_count();
  long long int current_chunk_size = (ub-lb) / (incr * num_tasks);
  OPA_store_ptr(&(ws_infos->current_chunk_size), (void*)current_chunk_size);
  OPA_store_ptr(&(ws_infos->steal_current_chunk_size), (void*)current_chunk_size);
  OPA_store_int(&(ws_infos->threshold_index), 0);
  int threshold_size = sqrt((ub - lb)/incr)*sqrt(ws_infos->chunk_size);
  if(threshold_size > (ub - lb)/incr) threshold_size = (ub - lb) / incr;
  if(threshold_size <=0)
    threshold_size = 1;

  int threshold_number = ((ub - lb)/incr) / threshold_size;
  if(threshold_number == 0) threshold_number = 1;
  ws_infos->threshold_number = threshold_number;
  OPA_store_int(&(ws_infos->rev_threshold_index), threshold_number - 1);
  ws_infos->inv_threshold_size = 1./(float)threshold_size;

  assert(OPA_load_int(&(ws_infos->nb_threads_stealing)) ==0);
  OPA_store_int(&(ws_infos->is_last_iter),0);

  while(OPA_load_int(&(ws_infos->is_last_iter)) == 0)
  {
    __get_chunk_and_execute(ws_infos,0,0);
  }

  mpc_common_spinlock_lock(&ws_infos->lock2);
  while(OPA_load_int(&(ws_infos->nb_threads_stealing)) > 0)
  {
    sctk_cpu_relax();
  }
  mpc_common_spinlock_unlock(&ws_infos->lock2);
#endif
}




static inline unsigned long long __get_chunk_and_execute_steal_from_end(mpc_workshare *ws_infos,int is_stealing, __attribute__((unused)) int is_in_collective)
{
  /* Function called when stealing from end (WS_STEAL_FROM_END=1) */
  long long int ret = 0,cur_index,new_cur_index, chunk_size,reverse_index,new_reverse_index,steal_chunk_size,not_stealing_chunk_size;
  int num_tasks;
  unsigned long long chunk_done = 0;
  int new_threshold_index = 0,not_stealing_threshold_index,anc_threshold_index,stealers_threshold_index;
  if(is_stealing)
  {
    if(ws_infos->steal_scheduling_type == MPC_WORKSHARE_SCHEDULE_GUIDED)
    {
      while(ret == 0)
      {
        long long current_chunk_size = (long long) OPA_load_ptr(&(ws_infos->current_chunk_size));
        reverse_index = (long long) OPA_load_ptr(&(ws_infos->reverse_index));
        if(reverse_index == ws_infos->lb - ws_infos->incr)
          return 0;
        cur_index = (long long) OPA_load_ptr(&(ws_infos->cur_index));

        num_tasks = mpc_topology_get_pu_count();
        if(ws_infos->scheduling_type == MPC_WORKSHARE_SCHEDULE_GUIDED)
        {
          OPA_store_ptr(&(ws_infos->current_chunk_size_anc),(void*)current_chunk_size);
          chunk_size = (ws_infos->incr > 0) ? (reverse_index - cur_index - current_chunk_size) / (ws_infos->incr * num_tasks) : cur_index - (reverse_index - current_chunk_size) / (-ws_infos->incr * num_tasks);
        }
        else
        {
          chunk_size = (ws_infos->incr > 0) ? (reverse_index - cur_index - ws_infos->chunk_size) / (ws_infos->incr * num_tasks) : (cur_index - reverse_index - ws_infos->chunk_size) / (-ws_infos->incr * num_tasks);
        }

        if(chunk_size < ws_infos->steal_chunk_size)
          chunk_size = ws_infos->steal_chunk_size;
        if(ws_infos->scheduling_type == MPC_WORKSHARE_SCHEDULE_GUIDED)
          not_stealing_chunk_size = (long long) OPA_load_ptr(&(ws_infos->current_chunk_size));
        else
          not_stealing_chunk_size = ws_infos->chunk_size;
        new_reverse_index = reverse_index - chunk_size * ws_infos->incr;
        if((reverse_index - cur_index - (chunk_size + not_stealing_chunk_size + ws_infos->chunk_size) * ws_infos->incr < 0 && ws_infos->incr > 0) || (reverse_index - cur_index - chunk_size * ws_infos->incr > 0 && ws_infos->incr < 0) )
        {
          return 0;
        }
        if((long long) OPA_load_ptr(&(ws_infos->cur_index)) !=cur_index)
          return 0;
        ret = (long long) OPA_cas_ptr(&(ws_infos->reverse_index),(void*)reverse_index,(void*)new_reverse_index);
        ret = (ret == reverse_index) ? 1 : 0;
        if(ret)
          (long long) OPA_cas_ptr(&(ws_infos->current_chunk_size),(void*)current_chunk_size,(void*)chunk_size);
      }

    }
    else
    {
      assert(ws_infos->steal_scheduling_type == MPC_WORKSHARE_SCHEDULE_DYNAMIC);
      while(ret == 0)
      {
        reverse_index = (long long) OPA_load_ptr(&(ws_infos->reverse_index));
        if(reverse_index == ws_infos->lb - ws_infos->incr)
          return 0;
        chunk_size = ws_infos->steal_chunk_size;
        new_reverse_index = reverse_index - chunk_size * ws_infos->incr;

        if(ws_infos->scheduling_type == MPC_WORKSHARE_SCHEDULE_GUIDED)
        {
          not_stealing_chunk_size = (long long) OPA_load_ptr(&(ws_infos->current_chunk_size));
        }
        else
        {
          not_stealing_chunk_size = ws_infos->chunk_size;

          new_threshold_index = (new_reverse_index - ws_infos->lb - chunk_size * ws_infos->incr) * ws_infos->inv_threshold_size;
          not_stealing_threshold_index = OPA_load_int(&(ws_infos->threshold_index));
          if(new_threshold_index - 1 > not_stealing_threshold_index)
            goto cas_ptr;
        }

        cur_index = (long long) OPA_load_ptr(&(ws_infos->cur_index));
        if((ws_infos->incr > 0 && reverse_index - cur_index - (chunk_size + not_stealing_chunk_size) * ws_infos->incr < 0 ) || (ws_infos->incr < 0 && cur_index - reverse_index - ((chunk_size + not_stealing_chunk_size) * (-ws_infos->incr)) < 0 ) )
        {
          return 0;
        }
        else
        {
cas_ptr:
          ret = (long long) OPA_cas_ptr(&(ws_infos->reverse_index),(void*)reverse_index,(void*)new_reverse_index);
          ret = (ret == reverse_index) ? 1 : 0;
          if(ret && ws_infos->scheduling_type == MPC_WORKSHARE_SCHEDULE_DYNAMIC)
          {
            int threshold_index = (reverse_index - ws_infos->lb - chunk_size* ws_infos->incr)*ws_infos->inv_threshold_size;

            if(threshold_index != new_threshold_index && threshold_index != ws_infos->threshold_number - 1)
            {
              OPA_decr_int(&(ws_infos->rev_threshold_index));
              assert(OPA_load_int(&(ws_infos->rev_threshold_index)) >=0);
            }
          }
        }
      }
    }
    ws_infos->func(ws_infos->shareds, new_reverse_index, reverse_index);
  }

  else // not stealing
  {
    if(ws_infos->steal_scheduling_type == MPC_WORKSHARE_SCHEDULE_GUIDED)
    {
      if(ws_infos->scheduling_type == MPC_WORKSHARE_SCHEDULE_GUIDED)
      {
        while(ret == 0)
        {
          num_tasks = mpc_topology_get_pu_count();
          cur_index = (long long) OPA_load_ptr(&(ws_infos->cur_index));
          reverse_index = (long long) OPA_load_ptr(&(ws_infos->reverse_index));

          long long current_chunk_size = (long long)OPA_load_ptr(&(ws_infos->current_chunk_size));
          chunk_size = (ws_infos->incr > 0) ? (reverse_index - cur_index - (num_tasks - 1) * ws_infos->steal_chunk_size) / (ws_infos->incr * num_tasks) : (cur_index - reverse_index - (num_tasks - 1) * ws_infos->steal_chunk_size) / (-ws_infos->incr * num_tasks);
          if(chunk_size < ws_infos->chunk_size)
            chunk_size = ws_infos->chunk_size;
          ret = (long long) OPA_cas_ptr(&(ws_infos->current_chunk_size),(void*) current_chunk_size,(void*) chunk_size);
          ret = (ret == current_chunk_size) ? 1 : 0;

        }

      }
      else
      {
        chunk_size = ws_infos->chunk_size;
      }

      cur_index = (long long) OPA_load_ptr(&(ws_infos->cur_index));
      reverse_index = (long long) OPA_load_ptr(&(ws_infos->reverse_index));

      new_cur_index = cur_index + chunk_size * ws_infos->incr;
      OPA_store_ptr(&(ws_infos->cur_index),(void*)new_cur_index);
      steal_chunk_size = (long long)OPA_load_ptr(&(ws_infos->current_chunk_size));
    }
    else
    {
      if(ws_infos->scheduling_type == MPC_WORKSHARE_SCHEDULE_GUIDED)
      {
        cur_index = (long long) OPA_load_ptr(&(ws_infos->cur_index));
        reverse_index = (long long) OPA_load_ptr(&(ws_infos->reverse_index));

        long long current_chunk_size = (long long) OPA_load_ptr(&(ws_infos->current_chunk_size));
        steal_chunk_size = ws_infos->steal_chunk_size;
        num_tasks = mpc_topology_get_pu_count();
        chunk_size = (ws_infos->incr > 0) ? (reverse_index - cur_index - current_chunk_size) / (ws_infos->incr * num_tasks) : (cur_index - reverse_index - current_chunk_size) / (-ws_infos->incr * num_tasks);
        if(chunk_size < ws_infos->chunk_size)
          chunk_size = ws_infos->chunk_size;
        OPA_store_ptr(&(ws_infos->current_chunk_size),(void*)chunk_size);
        new_cur_index = cur_index + chunk_size * ws_infos->incr;
        OPA_store_ptr(&(ws_infos->cur_index),(void*)new_cur_index);

      }
      else
      {
        cur_index = (long long) OPA_load_ptr(&(ws_infos->cur_index));

        chunk_size = ws_infos->chunk_size;
        new_cur_index = cur_index + chunk_size * ws_infos->incr;
        new_threshold_index = (new_cur_index - ws_infos->lb + ws_infos->steal_chunk_size* ws_infos->incr) *ws_infos->inv_threshold_size;
        anc_threshold_index = (cur_index - ws_infos->lb + ws_infos->steal_chunk_size* ws_infos->incr)*ws_infos->inv_threshold_size;
        if(anc_threshold_index != new_threshold_index)
        {
          OPA_incr_int(&(ws_infos->threshold_index));
        }
        stealers_threshold_index = OPA_load_int(&(ws_infos->rev_threshold_index));
        if(new_threshold_index < stealers_threshold_index -1)
        {
          OPA_store_ptr(&(ws_infos->cur_index),(void*)new_cur_index);
          ws_infos->func(ws_infos->shareds,cur_index,new_cur_index);
          return 0;

        }
        reverse_index = (long long) OPA_load_ptr(&(ws_infos->reverse_index));
        steal_chunk_size = ws_infos->steal_chunk_size;
        OPA_store_ptr(&(ws_infos->cur_index),(void*)new_cur_index);
      }
    }

    if((reverse_index - cur_index - (chunk_size + steal_chunk_size + ws_infos->chunk_size) * ws_infos->incr < 0 && ws_infos->incr > 0) || (cur_index - reverse_index - (chunk_size + steal_chunk_size + ws_infos->chunk_size) * -ws_infos->incr < 0 && ws_infos->incr < 0))
    {
      OPA_store_int(&(ws_infos->is_last_iter),1);
      if((long long)OPA_cas_ptr(&(ws_infos->reverse_index),(void*)reverse_index,(void*)(ws_infos->lb -ws_infos->incr)) != reverse_index)
      {
        while(OPA_load_int(&(ws_infos->nb_threads_stealing)) !=0){}
        reverse_index = (long long) OPA_load_ptr(&(ws_infos->reverse_index));
      }
      new_cur_index = reverse_index;
      OPA_store_int(&(ws_infos->is_last_iter),1);
    }

    if((cur_index < reverse_index && ws_infos->incr > 0) || (cur_index > reverse_index && ws_infos->incr < 0))
    {
      ws_infos->func(ws_infos->shareds,cur_index,new_cur_index);
    }
  }
  return chunk_done;
}

static inline unsigned long long __get_chunk_and_execute_steal_from_start(mpc_workshare *ws_infos, int is_stealing, int is_in_collective)
{
  /* Function called when stealing from start (is the default) */
  long long int ret = 0,cur_index,new_cur_index, chunk_size;
  unsigned long long chunk_done = 0;
  while(ret == 0)
  {
    cur_index = (long long) OPA_load_ptr(&(ws_infos->cur_index));
    if(is_stealing && !is_in_collective)
    {
      if(ws_infos->steal_scheduling_type == MPC_WORKSHARE_SCHEDULE_DYNAMIC) {
        chunk_size = ws_infos->steal_chunk_size;
      }
      else if(ws_infos->steal_scheduling_type == MPC_WORKSHARE_SCHEDULE_GUIDED) {
        int num_tasks = mpc_topology_get_pu_count();
        chunk_size = (ws_infos->ub - cur_index) / (ws_infos->incr * num_tasks);
        if(chunk_size < 0) chunk_size = - chunk_size;
        if(chunk_size < ws_infos->steal_chunk_size) chunk_size = ws_infos->steal_chunk_size;
      }
      else {
        fprintf(stderr,"Unknown steal scheduling type !");
        abort();
      }
    }
    else
    {
      if(ws_infos->scheduling_type == MPC_WORKSHARE_SCHEDULE_DYNAMIC) {
        chunk_size = ws_infos->chunk_size;
      }
      else if(ws_infos->scheduling_type == MPC_WORKSHARE_SCHEDULE_GUIDED) {
        int num_tasks = mpc_topology_get_pu_count();
        chunk_size = (ws_infos->ub - cur_index) / (ws_infos->incr * num_tasks);
        if(chunk_size < 0) chunk_size = - chunk_size;
        if(chunk_size < ws_infos->chunk_size) chunk_size = ws_infos->chunk_size;
      }

      else {
        fprintf(stderr,"Unknown scheduling type !");
        abort();
      }
    }
    new_cur_index = cur_index + chunk_size * ws_infos->incr;
    if((ws_infos->ub - cur_index - chunk_size * ws_infos->incr < 0 && ws_infos->incr > 0) || (ws_infos->ub - cur_index - chunk_size * ws_infos->incr > 0 && ws_infos->incr < 0))
    {
      new_cur_index = ws_infos->ub;
    }
    ret = (long long) OPA_cas_ptr(&(ws_infos->cur_index),(void*)cur_index,(void*)new_cur_index);
    ret = (ret == cur_index) ? 1 : 0;
  }
  if((new_cur_index >= ws_infos->ub && ws_infos->incr > 0) || (new_cur_index <= ws_infos->ub && ws_infos->incr < 0))
  {
    OPA_store_int(&(ws_infos->is_last_iter),1);
  }
  if((cur_index < ws_infos->ub && ws_infos->incr > 0) || (cur_index > ws_infos->ub && ws_infos->incr < 0))
  {
    ws_infos->func(ws_infos->shareds,cur_index,new_cur_index);

  }

  return chunk_done;
}


void __mpc_lowcomm_workshare_steal_wait_for_value(volatile int* data, int value,void (*func)(void *), void* arg, mpc_workshare* workshare, int rank)
{
  int cw_rank,target_cwrank;
  int is_in_collective = 0;
  mpc_workshare *ws_infos;
  if(rank != -1)
  {
    ws_infos = &workshare[rank];
    if(!ws_infos || ws_infos->is_allowed_to_steal == 0)
      return;
  }
  int target_rank = -2;
  target_rank = __choose_target(workshare,rank);
  if(target_rank != -1)
  {
    cw_rank = mpc_common_get_task_rank();
    ws_infos = &workshare[target_rank];
    target_cwrank = cw_rank + (target_rank - rank);

    target_cwrank = target_rank + (mpc_common_get_node_rank() * mpc_common_get_local_task_count());

    if(OPA_load_int(&(ws_infos->is_last_iter)) == 0)
    {
      mpc_common_spinlock_lock(&ws_infos->lock2);
      if(OPA_load_int(&(ws_infos->is_last_iter)) == 1)
      {
        mpc_common_spinlock_unlock(&ws_infos->lock2);
        return;
      }
      OPA_incr_int(&(ws_infos->nb_threads_stealing));
      mpc_common_spinlock_unlock(&ws_infos->lock2);
      is_in_collective = 1;
#ifdef MPC_MPI
      MPCX_Disguise(MPC_COMM_WORLD, target_cwrank);
#endif
      if(data && !is_in_collective)
        while(OPA_load_int(&(ws_infos->is_last_iter)) == 0 && (volatile int)*data != value)
        {
          __get_chunk_and_execute(ws_infos,1,0); // steal until I can continue my own execution
          if(func)
            func(arg);
        }
      else if(value == 1 && !data) // steal one chunk and leave
        __get_chunk_and_execute(ws_infos,1,is_in_collective);
      else
        while(OPA_load_int(&(ws_infos->is_last_iter)) == 0) // steal while there is some work
        {
          __get_chunk_and_execute(ws_infos,1, 1);//is_in_collective);
        }

      OPA_decr_int(&(ws_infos->nb_threads_stealing));
#ifdef MPC_MPI
      MPCX_Undisguise();
#endif
    }
  }
}

void mpc_lowcomm_workshare_steal()
{
#ifdef MPC_Threads
  if(mpc_thread_data_get() && mpc_thread_data_get()->workshare)
    __mpc_lowcomm_workshare_steal_wait_for_value(NULL,0,NULL,NULL, mpc_thread_data_get()->workshare, mpc_common_get_local_task_rank());
#endif
}

void mpc_lowcomm_workshare_steal_wait_for_value(__UNUSED__ volatile int* data, __UNUSED__ int value,__UNUSED__ void (*func)(void *), __UNUSED__ void* arg)
{
#ifdef MPC_Threads
  if(mpc_thread_data_get() && mpc_thread_data_get()->workshare)
    __mpc_lowcomm_workshare_steal_wait_for_value(NULL,0,NULL,NULL, mpc_thread_data_get()->workshare, mpc_common_get_local_task_rank());
#endif
}

__UNUSED__ static void __steal_one_chunk()
{
#ifdef MPC_Threads
  if(mpc_thread_data_get() && mpc_thread_data_get()->workshare)
    __mpc_lowcomm_workshare_steal_wait_for_value(NULL,1,NULL,NULL, mpc_thread_data_get()->workshare, mpc_common_get_local_task_rank());
#endif
}

void mpc_lowcomm_workshare_worker_steal(worker_workshare* worker_ws)
{
#ifdef MPC_Threads
    if(worker_ws)
      __mpc_lowcomm_workshare_steal_wait_for_value(NULL,0,NULL,NULL,(mpc_workshare*) worker_ws->workshare,worker_ws->mpi_rank);
#endif
}

static int __choose_target_roundrobin(mpc_workshare* workshare,int rank)
{
  int i,found_target = 0,target_rank = -1;
  mpc_workshare *ws_infos ;
  int num_tasks = mpc_common_get_local_task_count();
  if(num_tasks == 0) return -1;
  if(mpc_common_get_task_rank() == -1)
  {
    ws_infos = &workshare[rank];
    if(OPA_load_int(&(ws_infos->is_last_iter)) == 0)
    {

      return rank;
    }
  }
  i = (rank +1) % num_tasks;
  while(found_target == 0 && i != rank) {
    ws_infos = &workshare[i];
    if(OPA_load_int(&(ws_infos->is_last_iter)) == 0)
    {
      found_target = 1;
      target_rank = i;
    }
    i = (i+1) % num_tasks;
  }
  return target_rank;
}

static int __choose_target_topological(mpc_workshare* workshare,int rank)
{
  /* First try to steal on same numa node, then on same socket, then on node */
  int i;
  mpc_workshare *ws_infos ;

  int nb_cores_per_numa_node = ceil((float) mpc_common_get_local_task_count()/mpc_topology_get_numa_node_count());
  int numa_node_rank = rank/nb_cores_per_numa_node;
  int first_numa_rank = nb_cores_per_numa_node*numa_node_rank;
  int last_numa_rank = nb_cores_per_numa_node * (numa_node_rank + 1);
  for(i=first_numa_rank;i<last_numa_rank;i++)
  {
    ws_infos = &workshare[i];
    if(OPA_load_int(&(ws_infos->is_last_iter)) == 0)
    {
      return i;
    }
  }

  int nb_sockets = mpc_topology_get_socket_count();
  int nb_cores_per_socket = ceil((float)mpc_common_get_local_task_count()/nb_sockets);
  int socket_rank = rank/nb_cores_per_socket;
  int first_socket_rank = nb_cores_per_socket * socket_rank;
  int last_socket_rank = nb_cores_per_socket * (socket_rank + 1);

  for(i=first_socket_rank;i<last_socket_rank;i++)
  {
    if(i>=first_numa_rank && i <= last_numa_rank) continue;
    ws_infos = &workshare[i];
    if(OPA_load_int(&(ws_infos->is_last_iter)) == 0)
    {
      return i;
    }
  }

  for(i=0;i<mpc_common_get_local_task_count();i++)
  {
    if(i>=first_socket_rank && i < last_socket_rank) continue;
    ws_infos = &workshare[i];
    if(OPA_load_int(&(ws_infos->is_last_iter)) == 0)
    {
      return i;
    }
  }

  return -1;
}

static int __choose_target_strictly_topological(mpc_workshare* workshare, int rank)
{
  /* Steal only on same numa node or socket */
  int i;
  mpc_workshare *ws_infos ;

  int nb_cores_per_numa_node = ceil((float)mpc_common_get_local_task_count()/mpc_topology_get_numa_node_count());
  int numa_node_rank = rank/nb_cores_per_numa_node;
  int first_numa_rank = nb_cores_per_numa_node*numa_node_rank;
  int last_numa_rank = nb_cores_per_numa_node * (numa_node_rank + 1);
  int nb_sockets = mpc_topology_get_socket_count();
  int nb_cores_per_socket = ceil((float)mpc_common_get_local_task_count()/nb_sockets);
  int socket_rank = rank/nb_cores_per_socket;
  int first_socket_rank = nb_cores_per_socket * socket_rank;
  int last_socket_rank = nb_cores_per_socket * (socket_rank + 1);
  for(i=first_numa_rank;i<last_numa_rank;i++)
  {
    if(i>=mpc_common_get_local_task_count()) break;
    ws_infos = &workshare[i];
    if(OPA_load_int(&(ws_infos->is_last_iter)) == 0)
    {
      return i;
    }
  }

  for(i=first_socket_rank;i<last_socket_rank;i++)
  {
    if(i>=mpc_common_get_local_task_count()) break;
    if(i>=first_numa_rank && i <= last_numa_rank) continue;
    ws_infos = &workshare[i];
    if(OPA_load_int(&(ws_infos->is_last_iter)) == 0)
    {
      return i;
    }
  }
  return -1;
}

static int __choose_target_producer(mpc_workshare* workshare, int rank)
{
  /* Steal the MPI process with the most work */
  int i,target_rank = -1;
  mpc_workshare *ws_infos ;
  int num_tasks = mpc_common_get_local_task_count();
  i = rank;
  long long int max_iters=0;
  do
  {
    ws_infos = &workshare[i];
    if(ws_infos->incr == 0) continue;
    long long int cur_index = (long long int)OPA_load_ptr(&(ws_infos->cur_index));
    long long int reverse_index = (long long int)OPA_load_ptr(&(ws_infos->reverse_index));
    long long int nb_iters = ((reverse_index - cur_index) / ws_infos->incr);
    if(nb_iters > max_iters)
    {
      max_iters = nb_iters;
      target_rank = i;
    }
    i = (i+1) % num_tasks;
  }
  while(i != rank);
  return target_rank;
}

static int __choose_target_less_stealers(mpc_workshare* workshare, int rank)
{
  /* Steal the MPI process that has the less processes that are also stealing */
  int i, target_rank = -1;
  mpc_workshare *ws_infos ;
  if(rank == -1)
    rank = 0;

  int num_tasks = mpc_common_get_local_task_count();
  i = rank;
  int min=1000;
  do
  {
    ws_infos = &workshare[i];
    int threads_stealing = OPA_load_int(&(ws_infos->nb_threads_stealing));
    if(threads_stealing < min && OPA_load_int(&(ws_infos->is_last_iter)) == 0)
    {
      min = threads_stealing;
      target_rank = i;
      if(threads_stealing == 0)
        return target_rank;
    }
    i = (i+1) % num_tasks;
  }
  while(i != rank);
  return target_rank;
}

static int __choose_target_less_stealers_producer(mpc_workshare* workshare, int rank)
{
  int i, target_rank = -1;
  mpc_workshare *ws_infos ;
  long long int cur_index, reverse_index, nb_iters;
  int num_tasks = mpc_common_get_local_task_count();
  i = rank;
  int min_stealers=1000,max_iters=1;
  do
  {
    ws_infos = &workshare[i];
    int threads_stealing = OPA_load_int(&(ws_infos->nb_threads_stealing));

    if(threads_stealing <= min_stealers && OPA_load_int(&(ws_infos->is_last_iter)) == 0)
    {
      cur_index = (long long int)OPA_load_ptr(&(ws_infos->cur_index));
      reverse_index = (long long int)OPA_load_ptr(&(ws_infos->reverse_index));
      nb_iters = ((reverse_index - cur_index) / ws_infos->incr);
      if(nb_iters >= max_iters || threads_stealing < min_stealers)
      {
        min_stealers = threads_stealing;
        max_iters = nb_iters;
        target_rank = i;
      }
    }
    i = (i+1) % num_tasks;
  }
  while(i != rank);
  return target_rank;
}

static int __choose_target_random(mpc_workshare* workshare,int rank)
{
  int i,num_tries = 0,found_target = 0,target_rank = -1;
  mpc_workshare *ws_infos ;
  int num_tasks = mpc_common_get_local_task_count();
  while(found_target == 0 && num_tries < num_tasks) {
    i = rand() % num_tasks;
    num_tries ++;
    if(i != rank) {
      ws_infos = &workshare[i];
      if(OPA_load_int(&(ws_infos->is_last_iter)) == 0)
      {
        found_target = 1;
        target_rank = i;
      }
    }
  }
  return target_rank;
}
