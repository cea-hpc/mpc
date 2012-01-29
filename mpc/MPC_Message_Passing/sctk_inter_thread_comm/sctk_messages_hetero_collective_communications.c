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
/* #                                                                      # */
/* ######################################################################## */

#include <sctk_collective_communications.h>
#include <sctk_inter_thread_comm.h>
#include <sctk_communicator.h>
#include <sctk.h>
#include <sctk_spinlock.h>
#include <sctk_atomics.h>
#include <sctk_thread.h>
#include <string.h>
#include <math.h>

/************************************************************************/
/*PARAMETERS                                                            */
/************************************************************************/
static int BARRIER_ARRITY  = 8;
static int BROADCAST_ARITY_MAX = 32;
static int BROADCAST_MAX_SIZE = 1024;
static int BROADCAST_CHECK_THREASHOLD = 512;
static int ALLREDUCE_ARITY_MAX = 8;
static int ALLREDUCE_MAX_SIZE = 1024;
static int ALLREDUCE_MAX_NB_ELEM_SIZE = 1024;
static int ALLREDUCE_CHECK_THREASHOLD = 8192;
#define SCTK_MAX_ASYNC 32

/************************************************************************/
/*TOOLS                                                                 */
/************************************************************************/
static void sctk_free_hetero_messages(void* ptr){

}


typedef struct {
  sctk_request_t request;
  sctk_thread_ptp_message_t msg;
}sctk_hetero_messages_t;

typedef struct {
  sctk_hetero_messages_t msg_req[SCTK_MAX_ASYNC];
  int nb_used;
}sctk_hetero_messages_table_t;


/************************************************************************/
/*FUNCTONS                                                              */
/************************************************************************/

static void sctk_hetero_messages_send(const sctk_communicator_t communicator,int myself,int dest,int tag, void* buffer,size_t size,
				   specific_message_tag_t specific_message_tag, sctk_hetero_messages_t* msg_req, int check,int copy_in_send){

  sctk_init_header(&(msg_req->msg),myself,sctk_message_contiguous,sctk_free_hetero_messages,
		   sctk_message_copy);
  sctk_add_adress_in_message(&(msg_req->msg),buffer,size);
  sctk_set_header_in_message (&(msg_req->msg), tag, communicator, myself, dest,
			      &(msg_req->request), size,specific_message_tag);
  msg_req->msg.tail.need_check_in_wait = /* copy_in_send */1;
  sctk_send_message_try_check (&(msg_req->msg),check);
}

static void sctk_hetero_messages_recv(const sctk_communicator_t communicator,int src, int myself,int tag, void* buffer,size_t size,
				   specific_message_tag_t specific_message_tag, sctk_hetero_messages_t* msg_req,struct sctk_internal_ptp_s* ptp_internal, int check,
				   int copy_in_recv){

  sctk_init_header(&(msg_req->msg),myself,sctk_message_contiguous,sctk_free_hetero_messages,
		   sctk_message_copy);
  sctk_add_adress_in_message(&(msg_req->msg),buffer,size);
  sctk_set_header_in_message (&(msg_req->msg), tag, communicator,  src,myself,
			      &(msg_req->request), size,specific_message_tag);
  msg_req->msg.tail.need_check_in_wait = /* copy_in_recv */1;
  sctk_recv_message_try_check (&(msg_req->msg),ptp_internal,check);
}

static void sctk_hetero_messages_wait(sctk_hetero_messages_table_t* tab){
  int i;
  for(i = 0; i < tab->nb_used; i++){
    sctk_wait_message (&(tab->msg_req[i].request));
  }
  tab->nb_used = 0;
}



static sctk_hetero_messages_t* sctk_hetero_messages_get_item(sctk_hetero_messages_table_t* tab){
  sctk_hetero_messages_t* tmp;
  if(tab->nb_used == SCTK_MAX_ASYNC){
    sctk_hetero_messages_wait(tab);
  }

  tmp = &(tab->msg_req[tab->nb_used]);
  tab->nb_used++;
  return tmp;
}

static void sctk_hetero_messages_init_items(sctk_hetero_messages_table_t* tab){
    tab->nb_used = 0;
}

/************************************************************************/
/*BARRIER                                                               */
/************************************************************************/
static int int_cmp(const void *a, const void *b) {
  const int *ia = (const int *)a;
  const int *ib = (const int *)b;
  return *ia  - *ib;
}

static
void sctk_barrier_hetero_messages_inter(const sctk_communicator_t communicator,
    sctk_internal_collectives_struct_t * tmp){
  sctk_thread_data_t *thread_data;
  int myself;
  int my_rank;
  int *process_array;
  int total = sctk_get_process_nb_in_array(communicator);
  int total_max;
  int i;
  sctk_hetero_messages_table_t table;
  char c = 'c';
  struct sctk_internal_ptp_s* ptp_internal;
  int specific_tag = barrier_hetero_specific_message_tage;

  /* If only one process involved, we return */
  if (total == 1) return;

  sctk_nodebug("Start inter");
  sctk_hetero_messages_init_items(&table);

  thread_data = sctk_thread_data_get ();
  my_rank = sctk_get_rank (communicator, thread_data->task_id);
  process_array = sctk_get_process_array(communicator),
  myself = *((int*) bsearch((void*) &sctk_process_rank,
        process_array,
        total,sizeof(int), int_cmp));

  ptp_internal = sctk_get_internal_ptp(-1);

  total_max = log(total) / log(BARRIER_ARRITY);
  total_max = pow(BARRIER_ARRITY,total_max);
  if(total_max < total){
    total_max = total_max * BARRIER_ARRITY;
  }
  assume(total_max >= total);

  for(i = BARRIER_ARRITY; i <= total_max; i = i*BARRIER_ARRITY){
    if(myself % i == 0){
      int src;
      int j;

      src = myself;
      for(j = 1; j < BARRIER_ARRITY; j++){
        if((src + (j*(i/BARRIER_ARRITY))) < total){
          sctk_nodebug("Recv %d to %d", src + (j*(i/BARRIER_ARRITY)), myself);
          sctk_hetero_messages_recv(communicator,
              process_array[src + (j*(i/BARRIER_ARRITY))],
              process_array[myself],
              0,&c,1,barrier_hetero_specific_message_tage,sctk_hetero_messages_get_item(&table),ptp_internal,1,1);
        }
      }
      sctk_hetero_messages_wait(&table);
    } else {
      int dest;

      dest = (myself / i) * i;
      if(dest >= 0){
        sctk_nodebug("send %d to %d", myself, dest);
        sctk_hetero_messages_send(communicator,
            process_array[myself],
            process_array[dest],
            0,&c,1,barrier_hetero_specific_message_tage,sctk_hetero_messages_get_item(&table),0,1);
        sctk_nodebug("recv %d to %d", dest, myself);
        sctk_hetero_messages_recv(communicator,
            process_array[dest],
            process_array[myself],
            1,&c,1,barrier_hetero_specific_message_tage,sctk_hetero_messages_get_item(&table),ptp_internal,0,1);
        sctk_hetero_messages_wait(&table);
        break;
      }
    }
  }
  sctk_hetero_messages_wait(&table);

  for(; i >=BARRIER_ARRITY ; i = i / BARRIER_ARRITY){
    if(myself % i == 0){
      int dest;
      int j;

      dest = myself;
      for(j = 1; j < BARRIER_ARRITY; j++){
        if((dest + (j*(i/BARRIER_ARRITY))) < total){
          sctk_hetero_messages_send(communicator,
              process_array[myself],
              process_array[dest+(j*(i/BARRIER_ARRITY))],
              1,&c,1,barrier_hetero_specific_message_tage,sctk_hetero_messages_get_item(&table),1,1);
        }
      }
    }
  }
  sctk_hetero_messages_wait(&table);
  sctk_nodebug("End inter");
}

static
void sctk_barrier_hetero_messages(const sctk_communicator_t communicator,
			   sctk_internal_collectives_struct_t * tmp){
  int nb_tasks_in_node;
  sctk_barrier_hetero_messages_t *barrier;
  unsigned int generation;
  int task_id_in_node;

  nb_tasks_in_node = sctk_get_nb_task_local(communicator);
  barrier = &tmp->barrier.barrier_hetero_messages;
  generation = barrier->generation;
  task_id_in_node =
      OPA_fetch_and_incr_int(&barrier->tasks_entered_in_node);

  if (task_id_in_node == nb_tasks_in_node - 1) {

    sctk_barrier_hetero_messages_inter(communicator, tmp);

    OPA_store_int(&barrier->tasks_entered_in_node, 0);
      barrier->generation = generation + 1;
      sctk_atomics_write_barrier();
  } else {
    while (barrier->generation < generation + 1)
        sctk_thread_yield();
  }
}


void sctk_barrier_hetero_messages_init(sctk_internal_collectives_struct_t * tmp, sctk_communicator_t id){
  tmp->barrier_func = sctk_barrier_hetero_messages;

  sctk_barrier_hetero_messages_t *barrier;
  barrier = &tmp->barrier.barrier_hetero_messages;
  OPA_store_int(&barrier->tasks_entered_in_node, 0);
  barrier->generation = 0;
}

/************************************************************************/
/*Broadcast                                                             */
/************************************************************************/
void sctk_broadcast_hetero_messages_inter (void *buffer, const size_t size,
    const int root_process, const sctk_communicator_t communicator,
    struct sctk_internal_collectives_struct_s *tmp){

  /* If only one process involved, we return */
  int total = sctk_get_process_nb_in_array(communicator);
  if (total == 1) return;

  {
    sctk_thread_data_t *thread_data;
    int myself;
    int my_rank;
    int related_myself;
    int total_max;
    int i;
    sctk_hetero_messages_table_t table;
    int BROADCAST_ARRITY = 2;
    struct sctk_internal_ptp_s* ptp_internal;
    int specific_tag = broadcast_hetero_specific_message_tage;
    int *process_array;
    int root;

    sctk_hetero_messages_init_items(&table);

    BROADCAST_ARRITY = BROADCAST_MAX_SIZE / size;
    if(BROADCAST_ARRITY < 2){
      BROADCAST_ARRITY = 2;
    }
    if(BROADCAST_ARRITY > BROADCAST_ARITY_MAX){
      BROADCAST_ARRITY = BROADCAST_ARITY_MAX;
    }

    thread_data = sctk_thread_data_get ();
    my_rank = sctk_get_rank (communicator, thread_data->task_id);
    process_array = sctk_get_process_array(communicator);
    myself = *((int*) bsearch((void*) &sctk_process_rank,
        process_array,
        total,sizeof(int), int_cmp));
    root = *((int*) bsearch((void*) &root_process,
        process_array,
        total,sizeof(int), int_cmp));
    related_myself = (myself + total - root) % total;
    ptp_internal = sctk_get_internal_ptp(-1);

    total_max = log(total) / log(BROADCAST_ARRITY);
    total_max = pow(BROADCAST_ARRITY,total_max);
    if(total_max < total){
      total_max = total_max * BROADCAST_ARRITY;
    }
    assume(total_max >= total);

    for(i = BROADCAST_ARRITY; i <= total_max; i = i*BROADCAST_ARRITY){
      if(related_myself % i != 0){
        int dest;

        dest = (related_myself/i) * i;
        if(dest >= 0){
          sctk_hetero_messages_recv(communicator,
              process_array[(dest+root)%total],
              process_array[myself],
              root_process,
              buffer,size,specific_tag,sctk_hetero_messages_get_item(&table),ptp_internal,
              1,1);
          sctk_hetero_messages_wait(&table);
          break;
        }
      }
    }

    for(; i >=BROADCAST_ARRITY ; i = i / BROADCAST_ARRITY){
      if(related_myself % i == 0){
        int dest;
        int j;

        dest = related_myself;
        for(j = 1; j < BROADCAST_ARRITY; j++){
          if((dest + (j*(i/BROADCAST_ARRITY)))< total){
            sctk_hetero_messages_send(communicator,
                process_array[myself],
                process_array[(dest+root+(j*(i/BROADCAST_ARRITY))) % total],
                root_process,
                buffer,size,specific_tag,
            sctk_hetero_messages_get_item(&table),(size<BROADCAST_CHECK_THREASHOLD),(size<BROADCAST_CHECK_THREASHOLD));
          }
        }
      }
    }
    sctk_hetero_messages_wait(&table);
  }
}



void sctk_broadcast_hetero_messages (void *buffer, const size_t size,
				  const int root, const sctk_communicator_t communicator,
				  struct sctk_internal_collectives_struct_s *tmp){
  int nb_tasks_in_node;
  int task_id_in_node;
  int nb_task_exited;
  unsigned int generation;
  sctk_broadcast_hetero_messages_t *bcast;
  sctk_thread_data_t *thread_data;
  int myself;
  int is_root_on_node = 0;
  int root_process;
  int i;
  int *task_to_process;

  if(size == 0)  {
    sctk_barrier_hetero_messages(communicator,tmp);
    return;
  }

  thread_data = sctk_thread_data_get ();
  myself = sctk_get_rank (communicator, thread_data->task_id);
  nb_tasks_in_node = sctk_get_nb_task_local(communicator);
  bcast = &tmp->broadcast.broadcast_hetero_messages;
  generation = bcast->generation;
  task_id_in_node = OPA_fetch_and_incr_int(&bcast->tasks_entered_in_node);

  /* Looking if root is on node */
  root_process = sctk_get_process_rank_from_task_rank(root);
  if (root_process == sctk_process_rank)
    is_root_on_node = 1;

  if ( ((is_root_on_node) && (root == myself)) ||
      ((!is_root_on_node) && task_id_in_node == 0)) {

    /* Begin inter node communications */
    sctk_broadcast_hetero_messages_inter (buffer, size,
        root_process, communicator, tmp);
    /* End inter node communications */

    OPA_store_ptr(&bcast->buff_root, buffer);
    sctk_atomics_write_barrier();

    while (OPA_load_int(&bcast->tasks_exited_in_node) != nb_tasks_in_node - 1)
      sctk_thread_yield();

    /* Reinit all vars */
    OPA_store_int(&bcast->tasks_entered_in_node, 0);
    OPA_store_int(&bcast->tasks_exited_in_node, 0);
    OPA_store_ptr(&bcast->buff_root, NULL);
    bcast->generation = generation + 1;
    sctk_atomics_write_barrier();
  } else {
    void *buff_root = NULL;
    /* Wait until the buffer is ready */
    while  ((buff_root = OPA_load_ptr(&bcast->buff_root)) == NULL)
      sctk_thread_yield();

    memcpy(buffer, buff_root, size);

    OPA_incr_int(&bcast->tasks_exited_in_node);

    while (bcast->generation < generation + 1)
      sctk_thread_yield();
  }
}

void sctk_broadcast_hetero_messages_init(struct sctk_internal_collectives_struct_s * tmp, sctk_communicator_t id){
  tmp->broadcast_func = sctk_broadcast_hetero_messages;
  sctk_broadcast_hetero_messages_t *bcast;

  bcast = &tmp->broadcast.broadcast_hetero_messages;
  OPA_store_int(&bcast->tasks_entered_in_node, 0);
  OPA_store_int(&bcast->tasks_exited_in_node, 0);
  OPA_store_ptr(&bcast->buff_root, NULL);
  bcast->generation = 0;
}

/************************************************************************/
/*Allreduce                                                             */
/************************************************************************/
static void sctk_allreduce_hetero_messages_intern_inter (const void *buffer_in, void *buffer_out,
						const size_t elem_size,
						const size_t elem_number,
						void (*func) (const void *, void *, size_t,
							      sctk_datatype_t),
						const sctk_communicator_t communicator,
						const sctk_datatype_t data_type,
            struct sctk_internal_collectives_struct_s *tmp){

  int ALLREDUCE_ARRITY = 2;
  struct sctk_internal_ptp_s* ptp_internal;
  int total_max;
  sctk_thread_data_t *thread_data;
  sctk_hetero_messages_table_t table;
  void* buffer_tmp;
  void** buffer_table;
  int my_rank;
  int myself;
  size_t size = elem_size * elem_number;
  int specific_tag = allreduce_hetero_specific_message_tage;
  int total = sctk_get_process_nb_in_array(communicator);
  int i;
  int *process_array;

  /* If only one process involved, we return */
  if (total == 1) return;


  /*
     MPI require that the result of the allreduce is the same on all MPI tasks.
     This is an issue for MPI_SUM, MPI_PROD and user defined operation on floating
     point datatypes.
     */
  sctk_hetero_messages_init_items(&table);

  ALLREDUCE_ARRITY = ALLREDUCE_MAX_SIZE / size;
  if(ALLREDUCE_ARRITY < 2){
    ALLREDUCE_ARRITY = 2;
  }
  if(ALLREDUCE_ARRITY > ALLREDUCE_ARITY_MAX){
    ALLREDUCE_ARRITY = ALLREDUCE_ARITY_MAX;
  }

  buffer_tmp = sctk_malloc(size*(ALLREDUCE_ARRITY -1));
  buffer_table = sctk_malloc((ALLREDUCE_ARRITY -1) * sizeof(void*));

  {
    int j;
    for(j = 1; j < ALLREDUCE_ARRITY; j++){
      buffer_table[j-1] = ((char*)buffer_tmp) + (size * (j-1));
    }
  }

  thread_data = sctk_thread_data_get ();
  my_rank = sctk_get_rank (communicator, thread_data->task_id);
  process_array = sctk_get_process_array(communicator),
  myself = *((int*) bsearch((void*) &sctk_process_rank,
        process_array,
        total,sizeof(int), int_cmp));

  /* We get the PTP list -1  */
  ptp_internal = sctk_get_internal_ptp(-1);

  total_max = log(total) / log(ALLREDUCE_ARRITY);
  total_max = pow(ALLREDUCE_ARRITY,total_max);
  if(total_max < total){
    total_max = total_max * ALLREDUCE_ARRITY;
  }
  assume(total_max >= total);

  for(i = ALLREDUCE_ARRITY; i <= total_max; i = i*ALLREDUCE_ARRITY){
    if(myself % i == 0){
      int src;
      int j;
      src = myself;
      for(j = 1; j < ALLREDUCE_ARRITY; j++){
        if((src + (j*(i/ALLREDUCE_ARRITY))) < total){

          sctk_nodebug("Recv from %d",src + (j*(i/ALLREDUCE_ARRITY)));
          sctk_hetero_messages_recv(communicator,
              process_array[src + (j*(i/ALLREDUCE_ARRITY))],
              process_array[myself],
              0,buffer_table[j-1],size,
              specific_tag,
              sctk_hetero_messages_get_item(&table),ptp_internal,0,0);
        }
      }
      sctk_hetero_messages_wait(&table);
      for(j = 1; j < ALLREDUCE_ARRITY; j++){
        if((src + (j*(i/ALLREDUCE_ARRITY))) < total){
          func(buffer_table[j-1],buffer_out, elem_number,data_type);
        }
      }

    } else {
      int dest;

      dest = (myself / i) * i;
      if(dest >= 0){
        memcpy(buffer_tmp,buffer_out,size);
        sctk_nodebug("src %d Leaf send to %d",myself, dest);
        sctk_hetero_messages_send(communicator,
            process_array[myself],
            process_array[dest],
            0,buffer_tmp,size,
            specific_tag,
            sctk_hetero_messages_get_item(&table),1,1);

        sctk_nodebug("Leaf Recv from %d",dest);
        sctk_hetero_messages_recv(communicator,
            process_array[dest],
            process_array[myself],1,buffer_out,size,
            specific_tag,
            sctk_hetero_messages_get_item(&table),ptp_internal,1,1);
        sctk_hetero_messages_wait(&table);
        break;
      }

    }
  }
  sctk_hetero_messages_wait(&table);
  for(; i >=ALLREDUCE_ARRITY ; i = i / ALLREDUCE_ARRITY){
    if(myself % i == 0){
      int dest;
      int j;

      dest = myself;
      for(j = 1; j < ALLREDUCE_ARRITY; j++){
        if((dest + (j*(i/ALLREDUCE_ARRITY))) < total){
          sctk_nodebug("send to %d",dest+(j*(i/ALLREDUCE_ARRITY)));
          sctk_hetero_messages_send(communicator,
              process_array[myself],
              process_array[dest+(j*(i/ALLREDUCE_ARRITY))],
              1,buffer_out,size,
              specific_tag,
              sctk_hetero_messages_get_item(&table),
              (size<ALLREDUCE_CHECK_THREASHOLD),(size<ALLREDUCE_CHECK_THREASHOLD));
        }
      }
    }
  }
  sctk_hetero_messages_wait(&table);
  sctk_free(buffer_tmp);
  sctk_free(buffer_table);
}


static void sctk_allreduce_hetero_messages_hetero_intern (const void *buffer_in, void *buffer_out,
						const size_t elem_size,
						const size_t elem_number,
						void (*func) (const void *, void *, size_t,
							      sctk_datatype_t),
						const sctk_communicator_t communicator,
						const sctk_datatype_t data_type,
						struct sctk_internal_collectives_struct_s *tmp){
  if(elem_number == 0){
    sctk_barrier_hetero_messages(communicator,tmp);
  } else {
    sctk_nodebug("Starting allreduce");
    int task_id_in_node;
    int nb_tasks_in_node;
    sctk_allreduce_hetero_messages_t *allreduce;
    void **buff_in;
    void **buff_out;
    unsigned int generation;
    size_t size;

    size = elem_size * elem_number;
    assume(size);
    nb_tasks_in_node = sctk_get_nb_task_local(communicator);
    allreduce = &tmp->allreduce.allreduce_hetero_messages;
    generation = allreduce->generation;
    task_id_in_node =
      OPA_fetch_and_incr_int(&allreduce->tasks_entered_in_node);

    buff_in = (void **) allreduce->buff_in;
    buff_out = (void**) allreduce->buff_out;

    /* Fill the buffer entry for all tasks */
    buff_in[task_id_in_node] = buffer_in;
    buff_out[task_id_in_node] = buffer_out;
    sctk_atomics_write_barrier();

    /* Last entry */
    if (task_id_in_node == nb_tasks_in_node - 1) {
      int i;

      memcpy(buffer_out, buffer_in, size);

      /* Wait on all tasks and apply the reduction function.
       * FIXME: reduction function must be done in parallel */
      for (i=0; i < nb_tasks_in_node-1; ++i) {
        while (buff_in[i] == NULL) sctk_thread_yield();

        sctk_nodebug("Add content %d to buffer", *((int*) buff_in[i]));
        func(buff_in[i], buffer_out, elem_number, data_type);
      }

      sctk_nodebug("Buffer content : %d", *((int*) buffer_out));

      /* Begin allreduce inter-node */
      sctk_allreduce_hetero_messages_intern_inter (buffer_in, buffer_out,
					  elem_size, elem_number, func,
						communicator, data_type, tmp);
      /* End allreduce inter-node. Result is in buffer_out and must
       * be propagate to all other tasks */

      allreduce->generation = generation + 1;
      sctk_atomics_write_barrier();

      while (OPA_load_int(&allreduce->tasks_entered_in_node) != 1)
        sctk_thread_yield();

      buff_in[task_id_in_node] = NULL;
      buff_out[task_id_in_node] = NULL;
      OPA_store_int(&allreduce->tasks_entered_in_node, 0);

      allreduce->generation = generation + 2;
    } else {
      while (allreduce->generation < generation + 1)
        sctk_thread_yield();

      memcpy(buffer_out,
          buff_out[nb_tasks_in_node - 1],size);

      buff_in[task_id_in_node] = NULL;
      buff_out[task_id_in_node] = NULL;
      OPA_decr_int(&allreduce->tasks_entered_in_node);

      while (allreduce->generation < generation + 2)
        sctk_thread_yield();
    }
  sctk_nodebug("End allreduce");
  }
}

static void sctk_allreduce_hetero_messages (const void *buffer_in, void *buffer_out,
				   const size_t elem_size,
				   const size_t elem_number,
				   void (*func) (const void *, void *, size_t,
						 sctk_datatype_t),
				   const sctk_communicator_t communicator,
				   const sctk_datatype_t data_type,
				   struct sctk_internal_collectives_struct_s *tmp){
#warning "Add buffer splitting"
  sctk_allreduce_hetero_messages_hetero_intern(buffer_in,buffer_out,elem_size,elem_number,func,communicator,data_type,tmp);
}

void sctk_allreduce_hetero_messages_init(struct sctk_internal_collectives_struct_s * tmp, sctk_communicator_t id){
  tmp->allreduce_func = sctk_allreduce_hetero_messages;
  sctk_allreduce_hetero_messages_t *allreduce;
  int nb_tasks_in_node;

  nb_tasks_in_node = sctk_get_nb_task_local(id);
  allreduce = &tmp->allreduce.allreduce_hetero_messages;
  OPA_store_int(&allreduce->tasks_entered_in_node, 0);
  allreduce->generation = 0;
  allreduce->buff_in = sctk_malloc(nb_tasks_in_node * sizeof(void*));
  allreduce->buff_out = sctk_malloc(nb_tasks_in_node * sizeof(void*));
  memset(allreduce->buff_in, 0, nb_tasks_in_node * sizeof(void*));
  memset(allreduce->buff_out, 0, nb_tasks_in_node * sizeof(void*));
}

/************************************************************************/
/*Init                                                                  */
/************************************************************************/
void sctk_collectives_init_hetero_messages (sctk_communicator_t id){
  sctk_collectives_init(id,
			sctk_barrier_hetero_messages_init,
			sctk_broadcast_hetero_messages_init,
			sctk_allreduce_hetero_messages_init);
  /* if(sctk_process_rank == 0){ */
  /*   sctk_warning("Use low performances collectives"); */
  /* } */
}
