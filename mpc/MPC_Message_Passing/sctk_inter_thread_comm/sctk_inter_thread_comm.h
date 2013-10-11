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
/* #   - PERACHE Marc    marc.perache@cea.fr                              # */
/* #                                                                      # */
/* ######################################################################## */
#ifndef __SCTK_INTER_THREAD_COMMUNICATIONS_H_
#define __SCTK_INTER_THREAD_COMMUNICATIONS_H_

#include <sctk_config.h>
#include <sctk_debug.h>
#include <sctk_communicator.h>
#include <sctk_collective_communications.h>
#include <mpcmp.h>
#include <sctk_reorder.h>
#include <sctk_ib.h>
#include <opa_primitives.h>

#ifdef __cplusplus
extern "C"
{
#endif

  struct sctk_request_s;
  struct sctk_ib_msg_header_s;
  struct mpc_buffered_msg_s;
  struct sctk_internal_ptp_s;

#define SCTK_MESSAGE_PENDING 0
#define SCTK_MESSAGE_DONE 1
#define SCTK_MESSAGE_CANCELED 2

  /* Message for a process */
#define MASK_PROCESS_SPECIFIC 1<<9
  /* A control message */
#define MASK_CONTROL_MESSAGE  1<<10

  /* Message for a process with ordering */
#define MASK_PROCESS_SPECIFIC_W_ORDERING (1<<31 | MASK_PROCESS_SPECIFIC)

#define IS_PROCESS_SPECIFIC_MESSAGE_TAG_WITH_ORDERING(x) ( (MASK_PROCESS_SPECIFIC_W_ORDERING & x) == (MASK_PROCESS_SPECIFIC_W_ORDERING))

#define IS_PROCESS_SPECIFIC_MESSAGE_TAG(x) ( (MASK_PROCESS_SPECIFIC & x) == (MASK_PROCESS_SPECIFIC) )

/* For ondemand connexions */
#define MASK_PROCESS_SPECIFIC_ONDEMAND (1<<29 | MASK_PROCESS_SPECIFIC | MASK_CONTROL_MESSAGE)
#define IS_PROCESS_SPECIFIC_ONDEMAND(x) ( (MASK_PROCESS_SPECIFIC_ONDEMAND & x) == (MASK_PROCESS_SPECIFIC_ONDEMAND) )

/* For low memory consumption */
#define MASK_PROCESS_SPECIFIC_LOW_MEM (1<<28 | MASK_PROCESS_SPECIFIC | MASK_CONTROL_MESSAGE)
#define IS_PROCESS_SPECIFIC_LOW_MEM(x) ( (MASK_PROCESS_SPECIFIC_LOW_MEM & x) == (MASK_PROCESS_SPECIFIC_LOW_MEM) )

/* For user connexions */
#define MASK_PROCESS_SPECIFIC_USER (1<<27 | MASK_PROCESS_SPECIFIC | MASK_CONTROL_MESSAGE)
#define IS_PROCESS_SPECIFIC_USER(x) ( (MASK_PROCESS_SPECIFIC_USER & x) == (MASK_PROCESS_SPECIFIC_USER) )


/* Is the message a control message ? */
#define IS_PROCESS_SPECIFIC_CONTROL_MESSAGE(x) ( (MASK_CONTROL_MESSAGE & x) == (MASK_CONTROL_MESSAGE) )




  typedef enum {
    pt2pt_specific_message_tag = 1,
    barrier_specific_message_tag = 2,
    broadcast_specific_message_tag = 3,
    allreduce_specific_message_tag = 4,

    /* Process specific */
    process_specific_message_tag = MASK_PROCESS_SPECIFIC,
    /* On demand */
    ondemand_specific_message_tag =  MASK_PROCESS_SPECIFIC_ONDEMAND,
    /* Low memory */
    low_mem_specific_message_tag =  MASK_PROCESS_SPECIFIC_LOW_MEM,
    /* User */
    user_specific_message_tag =  MASK_PROCESS_SPECIFIC_USER,

    /* Collective */
    allreduce_hetero_specific_message_tage = MASK_PROCESS_SPECIFIC_W_ORDERING | allreduce_specific_message_tag,
    broadcast_hetero_specific_message_tage = MASK_PROCESS_SPECIFIC_W_ORDERING | broadcast_specific_message_tag,
    barrier_hetero_specific_message_tage = MASK_PROCESS_SPECIFIC_W_ORDERING | broadcast_specific_message_tag,

  }specific_message_tag_t;

  typedef enum {
    REQUEST_NULL = 0,
    REQUEST_SEND = 1,
    REQUEST_RECV = 2,
    REQUEST_SEND_COLL = 3,
    REQUEST_RECV_COLL = 4
  } sctk_request_type_t;

  typedef struct sctk_thread_message_header_s
  {

    int source;
    int destination;
    sctk_communicator_t communicator;
    int message_tag;
    int message_number;
    int glob_source;
    int glob_destination;
    char use_message_numbering;
    specific_message_tag_t specific_message_tag;

    size_t msg_size;
  } sctk_thread_message_header_t;

  typedef unsigned int sctk_pack_indexes_t;
  typedef unsigned long sctk_pack_absolute_indexes_t;
  typedef int sctk_count_t;

  typedef struct{
    size_t size;
    void* addr;
  }sctk_message_contiguous_t;

  typedef struct{
    sctk_count_t count;
    sctk_pack_indexes_t *begins;
    sctk_pack_indexes_t *ends;
    void* addr;
    size_t elem_size;
  }sctk_message_pack_std_list_t;

  typedef struct{
    sctk_count_t count;
    sctk_pack_absolute_indexes_t *begins;
    sctk_pack_absolute_indexes_t *ends;
    void* addr;
    size_t elem_size;
  }sctk_message_pack_absolute_list_t;

  typedef union{
    sctk_message_pack_absolute_list_t* absolute;
    sctk_message_pack_std_list_t* std;
  }sctk_message_pack_list_t;

  typedef union{
    sctk_message_pack_absolute_list_t absolute;
    sctk_message_pack_std_list_t std;
  }sctk_message_pack_default_t;

  typedef struct{
    size_t count;
    size_t max_count;
    sctk_message_pack_list_t list;
  }sctk_message_pack_t;

  typedef enum {
    sctk_message_contiguous,
    sctk_message_pack,
    sctk_message_pack_absolute,
    sctk_message_pack_undefined,
    sctk_message_network
  } sctk_message_type_t;

  typedef union {
    sctk_message_contiguous_t contiguous;
    sctk_message_pack_t pack;
  } sctk_message_t;

  typedef MPC_Request sctk_request_t;

  struct sctk_thread_ptp_message_s;

  typedef volatile struct sctk_msg_list_s{
    struct sctk_thread_ptp_message_s * msg;
    volatile struct sctk_msg_list_s *prev, *next;
  }sctk_msg_list_t;

typedef struct sctk_message_to_copy_s{
  struct sctk_thread_ptp_message_s * msg_send;
  struct sctk_thread_ptp_message_s * msg_recv;
  struct sctk_message_to_copy_s * prev, *next;
}sctk_message_to_copy_t;

  /*Data to tranfers in inter-process communications*/
  typedef struct {
    sctk_thread_message_header_t header;
    volatile int* volatile completion_flag;
    /* XXX:Specific to checksum */
    unsigned long checksum;
  }sctk_thread_ptp_message_body_t;

typedef struct {
  sctk_spinlock_t lock;
  volatile sctk_msg_list_t* list;
} sctk_internal_ptp_list_completing_t;

  /*Data not to tranfers in inter-process communications*/
  typedef struct {
    char remote_source;
    char remote_destination;

    int need_check_in_wait;

    sctk_request_t * request;

    /*Message data*/
    sctk_message_type_t message_type;
    sctk_message_t message;
    sctk_message_pack_default_t default_pack;

    /*Storage structs*/
    sctk_msg_list_t distant_list;
    sctk_message_to_copy_t copy_list;

    struct sctk_internal_ptp_s * internal_ptp;

    /*Destructor*/
    void (*free_memory)(void*);

    /*Copy operator*/
    void (*message_copy)(sctk_message_to_copy_t*);

    /*Reoder buffer struct*/
    sctk_reorder_buffer_t reorder;

    /* If the message has been buffered during the
     * Send function. If it is, we need to free the async
     * buffer when completing the message */
    struct mpc_buffered_msg_s * buffer_async;

    /* RDMA infos */
    void* rdma_src;
    void* route_table;

    /* XXX:Specific to IB */
    struct sctk_ib_msg_header_s ib;

  }sctk_thread_ptp_message_tail_t;

#define sctk_msg_get_use_message_numbering body.header.use_message_numbering
#define sctk_msg_get_source body.header.source
#define sctk_msg_get_destination body.header.destination
#define sctk_msg_get_glob_source body.header.glob_source
#define sctk_msg_get_glob_destination body.header.glob_destination
#define sctk_msg_get_communicator body.header.communicator
#define sctk_msg_get_message_tag body.header.message_tag
#define sctk_msg_get_message_number body.header.message_number
#define sctk_msg_get_specific_message_tag body.header.specific_message_tag
#define sctk_msg_get_msg_size body.header.msg_size
#define sctk_msg_get_completion_flag body.completion_flag

  typedef struct sctk_thread_ptp_message_s{
    sctk_thread_ptp_message_body_t body;
    sctk_thread_ptp_message_tail_t  tail;

    /* Pointers for chaining elements */
    struct sctk_thread_ptp_message_s* prev;
    struct sctk_thread_ptp_message_s* next;
    /* If the entry comes from a buffered list */
    char from_buffered;
  }sctk_thread_ptp_message_t;

  typedef struct sctk_perform_messages_s {
    sctk_request_t* request;
    struct sctk_internal_ptp_s *recv_ptp;
    struct sctk_internal_ptp_s *send_ptp;
    int remote_process;
    int source_task_id;
    int polling_task_id;
    /* If we are blocked inside a function similar to MPI_Wait */
    int blocking;
  } sctk_perform_messages_t;


  /**
     Check if the message if completed according to the message passed as a request
  */
  void sctk_perform_messages(struct sctk_perform_messages_s * wait);
  void sctk_perform_messages_wait_init_request_type(struct sctk_perform_messages_s * wait);
  void sctk_init_header (sctk_thread_ptp_message_t *tmp, const int myself,
			 sctk_message_type_t msg_type, void (*free_memory)(void*),
			 void (*message_copy)(sctk_message_to_copy_t*));
  void sctk_reinit_header (sctk_thread_ptp_message_t *tmp, void (*free_memory)(void*),
			   void (*message_copy)(sctk_message_to_copy_t*));
  sctk_thread_ptp_message_t *sctk_create_header (const int myself,sctk_message_type_t msg_type);
  void sctk_add_adress_in_message (sctk_thread_ptp_message_t *
				  restrict msg, void *restrict addr,
				  const size_t size);
  void sctk_add_pack_in_message (sctk_thread_ptp_message_t * msg,
				void *adr, const sctk_count_t nb_items,
				const size_t elem_size,
				sctk_pack_indexes_t * begins,
				sctk_pack_indexes_t * ends);
  void sctk_add_pack_in_message_absolute (sctk_thread_ptp_message_t *
					 msg, void *adr,
					 const sctk_count_t nb_items,
					 const size_t elem_size,
					 sctk_pack_absolute_indexes_t *
					 begins,
					 sctk_pack_absolute_indexes_t * ends);
  void sctk_set_header_in_message (sctk_thread_ptp_message_t *
				   msg, const int message_tag,
				   const sctk_communicator_t
				   communicator,
				   const int source,
				   const int destination,
				   sctk_request_t * request,
				   const size_t count,
				   specific_message_tag_t specific_message_tag);
  void sctk_wait_message (sctk_request_t * request);
  void sctk_wait_all (const int task, const sctk_communicator_t com);
  void sctk_probe_source_any_tag (int destination, int source,
				  const sctk_communicator_t comm,
				  int *status,
				  sctk_thread_message_header_t * msg);
  void sctk_probe_any_source_any_tag (int destination,
				      const sctk_communicator_t comm,
				      int *status,
				      sctk_thread_message_header_t * msg);
  void sctk_probe_source_tag (int destination, int source,
			      const sctk_communicator_t comm, int *status,
			      sctk_thread_message_header_t * msg);
  void sctk_probe_any_source_tag (int destination,
				  const sctk_communicator_t comm,
				  int *status,
				  sctk_thread_message_header_t * msg);
  void sctk_send_message (sctk_thread_ptp_message_t * msg);
  void sctk_send_message_try_check (sctk_thread_ptp_message_t * msg,int perform_check);
  void sctk_recv_message (sctk_thread_ptp_message_t * msg, struct sctk_internal_ptp_s* tmp,
      int need_check);
  void sctk_recv_message_try_check (sctk_thread_ptp_message_t * msg,struct sctk_internal_ptp_s* tmp,int perform_check);
  struct sctk_internal_ptp_s* sctk_get_internal_ptp(int glob_id);
  int sctk_is_net_message (int dest);
  void sctk_cancel_message (sctk_request_t * msg);
  void sctk_ptp_per_task_init (int i);
  void sctk_unregister_thread (const int i);

  void sctk_message_copy(sctk_message_to_copy_t* tmp);
  void sctk_message_copy_pack(sctk_message_to_copy_t* tmp);
  void sctk_message_copy_pack_absolute(sctk_message_to_copy_t* tmp);
  void sctk_notify_idle_message ();
  void sctk_notify_idle_message_inter ();
  void sctk_message_completion_and_free(sctk_thread_ptp_message_t* send,
					sctk_thread_ptp_message_t* recv);
  void sctk_complete_and_free_message (sctk_thread_ptp_message_t * msg);
  void sctk_rebuild_header (sctk_thread_ptp_message_t * msg);
  int sctk_determine_src_process_from_header (sctk_thread_ptp_message_body_t * body);
  void sctk_determine_glob_source_and_destination_from_header (
      sctk_thread_ptp_message_body_t* body, int *glob_source, int *glob_destination);

  void sctk_perform_messages_wait_init(
    struct sctk_perform_messages_s * wait, sctk_request_t * request, int blocking);

  sctk_reorder_list_t * sctk_ptp_get_reorder_from_destination(int task);

  void sctk_inter_thread_perform_idle (volatile int *data, int value,
				     void (*func) (void *), void *arg);
#ifdef __cplusplus
}
#endif

#endif
